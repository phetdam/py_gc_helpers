__doc__ = "Fixtures for :mod:`py_gch_example.tests`."

import pytest

from ..data import make_linear_regression, make_linear_binary_classification


@pytest.fixture(scope = "session")
def svm_data():
    r"""Data set for linear classification problem for an SVM.

    Features are zero-mean, identity covariance multivariate Gaussian with a
    centered output vector with unit variance Gaussian errors. There are 2000
    training examples and 500 test examples. Random seed is 7. ``output_mean``
    is set to ``8`` by default and gives the true intercept value. Labels are
    in :math:`\{-1, 1\} and appropriate for use with an SVM.

    :returns: ``X_train``, ``X_test``, ``y_train``, ``y_test``
    :rtype: tuple
    """
    return make_linear_binary_classification(
        n_train = 2000, n_test = 500, label_type = "+/-1", rng = 7
    )