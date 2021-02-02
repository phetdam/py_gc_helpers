# Makefile to build py_gch_example, test runner, and C++ demo.

# package name/source folder for extension source
PKG_NAME       = py_gch_demo
# directory for libcheck test runner code
CHECK_DIR      = check
# C and C++ compilers, of course
CC             = gcc
CPP            = g++
# dependencies for the extension modules
XDEPS          = $(wildcard $(PKG_NAME)/*.c)
# dependencies for test running code
CHECK_DEPS     = $(wildcard $(CHECK_DIR)/*.c)
# required Python source files in the package (modules and tests)
PYDEPS         = $(wildcard $(PKG_NAME)/*.py) $(wildcard $(PKG_NAME)/*/*.py)
# set python; on docker specify PYTHON value externally using absolute path
PYTHON        ?= python3
# flags to pass to setup.py build
BUILD_FLAGS    =
# directory to save distributions to; use absolute path on docker
DIST_FLAGS    ?= --dist-dir ./dist
# python compiler and linker flags for use when linking python into external C
# code; can be externally specified. gcc requires -fPIE.
PY_CFLAGS     ?= -fPIE $(shell python3-config --cflags)
# ubuntu needs --embed, else -lpythonx.y is omitted by --ldflags, which is a
# linker error. libpython3.8 is in /usr/lib/x86_64-linux-gnu for me.
PY_LDFLAGS    ?= $(shell python3-config --embed --ldflags)
# compile flags for compiling test runner. my libcheck is in /usr/local/lib.
# if C++: -Wl,--version-script,<version_script_file> might be needed later if i
# want better control over symbol export. -fvisibility=hidden is used for now
# to make sure none of the runner symbols are exported.
CHECK_CFLAGS   = $(PY_CFLAGS) -Iinclude # -fvisibility=hidden
# linker flags for compiling test runner
CHECK_LDFLAGS  = $(PY_LDFLAGS) -lcheck
# flags to pass to the libcheck test runner
RUNNER_FLAGS   =

# phony targets
.PHONY: clean dummy dist

# triggered if no target is provided
dummy:
	@echo "Please specify a target to build."

# removes local build, dist, egg-info
clean:
	@rm -vf *~
	@rm -vrf build
	@rm -vrf $(PKG_NAME).egg-info
	@rm -vrf dist

# build extension module locally in ./build from source files with setup.py
# triggers when any of the files that are required are touched/modified.
build: $(PYDEPS) $(XDEPS)
	@$(PYTHON) setup.py build $(BUILD_FLAGS)

# build in-place with build_ext --inplace. shared object for extension module
# will show up in the directory PKG_NAME.
inplace: $(XDEPS)
	@$(PYTHON) setup.py build_ext --inplace

# build test runner and run unit tests using check. show flags passed to g++
check: $(CHECK_DEPS) inplace
	$(CC) $(CHECK_CFLAGS) -o runner $(CHECK_DEPS) $(CHECK_LDFLAGS)
	@./runner $(RUNNER_FLAGS)

# make source and wheel
dist: build
	@$(PYTHON) setup.py sdist bdist_wheel $(DIST_FLAGS)