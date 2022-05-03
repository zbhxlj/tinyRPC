//
// Created by zbh on 5/3/22.
//

#include "glog/logging.h"
#include "glog/raw_logging.h"

#include "gtest/gtest.h"

TEST(GLog, Test) {
    VLOG(1) << "VLOG(1)";
    VLOG(2) << "VLOG(2)";
    LOG(INFO) << "INFO";
    LOG(WARNING) << "WARNING";
    LOG(ERROR) << "ERROR";
    RAW_LOG(ERROR, "RAW_LOG(ERROR)");
    RAW_LOG(INFO, "sssssssss %d", 256);
}

TEST(GLogDeathTest, Fatal) { ASSERT_DEATH((LOG(FATAL) << "sssssssss"), ""); }
