/**
 * @file py_gc_helpers.h
 * @brief Macros for enabling/disabling Python garbage collection from C/C++.
 */

#ifndef GCHELPERS_H
#define GCHELPERS_H
//py_gch.h
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#include "Python.h"
#endif /* PY_SSIZE_T_CLEAN */

#include <stdio.h>

/**
 * Unique prefix for variables defined in this file. It is recommended to
 * `#define GCHELPERS_UNIQ_PREFIX` to something you know is unique for your
 * code in order to prevent any naming conflicts.
 */
#ifndef GCHELPERS_UNIQ_PREFIX
#define GCHELPERS_UNIQ_PREFIX _GcHelpers_Unique_Prefix
#endif /* GCHELPERS_UNIQ_PREFIX */

// helper macros for macro-expanding token pasting
#define _BI_TOKEN_PASTE(x, y) x ## y
#define _XBI_TOKEN_PASTE(x, y) _BI_TOKEN_PASTE(x, y)
/**
 * Unique name for the `static PyObject *` holding the compilation unit's
 * reference to the Python `gc` module.
 */
#define GCHELPERS_GC_MOD _XBI_TOKEN_PASTE(GCHELPERS_UNIQ_PREFIX, _gc_mod)
/**
 * Unique name for the `static PyObject *` holding `gc.enable`
 */
#define GCHELPERS_GC_ENABLE \
  _XBI_TOKEN_PASTE(GCHELPERS_UNIQ_PREFIX, _gc_enable)
/**
 * Unique name for the `static PyObject *` holding `gc.disable`
 */
#define GCHELPERS_GC_DISABLE \
  _XBI_TOKEN_PASTE(GCHELPERS_UNIQ_PREFIX, _gc_disable)

// `static PyObject *` for module, gc.enable, gc.disable
static PyObject *GCHELPERS_GC_MOD = NULL;
static PyObject *GCHELPERS_GC_ENABLE = NULL;
static PyObject *GCHELPERS_GC_DISABLE = NULL;

/*
 * macros for disabling and possibly re-enabling the Python garbage collector
 * from within C code, importing the `enable` and `disable` methods from the
 * `gc` Python module since there is no public C API provided by that extension
 * module (see `cpython/Modules/gcmodule.c`). both macros can be used
 * independently of the other, although they are intended for use together.
 * 
 * note that `gc_disable_impl`, `gc_enable_impl` defined in `gcmodule.c` which
 * disable and enable garbage collection, respectively, both return `Py_None`.
 */

#define PyGCH_gc_enable _XBI_TOKEN_PASTE(GCHELPERS_UNIQ_PREFIX, _gch_gc_enable)

static void PyGCH_gc_enable(void) {

}

#define PY_GC_DISABLE_STR "PY_GC_DISABLE"
/**
 * Disables the Python garbage collector.
 * 
 * Not thread-safe.
 */
#define PY_GC_DISABLE \
  if (GCHELPERS_GC_MOD == NULL) { \
    GCHELPERS_GC_MOD = PyImport_ImportModule("gc"); \
  } \
  if (GCHELPERS_GC_MOD == NULL) { \
    fprintf(stderr, PY_GC_DISABLE_STR ": unable to import gc"); \
  } \
  else { \
    if (GCHELPERS_GC_DISABLE == NULL) { \
      GCHELPERS_GC_DISABLE = PyObject_GetAttrString( \
        GCHELPERS_GC_MOD, "disable" \
      ); \
    } \
    if (GCHELPERS_GC_DISABLE == NULL) { \
      fprintf(stderr, PY_GC_DISABLE_STR ": unable to import gc.enable"); \
    } \
    else { \
      Py_XDECREF(PyObject_CallObject(GCHELPERS_GC_DISABLE, NULL)); \
    } \
  }

#define PY_GC_ENABLE_STR "PY_GC_ENABLE"
/**
 * Enables the Python garbage collector.
 * 
 * Not thread-safe.
 */
#define PY_GC_ENABLE \
  if (GCHELPERS_GC_MOD == NULL) { \
    GCHELPERS_GC_MOD = PyImport_ImportModule("gc"); \
  } \
  if (GCHELPERS_GC_MOD == NULL) { \
    fprintf(stderr, PY_GC_ENABLE_STR ": unable to import gc"); \
  } \
  else { \
    if (GCHELPERS_GC_ENABLE == NULL) { \
      GCHELPERS_GC_ENABLE = PyObject_GetAttrString( \
        GCHELPERS_GC_MOD, "enable" \
      ); \
    } \
    if (GCHELPERS_GC_ENABLE == NULL) { \
      fprintf(stderr, PY_GC_ENABLE_STR ": unable to import gc.enable"); \
    } \
    else { \
      Py_XDECREF(PyObject_CallObject(GCHELPERS_GC_ENABLE, NULL)); \
    } \
  }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GCHELPERS_H */