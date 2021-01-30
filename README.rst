.. README.rst for py_gc_helpers

py_gch
======

A lightweight API for enabling/disabling Python garbage collection implemented
in the `gc`__ module from Python C extension code or C/C++ code embedding the
CPython interpreter.

Inspired by the fact that garbage collection implemented in `gcmodule.c`__ for
CPython doesn't expose the same API in C as it does in Python. That is, there
are no C analogues to calling ``gc.disable``, ``gc.enable`` and so on [#]_.

.. __: https://docs.python.org/3/library/gc.html

.. __: https://github.com/python/cpython/blob/master/Modules/gcmodule.c

.. [#] ``gc.disable`` calls `gc_disable_impl`__, which is static.

.. __: https://github.com/python/cpython/blob/master/Modules/gcmodule.c#L1499