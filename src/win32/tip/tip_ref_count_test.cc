// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "win32/tip/tip_ref_count.h"
#include "testing/googletest.h"
#include "testing/gunit.h"
#include "win32/tip/tip_dll_module.h"

namespace mozc {
namespace win32 {
namespace tsf {

class TipRefCountTest : public testing::Test {
 protected:
  static void SetUpTestCase() { TipDllModule::InitForUnitTest(); }
};

TEST_F(TipRefCountTest, AddRefRelease) {
  TipRefCount ref_count;

  EXPECT_EQ(ref_count.AddRefImpl(), 1) << "Initial count is zero.";
  EXPECT_EQ(ref_count.AddRefImpl(), 2);
  EXPECT_EQ(ref_count.ReleaseImpl(), 1);
  EXPECT_EQ(ref_count.ReleaseImpl(), 0);
  EXPECT_EQ(ref_count.ReleaseImpl(), 0);
}

TEST_F(TipRefCountTest, DllLock) {
  {
    TipRefCount ref_count;
    EXPECT_FALSE(TipDllModule::CanUnload());
  }
  EXPECT_TRUE(TipDllModule::CanUnload());
}

}  // namespace tsf
}  // namespace win32
}  // namespace mozc
