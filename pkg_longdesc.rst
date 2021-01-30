.. long description for the py_gch_demo example package

py_gch_demo
===========

An example package containing a Python C extension module that uses the C API
to ``gc`` provided by ``py_gch.h``.

Implements a linear support vector machine trained through stochastic minibatch
subgradient descent. The optimizer is Kingma and Ba's `Adam algorithm`__,
implemented within a C extension module.

.. __: https://arxiv.org/pdf/1412.6980.pdf