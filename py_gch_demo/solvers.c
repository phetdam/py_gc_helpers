/**
 * @file solvers.c
 * @brief A C extension module with an implementation for the Adam optimizer
 *     using the API provided by `py_gch.h`.
 */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL SOLVERS_ARRAY_API
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

// new immutable type for the optimization result returned by adam_optimizer
typedef struct {
  PyObject_HEAD;
  // final parameter guess (PyArrayObject * cast to PyObject *)
  PyObject *x;
  // final objective value
  double obj;
  // final gradient value (PyArrayObject * cast to PyObject *)
  PyObject *grad;
  // numbers of times objective and gradient were evaluated
  Py_ssize_t n_obj_eval;
  Py_ssize_t n_grad_eval;
  // iterations elapsed during runtime
  Py_ssize_t n_iter;
} GradSolverResult;

// string name for the GradSolverResult (just class name)
#define GradSolverResult_name "GradSolverResult"

/**
 * Deallocation function for the `GradSolverResult` type.
 * 
 * @param self `GradSolverResult *` reference to `GradSolverResult` instance
 */
static void GradSolverResult_dealloc(GradSolverResult *self) {
  // pointers might be NULL if called during __new__
  Py_XDECREF(self->x);
  Py_XDECREF(self->grad);
  // free the struct using the default function set to tp_free
  Py_TYPE(self)->tp_free((void *) self);
}

/**
 * `__new__` implementation for the `GradSolverResult`.
 * 
 * All initialization done here since the type is immutable. Python signature:
 * 
 * `GradSolverResult.__new__(x, obj, grad, n_obj_eval, n_grad_eval, n_iter)`
 * 
 * @param type `PyTypeObject *` reference to `GradSolverResult` type object
 * @param args `PyObject *` tuple of positional args
 * @param kwargs `PyObject *` dict of named args (ignored)
 */
static PyObject *GradSolverResult_new(
  PyTypeObject *type, PyObject *args, PyObject *kwargs
) {
  // simple sanity checks if called within module
  if (type == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "PyTypeObject *type is NULL");
    return NULL;
  }
  if (args == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "PyObject *args is NULL");
    return NULL;
  }
  // try to allocate new instance of the GradSolverResult
  GradSolverResult *self = (GradSolverResult *) type->tp_alloc(type, 0);
  // exception set on error so we can just return
  if (self == NULL) {
    return NULL;
  }
  /**
   * parse arguments; all positional. Py_DECREF on error (indicator set). note
   * that if there is a parsing error we set self->x and self->grad to NULL.
   * this way dealloc works correctly in the case that x and/or grad are
   * correctly parsed but a different argument causes an exception raise as in
   * this case a borrowed reference will get Py_XDECREF'd.
   */
  if (
    !PyArg_ParseTuple(
      args, "O!dO!nnn", &PyArray_Type, &(self->x), &(self->obj),
      &PyArray_Type, &(self->grad), &(self->n_obj_eval),
      &(self->n_grad_eval), &(self->n_iter)
    )
  ) {
    self->x = self->grad = NULL;
    Py_DECREF(self);
    return NULL;
  }
  // Py_INCREF x, grad since those references were borrowed. these will be
  // Py_DECREF'd when Py_DECREF is called on self.
  Py_INCREF(self->x);
  Py_INCREF(self->grad);
  // temporary PyArrayObject * variables for self->x, self->grad
  PyArrayObject *self_x = (PyArrayObject *) self->x;
  PyArrayObject *self_grad = (PyArrayObject *) self->grad;
  // number of dimensions for x, grad
  int x_ndim = PyArray_NDIM(self_x), grad_ndim = PyArray_NDIM(self_grad);
  // check that x, grad aren't zero-dimensional. on error, set exception
  // and Py_DECREF self as usual (since it's new)
  if (x_ndim == 0) {
    PyErr_SetString(PyExc_ValueError, "x must have at least 1 dimension");
    Py_DECREF(self);
    return NULL;
  }
  if (grad_ndim == 0) {
    PyErr_SetString(PyExc_ValueError, "grad must have at least 1 dimension");
    Py_DECREF(self);
    return NULL;
  }
  // check that x, grad have same number of dimensions
  if (x_ndim != grad_ndim) {
    PyErr_SetString(
      PyExc_ValueError, "x, grad must have the same number of dimensions"
    );
    Py_DECREF(self);
    return NULL;
  }
  // check that x and grad have the same shape (doesn't make sense otherwise)
  npy_intp *x_shape = PyArray_DIMS(self_x);
  npy_intp *grad_shape = PyArray_DIMS(self_grad);
  for (int i = 0; i < x_ndim; i++) {
    // if shape doesn't match in a dimension, raise exception + Py_DECREF
    if (x_shape[i] != grad_shape[i]) {
      // use ld format since npy_intp -> long int on x86-64
      PyErr_Format(PyExc_ValueError, "x, grad shapes differ on axis %ld", i);
      Py_DECREF(self);
      return NULL;
    }
  }
  // n_obj_eval, n_grad_eval, n_iter all need to be positive. Py_DECREF self
  // on error and set ValueError otherwise
  if (self->n_obj_eval < 1) {
    PyErr_SetString(PyExc_ValueError, "n_obj_eval must be positive");
    Py_DECREF(self);
    return NULL;
  }
  if (self->n_grad_eval < 1) {
    PyErr_SetString(PyExc_ValueError, "n_grad_eval must be positive");
    Py_DECREF(self);
    return NULL;
  }
  if (self->n_iter < 1) {
    PyErr_SetString(PyExc_ValueError, "n_iter must be positive");
    Py_DECREF(self);
    return NULL;
  }
  // return fully initialized instance (cast to suppress warning)
  return (PyObject *) self;
}

