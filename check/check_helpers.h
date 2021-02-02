/**
 * @file check_helpers.h
 * @brief Useful declarations for unit testing mixed C/Python code with Check.
 *     Must `#define CHECK_HELPERS_NO_DEFINE` for all source files used to
 *     build the test runner except for one.
 */

#ifndef CHECK_HELPERS_H
#define CHECK_HELPERS_H

#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#endif /* PY_SSIZE_T_CLEAN */

// empty macros to tag functions that require/don't require Python C API
#define PY_C_API_REQUIRED
#define NO_PY_C_API

/**
 * calls Py_FinalizeEx with error handling controlled by Py_Finalize_err_stop.
 * optionally can return a value ret from the function it is called from if
 * Py_FinalizeEx errors. the typical way to use the macro is
 * 
 * Py_FinalizeEx_handle_err(return_this_on_error)
 * return the_normal_return_value;
 */
#define Py_FinalizeEx_handle_err(ret) \
  if (Py_FinalizeEx() < 0) { \
    fprintf(stderr, "error: %s: Py_FinalizeEx error\n", __func__); \
    if (Py_Finalize_err_stop) { exit(120); } else { return ret; } \
  }

#ifdef CHECK_HELPERS_NO_DEFINE
extern int Py_Finalize_err_stop;
void py_setup(void);
void py_teardown(void);
#else
// whether to exit the test runner immediately if Py_FinalizeEx returns an
// error. set to false by default so other tests can run.
int Py_Finalize_err_stop;

/**
 * Python interpreter fixture setup to allow use of the Python C API
 */
void py_setup(void) {
  Py_Initialize();
}

/**
 * Python interpreter fixture teardown to finalize interpreter
 */
void py_teardown(void) {
  Py_FinalizeEx_handle_err()
}
#endif /* CHECK_HELPERS_NO_DEFINE */

#endif /* CHECK_HELPERS_H */