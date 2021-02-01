__doc__ = ":class:`py_gch_example.solvers.GradSolverResult` unit tests."

import numpy as np
import pytest

# pylint: disable=no-name-in-module
from ..solvers import GradSolverResult


@pytest.fixture(scope = "module")
def gsr_args():
    """Valid initialization arguments for ``GradSolverResult`` instance.

    In order, these are the final parameter guess, value of the objective
    function, value of the last evaluated gradient, number of times the
    objective was evaluated, number of times the gradient was evaluated, and
    the total number of iterations run.

    :rtype: tuple
    """
    return (
        np.array([1.2, 3.2, 0.9]), 19.2, np.array([1.2e-3, 1.33e-5, 5.6e-2]),
        122, 121, 121
    )


def test_GradSolverResult_sanity(gsr_args, tuple_replace):
    """Sanity checks for the inputs passed to ``GradSolverResult.__new__``.

    :param gsr_args: ``pytest`` fixture. See :func:`gsr_args`.
    :type gsr_args: tuple
    :param tuple_replace: ``pytest`` fixture. See package ``conftest.py``.
    :type tuple_replace: function
    """
    # parameter guess must be numpy.ndarray
    with pytest.raises(TypeError):
        GradSolverResult(*tuple_replace(gsr_args, (0, [1, 2, 1, 2])))
    # parameter guess must have at least one dimension
    with pytest.raises(ValueError, match = "x must have at least 1 dimension"):
        GradSolverResult(*tuple_replace(gsr_args, (0, np.array(9))))
    # parameter guess and gradient value must have the same dimensions
    with pytest.raises(
        ValueError, match = "x, grad must have the same number of dimensions"
    ):
        GradSolverResult(
            *tuple_replace(gsr_args, (0, np.array([[1, 2], [9, 3]])))
        )
    # parameter guess and gradient value must have the same shape
    with pytest.raises(ValueError, match = "x, grad shapes differ on axis 0"):
        GradSolverResult(*tuple_replace(gsr_args, (0, np.array([1, 2]))))
    # objective value must be castable to double
    with pytest.raises(TypeError):
        GradSolverResult(*tuple_replace(gsr_args, (1, "cheese")))
    # gradient value must be numpy.ndarray
    with pytest.raises(TypeError):
        GradSolverResult(*tuple_replace(gsr_args, (2, [1, 2, 1, 2])))
    # gradient value must have at least one dimension
    with pytest.raises(ValueError):
        GradSolverResult(*tuple_replace(gsr_args, (2, np.array(8))))
    # number of objective evals must be int
    with pytest.raises(TypeError):
        GradSolverResult(*tuple_replace(gsr_args, (3, 1.9)))
    # number of objective evals must be positive
    with pytest.raises(ValueError, match = "n_obj_eval must be positive"):
        GradSolverResult(*tuple_replace(gsr_args, (3, 0)))
    # number of gradient evals must be int
    with pytest.raises(TypeError):
        GradSolverResult(*tuple_replace(gsr_args, (4, 1.99)))
    # number of gradient evals must be int
    with pytest.raises(ValueError, match = "n_grad_eval must be positive"):
        GradSolverResult(*tuple_replace(gsr_args, (4, 0)))
    # number of iterations must be int
    with pytest.raises(TypeError):
        GradSolverResult(*tuple_replace(gsr_args, (5, 9.8)))
    # number of iterations must be positive
    with pytest.raises(ValueError, match = "n_iter must be positive"):
        GradSolverResult(*tuple_replace(gsr_args, (5, 0)))