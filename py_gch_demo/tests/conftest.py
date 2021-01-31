__doc__ = "Fixtures for :mod:`py_gch_demo.tests`."

import numpy as np
import pytest

from ..data import make_linear_regression, make_linear_binary_classification
from ..models import PrimalLinearSVC


@pytest.fixture(scope = "session")
def svm_data():
    r"""Data set for linear classification problem for an SVM.

    Features are zero-mean, identity covariance multivariate Gaussian with a
    centered output vector with unit variance Gaussian errors. There are 2000
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
def linsvm_x0():
    """An initial parameter guess for the ``PrimalLinearSVC`` for ``svm_data``.

    The guess is drawn from a multivariate normal distribution with zero mean
    and identity covariance. The vector returned by :func:`linsvm_x0` is the
    weight vector with the intercept concatenated to the end of the vector.

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