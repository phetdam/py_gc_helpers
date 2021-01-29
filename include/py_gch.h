/**
 * @file py_gch.h
 * @brief A lightweight API for accessing Python `gc` from C/C++.
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

/*
 * static PyObject * for the gc module and its members. see the note at the
 * top of the file for an explanation on PYGCH_NO_DEFINE.
 */
#ifdef PYGCH_NO_DEFINE
extern PyObject
  *PyGCH_mod_o, *PyGCH_enable_o, *PyGCH_disable_o, *PyGCH_isenabled_o;
#else
PyObject *PyGCH_mod_o = NULL;
PyObject *PyGCH_enable_o = NULL;
PyObject *PyGCH_disable_o = NULL;
PyObject *PyGCH_isenabled_o = NULL;
#endif /* PYGCH_NO_DEFINE */

/**
 * Returns `1` if `gc` has been imported, `0` otherwise.
 */
#define PyGCH_gc_imported() (PyGCH_mod_o == NULL) ? false : true

#ifdef PYGCH_NO_DEFINE
int PyGCH_gc_unique_import(void);
int PyGCH_gc_member_unique_import(char const *, PyObject **);
PyObject *PyGCH_gc_enable(void);
PyObject *PyGCH_gc_disable(void);
PyObject *PyGCH_gc_isenabled(void);
/*
Py_ssize_t PyGCH_gc_collect(void);
Py_ssize_t PyGCH_gc_collect_gen(int);
*/
#else
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
int PyGCH_gc_unique_import(void) {
  if (PyGCH_mod_o == NULL) {
    PyGCH_mod_o = PyImport_ImportModule("gc");
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
 * @returns `1` on sucess (`gc` member imported), `0` on failure.
 */
int PyGCH_gc_member_unique_import(char const *member_name, PyObject **dest) {
  // if we haven't imported gc, return false (exception will be set)
  if (!PyGCH_gc_unique_import()) {
    return false;
  }
  // if *dest is not NULL, assume that member has already been imported
  if (*dest != NULL) {
    return true;
  }
  // get attribute and write to dest. if err, *dest is NULL + exception set
  *dest = PyObject_GetAttrString(PyGCH_mod_o, member_name);
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
PyObject *PyGCH_gc_enable(void) {
  // get gc.enable if PyGCH_enable_o is NULL. exception set on error
  if (!PyGCH_gc_member_unique_import("enable", &PyGCH_enable_o)) {
    return;
  }
  // call gc.enable and return its return value
  return PyObject_CallObject(PyGCH_enable_o, NULL);
}

/**
 * Disables the Python garbage collector.
 * 
 * Sets a Python exception and returns `NULL` if `gc` or `gc.disable` cannot be
 * imported.
 * 
 * @returns New reference to `Py_None` on success, `NULL` on failure.
 */
PyObject *PyGCH_gc_disable(void) {
  // get gc.disable if PyGCH_disable_o is NULL. exception set on error
  if (!PyGCH_gc_member_unique_import("disable", &PyGCH_disable_o)) {
    return;
  }
  // call gc.disable and return its return value
  return PyObject_CallObject(PyGCH_disable_o, NULL);
}

/**
 * Checks if garbage collection is enabled.
 * 
 * Sets a Python exception and returns `NULL` if `gc` or `gc.isenabled` cannot
 * be imported.
 * 
 * @returns New reference to `Py_True` if garbage collection is enabled, new
 *     reference to `Py_False` if collection is disabled, `NULL` on error.
 */
PyObject *PyGCH_gc_isenabled(void) {
  // get gc.isenabled if PyGCH_isenabled_o is NULL. exception set on error
  if (!PyGCH_gc_member_unique_import("disable", &PyGCH_isenabled_o)) {
    return NULL;
  }
  // call gc.isenabled and return its return value
  return PyObject_CallObject(PyGCH_disable_o, NULL);
}
#endif /* PYGCH_NO_DEFINE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PY_GCH_H */