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

#include "win32/tip/tip_composition_util.h"

#include <msctf.h>
#include <objbase.h>
#include <windows.h>
#include <wrl/client.h>

#include "win32/base/tsf_profile.h"

namespace mozc {
namespace win32 {
namespace tsf {

using Microsoft::WRL::ComPtr;

ComPtr<ITfComposition> TipCompositionUtil::GetComposition(
    const ComPtr<ITfContext> &context, TfEditCookie edit_cookie){
  ComPtr<ITfCompositionView> composition_view =
      GetCompositionView(context, edit_cookie);
  if (!composition_view) {
    return nullptr;
  }
  ComPtr<ITfComposition> composition;
  if (FAILED(composition_view.As(&composition))) {
    return nullptr;
  }
  return composition;
}

ComPtr<ITfCompositionView> TipCompositionUtil::GetCompositionView(
    const ComPtr<ITfContext> &context, TfEditCookie edit_cookie) {
  ComPtr<ITfContextComposition> context_composition;
  if (FAILED(context.As(&context_composition))) {
    return nullptr;
  }

  ComPtr<IEnumITfCompositionView> enum_composition;
  if (FAILED(context_composition->FindComposition(edit_cookie, nullptr,
                                                  &enum_composition))) {
    return nullptr;
  }

  while (true) {
    ComPtr<ITfCompositionView> composition_view;
    ULONG num_fetched = 0;
    if (enum_composition->Next(1, &composition_view, &num_fetched) != S_OK) {
      return nullptr;
    }
    if (num_fetched != 1) {
      return nullptr;
    }
    GUID clsid = GUID_NULL;
    if (FAILED(composition_view->GetOwnerClsid(&clsid))) {
      continue;
    }
    if (!::IsEqualCLSID(TsfProfile::GetTextServiceGuid(), clsid)) {
      continue;
    }
    // Although TSF supports multiple composition, Mozc uses only one
    // composition at the same time. So the first one must be the only one.
    return composition_view;
  }
}

HRESULT TipCompositionUtil::ClearDisplayAttributes(ITfContext *context,
                                                   ITfComposition *composition,
                                                   TfEditCookie write_cookie) {
  HRESULT result = S_OK;

  // Retrieve the current composition range.
  ComPtr<ITfRange> composition_range;
  result = composition->GetRange(&composition_range);
  if (FAILED(result)) {
    return result;
  }

  // Get out the display attribute property
  ComPtr<ITfProperty> display_attribute;
  result = context->GetProperty(GUID_PROP_ATTRIBUTE, &display_attribute);
  if (FAILED(result)) {
    return result;
  }
  // Clear existing attributes.
  result = display_attribute->Clear(write_cookie, composition_range.Get());
  if (FAILED(result)) {
    return result;
  }
  return S_OK;
}

}  // namespace tsf
}  // namespace win32
}  // namespace mozc
