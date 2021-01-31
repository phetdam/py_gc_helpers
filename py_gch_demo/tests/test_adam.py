__doc__ = "Unit tests for the Adam implementation in ``py_gch_demo``."

import numpy as np
import pytest

# pylint: disable=no-name-in-module
from ..solvers import adam_optimizer


def test_adam_optimizer_sanity(linsvm, linsvm_x0, svm_data, adam_dummy_args):
    """Sanity check for ``adam_optimizer`` inputs.

    :param linsvm: ``pytest`` fixture. See package ``conftest.py``.
    :type linsvm: :class:`~py_gch_demo.models.PrimalLinearSVC`
    :param linsvm_x0: ``pytest`` fixture. See package ``conftest.py``.
    :type linsvm_x0: :class:`numpy.ndarray`
    :param svm_data: ``pytest`` fixture. See package ``conftest.py``.
    :type svm_data: tuple
    :param adam_dummy_args: ``pytest`` fixture. See package ``conftest.py``.
    :type adam_dummy_args: tuple
    """
    # unpack data and make reference to objective and gradient functions
    X, _, y, _ = svm_data
    obj, grad = linsvm._obj_func, linsvm._obj_grad
    # positional args that should be passed to a correct adam_optimizer call
    req_args = obj, grad, linsvm_x0, (X, y)
    # obj, grad must be callable
    with pytest.raises(TypeError, match = "obj must be callable"):
        adam_optimizer(None, grad, linsvm_x0)
    with pytest.raises(TypeError, match = "grad must be callable"):
        adam_optimizer(obj, None, linsvm_x0)
    # x0 must be an int or double array
    with pytest.raises(
        TypeError, match = "x0 must contain either ints or floats"
    ):
        adam_optimizer(obj, grad, np.array([None, "3", "not_a_num"]))
    # TypeError raised if casting is unsafe
    with pytest.raises(TypeError):
        adam_optimizer(obj, grad, np.array([1, 2, 3, 4], dtype = np.float128))
    # max_iter must be an int and must be positive
    with pytest.raises(TypeError):
        adam_optimizer(*req_args, max_iter = "oof")
    with pytest.raises(ValueError, match = "max_iter must be positive"):
        adam_optimizer(*req_args, max_iter = 0)
    # alpha must be float and must be positive
    with pytest.raises(TypeError):
        adam_optimizer(*req_args, alpha = "oowee")
    with pytest.raises(ValueError, match = "alpha must be positive"):
        adam_optimizer(*req_args, alpha = 0)
    # eps must be float and must be positive
    with pytest.raises(TypeError):
        adam_optimizer(*req_args, eps = "wahh")
    with pytest.raises(ValueError, match = "eps must be positive"):
        adam_optimizer(*req_args, eps = 0)
    # n_iter_no_change must be int and nonnegative
    with pytest.raises(TypeError):
        adam_optimizer(*req_args, n_iter_no_change = dict())
    with pytest.raises(
        ValueError, match = "n_iter_no_change must be nonnegative"
    ):
        adam_optimizer(*req_args, n_iter_no_change = -1)
    # warning should be raised if eps is too large. use dummy args since the
    # PrimalLinearSVC hasn't been fitted yet. todo: set obj, grad for a
    # linear regression problem (simple objective)
    with pytest.warns(UserWarning, match = "eps exceeds 1e-1"):
        adam_optimizer(*adam_dummy_args, eps = 1)
    # beta_1 must be float and within [0, 1)
    with pytest.raises(TypeError):
        adam_optimizer(*req_args, beta_1 = ())
    with pytest.raises(ValueError, match = r"beta_1 must be inside \[0, 1\)"):
        adam_optimizer(*req_args, beta_1 = 1)
    # beta_2 must be float and within [0, 1)
    with pytest.raises(TypeError):
        adam_optimizer(*req_args, beta_2 = ())
    with pytest.raises(ValueError, match = r"beta_2 must be inside \[0, 1\)"):
        adam_optimizer(*req_args, beta_2 = 1)