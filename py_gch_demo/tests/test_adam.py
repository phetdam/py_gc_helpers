__doc__ = "Unit tests for the Adam implementation in ``py_gch_example``."

import numpy as np
import pytest

# pylint: disable=no-name-in-module
from ..solvers import adam_optimizer


@pytest.mark.skip(reason = "not implemented")
def test_adam_optimizer_sanity(svm_data):
    """Sanity check for ``adam_optimizer`` inputs.

    :param svm_data: ``pytest`` fixture. See package ``conftest.py``.
    :type svm_data: tuple
    """
    # unpack data
    X_train, X_test, y_train, y_test = svm_data