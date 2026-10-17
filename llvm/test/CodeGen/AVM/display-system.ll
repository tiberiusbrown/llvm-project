; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O0 -stop-after=avm-system-service-regions < %s -o - | FileCheck %s --check-prefix=MIR
; RUN: opt -S -passes='default<O2>' < %s | FileCheck %s --check-prefix=OPT

declare i16 @llvm.avm.get.system.width()
declare i16 @llvm.avm.get.system.height()
declare i16 @llvm.avm.get.system.display.type()
declare void @llvm.avm.send.display.command(ptr, i16)
declare void @llvm.avm.display(i16)

define i16 @width() {
; CHECK-LABEL: width:
; CHECK: sys get_system_width
; MIR-LABEL: name: width
; MIR: = SYS_GET_SYSTEM_WIDTH_PSEUDO
  %v = call i16 @llvm.avm.get.system.width()
  ret i16 %v
}
define i16 @height() {
; CHECK-LABEL: height:
; CHECK: sys get_system_height
; MIR-LABEL: name: height
; MIR: = SYS_GET_SYSTEM_HEIGHT_PSEUDO
  %v = call i16 @llvm.avm.get.system.height()
  ret i16 %v
}
define i16 @display_type() {
; CHECK-LABEL: display_type:
; CHECK: sys get_system_display_type
; MIR-LABEL: name: display_type
; MIR: = SYS_GET_SYSTEM_DISPLAY_TYPE_PSEUDO
  %v = call i16 @llvm.avm.get.system.display.type()
  ret i16 %v
}
define void @commands(ptr %cmd) {
; CHECK-LABEL: commands:
; CHECK: ldi16 r5, 300
; CHECK: sys send_display_command
; MIR-LABEL: name: commands
; MIR: SYS_SEND_DISPLAY_COMMAND_PSEUDO {{.*}} :: (load (s2400), align 1)
  call void @llvm.avm.send.display.command(ptr %cmd, i16 300)
  ret void
}

; Source stores and hardware operations must keep their observable order.
define i8 @command_order(ptr %cmd) {
; CHECK-LABEL: command_order:
; CHECK: sys send_display_command
; CHECK: sys display
; CHECK: sys send_display_command
; OPT-LABEL: define{{.*}} i8 @command_order(
; OPT: store i8 -89, ptr %cmd
; OPT: call void @llvm.avm.send.display.command(ptr{{.*}} %cmd, i16 1)
; OPT: store i8 -90, ptr %cmd
; OPT: call void @llvm.avm.display(i16 0)
; OPT: call void @llvm.avm.send.display.command(ptr{{.*}} %cmd, i16 1)
; OPT: ret i8 -90
  store i8 167, ptr %cmd
  call void @llvm.avm.send.display.command(ptr %cmd, i16 1)
  store i8 166, ptr %cmd
  call void @llvm.avm.display(i16 0)
  call void @llvm.avm.send.display.command(ptr %cmd, i16 1)
  store i8 166, ptr %cmd
  %v = load i8, ptr %cmd
  ret i8 %v
}

; Queries are immutable: CSE and dead-call removal are permitted.
define i16 @query_cse() {
; OPT-LABEL: define{{.*}} i16 @query_cse()
; OPT-NEXT: ret i16 0
  %a = call i16 @llvm.avm.get.system.width()
  %b = call i16 @llvm.avm.get.system.width()
  call i16 @llvm.avm.get.system.height()
  %v = sub i16 %a, %b
  ret i16 %v
}
