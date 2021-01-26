.. README.rst for py_gc_helpers

py_gc_helpers
=============

Macros for enabling/disabling Python garbage collection from C extension code.

I wrote these macros since the garbage collection implemented in `gcmodule.c`__
for CPython doesn't expose the same API in C as it does in Python. That is,
there are no C analogues to calling ``gc.disable``, ``gc.enable`` and so on
[#]_.

.. __: https://github.com/python/cpython/blob/master/Modules/gcmodule.c

.. [#] The function called by ``gc.disable`` in the C source code is
   `gc_disable_impl`__ and is thus not part of the public API.

.. __: https://github.com/python/cpython/blob/master/Modules/gcmodule.c#L1499