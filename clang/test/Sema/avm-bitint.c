// RUN: %clang_cc1 -triple avm -std=c23 -fsyntax-only -verify %s

// C23 requires BITINT_MAXWIDTH to be at least ULLONG_WIDTH. AVM supports
// exactly that minimum, so wider types are rejected with the maximum width.
_Static_assert(__BITINT_MAXWIDTH__ == 64, "__BITINT_MAXWIDTH__");
_Static_assert(__BITINT_MAXWIDTH__ >= 64, "C23 minimum");

unsigned _BitInt(1) bit_one;
_BitInt(2) signed_minimum;
unsigned _BitInt(64) widest;
_BitInt(64) widest_signed;
unsigned _BitInt(9) narrow_odd;

unsigned _BitInt(65) too_wide; // expected-error {{unsigned _BitInt of bit sizes greater than 64 not supported}}
_BitInt(128) too_wide_signed; // expected-error {{signed _BitInt of bit sizes greater than 64 not supported}}
