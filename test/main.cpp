/**
 * @file main.cpp
 *
 * @brief Unit test entry point.
 */

#include "test/common.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
