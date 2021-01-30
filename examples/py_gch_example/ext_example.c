/**
 * @file ext_example.c
 * @brief An example C extension module using the API provided by `py_gch.h`.
 */

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <stdlib.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL EXT_EXAMPLE_ARRAY_API
// arrayobject.h gives access to the array API, npymath.h the core math library
#include "numpy/arrayobject.h"
#include "numpy/npy_math.h"

#include "py_gch.h"

PyDoc_STRVAR(
  module_doc,
  "An example C extension module using the API provided by ``py_gch.h``."
  "\n\n"
  "Contains a C implementation of `Kingma and Ba's [#]_ Adam optimizer."
  "\n\n"
  ".. [#] Kingma, D.P. & Ba, J. (2017). Adam: A method for stochastic\n"
  "   *arXiv*. https://arxiv.org/pdf/1412.6980.pdf"
);

PyDoc_STRVAR(
  adam_optimizer_doc,
  "adam_optimizer(obj, grad, x0, args=(), kwargs=None, max_iter=100, "
  "alpha=0.001, beta_1=0.9, beta_2=0.999, eps=1e-8, disable_gc=True)\n--\n\n"
  "A bare-bones implementation of Kingma and Ba's Adam optimizer [#]_."
  "\n\n"
  "Optimizer parameter defaults are the same as specified in the paper."
  "\n\n"
  ".. [#] Kingma, D.P. & Ba, J. (2017). Adam: A method for stochastic\n"
  "   *arXiv*. https://arxiv.org/pdf/1412.6980.pdf"
  "\n\n"
  ":param obj: Objective function to minimize. Must have signature\n"
  "    ``obj(x, *args, **kwargs)``. ``x`` must be a :class:`numpy.ndarray`\n"
  "    and the objective must return a scalar.\n"
  ":type obj: callable\n"
  ":param grad: Gradient of the objective, with signature\n"
  "    ``grad(x, *args, **kwargs)``. ``x`` must a :class:`numpy.ndarray` and\n"
  "    ``grad`` must return a :class:`numpy.ndarray` with the same shape as\n"
  "    the :class:`numpy.ndarray` ``x``.\n"
  ":type grad: callable\n"
  ":param x0: Initial guess for the parameter :class:`numpy.ndarray`\n"
  ":type x0: :class:`numpy.ndarray`"
  ":param args: Positional args to pass to ``obj``, ``grad``\n"
  ":type args: tuple, optional\n"
  ":param kwargs: Keyword args to pass to ``obj``, ``grad``\n"
  ":type kwargs: dict, optional\n"
  ":param max_iter: Maximum number of iterations to run before termination\n"
  ":type max_iter: int, optional\n"
  ":param alpha: The :math:`\\alpha` parameter for the Adam algorithm that\n"
  "    gives the stepsize. Must be positive."
  ":param alpha: float, optional\n"
  ":param beta_1: The :math:`\\beta_1` parameter for the Adam algorithm that\n"
  "    controls the decay of the first moment estimate. Must be a value in\n"
  "    :math:`[0, 1)`.\n"
  ":type beta_1: float, optional\n"
  ":param beta_2: The :math:`\\beta_2` parameter for the Adam algorithm that\n"
  "    controls the decay of the second uncentered moment estimate. Must be\n"
  "    a value in :math:`[0, 1)`.\n"
  ":type beta_2: float, optional\n"
  ":param eps: Positive fudge factor to prevent division by zero when\n"
  "    computing descent direction\n"
  ":type eps: float, optional\n"
  ":param disable_gc: ``True`` to disable garbage collection during\n"
  "    execution.\n"
  ":type disable_gc: bool, optional"
  ":returns: A new :class:`numpy.ndarray` giving an estimate for the\n"
  "    parameter ``x`` in the objective function.\n"
  ":rtype: :class:`numpy.ndarray`"
);
// Python argument names for adam_optimizer
static char *adam_optimizer_argnames[] = {
  "obj", "grad", "x0", "args", "kwargs", "max_iter", "alpha", "beta_1",
  "beta_2", "eps", "disable_gc", NULL
};
/**
 * A simple implementation of Kingma and Ba's Adam optimization algorithm.
 * 
 * Python signature:
 * 
 * `adam(obj, grad, x0, args=(), kwargs=None, max_iter=100, alpha=0.001,
 * beta_1=0.9, beta_2=0.999, eps=1e-8, disable_gc=True)`
 * 
 * @param self `PyObject *` self (ignored)
 * @param args `PyObject *` tuple of positional args
 * @param kwargs `PyObject *` dict of named args, `NULL` if none provided
 * @returns `PyArrayObject *` cast to `PyObject *` of parameters, the same
 *     shape as the original guess.
 */