/**
 * Custom `__repr__` implementation for the `GradSolverResult.
 * 
 * @param self `GradSolverResult *` self
 * @returns `PyObject *` Python Unicode object
 */
PyObject *GradSolverResult_repr(GradSolverResult *self) {
  // PyUnicode_FromFormat doesn't accept double formatting, so create new
  // Python object holding the objective function value
  PyObject *py_obj = PyFloat_FromDouble(self->obj);
  if (py_obj == NULL) {
    return NULL;
  }
  // create Python string representation. Py_DECREF py_obj on error
  PyObject *repr_str = PyUnicode_FromFormat(
    GradSolverResult_name "(x=%R, obj=%R, grad=%R, n_obj_eval=%zd, "
    "n_grad_eval=%zd, n_iter=%zd)", self->x, py_obj, self->grad,
    self->n_obj_eval, self->n_grad_eval, self->n_iter
  );
  if (repr_str == NULL) {
    Py_DECREF(py_obj);
    return NULL;
  }
  // Py_DECREF py_obj and return result
  Py_DECREF(py_obj);
  return repr_str;
}

// docstrings for the members of the GradSolverResult
PyDoc_STRVAR(
  GradSolverResult_x_doc, "Final parameter guess after optimization"
);
PyDoc_STRVAR(
  GradSolverResult_obj_doc, "Final value of the objective function"
);
PyDoc_STRVAR(
  GradSolverResult_grad_doc, "Final value of the objective gradient"
);
PyDoc_STRVAR(
  GradSolverResult_n_obj_eval_doc,
  "Total number of objective function evaluations"
);
PyDoc_STRVAR(
  GradSolverResult_n_grad_eval_doc,
  "Total number of gradient function evaluations"
);
PyDoc_STRVAR(
  GradSolverResult_n_iter_doc, "Total number of optimizer iterations performed"
);
// members for the GradSolverResult (all read-only)
static PyMemberDef GradSolverResult_members[] = {
  {
    "x",
    T_OBJECT_EX,
    offsetof(GradSolverResult, x),
    READONLY,
    GradSolverResult_x_doc
  },
  {
    "obj",
    T_DOUBLE,
    offsetof(GradSolverResult, obj),
    READONLY,
    GradSolverResult_obj_doc
  },
  {
    "grad",
    T_OBJECT_EX,
    offsetof(GradSolverResult, grad),
    READONLY,
    GradSolverResult_grad_doc
  },
  {
    "n_obj_eval",
    T_PYSSIZET,
    offsetof(GradSolverResult, n_obj_eval),
    READONLY,
    GradSolverResult_n_obj_eval_doc
  },
  {
    "n_grad_eval",
    T_PYSSIZET,
    offsetof(GradSolverResult, n_grad_eval),
    READONLY,
    GradSolverResult_n_grad_eval_doc
  },
  {
    "n_iter",
    T_PYSSIZET,
    offsetof(GradSolverResult, n_iter),
    READONLY,
    GradSolverResult_n_iter_doc
  },
  {NULL, 0, 0, 0, NULL}
};

