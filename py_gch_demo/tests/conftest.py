__doc__ = "Fixtures for :mod:`py_gch_demo.tests`."

import numpy as np
import pytest

from ..data import make_linear_regression, make_linear_binary_classification
from ..models import PrimalLinearSVC


@pytest.fixture(scope = "session")
def lr_data():
    """Data set of a linear regression problem.

    Features are zero-mean, identity covariance multivariate Gaussian with a
    translated output vector with unit variance Gaussian errors. There are
    2000 training examples and 500 test examples. Random seed is 7.
    ``output_mean`` is set to ``8`` and gives the true intercept value. Input
    dimensionality is 10.

    :returns: ``X_train``, ``X_test``, ``y_train``, ``y_test``
    :rtype: tuple
    """
    return make_linear_regression(
        n_train = 2000, n_test = 500, output_mean = 8, rng = 7
    )


@pytest.fixture(scope = "session")
def svm_data():
    r"""Data set for linear classification problem for an SVM.

    Features are zero-mean, identity covariance multivariate Gaussian with a
    translated output vector with unit variance Gaussian errors. There are 2000
    training examples and 500 test examples. Random seed is 7. ``output_mean``
    is set to ``8`` by default and gives the true intercept value. Labels are
    in :math:`\{-1, 1\} and appropriate for use with an SVM. The input
    dimensionality is 10.

    :returns: ``X_train``, ``X_test``, ``y_train``, ``y_test``
    :rtype: tuple
    """
    return make_linear_binary_classification(
        n_train = 2000, n_test = 500, label_type = "+/-1", rng = 7
    )


@pytest.fixture
def linsvm():
    """Returns an instance of ``PrimalLinearSVC`` for testing Adam.

    All parameters used are defaults except for ``seed = 7``. Note that since
    the scope of the instance is ``"function"`` by default, the state of the
    RNG is always fresh whenever it is first used in a test function.

    :rtype: :class:`py_gch_demo.models.PrimalLinearSVC`
    """
    return PrimalLinearSVC(seed = 7)


@pytest.fixture(scope = "session")
def data_x0():
    """An initial parameter guess for models fit on ``lr_data``, ``svm_data``.

    The guess is drawn from a multivariate normal distribution with zero mean
    and identity covariance. The vector returned by :func:`data_x0` is the
    weight vector with the intercept concatenated to the end of the vector.

    This fixture may also used as the initial guess for any linear model that
    has an input dimensionality of 10 + bias or any linear model with an input
    dimensionality of 11 but without a bias term.

    :returns: Initial parameter guess for the
        :class:`py_gch_demo.models.PrimalLinearSVC`, shape ``(11,)``.
    :rtype: :class:`numpy.ndarray`
    """
    rng = np.random.default_rng(999)
    return rng.multivariate_normal(np.zeros(11), np.eye(11))


@pytest.fixture(scope = "session")
def adam_dummy_args():
    """Dummy arguments for the Adam optimizer.

    Objective always returns 0 and gradient always returns 0. The initial guess
    is also (surprise) zero. If ``n_iter_no_change`` > 0, then the optimizer
    will terminate in ``n_iter_no_change`` iterations.

    :returns 3-tuple of dummy objective, dummy gradient, and dummy guess
    :rtype: tuple
    """
    return lambda x: 0, lambda x: np.zeros(3), np.zeros(3)


@pytest.fixture(scope = "session")
def adam_ridge_args(lr_data, data_x0):
    r"""Ridge regression objective arguments for the Adam optimizer.

    Objective function parameter is the linear regression weight vector with a
    concatenated bias and is a ridge regression objective, i.e. the expected
    :math:`\ell_2`-regularization is applied with a parameter ``reg_lambda``
    that is passed to the ``kwargs`` parameter of
    :func:`~py_gch_demo.solvers.adam_optimizer`. The gradient function is
    stochastic and evaluates on minibatches that are a fraction of the data.
    This fraction is specified by the ``batch_frac`` parameter and is ignored
    by the objective [#]_. The training data is passed as
    ``X_``, ``y_`` to the objective and its gradient function are passed to the
    ``args`` parameter of :func:`py_gch_demo.solvers.adam_optimizer`.

    The bias (intercept) term is not penalized.

    .. [#] The parameter must also be passed to the objective since ``args``,
       ``kwargs`` arguments passed to the Adam optimizer are shared by both
       the objective and the gradient functions.

    :param lr_data: ``pytest`` fixture. See :func:`lr_data`.
    :type lr_data: tuple
    :param data_x0: ``pytest`` fixture. See :func:`data_x0`.
    :type data_x0: :class:`numpy.ndarray`

    :returns: 5-tuple of the ridge objective, ridge gradient, starting guess,
        and ``(X, y)`` data passed to ``args``, and a dict containing
        ``reg_lambda`` and ``batch_frac`` passed to ``kwargs``.
    :rtype: tuple
    """
    # ridge objective function
    def ridge_obj(x, X_, y_, reg_lambda = 0.1, batch_frac = 0.2):
        # weights, bias
        w, b = x[:-1], x[-1]
        # return objective value
        return (
            np.power(y_ - X_ @ w - b, 2).sum() +
            reg_lambda * np.power(w, 2).sum()
        )
    
    # random number generator used by the gradient function
    rng = np.random.default_rng(7)
    
    # ridge gradient function
    def ridge_grad(x, X_, y_, reg_lambda = 0.1, batch_frac = 0.2):
        # weights, bias
        w, b = x[:-1], x[-1]
        # selected data indices for minibatch (draw without replacement)
        idx = rng.choice(
            y_.size, size = int(batch_frac * y_.size), replace = False
        )
        # get X_batch, y_batch
        X_batch, y_batch = X_[idx], y_[idx]
        # difference between regression targets and predictions
        pred_diff = y_batch - X_batch @ w - b
        # compute w stochastic gradient
        grad_w = -2 * X_batch.T @ pred_diff + 2 * reg_lambda * w
        # compute b stochastic derivative
        grad_b = -2 * pred_diff.sum()
        # return (w, b)
        return np.append(grad_w, grad_b)
    
    # unpack lr_data fixture to get X, y training data
    X, _, y, _ = lr_data

    # return objective, gradient, initial guess, data (training data from
    # the lr_data fixture), and values for reg_lambda, batch_frac
    return (
        ridge_obj, ridge_grad, data_x0, (X, y),
        dict(reg_lambda = 0.2, batch_frac = 0.25)
    )


@pytest.fixture(scope = "module")
def tuple_replace():
    """Returns a new tuple with elements replaced at various indices.

    A utility fixture which could be helpful for creating new input cases on
    the fly from existing tuples without needing to unpack the individual
    elements of the original tuple. New elements and their indices are
    specified in ``(idx, val)``pairs pass as positional arguments.

    :rtype: function
    """
    def _tuple_replacer(op, *args):
        # op (tuple) as a new list that can be mutated
        op_ = list(op)
        # for each idx, val pair, update
        for idx, val in args:
            op_[idx] = val
        # return new tuple from op_
        return tuple(op_)
    
    return _tuple_replacer