# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t
# RUN: llvm-objdump -d %t | FileCheck %s --check-prefix=DIS

sys buttons
# CHECK: sys buttons{{.*}}encoding: [0xd7,0x29]
# DIS: sys buttons

sys idle
# CHECK: sys idle{{.*}}encoding: [0xd7,0x2a]
# DIS: sys idle

sys generate_random_seed
# CHECK: sys generate_random_seed{{.*}}encoding: [0xd7,0x2b]
# DIS: sys generate_random_seed

sys save
# CHECK: sys save{{.*}}encoding: [0xd7,0x2c]
# DIS: sys save

sys load
# CHECK: sys load{{.*}}encoding: [0xd7,0x2d]
# DIS: sys load

sys save_exists
# CHECK: sys save_exists{{.*}}encoding: [0xd7,0x2e]
# DIS: sys save_exists