// docstring for the GradSolverResult
PyDoc_STRVAR(
  GradSolverResult_doc,
  "Simple class to hold results returned by first-order optimizers."
  "\n\n"
  "All members are read-only and ``__new__`` parameters are positional-only.\n"
  "Class is final and cannot be subtyped."
  "\n\n"
  ":param x: Final parameter guess returned by the optimizer. Cannot be\n"
  "    zero-dimensional and must match the shape of ``grad``.\n"
  ":type x: :class:`numpy.ndarray`\n"
  ":param obj: Final value of the objective function\n"
  ":type obj: float\n"
  ":param grad: Final value of the objective gradient. Cannot be\n"
  "    zero-dimensional and must match the shape of ``x``.\n"
  ":type grad: :class:`numpy.ndarray`\n"
  ":param n_obj_eval: Total number of objective function evaluations\n"
  ":type n_obj_eval: int\n"
  ":param n_grad_eval: Total number of gradient function evaluations\n"
  ":type n_grad_eval: int\n"
  ":param n_iter: Total number of iterations performed by optimizer\n"
  ":type n_iter: int\n"
);
// static PyTypeObject for the GradSolverResult type
static PyTypeObject GradSolverResult_type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // class name, class docstring, size of GradSolverResult struct
  .tp_name = "solvers." GradSolverResult_name,
  .tp_doc = GradSolverResult_doc,
  .tp_basicsize = sizeof(GradSolverResult),
  // doesn't contain any items, no Py_TPFLAGS_BASETYPE flag (final)
  .tp_itemsize = 0,
  .tp_flags = Py_TPFLAGS_DEFAULT,
  // custom new, dealloc, members, __repr__ implementation
  .tp_new = GradSolverResult_new,
  .tp_dealloc = (destructor) GradSolverResult_dealloc,
  .tp_members = GradSolverResult_members,
  .tp_repr = (reprfunc) GradSolverResult_repr
};

