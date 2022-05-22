#include "Descriptor.h"

#include <chrono>
#include <memory>
#include <utility>

#include "../base/Logging.h"
#include "../base/String.h"
#include "../base/chrono.h"
#include "../fiber/ConditionalVariable.h"
#include "../fiber/Fiber.h"
#include "../fiber/ThisFiber.h"
#include "../fiber/Timer.h"
#include "EventLoop.h"
#include "util/Socket.h"

using namespace std::literals;

namespace tinyRPC {

struct Descriptor::SeldomlyUsed {
  std::string name;

  std::atomic<bool> cleanup_queued{false};

  std::atomic<std::size_t> error_events{};
  std::atomic<bool> error_seen{};  // Prevent multiple `EPOLLERR`s.

  // Set to non-`None` once a cleanup event is pending. If multiple events
  // triggered cleanup (e.g., an error occurred and the descriptor is
  // concurrently being removed from the `EventLoop`), the first one wins.
  std::atomic<CleanupReason> cleanup_reason{CleanupReason::None};

  // For implementing `WaitForCleanup()`.
  fiber::Mutex cleanup_lk;
  fiber::ConditionVariable cleanup_cv;
  bool cleanup_completed{false};
};

Descriptor::Descriptor(Handle fd, Event events, const std::string& name)
    : read_mostly_{.fd = std::move(fd),
                   .event_mask = static_cast<int>(events),
                   .seldomly_used = std::make_unique<SeldomlyUsed>()} {
  restart_read_count_ = (read_mostly_.event_mask & EPOLLIN) ? 1 : 0;
  restart_write_count_ = (read_mostly_.event_mask & EPOLLOUT) ? 1 : 0;
  if (name.empty()) {
    read_mostly_.seldomly_used->name = "";
  } else {
    read_mostly_.seldomly_used->name = name;
  }
}

Descriptor::~Descriptor() {
  FLARE_CHECK(!Enabled(),
              "Descriptor {} is still associated with event loop when it's "
              "destructed.", fd());
}

void Descriptor::RestartReadIn(std::chrono::nanoseconds after) {
  if (after != 0ns) {
    // This keeps us alive until our task is called.
    auto ref = shared_from_this();

    auto timer = fiber::internal::CreateTimer(
        ReadSteadyClock() + after, [this, ref = std::move(ref)](auto timer_id) {
          fiber::internal::KillTimer(timer_id);
          RestartReadNow();
        });
    fiber::internal::EnableTimer(timer);
  } else {
    RestartReadNow();
  }
}

void Descriptor::RestartWriteIn(std::chrono::nanoseconds after) {
  if (after != 0ns) {
    auto timer = fiber::internal::CreateTimer(
        ReadSteadyClock() + after,
        [this, ref = shared_from_this()](auto timer_id) {
          fiber::internal::KillTimer(timer_id);
          RestartWriteNow();
        });
    fiber::internal::EnableTimer(timer);
  } else {
    RestartWriteNow();
  }
}

void Descriptor::Kill(CleanupReason reason) {
  FLARE_CHECK(reason != CleanupReason::None);
  auto expected = CleanupReason::None;
  if (read_mostly_.seldomly_used->cleanup_reason.compare_exchange_strong(
          expected, reason)) {
    GetEventLoop()->AddTask([this, ref = shared_from_this()] {
      GetEventLoop()->DisableDescriptor(this);
      cleanup_pending_.store(true);
      // From now on, no more call to `FireEvents()` will be made.

      QueueCleanupCallbackCheck();
    });
  }
}

void Descriptor::WaitForCleanup() {
  std::unique_lock lk(read_mostly_.seldomly_used->cleanup_lk);
  read_mostly_.seldomly_used->cleanup_cv.wait(
      lk, [this] { return read_mostly_.seldomly_used->cleanup_completed; });
}

const std::string& Descriptor::GetName() const {
  return read_mostly_.seldomly_used->name;
}

void Descriptor::FireEvents(int mask) {
  if (FLARE_UNLIKELY(mask & EPOLLERR)) {
    // `EPOLLERR` is handled first. In this case other events are ignored. You
    // don't want to read from / write to a file descriptor in error state.
    //
    // @sa: https://stackoverflow.com/a/37079607
    FireErrorEvent();
    return;
  }
  if (mask & EPOLLIN) {
    // TODO(luobogao): For the moment `EPOLLRDHUP` is not enabled in
    // `EventLoop`.
    FireReadEvent();
  }
  if (mask & EPOLLOUT) {
    FireWriteEvent();
  }
}

void Descriptor::FireReadEvent() {
  // Acquiring here guarantees thatever had done by prior call to `OnReadable()`
  // is visible to us (as the prior call ends with a releasing store to the
  // event count.).
  if (read_events_.fetch_add(1) == 0) {
    // `read_events_` was 0, so no fiber was calling `OnReadable`. Let's call
    // it then.
    fiber::StartFiberDetached([this] {
      // The reference we keep here keeps us alive until we leave.
      //
      // The reason why we can be destroyed while executing is that if someone
      // else is executing `QueueCleanupCallbackCheck()`, it only waits until
      // event counters (in this case, `read_event_`) reaches 0.
      //
      // If we're delayed long enough, it's possible that after we decremented
      // `read_event_` to zero, but before doing the rest job, we're destroyed.
      //
      // Need to hold a reference each time read event need to be handled is
      // unfortunate, but if we add yet another counter for counting outstanding
      // fibers like us, the overhead is the same (in both case it's a atomic
      // increment).
      auto ref = shared_from_this();

      do {
        auto rc = OnReadable();
        if (FLARE_LIKELY(rc == EventAction::Ready)) {
          continue;
        } else if (FLARE_UNLIKELY(rc == EventAction::Leaving)) {
          FLARE_CHECK(read_mostly_.seldomly_used->cleanup_reason != CleanupReason::None,
                      "Did you forget to call `Kill()`?");
          // We can only reset the counter in event loop's context.
          //
          // As `Kill()` as been called, by the time our task is run by the
          // event loop, this descriptor has been disabled, and no more call to
          // `FireReadEvent()` (the only one who increments `read_events_`), so
          // it's safe to reset the counter.
          //
          // Meanwhile, `QueueCleanupCallbackCheck()` is called after our task,
          // by the time it checked the counter, it's zero as expected.
          GetEventLoop()->AddTask([this] {
            read_events_.store(0);
            QueueCleanupCallbackCheck();
          });
          break;
        } else if (rc == EventAction::Suppress) {
          SuppressReadAndClearReadEventCount();

          // CAUTION: Here we break out before `read_events_` is drained. This
          // is safe though, as `SuppressReadAndClearReadEventCount()` will
          // reset `read_events_` to zero after is has disabled the event.
          break;
        }
      } while (read_events_.fetch_sub(1) !=
               1);  // Loop until we decremented `read_events_` to zero. If more
                    // data has come before `OnReadable()` returns, the loop
                    // condition will hold.
      QueueCleanupCallbackCheck();
    });
  }  // Otherwise someone else is calling `OnReadable`. Nothing to do then.
}

void Descriptor::FireWriteEvent() {
  if (write_events_.fetch_add(1) == 0) {
    fiber::StartFiberDetached([this] {
      auto ref = shared_from_this();
      do {
        auto rc = OnWritable();
        if (FLARE_LIKELY(rc == EventAction::Ready)) {
          continue;
        } else if (FLARE_UNLIKELY(rc == EventAction::Leaving)) {
          FLARE_CHECK(read_mostly_.seldomly_used->cleanup_reason != CleanupReason::None,
                      "Did you forget to call `Kill()`?");
          GetEventLoop()->AddTask([this] {
            write_events_.store(0);
            QueueCleanupCallbackCheck();
          });
          break;
        } else if (rc == EventAction::Suppress) {
          SuppressWriteAndClearWriteEventCount();
          break;  // `write_events_` can be non-zero. (@sa: `FireReadEvent` for
                  // more explanation.)
        }
      } while (write_events_.fetch_sub(1) !=
               1);  // Loop until `write_events_` reaches zero.
      QueueCleanupCallbackCheck();
    });
  }
}

void Descriptor::FireErrorEvent() {
  if (read_mostly_.seldomly_used->error_seen.exchange(true)) {
    FLARE_LOG_WARNING("Unexpected: Multiple `EPOLLERR` received.");
    return;
  }

  if (read_mostly_.seldomly_used->error_events.fetch_add(1) == 0) {
    fiber::StartFiberDetached([this] {
      auto ref = shared_from_this();
      OnError(io::util::GetSocketError(fd()));
      FLARE_CHECK_EQ(read_mostly_.seldomly_used->error_events.fetch_sub(1), 1);
      QueueCleanupCallbackCheck();
    });
  } else {
    FLARE_CHECK(!"Unexpected");
  }
}

void Descriptor::SuppressReadAndClearReadEventCount() {
  // This must be done in `EventLoop`. Otherwise order of calls to
  // `RearmDescriptor` is nondeterministic.
  GetEventLoop()->AddTask([this, ref = shared_from_this()] {
    // We reset `read_events_` to zero first, as it's left non-zero when we
    // leave `FireReadEvent()`.
    //
    // No race should occur. `FireReadEvent()` itself is called in `EventLoop`
    // (where we're running), so it can't race with us. The only other one who
    // can change `read_events_` is the fiber who called us, and it should
    // have break out the `while` loop immediately after calling us without
    // touching `read_events_` any more.
    //
    // So we're safe here.
    read_events_.store(0);

    // This is needed in case the descriptor is going to leave and its
    // `OnReadable()` returns `Suppress`.
    QueueCleanupCallbackCheck();

    if (Enabled()) {
      auto reached =
          restart_read_count_.fetch_sub(1) - 1;
      // If `reached` (i.e., `restart_read_count_` after decrement) reaches 0,
      // we're earlier than `RestartReadIn()`.

      // FIXME: For the moment there can be more `RestartRead` than read
      // suppression. This is caused by streaming RPC. `StreamIoAdaptor`
      // triggers a `RestartRead()` each time its internal buffer dropped down
      // below its buffer limit, but `Suppress` is only returned when the
      // system's buffer has been drained. While we're draining system's buffer,
      // `StreamIoAdaptor`'s internal buffer can reach and drop down from its
      // buffer limit several times since we keep feeding it before we finally
      // return `Suppress`.
      //
      // Let's see if we can return `Suppress` immediately when
      // `StreamIoAdaptor`'s internal buffer is filled up.

      FLARE_CHECK_NE(reached, -1);
      FLARE_CHECK(GetEventMask() & EPOLLIN);  // Were `EPOLLIN` to be removed,
                                              // it's us who remove it.
      if (reached == 0) {
        SetEventMask(GetEventMask() & ~EPOLLIN);
        GetEventLoop()->RearmDescriptor(this);
      } else {
        // Otherwise things get tricky. In this case we left system's buffer
        // un-drained, and `RestartRead` happens before us. From system's
        // perspective, this scenario just looks like we haven't drained its
        // buffer yet, so it won't return a `EPOLLIN` again.
        //
        // We have to either emulate one or remove and re-add the descriptor to
        // the event loop in this case.
        FireEvents(EPOLLIN);
      }
    }  // The descriptor is leaving otherwise, nothing to do.
  });
}

void Descriptor::SuppressWriteAndClearWriteEventCount() {
  GetEventLoop()->AddTask([this, ref = shared_from_this()] {
    // Largely the same as `SuppressReadAndClearReadEventCount()`.
    write_events_.store(0);
    QueueCleanupCallbackCheck();

    if (Enabled()) {
      auto reached =
          restart_write_count_.fetch_sub(1) - 1;

      FLARE_CHECK(reached == 0 || reached == 1,
                  "Unexpected restart-write count: {}", reached);
      FLARE_CHECK(GetEventMask() & EPOLLOUT);
      if (reached == 0) {
        SetEventMask(GetEventMask() & ~EPOLLOUT);
        GetEventLoop()->RearmDescriptor(this);
      } else {
        FireEvents(EPOLLOUT);
      }
    }  // The descriptor is leaving otherwise, nothing to do.
  });
}

void Descriptor::RestartReadNow() {
  GetEventLoop()->AddTask([this, ref = shared_from_this()] {
    if (Enabled()) {
      auto count = restart_read_count_.fetch_add(1);

      // `count` is 0 if `Suppress` was returned from `OnReadable`. `count` is
      // 1 if we're called before `Suppress` is returned. Any other values are
      // unexpected.
      //
      // NOT `CHECK`-ed, though. @sa: `SuppressReadAndClearReadEventCount()`.

      if (count == 0) {  // We changed it from 0 to 1.
        FLARE_CHECK_EQ(GetEventMask() & EPOLLIN, 0);
        SetEventMask(GetEventMask() | EPOLLIN);
        GetEventLoop()->RearmDescriptor(this);
      }  // Otherwise `Suppress` will see `restart_read_count_` non-zero, and
         // deal with it properly.
    }
  });
}

void Descriptor::RestartWriteNow() {
  GetEventLoop()->AddTask([this, ref = shared_from_this()] {
    if (Enabled()) {
      auto count = restart_write_count_.fetch_add(1);

      FLARE_CHECK(count == 0 || count == 1,
                  "Unexpected restart-write count: {}", count);
      if (count == 0) {
        FLARE_CHECK_EQ(GetEventMask() & EPOLLOUT, 0);
        SetEventMask(GetEventMask() | EPOLLOUT);
        GetEventLoop()->RearmDescriptor(this);
      }
    }
  });
}

void Descriptor::QueueCleanupCallbackCheck() {
  // Full barrier, hurt performance.
  //
  // Here we need it to guarantee that:
  //
  // - For `Kill()`, it's preceding store to `cleanup_pending_` cannot be
  //   reordered after reading `xxx_events_`;
  //
  // - For `FireXxxEvent()`, it's load of `cleanup_pending_` cannot be reordered
  //   before its store to `xxx_events_`.
  //
  // Either case, reordering leads to falsely treating the descriptor in use.
  if (FLARE_LIKELY(!cleanup_pending_)) {
    return;
  }

  // Given that the descriptor is removed from the event loop prior to setting
  // `cleanup_pending_` to zero, by reaching here we can be sure that no more
  // `FireEvents()` will be called. This in turns guarantees us that
  // `xxx_events_` can only be decremented (they're solely incremented by
  // `FireEvents()`.).
  //
  // So we check if all `xxx_events_` reached zero, and fire `OnCleanup()` if
  // they did.
  if (read_events_ == 0 &&
      write_events_ == 0 &&
      read_mostly_.seldomly_used->error_events == 0) {
    // Consider queue a call to `OnCleanup()` then.
    if (!read_mostly_.seldomly_used->cleanup_queued.exchange(
            true)) {
      // No need to take a reference to us, `OnCleanup()` has not been called.
      GetEventLoop()->AddTask([this] {
        // The load below acts as a fence (paired with `exchange` above). (But
        // does it make sense?)
        (void)read_mostly_.seldomly_used->cleanup_queued;

        // They can't have changed.
        FLARE_CHECK_EQ(read_events_, 0);
        FLARE_CHECK_EQ(write_events_, 0);
        FLARE_CHECK_EQ(read_mostly_.seldomly_used->error_events,0);

        // Hold a reference to us. `DetachDescriptor` decrements reference
        // count, which might be the last one.
        auto ref = shared_from_this();

        // Detach the descriptor and call user's `OnCleanup`.
        GetEventLoop()->DetachDescriptor(shared_from_this());
        OnCleanup(read_mostly_.seldomly_used->cleanup_reason);

        // Wake up any waiters on `OnCleanup()`.
        std::scoped_lock _(read_mostly_.seldomly_used->cleanup_lk);
        read_mostly_.seldomly_used->cleanup_completed = true;
        read_mostly_.seldomly_used->cleanup_cv.notify_one();
      });
    }
  }
}

}  // namespace tinyRPC
