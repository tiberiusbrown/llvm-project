// RUN: %clang_cc1 -triple avm -std=c++17 -fsyntax-only -verify -verify-ignore-unexpected=note %s

#define AS1 __attribute__((address_space(1)))
using data_ptr = int *;
using prog_ptr = int AS1 *;
using const_prog_ptr = const int AS1 *;

void take_data(data_ptr);
void take_prog(prog_ptr);

data_ptr return_bad_data(prog_ptr p) { return p; } // expected-error {{changes address space of pointer}}
prog_ptr return_bad_prog(data_ptr p) { return p; } // expected-error {{changes address space of pointer}}
prog_ptr return_prog(prog_ptr p) { return p; }
data_ptr return_data(data_ptr p) { return p; }

void invalid(data_ptr d, prog_ptr p, const_prog_ptr cp, bool choose) {
  data_ptr d1 = p; // expected-error {{changes address space of pointer}}
  prog_ptr p1 = d; // expected-error {{changes address space of pointer}}
  d = p; // expected-error {{changes address space of pointer}}
  p = d; // expected-error {{changes address space of pointer}}
  void (*data_fn)(data_ptr) = take_data;
  void (*prog_fn)(prog_ptr) = take_prog;
  data_fn(p); // expected-error {{changes address space of pointer}}
  prog_fn(d); // expected-error {{changes address space of pointer}}
  (void)(choose ? d : p); // expected-error {{incompatible operand types}}
  (void)(choose ? p : d); // expected-error {{incompatible operand types}}
  d = (data_ptr)p; // expected-error-re {{cast {{.*}} address spaces}}
  p = (prog_ptr)d; // expected-error-re {{cast {{.*}} address spaces}}
  d = static_cast<data_ptr>(p); // expected-error-re {{cast {{.*}} address spaces}}
  p = static_cast<prog_ptr>(d); // expected-error-re {{cast {{.*}} address spaces}}
  d = reinterpret_cast<data_ptr>(p); // expected-error-re {{cast {{.*}} address spaces}}
  p = reinterpret_cast<prog_ptr>(d); // expected-error-re {{cast {{.*}} address spaces}}
  d = const_cast<data_ptr>(p); // expected-error-re {{cast {{.*}} address spaces}}
  p = const_cast<prog_ptr>(d); // expected-error-re {{cast {{.*}} address spaces}}
  (void)(p - d); // expected-error {{non-overlapping address spaces}}
  (void)(d - p); // expected-error {{non-overlapping address spaces}}
  (void)(p < d); // expected-error {{non-overlapping address spaces}}
  (void)(d < p); // expected-error {{non-overlapping address spaces}}
  *p = 1; // expected-error {{address space 1 is read-only}}
  *cp = 1; // expected-error {{address space 1 is read-only}}
  (void)d1; (void)p1;
}

int valid(data_ptr d, prog_ptr p, prog_ptr q) {
  data_ptr d2 = d;
  prog_ptr p2 = p;
  d2 = d;
  p2 = q;
  d2 = 0;
  p2 = 0;
  d2 = nullptr;
  take_data(d);
  take_prog(p);
  p2 = nullptr;
  data_ptr d3 = nullptr;
  prog_ptr p3 = nullptr;
  data_ptr d4 = 0;
  prog_ptr p4 = 0;
  return_prog(p);
  return_data(d);
  prog_ptr ps = d2 ? p : q;
  data_ptr ds = p2 ? d : d2;
  return (p == q) + (d == d2) + *p + (d2 == d3) + (p2 == p3) +
         (d4 == d2) + (p4 == p2) + (ps == p2) + (ds == d2);
}