PyDoc_STRVAR(
  adam_optimizer_doc,
  "adam_optimizer(obj, grad, x0, args=(), kwargs=None, *, max_iter=200, "
  "n_iter_no_change=10, tol=1e-4, alpha=0.001, beta_1=0.9, beta_2=0.999, "
  "eps=1e-8, disable_gc=True)\n--\n\n"
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
  ":param args: Shared positional args to pass to ``obj``, ``grad``\n"
  ":type args: tuple, optional\n"
  ":param kwargs: Shared keyword args to pass to ``obj``, ``grad``\n"
  ":type kwargs: dict, optional\n"
  ":param max_iter: Maximum number of iterations to run before termination\n"
  ":type max_iter: int, optional\n"
  ":param n_iter_no_change: Number of iterations that objective function\n"
  "    value doesn't decrease by at least ``tol`` to trigger early stopping\n"
  ":type n_iter_no_change: int, optional\n"
  ":param tol: Minimum amount of objective value decrease between\n"
  "    iterations required to prevent triggering early stopping\n"
  ":type tol: float, optional\n"
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
  "obj", "grad", "x0", "args", "kwargs", "max_iter", "n_iter_no_change",
  "tol", "alpha", "beta_1", "beta_2", "eps", "disable_gc", NULL
};
/**
 * A simple implementation of Kingma and Ba's Adam optimization algorithm.
 * 
 * Python signature:
 * 
 * `adam_optimizer(obj, grad, x0, args=(), kwargs=None, *, max_iter=200,
 * n_iter_no_change=10, tol=1e-4, alpha=0.001, beta_1=0.9, beta_2=0.999,
 * eps=1e-8, disable_gc=True)`
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
  // max iterations, number of iterations objective improvement is less than
  // tol before early stopping is triggered
  Py_ssize_t max_iter = 200, n_iter_no_change = 10;
  // min iter improvement, Adam optimizer hyperparameters
  double tol = 1e-4, alpha = 0.001, beta_1 = 0.9, beta_2 = 0.999, eps = 1e-8;
  // whether or not to disable garbage collection
  int disable_gc = 1;
  // parse parameters. exception set on error
  if (
    !PyArg_ParseTupleAndKeywords(
      args, kwargs, "OOO!|O!O!$nndddddp", adam_optimizer_argnames, &obj,
      &grad, &PyArray_Type, (PyObject **) &x0, &PyTuple_Type, &f_args,
      &PyDict_Type, &f_kwargs, &max_iter, &n_iter_no_change, &tol, &alpha,
      &beta_1, &beta_2, &eps, &disable_gc
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
  // check that n_iter_no_change, tol is valid (must be nonnegative)
  if (n_iter_no_change < 0) {
    PyErr_SetString(
      PyExc_ValueError, "n_iter_no_change must be nonnegative\n"
    );
    return NULL;
  }
  if (tol < 0) {
    PyErr_SetString(PyExc_ValueError, "tol must be nonnegative");
    return NULL;
  }
  // warn if eps is too big
  if (eps >= 1e-1) { 
    if (
      PyErr_WarnEx(
        PyExc_UserWarning, "eps exceeds 1e-1; step sizes may be overly "
        "deflated. Consider passing a smaller value.", 1
      ) < 0
    ) { return NULL; }
  }
  // beta_1, beta_2 must be within [0, 1)
  if ((beta_1 < 0) || (beta_1 >= 1)) {
    PyErr_SetString(PyExc_ValueError, "beta_1 must be inside [0, 1)");
    return NULL;
  }
  if ((beta_2 < 0) || (beta_2 >= 1)) {
    PyErr_SetString(PyExc_ValueError, "beta_2 must be inside [0, 1)");
    return NULL;
  }
  // check that x0 is either int or double array
  if (!PyArray_ISINTEGER(x0) && !PyArray_ISFLOAT(x0)) {
    PyErr_SetString(PyExc_TypeError, "x0 must contain either ints or floats");
    return NULL;
  }
  // no need to check if array cannot be correctly cast as PyArray_FromArray
  // will raise an exception if safe casting fails
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
  // we know f_args either points to a tuple or is NULL so no need to check
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
      // all items (borrowed refs) in xf_args need to get Py_INCREF'd
      PyObject *xf_args_i = PyTuple_GET_ITEM(xf_args, i);
      Py_INCREF(xf_args_i);
      PyTuple_SET_ITEM(f_args, i + 1, xf_args_i);
    }
  }
  // evaluate objective function at current guess so we can track improvements
  PyObject *obj_val_ = PyObject_Call(obj, f_args, f_kwargs);
  // on error, need to Py_DECREF params, f_args (new references)
  if (obj_val_ == NULL) {
    Py_DECREF(params);
    Py_DECREF(f_args);
    return NULL;
  }
  // objective function value + Py_DECREF unneeded obj_val_
  double obj_val = PyFloat_AsDouble(obj_val_);
  Py_DECREF(obj_val_);
  // Py_DECFEF params, f_args (new refs on error)
  if (PyErr_Occurred()) {
    Py_DECREF(params);
    Py_DECREF(f_args);
    return NULL;
  }
  // improvement from last iteration
  double obj_imp = DBL_MAX;
  // consecutive iterations where obj value has not improved by at least tol
  Py_ssize_t n_no_change = 0;
  // number of iterations completed
  Py_ssize_t iter_i = 0;
  // number of times objective has been evaluated
  Py_ssize_t n_obj_eval = 1;
  // number of times gradient has been evaluated
  Py_ssize_t n_grad_eval = 0;
  /**
   * value of the gradient, first moment vector, second raw moment vector.
   * init grad_val to NULL since we want to save grad_val after we finish the
   * optimization loop. therefore, grad_val doesn't get Py_DECREF'd near the
   * end of the loop but rather at the beginning with a Py_XDECREF.
   */
  PyObject *grad_val, *grad_mean, *grad_var;
  grad_val = NULL;
  // initialize first and second moment vectors to all 0 (both C-order)
  grad_mean = PyArray_ZEROS(
    PyArray_NDIM(params), PyArray_DIMS(params), NPY_DOUBLE, 0
  );
  // on error, Py_DECREF params, f_args
  if (grad_mean == NULL) {
    Py_DECREF(params);
    Py_DECREF(f_args);
    return NULL;
  }
  grad_var = PyArray_ZEROS(
    PyArray_NDIM(params), PyArray_DIMS(params), NPY_DOUBLE, 0
  );
  // on error, Py_DECREF params, f_args, grad_mean (new reference)
  if (grad_var == NULL) {
    Py_DECREF(params);
    Py_DECREF(f_args);
    Py_DECREF(grad_mean);
    return NULL;
  }
  // while not converged (max_iter iterations not reached and number of
  // consecutive iterations with improvement < tol less than n_iter_no_change)
  do {
    // Py_XDECREF old gradient vector (NULL on first iteration)
    Py_XDECREF(grad_val);
    // get gradient value from grad + increment n_grad_eval
    grad_val = PyObject_Call(grad, f_args, f_kwargs);
    n_grad_eval++;
    // on error, need to Py_DECREF params, f_args, grad_mean, grad_var
    if (grad_val == NULL) {
      Py_DECREF(params);
      Py_DECREF(f_args);
      Py_DECREF(grad_mean);
      Py_DECREF(grad_var);
      return NULL;
    }
    // number of operands going into the multi-iterator
    int n_iter_ops = 4;
    // array of PyArrayObject * for params, grad_val, grad_mean, grad_var so
    // that we can use them together in the multi-iterator
    PyArrayObject *iter_ops[] = {
      params, (PyArrayObject *) grad_val, (PyArrayObject *) grad_mean,
      (PyArrayObject *) grad_var
    };
    /**
     * flags for each of the operands. grad_val is read-only but the other
     * operands are all read + write (updated). we also specify the data to
     * be in native byte order and to be aligned for each operand using the
     * NPY_ITER_NBO | NPY_ITER_ALIGNED flag combination.
     */
    npy_uint32 iter_op_flags[n_iter_ops];
    // flags for params
    iter_op_flags[0] = NPY_ITER_READWRITE | NPY_ITER_NBO | NPY_ITER_ALIGNED;
    // flags for grad_val
    iter_op_flags[1] = NPY_ITER_READONLY | NPY_ITER_NBO | NPY_ITER_ALIGNED;
    // flags for grad_mean
    iter_op_flags[2] = NPY_ITER_READWRITE | NPY_ITER_NBO | NPY_ITER_ALIGNED;
    // flags for grad_var
    iter_op_flags[3] = NPY_ITER_READWRITE | NPY_ITER_NBO | NPY_ITER_ALIGNED;
    /**
     * array of PyArray_Descr * to specify types to the multi-iterator. we
     * only need one because we can just borrow references. after initializing
     * the iterator we can Py_DECREF iter_op_types[0] since the iterator
     * internally Py_INCREF's each PyArray_Descr * provided.
     */
    PyArray_Descr *iter_op_dtypes[n_iter_ops];
    iter_op_dtypes[0] = PyArray_DescrFromType(NPY_DOUBLE);
    // on error, Py_DECREF params, f_args, grad_val grad_mean, grad_var
    if (iter_op_dtypes[0] == NULL) {
      Py_DECREF(params);
      Py_DECREF(f_args);
      Py_DECREF(grad_val);
      Py_DECREF(grad_mean);
      Py_DECREF(grad_var);
      return NULL;
    }
    // borrow references from iter_op_dtypes[0]
    for (int i = 1; i < n_iter_ops; i++) {
      iter_op_dtypes[i] = iter_op_dtypes[0];
    }
    /**
     * use multi-iterator to update grad_mean, grad_var, params using grad_val
     * values. since all numpy.ndarray types are NPY_DOUBLE, NPY_NO_CASTING
     * is passed as the casting flag. multi-iterator tracks C index and goes
     * through elements in original (C) order.
     */
    NpyIter *iter = NpyIter_MultiNew(
      n_iter_ops, iter_ops, NPY_ITER_C_INDEX, NPY_KEEPORDER,
      NPY_NO_CASTING, iter_op_flags, iter_op_dtypes
    );
    // don't need the PyArray_Descr * anymore
    Py_DECREF(iter_op_dtypes[0]);
    // on error, Py_DECREF params, f_args, grad_val, grad_mean, grad_var
    if (iter == NULL) {
      Py_DECREF(params);
      Py_DECREF(f_args);
      Py_DECREF(grad_val);
      Py_DECREF(grad_mean);
      Py_DECREF(grad_var);
      return NULL;
    }
    // get NpyIter_IterNextFunc that handles iteration
    NpyIter_IterNextFunc *iternext = NpyIter_GetIterNext(iter, NULL);
    // on error, Py_DECREF params, f_args, grad_val, grad_mean, grad_var and
    // deallocate iter using NpyIter_Deallocate
    if (iter == NULL) {
      Py_DECREF(params);
      Py_DECREF(f_args);
      Py_DECREF(grad_val);
      Py_DECREF(grad_mean);
      Py_DECREF(grad_var);
      // if there is an error we are returning NULL anyways
      NpyIter_Deallocate(iter);
      return NULL;
    }
    // pointer to array of pointers to next iteration elements
    char **iter_op_data = NpyIter_GetDataPtrArray(iter);
    // we update each element of params, grad_mean, grad_var
    do {
      // pointers to next element of params, grad_val, grad_mean, grad_var
      double *param_ep = (double *) iter_op_data[0];
      double *grad_val_ep = (double *) iter_op_data[1];
      double *grad_mean_ep = (double *) iter_op_data[2];
      double *grad_var_ep = (double *) iter_op_data[3];
      // update element of biased first moment estimate
      *grad_mean_ep = beta_1 * (*grad_mean_ep) + (1 - beta_1) * (*grad_val_ep);
      // update element of biased second raw moment estimate
      *grad_var_ep = beta_2 * (*grad_var_ep) + (1 - beta_2) * (*grad_var_ep);
      // update element of parameter using bias-corrected first and second
      // moment element estimates (no temp variable)
      *param_ep = (
        *param_ep - alpha * (*grad_mean_ep) / (1 - pow(beta_1, iter_i + 1)) /
        (sqrt((*grad_var_ep) / (1 - pow(beta_2, iter_i + 1))) + eps)
      );
    } while (iternext(iter));
    // deallocate multi-iterator now that we are done updating. on error we
    // Py_DECREF params, f_args, grad_val, grad_mean, grad_var
    if (NpyIter_Deallocate(iter) != NPY_SUCCEED) {
      Py_DECREF(params);
      Py_DECREF(f_args);
      Py_DECREF(grad_val);
      Py_DECREF(grad_mean);
      Py_DECREF(grad_var);
      return NULL;
    }
    // compute new objective value, get as PyObject * + increment n_obj_eval
    obj_val_ = PyObject_Call(obj, f_args, f_kwargs);
    n_obj_eval++;
    // on error, must Py_DECREF params, f_args, grad_val, grad_mean, grad_var
    if (obj_val_ == NULL) {
      Py_DECREF(params);
      Py_DECREF(f_args);
      Py_DECREF(grad_val);
      Py_DECREF(grad_mean);
      Py_DECREF(grad_var);
      return NULL;
    }
    // use obj_imp to hold old value of objective value
    obj_imp = obj_val;
    // get new objective value as double + Py_DECREF unneeded obj_val_
    obj_val = PyFloat_AsDouble(obj_val_);
    Py_DECREF(obj_val_);
    // on error, Py_DECREF params, f_args, grad_val, grad_mean, grad_var
    if (PyErr_Occurred()) {
      Py_DECREF(params);
      Py_DECREF(f_args);
      Py_DECREF(grad_val);
      Py_DECREF(grad_mean);
      Py_DECREF(grad_var);
      return NULL;
    }
    // compute improvement from last iteration (obj_imp has old obj_val)
    obj_imp = obj_imp - obj_val;
    // if obj_imp (improvement of objective) < tol, increment n_no_change
    if (obj_imp < tol) {
      n_no_change++;
    }
    // else if n_no_change not zero, set to 0
    else if (n_no_change > 0) {
      n_no_change = 0;
    }
    // increment iter_i
    iter_i++;
  } while ((iter_i < max_iter) && (n_no_change < n_iter_no_change));
  // Py_DECREF f_args, grad_mean, grad_var (no longer needed)
  Py_DECREF(f_args);
  Py_DECREF(grad_mean);
  Py_DECREF(grad_var);
  /**
   * we now build new argument tuple from params, obj_val (convert), grad_val,
   * n_obj_eval (convert), n_grad_eval (convert), iter_i (convert). first,
   * we need to convert obj_val to Python float and n_* to Python int.
   */
  obj_val_ = PyFloat_FromDouble(obj_val);
  // on error, Py_DECREF params and grad_val
  if (obj_val_ == NULL) {
    Py_DECREF(params);
    Py_DECREF(grad_val);
    return NULL;
  }
  PyObject *n_obj_eval_ = PyLong_FromSsize_t(n_obj_eval);
  // on error, Py_DECREF params, grad_val, obj_val_ (new ref)
  if (n_obj_eval_ == NULL) {
    Py_DECREF(params);
    Py_DECREF(grad_val);
    Py_DECREF(obj_val_);
    return NULL;
  }
  PyObject *n_grad_eval_ = PyLong_FromSsize_t(n_grad_eval);
  // on error, Py_DECREF params, grad_val, obj_val_, n_obj_eval_
  if (n_obj_eval_ == NULL) {
    Py_DECREF(params);
    Py_DECREF(grad_val);
    Py_DECREF(obj_val_);
    Py_DECREF(n_obj_eval_);
    return NULL;
  }
  PyObject *n_iter_ = PyLong_FromSsize_t(iter_i);
  // on error, Py_DECREF params, grad_val, obj_val_, n_obj_eval_, n_grad_eval_
  if (n_obj_eval_ == NULL) {
    Py_DECREF(params);
    Py_DECREF(grad_val);
    Py_DECREF(obj_val_);
    Py_DECREF(n_obj_eval_);
    Py_DECREF(n_grad_eval_);
    return NULL;
  }
  /**
   * create tuple from params, obj_val_, grad_val, n_obj_eval_, n_grad_eval_,
   * n_iter_ to pass to GradSolverResult_new. since the references are borrowed
   * (no reference is stolen) by PyTuple_Pack, we don't Py_DECREF on success.
   */
  PyObject *gsr_args = PyTuple_Pack(
    6, params, obj_val_, grad_val, n_obj_eval_, n_grad_eval_, n_iter_
  );
  // Py_DECREF params, obj_val_, grad_val, n_obj_eval_, n_grad_eval_, n_iter_
  // on failure (on success we leave the refs be)
  if (gsr_args == NULL) {
    Py_DECREF(params);
    Py_DECREF(grad_val);
    Py_DECREF(obj_val_);
    Py_DECREF(n_obj_eval_);
    Py_DECREF(n_grad_eval_);
    Py_DECREF(n_iter_);
    return NULL;
  }
  // return result from GradSolverResult_new (NULL if err with exception set)
  return GradSolverResult_new(&GradSolverResult_type, gsr_args, NULL);
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
  "solvers",
  module_doc,
  -1,
  mod_methods
};

// module initialization function (external linkage)
PyMODINIT_FUNC PyInit_solvers(void) {
  // if type isn't ready, error indicator set
  if (PyType_Ready(&GradSolverResult_type) < 0) {
    return NULL;
  }
  // sets error indicator and returns NULL on error automatically
  import_array();
  // create module; if NULL, error
  PyObject *module = PyModule_Create(&mod_struct);
  if (module == NULL) {
    return NULL;
  }
  // PyModule_AddObject only steals a reference on success, so on failure, we
  // need to Py_DECREF &GradSolverResult_type (and module)
  Py_INCREF(&GradSolverResult_type);
  if (
    PyModule_AddObject(
      module, "GradSolverResult", (PyObject *) &GradSolverResult_type
    ) < 0
  ) {
    Py_DECREF(&GradSolverResult_type);
    Py_DECREF(module);
    return NULL;
  }
  return module;
}