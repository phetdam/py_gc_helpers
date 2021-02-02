/**
 * @file py_gch_suite.c
 * @brief Creates test suite for the API provided by `py_gch.h`.
 */

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <stdio.h>

#include <check.h>

#include "py_gch.h"
#define CHECK_HELPERS_NO_DEFINE
#include "check_helpers.h"

// macros that make the debugging process easier
#define gc_module_ PYGCH_API_UNIQ_SYMBOL[0]
#define gc_module ((PyObject *) gc_module_)

/**
 * Test that `PyGCH_unique_import` really imports `gc` once.
 * 
 * @note Reference counting is sloppy here, as on failure there are a couple
 *     places where `Py_DECREF` is necessary.
 */
PY_C_API_REQUIRED START_TEST(test_unique_import) {
  // set gc_module to NULL so this test is reproducible. we need the separate
  // gc_module_ macro since the cast in gc_module doesn't let us write.
  gc_module_ = NULL;
  // run unique import. it should return true
  ck_assert_msg(PyGCH_gc_unique_import(), "PyGCH_unique_import failed (1)");
  // current reference count to the gc module (isn't necessarily 1)
  Py_ssize_t gc_refcnt = Py_REFCNT(gc_module);
  // run again to see if the reference count changes
  ck_assert_msg(PyGCH_gc_unique_import(), "PyGCH_unique_import failed (2)");
  // reference count should not have changed. if it has, we should do cleanup,
  // but that's ok since the interpreter is going to be finalized anyways.
  ck_assert_double_eq(Py_REFCNT(gc_module), gc_refcnt);
} END_TEST

/**
 * Create test suite `"py_gch_suite"` using static tests defined above.
 * 
 * The `"py_core"` test case uses the `py_setup` and `py_teardown` functions
 * provided by `check_helpers.h` to set up a checked fixture (runs in forked
 * address space at the start and end of each unit test, so if the Python
 * interpreter gets killed we can get a fresh one for subsequent tests).
 * 
 * @param timeout `double` number of seconds for the test case's timeout
 * @returns libcheck `Suite *`, `NULL` on error
 */
Suite *make_py_gch_suite(double timeout) {
  // if timeout is nonpositive, print error and return NULL
  if (timeout <= 0) {
    fprintf(stderr, "error: %s: timeout must be positive\n", __func__);
    return NULL;
  }
  // create suite + add test case
  Suite *suite = suite_create("py_gch_suite");
  TCase *tc_core = tcase_create("py_core");
  // set test case timeout
  tcase_set_timeout(tc_core, timeout);
  // add py_setup and py_teardown to tc_core test case (required)
  tcase_add_checked_fixture(tc_core, py_setup, py_teardown);
  // register cases together with tests, add cases to suite, and return suite
  tcase_add_test(tc_core, test_unique_import);
  suite_add_tcase(suite, tc_core);
  return suite;
}