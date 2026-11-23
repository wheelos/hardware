// Copyright 2026 The WheelOS Team. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// LD_PRELOAD probe for quantifying memcpy/memmove copy amplification and heap
// delta without intercepting malloc/free directly (avoids dlsym recursion).

#define _GNU_SOURCE
#include <dlfcn.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef void* (*memcpy_fn)(void*, const void*, size_t);
typedef void* (*memmove_fn)(void*, const void*, size_t);

static memcpy_fn g_real_memcpy = NULL;
static memmove_fn g_real_memmove = NULL;

static uint64_t g_total_memcpy_calls = 0;
static uint64_t g_total_memcpy_bytes = 0;
static uint64_t g_large_memcpy_calls = 0;
static uint64_t g_large_memcpy_bytes = 0;
static uint64_t g_memmove_calls = 0;
static uint64_t g_memmove_bytes = 0;

static size_t g_init_uordblks = 0;
static size_t g_init_hblkhd = 0;
static __thread int tls_resolving = 0;

static const size_t kLargeCopyThreshold = 4096;

static void* fallback_memcpy(void* dst, const void* src, size_t n) {
  unsigned char* d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;
  for (size_t i = 0; i < n; ++i) {
    d[i] = s[i];
  }
  return dst;
}

static void* fallback_memmove(void* dst, const void* src, size_t n) {
  unsigned char* d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;
  if (d < s) {
    for (size_t i = 0; i < n; ++i) {
      d[i] = s[i];
    }
  } else if (d > s) {
    for (size_t i = n; i > 0; --i) {
      d[i - 1] = s[i - 1];
    }
  }
  return dst;
}

static void resolve_symbols(void) {
  if (tls_resolving) {
    return;
  }
  tls_resolving = 1;
  if (__atomic_load_n(&g_real_memcpy, __ATOMIC_ACQUIRE) == NULL) {
    memcpy_fn fn = (memcpy_fn)dlsym(RTLD_NEXT, "memcpy");
    __atomic_store_n(&g_real_memcpy, fn, __ATOMIC_RELEASE);
  }
  if (__atomic_load_n(&g_real_memmove, __ATOMIC_ACQUIRE) == NULL) {
    memmove_fn fn = (memmove_fn)dlsym(RTLD_NEXT, "memmove");
    __atomic_store_n(&g_real_memmove, fn, __ATOMIC_RELEASE);
  }
  tls_resolving = 0;
}

__attribute__((constructor)) static void alloc_probe_init(void) {
  resolve_symbols();
  struct mallinfo2 mi = mallinfo2();
  g_init_uordblks = mi.uordblks;
  g_init_hblkhd = mi.hblkhd;
}

void* memcpy(void* dst, const void* src, size_t n) {
  memcpy_fn fn = __atomic_load_n(&g_real_memcpy, __ATOMIC_ACQUIRE);
  if (fn == NULL) {
    resolve_symbols();
    fn = __atomic_load_n(&g_real_memcpy, __ATOMIC_ACQUIRE);
  }
  __atomic_fetch_add(&g_total_memcpy_calls, 1, __ATOMIC_RELAXED);
  __atomic_fetch_add(&g_total_memcpy_bytes, (uint64_t)n, __ATOMIC_RELAXED);
  if (n >= kLargeCopyThreshold) {
    __atomic_fetch_add(&g_large_memcpy_calls, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&g_large_memcpy_bytes, (uint64_t)n, __ATOMIC_RELAXED);
  }
  if (fn == NULL) {
    return fallback_memcpy(dst, src, n);
  }
  return fn(dst, src, n);
}

void* memmove(void* dst, const void* src, size_t n) {
  memmove_fn fn = __atomic_load_n(&g_real_memmove, __ATOMIC_ACQUIRE);
  if (fn == NULL) {
    resolve_symbols();
    fn = __atomic_load_n(&g_real_memmove, __ATOMIC_ACQUIRE);
  }
  __atomic_fetch_add(&g_memmove_calls, 1, __ATOMIC_RELAXED);
  __atomic_fetch_add(&g_memmove_bytes, (uint64_t)n, __ATOMIC_RELAXED);
  if (n >= kLargeCopyThreshold) {
    __atomic_fetch_add(&g_large_memcpy_calls, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&g_large_memcpy_bytes, (uint64_t)n, __ATOMIC_RELAXED);
  }
  if (fn == NULL) {
    return fallback_memmove(dst, src, n);
  }
  return fn(dst, src, n);
}

__attribute__((destructor)) static void alloc_probe_fini(void) {
  const char* out_path = getenv("WHEELOS_ALLOC_PROBE_OUT");
  if (out_path == NULL || out_path[0] == '\0') {
    return;
  }
  struct mallinfo2 mi = mallinfo2();
  int64_t uord_delta = (int64_t)mi.uordblks - (int64_t)g_init_uordblks;
  int64_t mmap_delta = (int64_t)mi.hblkhd - (int64_t)g_init_hblkhd;

  FILE* fp = fopen(out_path, "w");
  if (fp == NULL) {
    return;
  }
  fprintf(fp,
          "{\n"
          "  \"large_copy_threshold_bytes\": %zu,\n"
          "  \"total_memcpy_calls\": %llu,\n"
          "  \"total_memcpy_bytes\": %llu,\n"
          "  \"large_memcpy_calls\": %llu,\n"
          "  \"large_memcpy_bytes\": %llu,\n"
          "  \"memmove_calls\": %llu,\n"
          "  \"memmove_bytes\": %llu,\n"
          "  \"heap_uordblks_delta_bytes\": %lld,\n"
          "  \"heap_mmap_delta_bytes\": %lld\n"
          "}\n",
          kLargeCopyThreshold,
          (unsigned long long)__atomic_load_n(&g_total_memcpy_calls,
                                             __ATOMIC_RELAXED),
          (unsigned long long)__atomic_load_n(&g_total_memcpy_bytes,
                                             __ATOMIC_RELAXED),
          (unsigned long long)__atomic_load_n(&g_large_memcpy_calls,
                                             __ATOMIC_RELAXED),
          (unsigned long long)__atomic_load_n(&g_large_memcpy_bytes,
                                             __ATOMIC_RELAXED),
          (unsigned long long)__atomic_load_n(&g_memmove_calls,
                                             __ATOMIC_RELAXED),
          (unsigned long long)__atomic_load_n(&g_memmove_bytes,
                                             __ATOMIC_RELAXED),
          (long long)uord_delta,
          (long long)mmap_delta);
  fclose(fp);
}
