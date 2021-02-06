/**
 * @file py_gch.h
 * @brief A lightweight API for accessing Python `gc` from C/C++.
 * @note Before including `py_gch.h`, it is highly recommended to
 *     `#define PYGCH_API_UNIQ_SYMBOL <unique_tag>` to minimize any naming
 *     conflicts when linking with external libraries. Python C extension
 *     modules are typically advised to define all their members, except for
 *     the `PyInit_*` function, to be `static`. Name clashes can happen within
 *     your code however if you use a name already defined by `py_gch.h`.
 * @note When including `py_gch.h` in multiple files, users must
 *     `#define PYGCH_NO_DEFINE` before the `#include` in all files except one.
 * 
 * Lightweight API for accessing some members in Python's `gc` module from C
 * extension modules or C/C++ code embedding the Python interpreter. There is
 * no public C API provided by the `gc` C extension module (see
 * `cpython/Modules/gcmodule.c`) which is why this header file was created.
 * 
 * @note `gc_disable_impl`, `gc_enable_impl` defined in `gcmodule.c` which
 *     disable and enable garbage collection, respectively, return `Py_None`.
 */

#ifndef PY_GCH_H
#define PY_GCH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#endif /* PY_SSIZE_T_CLEAN */

#include <stdbool.h>
#include <stdio.h>

#ifndef PYGCH_API_UNIQ_SYMBOL
#define PYGCH_API_UNIQ_SYMBOL _PyGCH_API
#endif /* PYGCH_API_UNIQ_SYMBOL */

/*
 * macro for setting all initially NULL members in PYGCH_API_UNIQ_SYMBOL back
 * to NULL. this should never be called directly by a user, but exists since
 * py_gch.h functions determine whether or not to import a member from gc/gc
 * itself based on whether the corresponding element in PYGCH_APY_UNIQ_SYMBOL
 * is NULL or not. every time the interpreter is finalized, these elemenets
 * need to be set to NULL again since the pointer values become invalidated.
 */
#define _PyGCH_NULLIFY_API() \
  PYGCH_API_UNIQ_SYMBOL[0] = PYGCH_API_UNIQ_SYMBOL[5] = \
  PYGCH_API_UNIQ_SYMBOL[7] = PYGCH_API_UNIQ_SYMBOL[9] = NULL;
// replacements for Py_Finalize[Ex] that call _PyGCH_NULLFIY_API. note that we
// cannot use a macro for Py_FinalizeEx if we want to keep the return value.
#define PyGCH_Finalize() \
  Py_Finalize(); \
  _PyGCH_NULLIFY_API();
#define PyGCH_FinalizeEx \
  (*(int (*)(void)) PYGCH_API_UNIQ_SYMBOL[1])
// macros that give names to the py_gch.h API
#define PyGCH_gc_imported() \
  (((PyObject *) PYGCH_API_UNIQ_SYMBOL[0] == NULL) ? false : true)
#define PyGCH_gc_unique_import \
  (*(int (*)(void)) PYGCH_API_UNIQ_SYMBOL[2])
#define PyGCH_gc_member_unique_import \
  (*(int (*)(char const *, PyObject **)) PYGCH_API_UNIQ_SYMBOL[3])
// all gc function/member macros should have odd indices
#define PyGCH_gc_enable \
  (*(PyObject *(*)(void)) PYGCH_API_UNIQ_SYMBOL[4])
#define PyGCH_gc_disable \
  (*(PyObject *(*)(void)) PYGCH_API_UNIQ_SYMBOL[6])
#define PyGCH_gc_isenabled \
  (*(PyObject *(*)(void)) PYGCH_API_UNIQ_SYMBOL[8])

#ifdef PYGCH_NO_DEFINE
extern void **PYGCH_API_UNIQ_SYMBOL
#else
/*
 * We need to declare all the functions before defining the void **
 * PYGCH_API_UNIQ_SYMBOL array or else the compiler will give an error.
 */
static int _PyGCH_FinalizeEx(void);
static int gc_unique_import(void);
static int gc_member_unique_import(char const *, PyObject **);
static PyObject *gc_enable(void);
static PyObject *gc_disable(void);
static PyObject *gc_isenabled(void);

// void ** holding the entire API
void *PYGCH_API_UNIQ_SYMBOL[] = {
  // pointer to hold the gc module
  NULL,
  // wrapper for Py_FinalizeEx that correctly NULL-ifies the NULL API elements
  (void *) _PyGCH_FinalizeEx,
  // utility functions provided by py_gch.h
  (void *) gc_unique_import,
  (void *) gc_member_unique_import,
  /*
   * function pointers for the API. each two elements composes a "unit", where
   * each unit is the pair (function pointer, PyObject *). The PyObject *
   * represents the imported function from gc and the function pointer is its
   * corresponding C API function provided by py_gch.h.
   */
  (void *) gc_enable, NULL,
  (void *) gc_disable, NULL,
  (void *) gc_isenabled, NULL
};

