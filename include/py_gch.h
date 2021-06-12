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
#include <stddef.h>
#include <stdio.h>

#ifndef PYGCH_API_UNIQ_SYMBOL
#define PYGCH_API_UNIQ_SYMBOL _PyGCH_API
#endif /* PYGCH_API_UNIQ_SYMBOL */

// typedef gc flags as size_t
typedef size_t PyGCH_flag_t;

// macros to give the addresses for each member provided by the API. these
// should not be used by users and considered internal to py_gch.h.
#define _PyGCH_addr_gc (PYGCH_API_UNIQ_SYMBOL + 0)
#define _PyGCH_addr_FinalizeEx (PYGCH_API_UNIQ_SYMBOL + 1)
#define _PyGCH_addr_gc_unique_import (PYGCH_API_UNIQ_SYMBOL + 2)
#define _PyGCH_addr_gc_member_unique_import (PYGCH_API_UNIQ_SYMBOL + 3)
#define _PyGCH_addr_gc_get_garbage (PYGCH_API_UNIQ_SYMBOL + 4)
#define _PyGCH_addr_gc_get_callbacks (PYGCH_API_UNIQ_SYMBOL + 6)
#define _PyGCH_addr_gc_get_flag (PYGCH_API_UNIQ_SYMBOL + 8)
#define _PyGCH_addr_gc_enable (PYGCH_API_UNIQ_SYMBOL + 14)
#define _PyGCH_addr_gc_disable (PYGCH_API_UNIQ_SYMBOL + 16)
#define _PyGCH_addr_gc_isenabled (PYGCH_API_UNIQ_SYMBOL + 18)
#define _PyGCH_addr_gc_collect_gen (PYGCH_API_UNIQ_SYMBOL + 20)
#define _PyGCH_addr_gc_COLLECT_GEN (PYGCH_API_UNIQ_SYMBOL + 22)

/*
 * macro for setting all initially NULL members in PYGCH_API_UNIQ_SYMBOL back
 * to NULL. this should never be called directly by a user, but exists since
 * py_gch.h functions determine whether or not to import a member from gc/gc
 * itself based on whether the corresponding element in PYGCH_APY_UNIQ_SYMBOL
 * is NULL or not. every time the interpreter is finalized, these elemenets
 * need to be set to NULL again since the pointer values become invalidated.
 */
#define _PyGCH_NULLIFY_API() \
  *_PyGCH_addr_gc = \
  *(_PyGCH_addr_gc_get_garbage + 1) = \
  *(_PyGCH_addr_gc_get_callbacks + 1) = \
  *(_PyGCH_addr_gc_get_flag + 1) = \
  *(_PyGCH_addr_gc_get_flag + 2) = \
  *(_PyGCH_addr_gc_get_flag + 3) = \
  *(_PyGCH_addr_gc_get_flag + 4) = \
  *(_PyGCH_addr_gc_get_flag + 5) = \
  *(_PyGCH_addr_gc_enable + 1) = \
  *(_PyGCH_addr_gc_disable + 1) = \
  *(_PyGCH_addr_gc_isenabled + 1) = \
  *(_PyGCH_addr_gc_collect_gen + 1) = \
  *(_PyGCH_addr_gc_COLLECT_GEN + 1) = NULL;
// replacements for Py_Finalize[Ex] that call _PyGCH_NULLFIY_API. note that we
// cannot use a macro for Py_FinalizeEx if we want to keep the return value.
#define PyGCH_Finalize() \
  Py_Finalize(); \
  _PyGCH_NULLIFY_API();
#define PyGCH_FinalizeEx \
  (*(int (*)(void)) *_PyGCH_addr_FinalizeEx)
// macros that give names to the py_gch.h API utility functions
#define PyGCH_gc_imported() \
  (((PyObject *) *_PyGCH_addr_gc == NULL) ? false : true)
#define PyGCH_gc_unique_import \
  (*(int (*)(void)) *_PyGCH_addr_gc_unique_import)
#define PyGCH_gc_member_unique_import \
  (*(int (*)(char const *, PyObject **)) *_PyGCH_addr_gc_member_unique_import)
// macro to give name to flag retrieval function
#define _PyGCH_gc_get_flag \
  (*(PyGCH_flag_t (*)(char const *, void **)) *_PyGCH_addr_gc_get_flag)
