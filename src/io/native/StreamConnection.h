#ifndef _SRC_IO_NATIVE_STREAM_CONNECTION_H_
#define _SRC_IO_NATIVE_STREAM_CONNECTION_H_

#include <mutex>
#include <memory>

#include "../Descriptor.h"
#include "../detail/WritingBufferList.h"
#include "../StreamConnection.h"
#include "../util/StreamIO.h"
#include "../../base/MaybeOwning.h"

namespace tinyRPC {

// This class represents a TCP connection.
class NativeStreamConnection final : public Descriptor,
                                     public StreamConnection {
 public:
  struct Options {
    // Handler for consuming data and accepting several callbacks.
    MaybeOwning<StreamConnectionHandler> handler;

    std::shared_ptr<AbstractStreamIo> stream_io;

    // Maximum number of not-yet-processed bytes allowed.
    //
    // Set it to `std::numeric_limits<std::size_t>::max()` if you don't want
    // to set a limit (not recommended).
    std::size_t read_buffer_size = 0;  // Default value is invalid.

    // There's no `write_buffer_size`. So long as we're not allowed to block,
    // there's nothing we can do about too many pending writes.
  };

  explicit NativeStreamConnection(Handle fd, Options options);
  ~NativeStreamConnection();

  // Start handshaking with remote peer.
  void StartHandshaking() override;

  // Write `buffer` to the remote side.
  bool Write(const std::string& buffer, std::uintptr_t ctx) override;

  // Restart reading data.
  void RestartRead() override;

  void Stop() override;
  void Join() override;

 private:
  // Note: We only read a handful number of bytes and returns `err != EAGAIN`
  // to the caller (`EventLoop`) so as to not block events of other
  // `Descriptor`s.
  EventAction OnReadable() override;

  // We only write a handful number of bytes on each call.
  EventAction OnWritable() override;

  // An error occurred.
  void OnError(int err) override;

  void OnCleanup(CleanupReason reason) override;

  // Call user's callback to consume read buffer.
  EventAction ConsumeReadBuffer();

  enum class FlushStatus {
    Flushed,
    SystemBufferSaturated,
    PartialWrite,    // We wrote something out, but the connection is
                     // subsequently closed.
    NothingWritten,  // We wrote nothing out, the connection has been closed (by
                     // remote side) or reached an error state before.
    Error
  };

  FlushStatus FlushWritingBuffer(std::size_t max_bytes);

  AbstractStreamIo::HandshakingStatus DoHandshake(bool from_on_readable);

 private:
  struct HandshakingState {
    std::atomic<bool> done{false};

    std::mutex lock;
    bool need_restart_read{true};  // Not enabled by default.
    bool pending_restart_writes{false};
  };

  Options options_;

  // Describes state of handshaking.
  HandshakingState handshaking_state_;

  // Accessed by reader.
  std::string read_buffer_;

  // Accessed by writers, usually a different thread.
   io::detail::WritingBufferList
      writing_buffers_;
};

}  // namespace tinyRPC

#endif  
