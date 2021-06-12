/**
 * @file gtest_helpers.h
 * @brief Test fixtures and useful macros to use with Google Test.
 */

#ifndef GTEST_HELPERS_H
#define GTEST_HELPERS_H

#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#endif /* PY_SSIZE_T_CLEAN */

extern "C" {
#include <stdio.h>
}

#include "gtest/gtest.h"

using namespace std;

namespace gtest_helpers {

// fixture for tests that require the Python C API
class PyReqTest: public testing::Test {
  protected:
  // calls Py_Initialize to start the (debug) Python interpreter
  void SetUp() override {
    Py_Initialize();
    // print error if not initialized
    if (!Py_IsInitialized()) {
      fprintf(
        stderr, "%s: fatal: Python interpreter not initialized\n", __func__
      );
      return;
    }
  }
  // finalizes the interpreter
  void TearDown() override {
    // print to stderr if there's an issue but don't stop
    if (Py_FinalizeEx() < 0) {
      // TestInfo point
      const testing::TestInfo * const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
      // if NULL, there is no test running
      if (test_info == NULL) {
        fprintf(stderr, "%s: fatal: no test running\n", __func__);
        return;
      }
      // otherwise we have the name of the suite and the test name
      fprintf(
        stderr, "%s::%s: Py_FinalizeEx error\n",
        test_info->test_suite_name(), test_info->name()
      );
    }
  }
};

} /* gtest_helpers */

#endif /* GTEST_HELPERS_H */