static PyObject *adam_impl(PyObject *self, PyObject *args, PyObject *kwargs) {
  // objective function, gradient function, initial parameter guess (required)
  PyObject *obj, *grad;
  PyArrayObject *x0;
  /**
   * args and kwargs passed to both obj and grad. note that f_args should also
   * include x0 as the first element so that it's easier to pass all the
   * arguments to PyObject_Call.
   */
  PyObject *f_args = NULL, *f_kwargs = NULL;
  // max iterations
  Py_ssize_t max_iter = 100;
  // Adam optimzer hyperparameters
  double alpha = 0.001, beta_1 = 0.9, beta_2 = 0.999, eps = 1e-8;
  // whether or not to disable garbage collection
  int disable_gc = 1;
  // parse parameters. exception set on error
  if (
    !PyArg_ParseTupleAndKeywords(
      args, kwargs, "OOO!|O!O!nddddp", adam_optimizer_argnames, &obj, &grad,
      (PyObject **) &x0, &PyArray_Type, &f_args, &PyTuple_Type, &f_kwargs,
      &PyDict_Type, &max_iter, &alpha, &beta_1, &beta_2, &eps, &disable_gc
    )
  ) { return NULL; }
  // check if obj and grad are callable
  if (!PyCallable_Check(obj)) {
    PyErr_SetString(PyExc_TypeError, "obj must be callable");
    return NULL;
  }
  if (!PyCallable_Check(grad)) {
    PyErr_SetString(PyExc_TypeError, "grad must be callable");
    return NULL;
  }
  // check that max_iter, alpha, eps are valid (must be positive)
  if (max_iter < 1) {
    PyErr_SetString(PyExc_ValueError, "max_iter must be positive");
    return NULL;
  }
  if (alpha <= 0) {
    PyErr_SetString(PyExc_ValueError, "alpha must be positive");
    return NULL;
  }
  if (eps <= 0) {
    PyErr_SetString(PyExc_ValueError, "eps must be positive");
    return NULL;
  }
  // warn if eps is too big
  if (eps >= 1e-1) { 
    if (
      PyErr_WarnEx(
        PyExc_UserWarning, "eps excees 1e-1; step sizes may be overly "
        "deflated. Consider passing a smaller value.", 1
      ) < 0
    ) { return NULL; }
  }
  // beta_1, beta_2 must be within [0, 1)
  if ((beta_1 < 0) || (beta_1 >= 1)) {
    PyErr_SetString(PyExc_ValueError, "beta_1 must be inside [0, 1)");
    return NULL;
  }
  if ((beta_2 < 0) || (beta_2 >= 2)) {
    PyErr_SetString(PyExc_ValueError, "beta_2 must be inside [0, 1)");
    return NULL;
  }
  // check that x0 is either int or double array
  if (!PyArray_ISINTEGER(x0) && !PyArray_ISFLOAT(x0)) {
    PyErr_SetString(PyExc_TypeError, "x0 must contain either ints or floats");
    return NULL;
  }
  // check if array can be cast to NPY_DOUBLE safely
  if (!PyArray_CanCastSafely(PyArray_TYPE(x0), NPY_DOUBLE)) {
    if (
      PyErr_WarnEx(
        PyExc_UserWarning, "x0 cannot be safely cast to NPY_DOUBLE. precision "
        "may be lost during computation", 1
      ) < 0
    ) { return NULL; }
  }
  /**
   * create new output array containing NPY_DOUBLE values of x0. a reference
   * to the PyArrayDescr * is stolen by this function. NPY_ARRAY_CARRAY flags
   * equals NPY_ARRAY_C_CONTIGUOUS | NPY_ARRAY_ALIGNED | NPY_ARRAY_WRITEABLE.
   */
  PyArrayObject *params = (PyArrayObject *) PyArray_FromArray(
    x0, PyArray_DescrFromType(NPY_DOUBLE), NPY_ARRAY_CARRAY
  );
  // nothing to do on error (all other refs are borrowed)
  if (params == NULL) {
    return NULL;
  }
  // pack all positional args for obj, grad (params, f_args) into a new tuple.
  // if f_args is NULL, then there aren't any other positional args to add.
  Py_ssize_t n_pos_f_args = 1;
  // we know f_args points to a tuple, so no need to check
  if (f_args != NULL) {
    n_pos_f_args = n_pos_f_args + PyTuple_GET_SIZE(f_args);
  }
  // new pointer to original f_args (temporary var)
  PyObject *xf_args = f_args;
  // new f_args (ignore dropping of borrowed ref)
  f_args = PyTuple_New(n_pos_f_args);
  // on error, we need to Py_DECREF params, which is a new ref
  if (f_args == NULL) {
    Py_DECREF(params);
    return NULL;
  }
  // fill f_args with params and elements of xf_args (Py_INCREF all of them)
  Py_INCREF(params);
  PyTuple_SET_ITEM(f_args, 0, (PyObject *) params);
  if (xf_args != NULL) {
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(xf_args); i++) {
      // all items in xf_args need to get Py_INCREF'd
      PyObject *xf_args_i = PyTuple_GET_ITEM(xf_args, i);
      Py_INCREF(xf_args_i);
      PyTuple_SET_ITEM(f_args, i + 1, xf_args_i);
    }
  }
  // Py_DECREF f_args
  Py_DECREF(f_args);
  // return final parameters
  return (PyObject *) params;
}

// method table
static PyMethodDef mod_methods[] = {
  {
    "adam_optimizer",
    (PyCFunction) adam_impl,
    METH_VARARGS | METH_KEYWORDS,
    adam_optimizer_doc  
  },
  {NULL, NULL, 0, NULL}
};

// static module definition
static PyModuleDef mod_struct = {
  PyModuleDef_HEAD_INIT,
  "ext_example",
  module_doc,
  -1,
  mod_methods
};

PyMODINIT_FUNC PyInit_ext_example(void) {
  // sets error indicator and returns NULL on error automatically
  import_array();
  // create module; if NULL, error
  return PyModule_Create(&mod_struct);
}