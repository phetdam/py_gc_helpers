__doc__ = "Models for the Adam optimizer to try and optimize."

from functools import partial
import numpy as np


class PrimalLinearSVC:
    r"""Simple linear support vector machine class. Primal formulation only.

    Fitting by stochastic subgradient descent is done with the Adam optimizer.
    Any parameters not described below are passed to the Adam optimizer.

    .. note::

       Only supports binary classification tasks.

    Initial parameter vector is drawn from a multivariate Gaussian with zero
    mean and identity covariance.

    :param reg_lambda: Coefficient scaling the squared :math:`\ell_2`-norm of
        the weight vector in the objective. Increase for more regularization/
        weight decay.
    :type reg_lambda: float, optional
    :param batch_size: Batch size to evaluate objective and compute gradient
        with. If float in :math:`(0, 1)`, then the batch size is
        ``batch_size * n_samples``.
    :type batch_size: float or int, optional
    :param seed: Seed for the RNGs used by the objective and gradient functions
        when performing stochastic gradient descent.
    :type seed: int, optional
    """
    def __init__(
        self, reg_lambda = 1, batch_size = 0.2, max_iter = 200,
        n_iter_no_change = 10, tol = 1e-4, alpha = 0.001, beta_1 = 0.9,
        beta_2 = 0.999, eps = 1e-8, disable_gc = False, seed = None
    ):
        # check and set new parameters that aren't checked by optimizer
        if isinstance(reg_lambda, (int, float)):
            if reg_lambda < 0:
                raise ValueError("reg_lambda must be nonnegative")
        else:
            raise TypeError("reg_lambda must be int or float")
        self.reg_lambda = reg_lambda
        if isinstance(batch_size, float):
            if batch_size <= 0 or batch_size >= 1:
                raise ValueError("float batch_size must be in (0, 1)")
        elif isinstance(batch_size, int):
            if batch_size < 1:
                raise ValueError("int batch_size must be positive")
        else:
            raise TypeError("batch_size must be int or float")
        self.batch_size = batch_size
        # if seed is None, draw a random number to seed the RNG with
        if seed is None:
            rng = np.random.default_rng()
            seed = rng.integers(9999)
        if isinstance(seed, int):
            if seed <= 0:
                raise ValueError("seed must be positive")
        else:
            raise TypeError("seed must be int")
        self.seed = seed
        # initialize RNG
        self._rng = np.random.default_rng(self.seed)
        # optimizer parameters are all checked by the optimization routine
        self.max_iter = max_iter
        self.n_iter_no_change = n_iter_no_change
        self.tol = tol
        self.alpha = alpha
        self.beta_1 = beta_1
        self.beta_2 = beta_2
        self.eps = eps
        self.disable_gc = disable_gc

    def _obj_func(self, wb, X, y):
        """Objective function for the :class:`PrimalLinearSVC`.

        .. note::

           Users should not call this function. Consider it internal.

        .. note::

           As per correct practice, the bias term is *not* penalized, i.e. the
           value of :attr:`reg_lambda` has no effect. The objective is also
           evaluated on the *entire* training set unlike the gradient, which is
           a minibatch gradient.

        :param wb: Weight vector and bias, shape ``(n_features + 1,)``.
        :type wb: :class:`numpy.ndarray`
        :param X: Input matrix, shape ``(n_samples, n_features)``
        :type X: :class:`numpy.ndarray`
        :param y: Vector of output labels, shape ``(n_samples,)``. Must
            only contain two unique label values.
        :rtype: float
        """
        w, b = wb[:-1], wb[-1]
        return (
            np.maximum(0, 1 - y * (X @ w + b)).sum() +
            self.reg_lambda * np.power(w, 2).sum()
        )

    def _obj_grad(self, wb, X, y):
        """Gradient of :meth:`_obj_func` for the :class:`PrimalLinearSVC`.

        .. note::

           Users should not call this function. Consider it internal.

        .. note::

           As per correct practice, the bias term is *not* penalized, i.e. the
           value of :attr:`reg_lambda` has no effect on the intercept change.
           However, it is obvious from the formula that the bias update is
           very crude, essentially +/- the learning rate. Also, unlike the
           objective, the gradient is evaluated on a minibatch only.

        :param wb: Weight vector and bias, shape ``(n_features + 1,)``.
        :type wb: :class:`numpy.ndarray`
        :param X: Input matrix, shape ``(n_samples, n_features)``
        :type X: :class:`numpy.ndarray`
        :param y: Vector of output labels, shape ``(n_samples,)``. Must
            only contain two unique label values.
        :rtype: :class:`numpy.ndarray`
        """
        # get indices for minibatch, size self.minibatch_size_. we don't draw
        # with replacement since we want a strict subset
        idx = self._rng.choice(
            y.size, size = self.minibatch_size_, replace = False
        )
        # get X, y minibatches and w, b
        X_batch, y_batch, w, b = X[idx], y[idx], wb[:-1], wb[-1]
        # array indicating which examples contribute to the gradient
        update_ind = np.where(y_batch * (X_batch @ w + b) < 1, 1, 0)
        # compute w gradient and b derivative
        grad_w = (
            -(update_ind * y_batch * X_batch.T).sum(axis = 1) +
            2 * self.reg_lambda * w
        )
        grad_b = -(update_ind * y_batch).sum()
        # return w, b gradient
        return np.append(grad_w, grad_b)

    def fit(self, X, y):
        """Fit the linear support vector machine using Adam.

        :param X: Input matrix, shape ``(n_samples, n_features)``
        :type X: :class:`numpy.ndarray`
        :param y: Vector of output labels, shape ``(n_samples,)``. Must
            only contain two unique label values.
        :type y: :class:`numpy.ndarray`
        :rtype: self
        """
        # check input and output shapes
        if X.shape[0] != y.shape[0]:
            raise ValueError("X, y must have the same number of samples")
        if len(X.shape) != 2:
            raise ValueError("X must have shape (n_samples, n_features)")
        if len(y.shape) != 1:
            raise ValueError("y must have shape (n_features,)")
        # check that self.batch_size doesn't exceed len(y)
        if self.batch_size >= y.size:
            raise ValueError(
                "batch_size cannot exceed size of data set. Pass a float in "
                "(0, 1) to have the batch size scale with the data size"
            )
        # check that y only has two unique labels
        labels = np.unique(y)
        if len(labels) != 2:
            raise RuntimeError(
                "Model can only fit on binary classification tasks. Expected "
                f"two unique labels, got {len(labels)}"
            )
        # sort labels and set as attribute
        labels.sort()
        self.classes_ = labels
        # translate y in to -1, 1
        y = np.where(y == labels[0], -1, 1)
        # set n_features as attribute
        self.n_features_ = X.shape[1]
        # batch size to be passed to the gradient function. need to handle the
        # the case where batch_size is a float and when it is an int
        if self.batch_size > 0 and self.batch_size < 1:
            self.minibatch_size_ = int(self.batch_size * y.size)
        else:
            self.minibatch_size_ = self.batch_size
        # initial random parameter guess
        x0 = self._rng.multivariate_normal(
            np.zeros(self.n_features_), np.eye(self.n_features_)
        )
        # pass objective, gradient functions, X, y hyperparams to optimizer.
        # the returned numpy.ndarray are the final chosen parameters.
        """
        params = adam_optimizer(
            self._obj_func, self._obj_grad, x0, args = (X, y),
            max_iter = self.max_iter, n_iter_no_change = self.n_iter_no_change,
            tol = self.tol, alpha = self.alpha, beta_1 = self.beta_1,
            beta_2 = self.beta_2, eps = self.eps, disable_gc = self.disable_gc
        )
        """
        # set weights (coef_) and intercept/bias (intercept_)
        #self.coef_ = params[:-1]
        #self.intercept_ = params[-1]
        # return self for method chaining
        return self