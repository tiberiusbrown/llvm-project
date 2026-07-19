AVM_SYS_DEF(0x00, debug_putc)
AVM_SYS_DEF(0x01, debug_break)
AVM_SYS_DEF(0x02, millis)
AVM_SYS_DEF(0x03, millis32)
AVM_SYS_DEF(0x04, sinf)
AVM_SYS_DEF(0x05, cosf)
AVM_SYS_DEF(0x06, atan2f)
AVM_SYS_DEF(0x07, tanf)
AVM_SYS_DEF(0x08, expf)
AVM_SYS_DEF(0x09, logf)
AVM_SYS_DEF(0x0a, log2f)
AVM_SYS_DEF(0x0b, log10f)
AVM_SYS_DEF(0x0c, powf)
AVM_SYS_DEF(0x0d, hypotf)
AVM_SYS_DEF(0x0e, fmodf)
AVM_SYS_DEF(0x0f, memcpy)
AVM_SYS_DEF(0x10, memcpy_p)
AVM_SYS_DEF(0x11, memset)

#ifdef AVM_SYS_DEF
#undef AVM_SYS_DEF
#endif
