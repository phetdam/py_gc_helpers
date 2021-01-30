__doc__ = "Utilities for data set generation."

from functools import partial
import numpy as np


def make_linear_regression(
    n_train = 800, n_test = 200, n_features = 10, feature_cov = None,
    output_mean = 0, output_std = 1, rng = None
):
    """Returns data suitable for fitting a linear regression model.

    Input features are zero-mean Gaussian; their covariance may be specified
    with a covariance matrix. The output mean (model bias) and standard
    deviation of Gaussian errors of the output may also be specified.

    The "true" parameters of the model are drawn uniformly and independently
    from the unit interval.

    :param n_train: Number of training samples to generate
    :type n_train: int, optional
    :param n_test: Number of test samples to generate
    :type n_test: int, optional
    :param n_features: Input dimensionality
    :type n_features: int, optional
    :param feature_cov: Optional covariance matrix for the input features. If
        not provided, the identity matrix will be used. If a float is provided,
        then ``feature_cov`` times the identity matrix will be used.
    :type feature_cov: float or :class:`numpy.ndarray`, optional
    :param output_mean: Mean of the regression output. Since inputs are
        centered, it is identical to the true bias/intercept coefficient term
        in a linear regression model.
    :type output_mean: float, optional
    :param output_std: Standard deviation of the Gaussian regression errors.
    :type output_std: float, optional
    :param rng: Int seed for the new ``numpy`` RNG or an existing instance
    :type rng: int or :class:`numpy.random._generator.Generator`, optional
    :returns: 4-tuple of ``X_train, X_test, y_train, y_test`` data
    :rtype: tuple
    """
    # set defaults
    if feature_cov is None:
        feature_cov = np.eye(n_features)
    elif isinstance(feature_cov, (int, float)):
        feature_cov = feature_cov * np.eye(n_features)
    if rng is None:
        rng = np.random.default_rng()
    elif isinstance(rng, int):
        rng = np.random.default_rng(rng)
    # get "true" parameters
    weights = rng.random(size = n_features)
    # callable to generate input examples. we pass check_valid = "raise" so
    # that an exception is raised if feature_cov is not positive semidefinite
    make_inputs = partial(
        rng.multivariate_normal, np.zeros(n_features), feature_cov,
        check_valid = "raise"
    )
    # callable to generate additive noise for outputs
    output_noise = partial(rng.normal, scale = output_std)
    # make training and test data + return
    X_train, X_test = make_inputs(size = n_train), make_inputs(size = n_test)
    y_train = X_train @ weights + output_mean + output_noise(size = n_train)
    y_test = X_test @ weights + output_mean + output_noise(size = n_test)
    return X_train, X_test, y_train, y_test


def make_linear_binary_classification(
    n_train = 800, n_test = 200, n_features = 10, feature_cov = None,
    output_mean = 8, output_std = 1, label_type = "0-1", rng = None
):
    r"""Returns data suitable for fitting a linear binary classification model.

    A wrapper around :func:`make_linear_regression`.

    Input features are zero-mean Gaussian; their covariance may be specified
    with a covariance matrix. The output mean (model bias) and standard
    deviation of Gaussian errors of the output may also be specified. For this
    function, the output mean also serves as a threshold to distinguish between
    positive (above threshold) and negative (below threshold) examples.

    The "true" parameters of the model are drawn uniformly and independently
    from the unit interval.

    :param n_train: Number of training samples to generate
    :type n_train: int, optional
    :param n_test: Number of test samples to generate
    :type n_test: int, optional
    :param n_features: Input dimensionality
    :type n_features: int, optional
    :param feature_cov: Optional covariance matrix for the input features. If
        not provided, the identity matrix will be used. If a float is provided,
        then ``feature_cov`` times the identity matrix will be used.
    :type feature_cov: float or :class:`numpy.ndarray`, optional
    :param output_mean: Mean of the regression output. Since inputs are
        centered, it is identical to the true bias/intercept coefficient term
        in a linear regression model.
    :type output_mean: float, optional
    :param output_std: Standard deviation of the Gaussian regression errors.
    :type output_std: float, optional
    :param label_type: Type of binary labels to generate. Pass ``"0-1"`` to
        have labels from :math:`\{0, 1\}` or ``"+/-1" for labels from
        :math:`\{-1, 1\}`.
    :type label_type: str, optional
    :param rng: Int seed for the new ``numpy`` RNG or an existing instance
    :type rng: int or :class:`numpy.random._generator.Generator`, optional
    :returns: 4-tuple of ``X_train, X_test, y_train, y_test`` data
    :rtype: tuple
    """
    # check and set labels
    if label_type != "0-1" and label_type != "+/-1":
        raise ValueError("label_type must be either \"0-1\" or \"+/-1\"")
    if label_type == "0-1":
        labels = np.array([0, 1])
    elif label_type == "+/-1":
        labels = np.array([-1, 1])
    # get linear regression problem from make_linear_regression
    X_train, X_test, y_train, y_test = make_linear_regression(
        n_train = n_train, n_test = n_test, n_features = n_features,
        feature_cov = feature_cov, output_mean = output_mean,
        output_std = output_std, rng = rng
    )
    # replace y_train, y_test with labels
    y_train = np.where(y_train < output_mean, labels[0], labels[1])
    y_test = np.where(y_test < output_mean, labels[0], labels[1])
    # done, return new data
    return X_train, X_test, y_train, y_test