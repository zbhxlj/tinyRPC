#include "FiberEntity.h"
#include "../../../include/gtest/gtest.h"
#include "../../../include/glog/logging.h"

namespace tinyRPC::fiber::detail{

TEST(BasicFiberEntityTest, GetMaster) {
  SetUpMasterFiberEntity();
  auto* fiber = GetMasterFiberEntity();
  ASSERT_TRUE(!!fiber);
}

TEST(BasicFiberEntityTest, CreateDestroy) {
  auto fiber = CreateFiberEntity(nullptr, [] {}, nullptr);
  ASSERT_TRUE(!!fiber);
  FreeFiberEntity(fiber);
}

TEST(BasicFiberEntityTest, GetStackTop) {
  auto fiber = CreateFiberEntity(nullptr, [] {}, nullptr);
  ASSERT_TRUE(fiber->GetStackHighAddr());
  FreeFiberEntity(fiber);
}

TEST(BasicFiberEntityTest, Switch) {
  SetUpMasterFiberEntity();
  FiberEntity* master = GetMasterFiberEntity();
  int x = 0;
  auto fiber = CreateFiberEntity(nullptr, [&] {
    x = 10;
    // Jump back to the master fiber.
    master->Resume();
  }, nullptr);
  fiber->Resume();

  // Back from `cb`.
  ASSERT_EQ(10, x);
  FreeFiberEntity(fiber);
}

TEST(BasicFiberEntityTest, GetCurrent) {
  SetUpMasterFiberEntity();
  FiberEntity* master = GetMasterFiberEntity();
  ASSERT_EQ(master, GetCurrentFiberEntity());

  FiberEntity* ptr;
  auto fiber = CreateFiberEntity(nullptr, [&] {
    ASSERT_EQ(GetCurrentFiberEntity(), ptr);
    GetMasterFiberEntity()->Resume();  // Equivalent to `master->Resume().`
  }, nullptr);
  ptr = fiber;
  fiber->Resume();

  ASSERT_EQ(master, GetCurrentFiberEntity());  // We're back.
  FreeFiberEntity(fiber);
}

TEST(BasicFiberEntityTest, OnResume) {
  SetUpMasterFiberEntity();
  FiberEntity* master = GetMasterFiberEntity();
  struct Context {
    FiberEntity* expected;
    bool tested = false;
  };
  bool fiber_run = false;

  auto fiber = CreateFiberEntity(nullptr, [&] {
    GetMasterFiberEntity()->Resume();
    fiber_run = true;
    GetMasterFiberEntity()->Resume();
  }, nullptr);

  Context ctx;
  ctx.expected = fiber;
  fiber->Resume();  // We (master fiber) will be immediately resumed by
                    // `fiber_cb`.
  fiber->OnResume([&] {
    ASSERT_EQ(GetCurrentFiberEntity(), ctx.expected);
    ctx.tested = true;
  });

  ASSERT_TRUE(ctx.tested);
  ASSERT_TRUE(fiber_run);
  ASSERT_EQ(master, GetCurrentFiberEntity());
  FreeFiberEntity(fiber);
}

TEST(BasicFiberEntityTest, FiberLocalStorage) {
  SetUpMasterFiberEntity();
  FiberEntity* master = GetMasterFiberEntity();
  static const std::size_t kSlots[] = {
      0,
      1,
      5,
      9999,
  };

  for (auto slot_index : kSlots) {
    auto self = GetCurrentFiberEntity();

    auto&& slot = *self->GetFiberLocalStorage(slot_index);

    slot = MakeErased<int>(5);

    bool fiber_run = false;
    auto fiber = CreateFiberEntity(nullptr, [&] {
      auto self = GetCurrentFiberEntity();
      auto fls = self->GetFiberLocalStorage(slot_index);
      ASSERT_FALSE(*fls);

      GetMasterFiberEntity()->Resume();

      ASSERT_EQ(fls, self->GetFiberLocalStorage(slot_index));
      *fls = MakeErased<int>(10);

      GetMasterFiberEntity()->Resume();

      ASSERT_EQ(fls, self->GetFiberLocalStorage(slot_index));
      ASSERT_EQ(10, *reinterpret_cast<int*>(fls->Get()));
      fiber_run = true;

      GetMasterFiberEntity()->Resume();
    }, nullptr);

    ASSERT_EQ(self, GetMasterFiberEntity());
    auto fls = self->GetFiberLocalStorage(slot_index);
    ASSERT_EQ(5, *reinterpret_cast<int*>(fls->Get()));

    fiber->Resume();

    ASSERT_EQ(fls, self->GetFiberLocalStorage(slot_index));

    fiber->Resume();

    ASSERT_EQ(5, *reinterpret_cast<int*>(fls->Get()));
    ASSERT_EQ(fls, self->GetFiberLocalStorage(slot_index));
    *reinterpret_cast<int*>(fls->Get()) = 7;

    fiber->Resume();

    ASSERT_EQ(7, *reinterpret_cast<int*>(fls->Get()));
    ASSERT_EQ(fls, self->GetFiberLocalStorage(slot_index));

    ASSERT_TRUE(fiber_run);
    ASSERT_EQ(master, GetCurrentFiberEntity());
    FreeFiberEntity(fiber);
  }
}

TEST(BasicFiberEntityTest, ResumeOnMaster) {
  SetUpMasterFiberEntity();
  FiberEntity* master = GetMasterFiberEntity();
  struct Context {
    FiberEntity* expected;
    bool tested = false;
  };
  
  Context ctx;

  auto fiber = CreateFiberEntity(nullptr, [&] {
    master->OnResume([&] {
      ASSERT_EQ(GetCurrentFiberEntity(), ctx.expected);
      ctx.tested = true;
      // Continue running master fiber on return.
    });
  }, nullptr);

  ctx.expected = GetMasterFiberEntity();
  fiber->Resume();

  ASSERT_TRUE(ctx.tested);
  ASSERT_EQ(master, GetCurrentFiberEntity());
  FreeFiberEntity(fiber);
}

} // namespace tinyRPC::fiber::detail  