/**
 * Wrapper around `Py_FinalizeEx` that calls `_PyGCH_NULLIFY_API`.
 * 
 * @returns `-1` on error, `0` otherwise.
 */
static int
_PyGCH_FinalizeEx(void) {
  int ret = Py_FinalizeEx();
  _PyGCH_NULLIFY_API();
  return ret;
}

/**
 * Imports `gc` if not imported, else no-op.
 * 
 * A Python exception is set on failure.
 * 
 * @note May only be called from within a C extension module or after
 *     `Py_Initialize` has been called.
 * 
 * @returns `1` on success (`gc` imported), `0` on failure.
 */
static int
gc_unique_import(void) {
  if (PYGCH_API_UNIQ_SYMBOL[0] == NULL) {
    PYGCH_API_UNIQ_SYMBOL[0] = (void *) PyImport_ImportModule("gc");
  }
  return PyGCH_gc_imported();
}

/**
 * Imports a member from `gc` to address `dest` if not imported, else no-op.
 * 
 * `gc` is imported with PyGCH_gc_unique_import(void) if it has not yet been
 * imported, and if `gc` can't be imported, a Python exception is set and the
 * function will return `0`.
 * 
 * @note `dest` must be such that `*dest == NULL`. Otherwise, the function will
 *     return `1` but without actually importing the member from `gc`.
 * 
 * @param member_name Name of the member from `gc` to import
 * @param dest Address of the `PyObject *` to store the reference to the member
 * @returns `1` on success (`gc` member imported), `0` on failure.
 */
static int
gc_member_unique_import(char const *member_name, PyObject **dest) {
  // if we haven't imported gc, return false (exception will be set)
  if (!PyGCH_gc_unique_import()) {
    return false;
  }
  // if *dest is not NULL, assume that member has already been imported
  if (*dest != NULL) {
    return true;
  }
  // get attribute and write to dest. if err, *dest is NULL + exception set
  *dest = PyObject_GetAttrString(
    (PyObject *) PYGCH_API_UNIQ_SYMBOL[0], member_name
  );
  if (*dest == NULL) {
    return false;
  }
  return true;
}

/**
 * Enables the Python garbage collector.
 * 
 * Sets a Python exception and returns `NULL` if `gc` or `gc.enable` cannot be
 * imported.
 * 
 * @returns New reference to `Py_None` on success, `NULL` on failure.
 */
static PyObject *
gc_enable(void) {
  // get gc.enable if pointer is NULL. exception set on error
  if (
    !PyGCH_gc_member_unique_import(
      "enable", (PyObject **) (PYGCH_API_UNIQ_SYMBOL + 5)
    )
  ) {
    return NULL;
  }
  /**
   * call gc.enable and Py_XDECREF its return value before returning.
   * ordinarily this would be a horrible error, but since Py_None starts with a
   * reference count of 1, this is not a horrible thing to do.
   */
  PyObject *py_return = PyObject_CallObject(
    (PyObject *) PYGCH_API_UNIQ_SYMBOL[5], NULL
  );
  Py_XDECREF(py_return);
  return py_return;
}

/**
 * Disables the Python garbage collector.
 * 
 * Sets a Python exception and returns `NULL` if `gc` or `gc.disable` cannot be
 * imported.
 * 
 * @returns Borrowed reference to `Py_None` on success, `NULL` on failure.
 */
static PyObject *
gc_disable(void) {
  // get gc.disable if pointer is NULL. exception set on error
  if (
    !PyGCH_gc_member_unique_import(
      "disable", (PyObject **) (PYGCH_API_UNIQ_SYMBOL + 7)
    )
  ) {
    return NULL;
  }
  // call gc.disable and Py_XDECREF its return value before returning
  PyObject *py_return = PyObject_CallObject(
    (PyObject *) PYGCH_API_UNIQ_SYMBOL[7], NULL
  );
  Py_XDECREF(py_return);
  return py_return;
}

/**
 * Checks if garbage collection is enabled.
 * 
 * Sets a Python exception and returns `NULL` if `gc` or `gc.isenabled` cannot
 * be imported.
 * 
 * @returns Borrowed reference to `Py_True` if garbage collection is enabled,
 *     borrowed reference to `Py_False` if garbage collection is disabled,
 *     otherwise `NULL` on error.
 */
static PyObject *
gc_isenabled(void) {
  // get gc.isenabled if pointer is NULL. exception set on error
  if (
    !PyGCH_gc_member_unique_import(
      "isenabled", (PyObject **) (PYGCH_API_UNIQ_SYMBOL + 9)
    )
  ) {
    return NULL;
  }
  /**
   * call gc.isenabled and Py_XDECREF its return value for borrowed ref. like
   * Py_None, Py_True and Py_False start out with reference counts of 1.
   */
  PyObject *py_return = PyObject_CallObject(
    (PyObject *) PYGCH_API_UNIQ_SYMBOL[9], NULL
  );
  Py_XDECREF(py_return);
  return py_return;
}

#endif /* PYGCH_NO_DEFINE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PY_GCH_H */