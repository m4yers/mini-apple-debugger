#include "gtest/gtest.h"

TEST(debugger_true_test, TrueIsTrue) { ASSERT_TRUE(true); }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}