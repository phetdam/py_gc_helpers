__doc__ = "Unit tests for :class:`py_gch_example.models.PrimalLinearSVC`."

import numpy as np
import pytest

from ..models import PrimalLinearSVC


def test_constructor_sanity():
    "Test that parameters not checked by the optimizer are correctly checked."
    # reg_lambda can only be int or float
    with pytest.raises(TypeError, match = "reg_lambda must be int or float"):
        PrimalLinearSVC(reg_lambda = "foo")
    # reg_lambda must be positive
    with pytest.raises(ValueError, match = "reg_lambda must be nonnegative"):
        PrimalLinearSVC(reg_lambda = -1)
    # batch_size must be int or float
    with pytest.raises(TypeError, match = "batch_size must be int or float"):
        PrimalLinearSVC(batch_size = "cheese")
    # batch size must be within (0, 1) if float
    with pytest.raises(
        ValueError, match = r"float batch_size must be in \(0, 1\)"
    ):
        PrimalLinearSVC(batch_size = 0.)
    with pytest.raises(
        ValueError, match = r"float batch_size must be in \(0, 1\)"
    ):
        PrimalLinearSVC(batch_size = 1.)
    # batch_size must be positive if int
    with pytest.raises(ValueError, match = "int batch_size must be positive"):
        PrimalLinearSVC(batch_size = 0)
    # seed must be int
    with pytest.raises(TypeError, match = "seed must be int"):
        PrimalLinearSVC(seed = 1.)
    # seed must be positive
    with pytest.raises(ValueError, match = "seed must be positive"):
        PrimalLinearSVC(seed = 0)
    # check that reg_lambda, batch_size, and seed are assign correctly
    svc = PrimalLinearSVC(reg_lambda = 2, batch_size = 0.3, seed = 7)
    assert svc.reg_lambda == 2
    assert svc.batch_size == 0.3
    assert svc.seed == 7
    # RNG states should be as expected
    assert (
        np.random.default_rng(7)._bit_generator.state == 
        svc._rng._bit_generator.state
    )


def test_obj_func(linsvm, svm_data):
    """Test that the :class:`PrimalLinearSVC` objective works as intended.

    :param linsvm: ``pytest`` fixture. See package ``conftest.py``.
    :type linsvm: :class:`~py_gch_example.models.PrimalLinearSVC`
    :param svm_data: ``pytest`` fixture. See package ``conftest.py``.
    :type svm_data: tuple
    """
    # unpack data
    X_train, _, y_train, _ = svm_data
    # number of data features
    n_features = X_train.shape[1]
    # allocate random weight + bias vector. weights w, bias b
    rng = np.random.default_rng(7)
    wb = rng.multivariate_normal(
        np.zeros(n_features + 1), np.eye(n_features + 1)
    )
    w, b = wb[:-1], wb[-1]
    # add minibatch_size_ attribute to linsvm (not fitted). batch_size is
    # float by default so this computation of minibatch_size_ is valid.
    linsvm.minibatch_size_ = int(linsvm.batch_size * y_train.size)
    # get value of objective
    obj_act = linsvm._obj_func(wb, X_train, y_train)
    # compute expected value of objective
    obj_ex = (
        np.maximum(0, 1 - y_train * (X_train @ w + b)).sum() +
        linsvm.reg_lambda * np.power(w, 2).sum()
    )
    # check that they are pretty close
    np.testing.assert_allclose(obj_act, obj_ex)


def test_obj_grad(linsvm, svm_data):
    """Test that the :class:`PrimalLinearSVC` gradient works as intended.

    :param linsvm: ``pytest`` fixture. See package ``conftest.py``.
    :type linsvm: :class:`~py_gch_example.models.PrimalLinearSVC`
    :param svm_data: ``pytest`` fixture. See package ``conftest.py``.
    :type svm_data: tuple
    """
    # unpack data
    X_train, _, y_train, _ = svm_data
    # number of data samples, number of data features
    n_samples, n_features = X_train.shape
    # allocate random weight + bias vector. weights w, bias b
    rng = np.random.default_rng(7)
    wb = rng.multivariate_normal(
        np.zeros(n_features + 1), np.eye(n_features + 1)
    )
    w, b = wb[:-1], wb[-1]
    # add minibatch_size_ attribute to linsvm (not fitted). batch_size is
    # float by default so this computation of minibatch_size_ is valid.
    linsvm.minibatch_size_ = int(linsvm.batch_size * y_train.size)
    # get minibatch gradient
    grad_act = linsvm._obj_grad(wb, X_train, y_train)
    # get minibatch indices (use fresh RNG) and minibatch data
    rng = np.random.default_rng(linsvm.seed)
    idx = rng.choice(
        n_samples, size = int(linsvm.batch_size * n_samples), replace = False
    )
    X_batch, y_batch = X_train[idx], y_train[idx]
    # indicator array of examples contributing to gradient
    update_ind = np.where(y_batch * (X_batch @ w + b) < 1, 1, 0)
    # compute expected value of minibatch gradient
    grad_w = (
        -(update_ind * y_batch * X_batch.T).sum(axis = 1) +
        2 * linsvm.reg_lambda * w
    )
    grad_b = -(update_ind * y_batch).sum()
    grad_ex = np.append(grad_w, grad_b)
    # check that they are pretty close
    np.testing.assert_allclose(grad_act, grad_ex)