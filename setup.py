# setup.py for building py_gch_examples package. extension modules can't be
# built without setup.py so we can't use the new PEP 517 format.

from numpy import get_include
from setuptools import Extension, setup

# package name (also the name of the extension module)
_PACKAGE_NAME = "py_gch_demo"
# project URL
_PROJECT_URL = "https://github.com/phetdam/py_gch"
# name of the C extension module (also name of its source .c file)
_EXT_NAME = "solvers"


def _setup():
    # get version
    with open("VERSION", "r") as vf:
        version = vf.read().rstrip()
    # short and long descriptions
    short_desc = (
        "An example Python package containing a C extension module that uses "
        "the C API to gc provided by the py_gch.h header file."
    )
    with open("pkg_longdesc.rst", "r") as rf:
        long_desc = rf.read()
    # perform setup
    setup(
        name = _PACKAGE_NAME,
        version = version,
        description = short_desc,
        long_description = long_desc,
        long_description_content_type = "text/x-rst",
        author = "Derek Huang",
        author_email = "djh458@stern.nyu.edu",
        license = "MIT",
        url = _PROJECT_URL,
        classifiers = [
            "License :: OSI Approved :: MIT License",
            "Operating System :: POSIX :: Linux",
            "Programming Language :: Python :: 3.6",
            "Programming Language :: Python :: 3.7",
            "Programming Language :: Python :: 3.8"
        ],
        project_urls = {"Source": _PROJECT_URL},
        python_requires = ">=3.6",
        packages = [_PACKAGE_NAME, _PACKAGE_NAME + ".tests"],
        install_requires = ["numpy>=1.19"],
        # no extra package name for the extension module
        ext_package = _PACKAGE_NAME,
        ext_modules = [
            Extension(
                name = _EXT_NAME,
                # allows numpy and py_gch.h to be included
                include_dirs = [get_include(), "include"],
                sources = [
                    _PACKAGE_NAME + "/" + _EXT_NAME + ".c"
                ],
            )
        ]
    )


if __name__ == "__main__":
    _setup()