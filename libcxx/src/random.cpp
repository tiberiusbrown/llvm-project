//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <__config>

#if defined(_LIBCPP_USING_WIN32_RANDOM)
// Must be defined before including stdlib.h to enable rand_s().
#  define _CRT_RAND_S
#endif // defined(_LIBCPP_USING_WIN32_RANDOM)

#if !defined(_LIBCPP_USING_AVM_RANDOM)
#  include <__system_error/throw_system_error.h>
#endif
#include <limits>
#include <random>
#include <string>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_LIBCPP_USING_AVM_RANDOM)
#  include <avm.h>
#  include <exception>

// A platform may replace the sampler without changing the random_device ABI.
// Sampling does not consume or reseed the C runtime's deterministic generator.
extern "C" __attribute__((weak)) uint16_t __avm_random_device_sample() noexcept { return avm_generate_random_seed(); }

namespace {
class __avm_random_device_error final : public std::exception {
  const char* __message_;

public:
  explicit __avm_random_device_error(const char* __message) noexcept : __message_(__message) {}
  const char* what() const noexcept override { return __message_; }
};

[[noreturn]] __attribute__((noinline)) void __avm_random_device_fail(const char* __message) {
  // No message allocation is needed when entropy is unavailable.
  throw __avm_random_device_error(__message);
}

__attribute__((noinline)) int __avm_random_bit() noexcept {
  // Discard equal pairs to remove first-order bias. Bound retries so a stuck
  // source reports an error rather than keeping the caller in a sampling loop.
  for (unsigned __attempt = 0; __attempt != 64; ++__attempt) {
    uint16_t __first  = __avm_random_device_sample();
    uint16_t __second = __avm_random_device_sample();
    if ((__first ^ __second) & 1)
      return __first & 1;
  }
  return -1;
}

__attribute__((noinline)) bool __avm_random_word(unsigned& __value) noexcept {
  __value = 0;
  for (unsigned __i = 0; __i != std::numeric_limits<unsigned>::digits; ++__i) {
    int __bit = __avm_random_bit();
    if (__bit < 0)
      return false;
    __value = (__value << 1) | static_cast<unsigned>(__bit);
  }
  return true;
}
} // namespace
#endif

#if defined(_LIBCPP_USING_GETENTROPY)
#  include <sys/random.h>
#elif defined(_LIBCPP_USING_DEV_RANDOM)
#  include <fcntl.h>
#  include <unistd.h>
#  if __has_include(<sys/ioctl.h>) && __has_include(<linux/random.h>)
#    include <linux/random.h>
#    include <sys/ioctl.h>
#  endif
#elif defined(_LIBCPP_USING_FUCHSIA_CPRNG)
#  include <zircon/syscalls.h>
#endif

_LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_EXPLICIT_ABI_ANNOTATIONS

#if defined(_LIBCPP_USING_AVM_RANDOM)

random_device::random_device(const string& __token) {
  if (__token != "avm" && __token != "/dev/urandom")
    __avm_random_device_fail("random_device: unsupported AVM source");
}

random_device::~random_device() {}

unsigned random_device::operator()() {
  result_type __value;
  // Release all sampling frames before entering the exception matcher.
  if (!__avm_random_word(__value))
    __avm_random_device_fail("random_device: AVM entropy unavailable");
  return __value;
}

#elif defined(_LIBCPP_USING_GETENTROPY)

random_device::random_device(const string& __token) {
  if (__token != "/dev/urandom")
    std::__throw_system_error(ENOENT, ("random device not supported " + __token).c_str());
}

random_device::~random_device() {}

unsigned random_device::operator()() {
  unsigned r;
  size_t n = sizeof(r);
  int err  = getentropy(&r, n);
  if (err)
    std::__throw_system_error(errno, "random_device getentropy failed");
  return r;
}

#elif defined(_LIBCPP_USING_ARC4_RANDOM)

random_device::random_device(const string&) {}

random_device::~random_device() {}

unsigned random_device::operator()() { return arc4random(); }

#elif defined(_LIBCPP_USING_DEV_RANDOM)

random_device::random_device(const string& __token) : __f_(open(__token.c_str(), O_RDONLY)) {
  if (__f_ < 0)
    std::__throw_system_error(errno, ("random_device failed to open " + __token).c_str());
}

random_device::~random_device() { close(__f_); }

unsigned random_device::operator()() {
  unsigned r;
  size_t n = sizeof(r);
  char* p  = reinterpret_cast<char*>(&r);
  while (n > 0) {
    ssize_t s = read(__f_, p, n);
    if (s == 0)
      std::__throw_system_error(ENOMSG, "random_device got EOF");
    if (s == -1) {
      if (errno != EINTR)
        std::__throw_system_error(errno, "random_device got an unexpected error");
      continue;
    }
    n -= static_cast<size_t>(s);
    p += static_cast<size_t>(s);
  }
  return r;
}

#elif defined(_LIBCPP_USING_WIN32_RANDOM)

random_device::random_device(const string& __token) {
  if (__token != "/dev/urandom")
    std::__throw_system_error(ENOENT, ("random device not supported " + __token).c_str());
}

random_device::~random_device() {}

unsigned random_device::operator()() {
  unsigned r;
  errno_t err = rand_s(&r);
  if (err)
    std::__throw_system_error(err, "random_device rand_s failed.");
  return r;
}

#elif defined(_LIBCPP_USING_FUCHSIA_CPRNG)

random_device::random_device(const string& __token) {
  if (__token != "/dev/urandom")
    std::__throw_system_error(ENOENT, ("random device not supported " + __token).c_str());
}

random_device::~random_device() {}

unsigned random_device::operator()() {
  // Implicitly link against the vDSO system call ABI without
  // requiring the final link to specify -lzircon explicitly when
  // statically linking libc++.
#  pragma comment(lib, "zircon")

  // The system call cannot fail.  It returns only when the bits are ready.
  unsigned r;
  _zx_cprng_draw(&r, sizeof(r));
  return r;
}

#else
#  error "Random device not implemented for this architecture"
#endif

double random_device::entropy() const noexcept {
#if defined(_LIBCPP_USING_AVM_RANDOM)
  // The hardware source has no calibrated entropy estimate.
  return 0;
#elif defined(_LIBCPP_USING_DEV_RANDOM) && defined(RNDGETENTCNT)
  int ent;
  if (::ioctl(__f_, RNDGETENTCNT, &ent) < 0)
    return 0;

  if (ent < 0)
    return 0;

  if (ent > std::numeric_limits<result_type>::digits)
    return std::numeric_limits<result_type>::digits;

  return ent;
#elif defined(_LIBCPP_USING_ARC4_RANDOM) || defined(_LIBCPP_USING_FUCHSIA_CPRNG)
  return std::numeric_limits<result_type>::digits;
#else
  return 0;
#endif
}

_LIBCPP_END_EXPLICIT_ABI_ANNOTATIONS
_LIBCPP_END_NAMESPACE_STD
