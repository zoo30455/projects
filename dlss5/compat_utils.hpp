/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */

// Compatibility shims: the DLSS5 add-on source (v8.5.0-rc10) was written
// against a RenoDX utils revision that is not public.  These provide the
// few helpers it expects on top of upstream RenoDX utils.

#pragma once

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <mutex>

#include <detours.h>

#include "../../utils/directx.hpp"
#include "../../utils/vtable.hpp"

namespace renodx::utils::directx {

// The COM identity of an object (QueryInterface(IUnknown)), used only as a
// value to compare; never dereferenced by callers.
inline IUnknown* NativeIdentity(IUnknown* object) {
  if (object == nullptr) return nullptr;
  IUnknown* identity = nullptr;
  if (FAILED(object->QueryInterface(IID_PPV_ARGS(&identity))) || identity == nullptr) {
    return object;
  }
  identity->Release();
  return identity;
}

template <typename A, typename B>
inline bool SameNativeObject(A* a, B* b) {
  if (a == nullptr || b == nullptr) return false;
  if (static_cast<void*>(a) == static_cast<void*>(b)) return true;
  return NativeIdentity(a) == NativeIdentity(b);
}

}  // namespace renodx::utils::directx

namespace renodx::utils::vtable {

// Detours allows a single pending transaction per process; every add-on
// attach/detach serializes on this mutex.
inline std::mutex& TransactionMutex() {
  static std::mutex mutex;
  return mutex;
}

// Times a DetourTransactionBegin found another thread's transaction pending.
inline std::atomic_uint64_t transaction_contended{0};

// Begins a Detours transaction, retrying briefly while another module owns
// the pending one.  False: give up for now (callers retry later).
inline bool BeginTransaction() {
  for (int attempt = 0; attempt < 100; ++attempt) {
    if (DetourTransactionBegin() == NO_ERROR) return true;
    transaction_contended.fetch_add(1, std::memory_order_relaxed);
    Sleep(1);
  }
  return false;
}

}  // namespace renodx::utils::vtable
