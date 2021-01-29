# setup.py for building py_gch_examples package. extension modules can't be
# built without setup.py so we can't use the new PEP 517 format.

from setuptools import Extension, setup

# package name (also the name of the extension module)
_PACKAGE_NAME = "py_gch_example"
# project URL
_PROJECT_URL = "https://github.com/phetdam/py_gch"


def _setup():
    # get version
    with open("examples/ext_version", "r") as vf:
        version = vf.read().rstrip()
    # short and long descriptions
    short_desc = (
        "An example Python C extension module that uses the C API to gc "
        "provided by the py_gch.h header file."
    )
    with open("examples/ext_longdesc.rst", "r") as rf:
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
        # no extra package name for the extension module
        ext_package = _PACKAGE_NAME,
        ext_modules = [
            Extension(
                name = "ext_example",
                # allows py_gch.h to be included
                include_dirs = ["include"],
                sources = ["examples/" + _PACKAGE_NAME + ".c"],
            ),
        ]
    )


if __name__ == "__main__":
    _setup()