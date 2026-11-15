// RUN: %clang_cc1 -triple avm -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple avm -x c++ -std=c++20 -fsyntax-only -verify %s

#define AS1 __attribute__((address_space(1)))

typedef unsigned char LargeArray[65537UL];
extern const LargeArray AS1 program_array;
LargeArray ram_array; // expected-error {{too large for the address space (maximum allowed size of 65'535 bytes)}}
extern LargeArray extern_ram_array; // expected-error {{too large for the address space (maximum allowed size of 65'535 bytes)}}

struct LargeRecord {
  unsigned char first[32768UL];
  unsigned char second[32768UL];
  unsigned char last;
};
extern const struct LargeRecord AS1 program_record;
struct LargeRecord ram_record; // expected-error {{too large for the address space (maximum allowed size of 65'535 bytes)}}

union LargeUnion {
  LargeArray array;
  struct LargeRecord record;
};
extern const union LargeUnion AS1 program_union;
union LargeUnion ram_union; // expected-error {{too large for the address space (maximum allowed size of 65'535 bytes)}}

extern const unsigned char AS1 largest_array[16777215UL];
extern const unsigned long long AS1 largest_word_array[2097151UL];
extern const unsigned char AS1 oversized_array[16777216UL]; // expected-error {{array is too large}}
extern const unsigned long long AS1 oversized_word_array[2097152UL]; // expected-error {{array is too large}}

struct OversizedRecord { // expected-error {{exceeds maximum allowed size of 16777216 bytes}}
  unsigned char first[16777215UL];
  unsigned char last;
};
extern const struct OversizedRecord AS1 oversized_record; // expected-error {{too large for the address space (maximum allowed size of 16'777'215 bytes)}}

#define X2(x) x, x
#define X4(x) X2(x), X2(x)
#define X8(x) X4(x), X4(x)
#define X16(x) X8(x), X8(x)
#define X32(x) X16(x), X16(x)
#define X64(x) X32(x), X32(x)
#define X128(x) X64(x), X64(x)
#define X256(x) X128(x), X128(x)
#define X512(x) X256(x), X256(x)
#define X1024(x) X512(x), X512(x)
#define X2048(x) X1024(x), X1024(x)
#define X4096(x) X2048(x), X2048(x)
#define X8192(x) X4096(x), X4096(x)
#define X16384(x) X8192(x), X8192(x)
#define X32768(x) X16384(x), X16384(x)
#define X65536(x) X32768(x), X32768(x)

const unsigned char AS1 inferred_program[] = {X65536(0), 23};
unsigned char inferred_ram[] = {X65536(0), 23}; // expected-error {{too large for the address space (maximum allowed size of 65'535 bytes)}}

#ifndef __cplusplus
const unsigned char AS1 designated_program[] = {[65536UL] = 23};
const unsigned char AS1 oversized_designated[] = {[16777216UL] = 23}; // expected-error {{array is too large}}
const unsigned char AS1 invalid_designated[65537UL] = {[16777216UL] = 23}; // expected-error {{array designator index (16777216) exceeds array bounds (65537)}}
#endif

void local_ram(void) {
  LargeArray local; // expected-error {{too large for the address space (maximum allowed size of 65'535 bytes)}}
}

#ifdef __cplusplus
void new_ram() {
  (void)new unsigned long long[8192]; // expected-error {{array is too large}}
  (void)new LargeRecord; // expected-error {{too large for the address space (maximum allowed size of 65'535 bytes)}}
}
#endif
