// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -std=c99 -emit-llvm -o - %s | FileCheck %s

typedef __WCHAR_TYPE__ wchar_t;

_Static_assert(sizeof(wchar_t) == 1, "wchar_t size");
_Static_assert((wchar_t)-1 == 255, "unsigned wchar_t");
_Static_assert(sizeof(__WINT_TYPE__) == 2, "wint_t size");
_Static_assert(sizeof(L'A') == 1, "wide character size");
_Static_assert(sizeof(L"AVM") == 4, "wide literal size");

wchar_t wide_c[] = L"AVM";

wchar_t roundtrip_c(wchar_t x) {
    return x;
}

// CHECK: @wide_c ={{.*}} global [4 x i8] c"AVM\00"
// CHECK: define{{.*}} i8 @roundtrip_c(i8 noundef zeroext %x)
