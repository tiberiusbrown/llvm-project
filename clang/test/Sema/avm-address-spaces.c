// RUN: %clang_cc1 -triple avm -fsyntax-only -verify -verify-ignore-unexpected=note %s

#define AS1 __attribute__((address_space(1)))
#define NULL ((void *)0)
typedef int *data_ptr;
typedef int AS1 *prog_ptr;
typedef const int AS1 *const_prog_ptr;

void take_data(data_ptr);
void take_prog(prog_ptr);

data_ptr return_bad_data(prog_ptr p) { return p; } // expected-error {{changes address space of pointer}}
prog_ptr return_bad_prog(data_ptr p) { return p; } // expected-error {{changes address space of pointer}}
prog_ptr return_prog(prog_ptr p) { return p; }
data_ptr return_data(data_ptr p) { return p; }

void invalid(data_ptr d, prog_ptr p, const_prog_ptr cp, int choose) {
  data_ptr d1 = p; // expected-error {{changes address space of pointer}}
  prog_ptr p1 = d; // expected-error {{changes address space of pointer}}
  d = p; // expected-error {{changes address space of pointer}}
  p = d; // expected-error {{changes address space of pointer}}
  take_data(p); // expected-error {{changes address space of pointer}}
  take_prog(d); // expected-error {{changes address space of pointer}}
  (void)(choose ? d : p); // expected-error {{non-overlapping address spaces}}
  (void)(choose ? p : d); // expected-error {{non-overlapping address spaces}}
  d = (data_ptr)p; // expected-error-re {{cast {{.*}} address spaces}}
  p = (prog_ptr)d; // expected-error-re {{cast {{.*}} address spaces}}
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
  d2 = NULL;
  take_data(d);
  take_prog(p);
  p2 = 0;
  p2 = NULL;
  data_ptr d3 = 0;
  data_ptr d4 = NULL;
  prog_ptr p3 = 0;
  prog_ptr p4 = NULL;
  return_prog(p);
  return_data(d);
  prog_ptr ps = d2 ? p : q;
  data_ptr ds = p2 ? d : d2;
  return (p == q) + (d == d2) + *p + (d3 == d4) + (p3 == p4) +
         (ps == p2) + (ds == d2);
}
