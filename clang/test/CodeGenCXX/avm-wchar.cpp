// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -std=c++11 -emit-llvm -o - %s | FileCheck %s

template<class A, class B>
struct same_type {
    static constexpr bool value = false;
};

template<class A>
struct same_type<A, A> {
    static constexpr bool value = true;
};

static_assert(sizeof(wchar_t) == 1, "wchar_t size");
static_assert(alignof(wchar_t) == 1, "wchar_t alignment");
static_assert(static_cast<wchar_t>(-1) == 255,
              "unsigned wchar_t");
static_assert(!same_type<wchar_t, unsigned char>::value,
              "C++ wchar_t must remain a distinct type");

static_assert(sizeof(L'A') == 1, "wide character size");
static_assert(sizeof(L"AVM") == 4, "wide string size");

static_assert(sizeof(char16_t) == 2, "char16_t unchanged");
static_assert(sizeof(char32_t) == 4, "char32_t unchanged");

wchar_t wide_cpp[] = L"AVM";

extern "C" wchar_t roundtrip_cpp(wchar_t x) {
    return x;
}

// CHECK: @wide_cpp ={{.*}} global [4 x i8] c"AVM\00"
// CHECK: define{{.*}} i8 @roundtrip_cpp(i8 noundef zeroext %x)