// macros for the gc flags. these work even if gc flag values change.
#define PyGCH_gc_DEBUG_STATS \
  _PyGCH_gc_get_flag("DEBUG_STATS", _PyGCH_addr_gc_get_flag + 1)
#define PyGCH_gc_DEBUG_COLLECTABLE \
  _PyGCH_gc_get_flag("DEBUG_COLLECTABLE", _PyGCH_addr_gc_get_flag + 2)
#define PyGCH_gc_DEBUG_UNCOLLECTABLE \
  _PyGCH_gc_get_flag("DEBUG_UNCOLLECTABLE", _PyGCH_addr_gc_get_flag + 3)
#define PyGCH_gc_DEBUG_SAVEALL \
  _PyGCH_gc_get_flag("DEBUG_SAVEALL", _PyGCH_addr_gc_get_flag + 4)
#define PyGCH_gc_DEBUG_LEAK \
  _PyGCH_gc_get_flag("DEBUG_LEAK", _PyGCH_addr_gc_get_flag + 5)
// macro to give names to gc.garbage, gc.callbacks retrieval functions
#define _PyGCH_gc_get_garbage \
  (*(PyObject *(*)(void)) *_PyGCH_addr_gc_get_garbage)
#define _PyGCH_gc_get_callbacks \
  (*(PyObject *(*)(void)) *_PyGCH_addr_gc_get_callbacks)
// macros for gc.garbage, gc.callbacks borrowed references
#define PyGCH_gc_garbage _PyGCH_gc_get_garbage()
#define PyGCH_gc_callbacks _PyGCH_gc_get_callbacks()
// macros corresponding to py_gch.h API functions that wrap gc functions
#define PyGCH_gc_enable \
  (*(PyObject *(*)(void)) *_PyGCH_addr_gc_enable)
#define PyGCH_gc_disable \
  (*(PyObject *(*)(void)) *_PyGCH_addr_gc_disable)
#define PyGCH_gc_isenabled \
  (*(PyObject *(*)(void)) *_PyGCH_addr_gc_isenabled)
#define PyGCH_gc_collect_gen \
  (*(PyObject *(*)(Py_ssize_t)) *_PyGCH_addr_gc_collect_gen)
#define PyGCH_gc_COLLECT_GEN \
  (*(Py_ssize_t (*)(Py_ssize_t)) *_PyGCH_addr_gc_COLLECT_GEN)
#define PyGCH_gc_collect() PyGCH_gc_collect_gen(-1)
#define PyGCH_gc_COLLECT() PyGCH_gc_COLLECT_GEN(-1)

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
static PyObject *gc_get_garbage(void);
static PyObject *gc_get_callbacks(void);
static PyGCH_flag_t gc_get_flag(char const *, void **);
static PyObject *gc_enable(void);
static PyObject *gc_disable(void);
static PyObject *gc_isenabled(void);
static PyObject *gc_collect_gen(Py_ssize_t);
static Py_ssize_t gc_COLLECT_GEN(Py_ssize_t);

/*
 * note: API scheduled for big overhaul! individual wrappers are to also take
 * a void ** address for the PyObject * for the PyCFunction or Python member
 * they are referring to, which will allow fewer hardcoding of API indices in
 * the code (these are a pain to update). only the macros will have hardcoded
 * indices. this design decision should have been made earlier.
 */

