.. README.rst for py_gc_helpers

py_gc_helpers
=============

Macros for enabling/disabling Python garbage collection from C extension code.

I wrote these macros since the garbage collection implemented in `gcmodule.c`__
for CPython doesn't expose the same API in C as it does in Python. That is,
there are no C analogues to calling ``gc.disable``, ``gc.enable`` and so on
[#]_.

.. __: https://github.com/python/cpython/blob/master/Modules/gcmodule.c

.. [#] ``gc.disable`` calls `gc_disable_impl`__, which is static.

.. __: https://github.com/python/cpython/blob/master/Modules/gcmodule.c#L1499