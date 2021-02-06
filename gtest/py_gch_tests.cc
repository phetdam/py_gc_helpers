/**
 * @file py_gch_tests.cc
 * @brief `gtest` Unit tests for the API provided by `py_gch.h`.
 */

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include "gtest/gtest.h"

#include "gtest_helpers.h"
#include "py_gch.h"

// macros that make the debugging process easier
#define gc_module_ PYGCH_API_UNIQ_SYMBOL[0]
#define gc_module ((PyObject *) gc_module_)

// gtest doesn't allow namespace prefix before the PyReqTest name (why?)
using gtest_helpers::PyReqTest;

// empty namespace just to prevent global namespace pollution (good practice)

namespace {

class PyGCHTest: public PyReqTest {
  protected:
  // override TearDown to NULL-ify API after interpreter finalization
  void TearDown() override {
    // call super's TearDown method + _PyGCH_NULLIFY_API
    PyReqTest::TearDown();
    _PyGCH_NULLIFY_API();
  }
};

/**
 * Test that `PyGCH_gc_unique_import` really imports `gc` once.
 * 
 * @note Reference counting is sloppy here, as on failure there are a couple
 *     places where `Py_DECREF` is necessary.
 */
TEST_F(PyGCHTest, gcUniqueImport) {
  // run unique import. it should return true
  ASSERT_TRUE(PyGCH_gc_unique_import()) << "PyGCH_unique_import failed (1)";
  // current reference count to the gc module (isn't necessarily 1)
  Py_ssize_t refcnt = Py_REFCNT(gc_module);
  // run again to see if the reference count changes
  ASSERT_TRUE(PyGCH_gc_unique_import()) << "PyGCH_unique_import failed (2)";
  // reference count should not have changed. if it has, we should do cleanup,
  // but that's ok since the interpreter is going to be finalized anyways.
  ASSERT_EQ(Py_REFCNT(gc_module), refcnt)
    << "Reference count of gc module has changed";
} 

/**
 * Test that `PyGCH_gc_member_unique_import` imports members once.
 */
TEST_F(PyGCHTest, gcMemberUniqueImport) {
  // target to write PyObject * of imported member to. must start as  NULL.
  PyObject *member = NULL;
  // import gc.enable; PyGCH_gc_member_unique_import should return true
  ASSERT_TRUE(PyGCH_gc_member_unique_import("enable", &member))
    << "PyGCH_gc_member_unique_import failed (1)";
  // get reference count of member
  Py_ssize_t refcnt = Py_REFCNT(member);
  // call PyGCH_gc_member_unique_import again (should return true)
  ASSERT_TRUE(PyGCH_gc_member_unique_import("enable", &member))
    << "PyGCH_gc_member_unique_import failed (2)";
  // reference count should not have changed
  ASSERT_EQ(Py_REFCNT(member), refcnt)
    << "Reference count of gc member (gc.enable) has changed";
} 

/**
 * Test that `PyGCH_gc_enable`, `PyGCH_gc_disable`, `PyGCH_gc_isenabled` work.
 * 
 * @note We have to test them all together because `PyGCH_gc_isenabled` is
 *     needed to check whether the collector is enabled/disabled.
 */
TEST_F(PyGCHTest, testEnableDisable) {
  // borrowed ref returned by PyGCH_is_gc_enabled should be Py_True
  ASSERT_EQ(Py_True, PyGCH_gc_isenabled())
    << "PyGCH_gc_isenabled did not return Py_True";
  // PyGCH_gc_disable should return Py_None + PyGCH_gc_isenabled should then
  // be equal to Py_False (all refs are borrowed)
  ASSERT_EQ(Py_None, PyGCH_gc_disable())
    << "PyGCH_gc_disable did not return Py_None";
  ASSERT_EQ(Py_False, PyGCH_gc_isenabled())
    << "PyGCH_gc_isenabled did not return Py_False";
  // PyGCH_gc_enable should return Py_None + PyGCH_gc_isenabled should then
  // return a borrowed ref to Py_True
  ASSERT_EQ(Py_None, PyGCH_gc_enable())
    << "PyGCH_gc_enable did not return Py_None";
  ASSERT_EQ(Py_True, PyGCH_gc_isenabled())
    << "PyGCH_gc_isenabled did not return Py_True";
}

} /* namespace */