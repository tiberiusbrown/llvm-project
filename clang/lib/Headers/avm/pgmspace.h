//===--- pgmspace.h - AVM program-space convenience API -----------------===//

#ifndef __AVM_PGMSPACE_H
#define __AVM_PGMSPACE_H

#include <stdint.h>

#define AVM_PROGMEM __attribute__((address_space(1)))

typedef const char AVM_PROGMEM *avm_flash_string_t;
typedef const void AVM_PROGMEM *avm_progmem_cptr;

#define AVM_PSTR(s) __builtin_avm_flash_string(s)
#define F(s) AVM_PSTR(s)

void *memcpy_P(void *dst, avm_progmem_cptr src, uint16_t size);

#define memcpy_P(dst, src, size) __builtin_avm_memcpy_p((dst), (src), (size))

#endif // __AVM_PGMSPACE_H
