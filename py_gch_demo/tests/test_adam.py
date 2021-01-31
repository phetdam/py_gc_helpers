__doc__ = "Unit tests for the Adam implementation in ``py_gch_demo``."

import numpy as np
import pytest

# pylint: disable=no-name-in-module
from ..solvers import adam_optimizer


def test_adam_optimizer_sanity(linsvm, linsvm_x0, svm_data):
    """Sanity check for ``adam_optimizer`` inputs.

    :param linsvm: ``pytest`` fixture. See package ``conftest.py``.
    :type linsvm: :class:`~py_gch_demo.models.PrimalLinearSVC`
    :param linsvm_x0: ``pytest`` fixture. See package ``conftest.py``.
    :type linsvm_x0: :class:`numpy.ndarray`
    :param svm_data: ``pytest`` fixture. See package ``conftest.py``.
    :type svm_data: tuple
    """
    # unpack data and make reference to objective and gradient functions
    X, _, y, _ = svm_data
    obj, grad = linsvm._obj_func, linsvm._obj_grad
    # positional args that should be passed to a correct adam_optimizer call
    req_args = obj, grad, linsvm_x0, (X, y)
    # obj, grad must be callable
    with pytest.raises(TypeError, match = "obj must be callable"):
        adam_optimizer(None, grad, linsvm_x0, args = (X, y))
    with pytest.raises(TypeError, match = "grad must be callable"):
        adam_optimizer(obj, None, linsvm_x0, args = (X, y))
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