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

#include "win32/tip/tip_candidate_list.h"

#include <ctffunc.h>
#include <objbase.h>
#include <oleauto.h>
#include <wrl/client.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/win32/com.h"
#include "win32/tip/tip_ref_count.h"

namespace mozc {
namespace win32 {
namespace tsf {

namespace {

using Microsoft::WRL::ComPtr;

class CandidateStringImpl final : public ITfCandidateString {
 public:
  CandidateStringImpl(ULONG index, const std::wstring_view value)
      : index_(index), value_(value) {}
  CandidateStringImpl(const CandidateStringImpl &) = delete;
  CandidateStringImpl &operator=(const CandidateStringImpl &) = delete;

  // The IUnknown interface methods.
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID &interface_id,
                                                   void **object) {
    if (!object) {
      return E_INVALIDARG;
    }

    // Find a matching interface from the ones implemented by this object.
    // This object implements IUnknown and ITfEditSession.
    if (::IsEqualIID(interface_id, IID_IUnknown)) {
      *object = static_cast<IUnknown *>(this);
    } else if (IsEqualIID(interface_id, IID_ITfCandidateString)) {
      *object = static_cast<ITfCandidateString *>(this);
    } else {
      *object = nullptr;
      return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef() { return ref_count_.AddRefImpl(); }

  virtual ULONG STDMETHODCALLTYPE Release() {
    const ULONG count = ref_count_.ReleaseImpl();
    if (count == 0) {
      delete this;
    }
    return count;
  }

 private:
  // The ITfCandidateString interface methods.
  virtual HRESULT STDMETHODCALLTYPE GetString(BSTR *str) {
    if (str == nullptr) {
      return E_INVALIDARG;
    }
    *str = ::SysAllocStringLen(value_.data(), value_.size());
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetIndex(ULONG *index) {
    if (index == nullptr) {
      return E_INVALIDARG;
    }
    *index = index_;
    return S_OK;
  }

  TipRefCount ref_count_;
  const ULONG index_;
  const std::wstring value_;
};

class EnumTfCandidatesImpl final : public IEnumTfCandidates {
 public:
  explicit EnumTfCandidatesImpl(const std::vector<std::wstring> &candidates)
      : candidates_(candidates), current_(0) {}
  EnumTfCandidatesImpl(const EnumTfCandidatesImpl &) = delete;
  EnumTfCandidatesImpl &operator=(const EnumTfCandidatesImpl &) = delete;

  // The IUnknown interface methods.
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID &interface_id,
                                                   void **object) {
    if (!object) {
      return E_INVALIDARG;
    }

    // Find a matching interface from the ones implemented by this object.
    // This object implements IUnknown and ITfEditSession.
    if (::IsEqualIID(interface_id, IID_IUnknown)) {
      *object = static_cast<IUnknown *>(this);
    } else if (IsEqualIID(interface_id, IID_IEnumTfCandidates)) {
      *object = static_cast<IEnumTfCandidates *>(this);
    } else {
      *object = nullptr;
      return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef() { return ref_count_.AddRefImpl(); }

  virtual ULONG STDMETHODCALLTYPE Release() {
    const ULONG count = ref_count_.ReleaseImpl();
    if (count == 0) {
      delete this;
    }
    return count;
  }

 private:
  virtual HRESULT STDMETHODCALLTYPE Clone(IEnumTfCandidates **enum_candidates) {
    if (enum_candidates == nullptr) {
      return E_INVALIDARG;
    }
    auto impl = MakeComPtr<EnumTfCandidatesImpl>(candidates_);
    if (!impl) {
      return E_OUTOFMEMORY;
    }
    *enum_candidates = impl.Detach();
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE Next(ULONG count,
                                         ITfCandidateString **candidate_string,
                                         ULONG *fetched_count) {
    if (candidate_string == nullptr) {
      return E_INVALIDARG;
    }
    ULONG dummy = 0;
    if (fetched_count == nullptr) {
      fetched_count = &dummy;
    }
    const auto candidates_size = candidates_.size();
    *fetched_count = 0;
    for (ULONG i = 0; i < count; ++i) {
      if (current_ >= candidates_size) {
        *fetched_count = i;
        return S_FALSE;
      }
      candidate_string[i] =
          MakeComPtr<CandidateStringImpl>(current_, candidates_[current_])
              .Detach();
      ++current_;
    }
    *fetched_count = count;
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE Reset() {
    current_ = 0;
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE Skip(ULONG count) {
    if ((candidates_.size() - current_) < count) {
      current_ = candidates_.size();
      return S_FALSE;
    }
    current_ += count;
    return S_OK;
  }

  TipRefCount ref_count_;

  std::vector<std::wstring> candidates_;
  size_t current_;
};

class CandidateListImpl final : public ITfCandidateList {
 public:
  template <typename Candidates>
  CandidateListImpl(Candidates &&candidates,
                    std::unique_ptr<TipCandidateListCallback> callback)
      : candidates_(std::forward<Candidates>(candidates)),
        callback_(std::move(callback)) {}
  CandidateListImpl(const CandidateListImpl &) = delete;
  CandidateListImpl &operator=(const CandidateListImpl &) = delete;

  // The IUnknown interface methods.
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID &interface_id,
                                                   void **object) {
    if (!object) {
      return E_INVALIDARG;
    }

    // Find a matching interface from the ones implemented by this object.
    // This object implements IUnknown and ITfEditSession.
    if (::IsEqualIID(interface_id, IID_IUnknown)) {
      *object = static_cast<IUnknown *>(this);
    } else if (IsEqualIID(interface_id, IID_ITfCandidateList)) {
      *object = static_cast<ITfCandidateList *>(this);
    } else {
      *object = nullptr;
      return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef() { return ref_count_.AddRefImpl(); }

  virtual ULONG STDMETHODCALLTYPE Release() {
    const ULONG count = ref_count_.ReleaseImpl();
    if (count == 0) {
      delete this;
    }
    return count;
  }

 private:
  // The ITfCandidateList interface methods.
  virtual HRESULT STDMETHODCALLTYPE
  EnumCandidates(IEnumTfCandidates **enum_candidate) {
    if (enum_candidate == nullptr) {
      return E_INVALIDARG;
    }
    auto impl = MakeComPtr<EnumTfCandidatesImpl>(candidates_);
    if (!impl) {
      return E_OUTOFMEMORY;
    }
    *enum_candidate = impl.Detach();
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE
  GetCandidate(ULONG index, ITfCandidateString **candidate_string) {
    if (candidate_string == nullptr) {
      return E_INVALIDARG;
    }
    if (index >= candidates_.size()) {
      return E_FAIL;
    }
    *candidate_string =
        MakeComPtr<CandidateStringImpl>(index, candidates_[index]).Detach();
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetCandidateNum(ULONG *count) {
    if (count == nullptr) {
      return E_INVALIDARG;
    }
    *count = static_cast<ULONG>(candidates_.size());
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE
  SetResult(ULONG index, TfCandidateResult candidate_result) {
    if (candidates_.size() <= index) {
      return E_INVALIDARG;
    }
    if ((candidate_result == CAND_FINALIZED) && (callback_.get() != nullptr)) {
      callback_->OnFinalize(index, candidates_[index]);
      callback_.reset();
    }
    return S_OK;
  }

  TipRefCount ref_count_;
  std::vector<std::wstring> candidates_;
  std::unique_ptr<TipCandidateListCallback> callback_;
};

}  // namespace

// static
ComPtr<ITfCandidateList> TipCandidateList::New(
    const std::vector<std::wstring> &candidates,
    std::unique_ptr<TipCandidateListCallback> callback) {
  return MakeComPtr<CandidateListImpl>(candidates, std::move(callback));
}

ComPtr<ITfCandidateList> TipCandidateList::New(
    std::vector<std::wstring> &&candidates,
    std::unique_ptr<TipCandidateListCallback> callback) {
  return MakeComPtr<CandidateListImpl>(std::move(candidates),
                                       std::move(callback));
}

}  // namespace tsf
}  // namespace win32
}  // namespace mozc