// void ** holding the entire API
void *PYGCH_API_UNIQ_SYMBOL[] = {
  // pointer to hold the gc module
  NULL,
  // wrapper for Py_FinalizeEx that correctly NULL-ifies the NULL API elements
  (void *) _PyGCH_FinalizeEx,
  // utility functions provided by py_gch.h
  (void *) gc_unique_import,
  (void *) gc_member_unique_import,
  // functions used to retrieve gc.garbage and gc.callbacks. the NULL elements
  // are the slots to cache PyObject * to gc.garbage, gc.callbacks
  (void *) gc_get_garbage, NULL,       /* 4, 5 */
  (void *) gc_get_callbacks, NULL,     /* 6, 7 */
  // function to get flag values from gc + pointers to cache the flag values
  (void *) gc_get_flag,
  NULL, NULL, NULL, NULL, NULL,        /* 9, 10, 11, 12, 13 */
  /*
   * function pointer pairs for the API. each two elements composes a "unit",
   * where each unit is the pair (function pointer, PyObject *). The PyObject *
   * represents the imported function/member from gc and the function pointer
   * is its corresponding C API retrieval function provided by py_gch.h.
   */
  (void *) gc_enable, NULL,            /* 14, 15 */    /* C func, PyObject * */
  (void *) gc_disable, NULL,           /* 16, 17 */
  (void *) gc_isenabled, NULL,         /* 18, 19 */
  (void *) gc_collect_gen, NULL,       /* 20, 21 */
  (void *) gc_COLLECT_GEN, NULL,       /* 22, 23 (23 always NULL) */
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
  if (*_PyGCH_addr_gc == NULL) {
    *_PyGCH_addr_gc = (void *) PyImport_ImportModule("gc");
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
  if (!gc_unique_import()) {
    return false;
  }
  // if *dest is not NULL, assume that member has already been imported
  if (*dest != NULL) {
    return true;
  }
  // get gc attribute and write to dest. if err, *dest is NULL + exception set
  *dest = PyObject_GetAttrString((PyObject *) *_PyGCH_addr_gc, member_name);
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
    !gc_member_unique_import(
      "enable", (PyObject **) (_PyGCH_addr_gc_enable + 1)
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
    (PyObject *) *(_PyGCH_addr_gc_enable + 1), NULL
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
    !gc_member_unique_import(
      "disable", (PyObject **) (_PyGCH_addr_gc_disable + 1)
    )
  ) {
    return NULL;
  }
  // call gc.disable and Py_XDECREF its return value before returning
  PyObject *py_return = PyObject_CallObject(
    (PyObject *) *(_PyGCH_addr_gc_disable + 1), NULL
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
    !gc_member_unique_import(
      "isenabled", (PyObject **) (_PyGCH_addr_gc_isenabled + 1)
    )
  ) {
    return NULL;
  }
  /**
   * call gc.isenabled and Py_XDECREF its return value for borrowed ref. like
   * Py_None, Py_True and Py_False start out with reference counts of 1.
   */
  PyObject *py_return = PyObject_CallObject(
    (PyObject *) *(_PyGCH_addr_gc_isenabled + 1), NULL
  );
  Py_XDECREF(py_return);
  return py_return;
}

/**
 * Runs collection on the specified generation.
 * 
 * @param gen Generation to run collection on, `0` to `2` inclusive. Pass `-1`
 *     to run a full collection, same as invoking `gc.collect` without args.
 * @returns New reference to a `PyLongObject` giving the number of unreachable
 *     objects or `NULL` on failure, where an exception will be set.
 */
static PyObject *
gc_collect_gen(Py_ssize_t gen) {
  // get gc.collect if pointer is NULL. exception set on error
  if (
    !gc_member_unique_import(
      "collect", (PyObject **) (_PyGCH_addr_gc_collect_gen + 1)
    )
  ) {
    return NULL;
  }
  // if gen == -1, then call without arguments and return value. note that we
  // must clear error indicator, as set error indicator causes fatal crash
  if (gen == -1) {
    PyErr_Clear();
    return PyObject_CallObject(
      (PyObject *) *(_PyGCH_addr_gc_collect_gen + 1), NULL
    );
  }
  // new argument tuple to pass to gc.collect
  PyObject *args = PyTuple_New(1);
  if (args == NULL) {
    return NULL;
  }
  // convert gen to PyLongObject. on error, return NULL (exception set)
  PyObject *gen_ = PyLong_FromSsize_t(gen);
  if (gen_ == NULL) {
    return NULL;
  }
  // steal reference using PyTuple_SET_ITEM
  PyTuple_SET_ITEM(args, 0, gen_);
  // call gc.collect with args and get return value (NULL on error). again,
  // must clear error indicator (if set) to prevent fatal crash
  PyErr_Clear();
  PyObject *res = PyObject_CallObject(
    (PyObject *) *(_PyGCH_addr_gc_collect_gen + 1), args
  );
  // clean up and return
  Py_DECREF(args);
  return res;
}

/**
 * `gc_collect_gen` wrapper that returns `Py_ssize_t` instead of `PyObject *`.
 * 
 * @param gen Generation to run collection on, `0` to `2` inclusive. Pass `-1`
 *     to run a full collection, same as invoking `gc.collect` without args.
 * @returns The nonnegative number of unreachable objects or `-1` on error.
 *     An exception will be automatically set on error.
 */
static Py_ssize_t
gc_COLLECT_GEN(Py_ssize_t gen) {
  // get result from gc_collect_gen. if NULL, return -1 (error indicator set)
  PyObject *res_ = gc_collect_gen(gen);
  if (res_ == NULL) {
    return -1;
  }
  // convert back to Py_ssize_t. -1 on error (indicator set).
  Py_ssize_t res = PyLong_AsSsize_t(res_);
  // Py_DECREF res_ and return res (-1 on error)
  Py_DECREF(res_);
  return res;
}

/**
 * Special function for importing a flag from `gc`.
 * 
 * Instead of the pointer to the member itself being stored in the `void **`
 * API, on successful importing and conversion to `size_t` the flag itself
 * is cast to `void *` and stored in the API. this way, retrieval is faster
 * than constantly re-converting the member.
 * 
 * @param flag_name Name of the flag to retrieve
 * @param api_addr `void **` address to store the flag value as a `void *`
 * @returns Flag value as a `PyGCH_flag_t` or `0` on failure with set exception.
 *     If a flag is supposed to be zero, use `PyErr_Occurred` to check whether
 *     an exception has been set or not.
 */
static PyGCH_flag_t
gc_get_flag(char const *flag_name, void **api_addr) {
  // if *api_addr is not NULL, cast value to PyGCH_flag_t and return
  if (*api_addr != NULL) {
    return (PyGCH_flag_t) *api_addr;
  }
  // else get gc.flag_name if *api_addr is NULL. exception set on error
  if (!gc_member_unique_import(flag_name, (PyObject **) api_addr)) {
    return 0;
  }
  /*
   * cast to unsigned long, Py_DECREF *api_addr, and set it to NULL. we do
   * this since the ref is no longer needed and in case conversion fails so
   * that this function can be called again to re-import the flag member.
   */
  PyGCH_flag_t flag = PyLong_AsUnsignedLongMask((PyObject *) *api_addr);
  Py_DECREF((PyObject *) *api_addr);
  *api_addr = NULL;
  // -1 and exception set on error.
  if (flag == (unsigned long) -1) {
    return 0;
  }
  // write flag to api_addr, casting it to a void *, and return flag value
  *api_addr = (void *) flag;
  return flag;
}

/**
 * Returns a borrowed reference to `gc.garbage`.
 * 
 * After the initial retrieval of `gc.garbage` using `PyObject_GetAttrString`,
 * further calls to `gc_get_garbage` return a cached pointer to `gc.garbage`.
 * 
 * @returns Borrowed reference to `gc.garbage`, with an exception set and
 *     `NULL` returned on error.
 */
static PyObject *
gc_get_garbage(void) {
  // if pointer is not NULL, just return its value
  if (*(_PyGCH_addr_gc_get_garbage + 1) != NULL) {
    return (PyObject *) *(_PyGCH_addr_gc_get_garbage + 1);
  }
  // get gc.garbage if pointer is NULL. exception set on error
  if (
    !gc_member_unique_import(
      "garbage", (PyObject **) (_PyGCH_addr_gc_get_garbage + 1)
    )
  ) {
    return NULL;
  }
  // return pointer to gc.garbage
  return (PyObject *) *(_PyGCH_addr_gc_get_garbage + 1);
}

/**
 * Returns a borrowed reference to `gc.callbacks`.
 * 
 * After initial retrieval of `gc.callbacks` using `PyObject_GetAttrString`,
 * further calls to `gc_get_callbacks` return a cached pointer to `gc.garbage`.
 * 
 * @returns Borrowed reference to `gc.callbacks`, with an exception set and
 *     `NULL` returned on error.
 */
static PyObject *
gc_get_callbacks(void) {
  // if pointer is not NULL, just return its value
  if (*(_PyGCH_addr_gc_get_callbacks + 1) != NULL) {
    return (PyObject *) *(_PyGCH_addr_gc_get_callbacks + 1);
  }
  // get gc.garbage if pointer is NULL. exception set on error
  if (
    !gc_member_unique_import(
      "callbacks", (PyObject **) (_PyGCH_addr_gc_get_callbacks + 1)
    )
  ) {
    return NULL;
  }
  // return pointer to gc.garbage
  return (PyObject *) *(_PyGCH_addr_gc_get_callbacks + 1);
}

#endif /* PYGCH_NO_DEFINE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PY_GCH_H */