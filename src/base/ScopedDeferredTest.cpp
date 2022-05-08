#include <gtest/gtest.h>

#include "ScopedDeferred.h"

namespace tinyRPC{
    TEST(ScopedDeferred, All){
        bool state = true;
        {
            // Class template argument deduction.
            ScopedDeferred defer([&state](){ state = false; });
            ASSERT_EQ(true, state);
        }
        ASSERT_EQ(false, state);
    }
}
