.. README.rst for py_gc_helpers

py_gch
======

   Note:

   Development has halted indefinitely. An issue arose during testing where
   calls to ``gc.collect`` using `PyObject_CallObject`__ would irregularly
   cause the interpreter to crash with the message "Fatal Python error:
   unexpected exception during garbage collection". This seems tied to the
   implementation of `gc_collect_with_callback`__, whose code is surrounded
   by two ``assert`` statements checking whether the error indicator has been
   set or not. One unit test purposefully sets the error indicator by passing
   an invalid generation number; this may have caused the crash, but the
   effects don't seem to be reproducible.

A lightweight API for enabling/disabling Python garbage collection implemented
in the `gc`__ module from Python C extension code or C/C++ code embedding the
CPython interpreter.

Inspired by the fact that garbage collection implemented in `gcmodule.c`__ for
CPython doesn't expose the same API in C as it does in Python. That is, there
are no C analogues to calling ``gc.disable``, ``gc.enable`` and so on [#]_.

.. __: https://docs.python.org/3/c-api/call.html#c.PyObject_CallObject

.. __: https://github.com/python/cpython/blob/main/Modules/
   gcmodule.c#L1407-L1417


Quickstart
----------

All you need is ``py_gch.h`` from the ``include`` directory. You can either
copy it directly into your project or copy it into ``/usr/local/include``
or any other include directory checked by your C/C++ preprocessor.

Python C extension modules
~~~~~~~~~~~~~~~~~~~~~~~~~~

TBA.

C/C++ code ``#include``\ ing ``Python.h``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TBA.

Contents
--------

TBA.

.. __: https://docs.python.org/3/library/gc.html

.. __: https://github.com/python/cpython/blob/master/Modules/gcmodule.c

.. [#] ``gc.disable`` calls `gc_disable_impl`__, which is static.

.. __: https://github.com/python/cpython/blob/master/Modules/gcmodule.c#L1499