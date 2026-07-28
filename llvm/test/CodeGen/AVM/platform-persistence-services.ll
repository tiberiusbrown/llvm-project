; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=CHECK
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=CHECK
; RUN: llc -mtriple=avm -O0 -stop-after=avm-system-service-regions < %s -o - \
; RUN:   | FileCheck %s --check-prefix=MIR

declare i16 @llvm.avm.buttons()
declare void @llvm.avm.idle()
declare i16 @llvm.avm.generate.random.seed()
declare void @llvm.avm.save()
declare i16 @llvm.avm.load()
declare i16 @llvm.avm.save.exists()

define i8 @platform_buttons() {
; CHECK-LABEL: platform_buttons:
; CHECK:       sys buttons
  %wide = call i16 @llvm.avm.buttons()
  %result = trunc i16 %wide to i8
  ret i8 %result
}

define void @platform_idle() {
; CHECK-LABEL: platform_idle:
; CHECK:       sys idle
  call void @llvm.avm.idle()
  ret void
}

define i16 @platform_random_seed() {
; CHECK-LABEL: platform_random_seed:
; CHECK:       sys generate_random_seed
  %result = call i16 @llvm.avm.generate.random.seed()
  ret i16 %result
}

define void @persistence_save() {
; CHECK-LABEL: persistence_save:
; CHECK:       sys save
  call void @llvm.avm.save()
  ret void
}

define i1 @persistence_load() {
; CHECK-LABEL: persistence_load:
; CHECK:       sys load
  %wide = call i16 @llvm.avm.load()
  %result = icmp ne i16 %wide, 0
  ret i1 %result
}

define i1 @persistence_save_exists() {
; CHECK-LABEL: persistence_save_exists:
; CHECK:       sys save_exists
  %wide = call i16 @llvm.avm.save.exists()
  %result = icmp ne i16 %wide, 0
  ret i1 %result
}

define void @persistence_sequence() {
; CHECK-LABEL: persistence_sequence:
; CHECK:       sys save
; CHECK:       sys load
; CHECK:       sys save_exists
  call void @llvm.avm.save()
  call i16 @llvm.avm.load()
  call i16 @llvm.avm.save.exists()
  ret void
}

; MIR-LABEL: name: platform_buttons
; MIR:       {{%[0-9]+}}:gpr16 = SYS_BUTTONS_PSEUDO
; MIR-LABEL: name: platform_idle
; MIR:       SYS_IDLE_PSEUDO
; MIR-LABEL: name: platform_random_seed
; MIR:       {{%[0-9]+}}:gpr16 = SYS_GENERATE_RANDOM_SEED_PSEUDO
; MIR-LABEL: name: persistence_save
; MIR:       SYS_SAVE_PSEUDO :: (load unknown-size, align 1)
; MIR-LABEL: name: persistence_load
; MIR:       {{%[0-9]+}}:gpr16 = SYS_LOAD_PSEUDO :: (store unknown-size, align 1)
; MIR-LABEL: name: persistence_save_exists
; MIR:       {{%[0-9]+}}:gpr16 = SYS_SAVE_EXISTS_PSEUDO
; MIR-LABEL: name: persistence_sequence
; MIR:       SYS_SAVE_PSEUDO :: (load unknown-size, align 1)
; MIR:       {{%[0-9]+}}:gpr16 = SYS_LOAD_PSEUDO :: (store unknown-size, align 1)
; MIR:       {{%[0-9]+}}:gpr16 = SYS_SAVE_EXISTS_PSEUDO
