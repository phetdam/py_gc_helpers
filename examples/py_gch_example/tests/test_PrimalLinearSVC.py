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