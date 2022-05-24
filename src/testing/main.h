#ifndef _SRC_TESTING_MAIN_H_
#define _SRC_TESTING_MAIN_H_

namespace tinyRPC::testing {

// Do a full initialization and run all tests (in worker pool).
int InitAndRunAllTests(int* argc, char** argv);

// It's possible to define a "weak" `main` and somehow instruct building system
// to link to our `main`, so as to provide end user with the same experience as
// of gtest (not requiring users to define `main` at all).
//
// But it's just too intrusive..
//
// If you need to do some initialization yourself before running UTs, call
// `InitAndRunAllTests(...)` (see above) yourself.
#define TINYRPC_TEST_MAIN                                       \
  int main(int argc, char** argv) {                           \
    return ::tinyRPC::testing::InitAndRunAllTests(&argc, argv); \
  }

}  // namespace tinyRPC::testing

#endif  
