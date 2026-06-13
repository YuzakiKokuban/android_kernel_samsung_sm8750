# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.


from boards import Board

class BoardQCM6490(Board):
    def __init__(self, socid):
        super(BoardQCM6490, self).__init__()
        self.socid = socid
        self.board_num = "qcm6490"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x9071c0
        self.phys_offset = 0xA0000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146aa6d0
        self.wdog_addr = 0x146aa658
        self.imem_file_name = 'OCIMEM.BIN'
        self.arm_smmu_v12 = True

class BoardQCM6490SVM(Board):
    def __init__(self, socid):
        super(BoardQCM6490SVM, self).__init__()
        self.socid = socid
        self.board_num = "qcm6490svm"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x9071c0
        self.phys_offset = 0xD0780000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146aa6d0
        self.wdog_addr = 0x146aa658
        self.imem_file_name = 'OCIMEM.BIN'
        self.arm_smmu_v12 = True

class BoardQCS9100(Board):
    def __init__(self, socid):
        super(BoardQCS9100, self).__init__()
        self.socid = socid
        self.board_num = "qcs9100"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x9071c0
        self.phys_offset = 0xA0000000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146aa6d0
        self.wdog_addr = 0x146aa658
        self.imem_file_name = 'OCIMEM.BIN'
        self.arm_smmu_v12 = True

class BoardQCS9100SVM(Board):
    def __init__(self, socid):
        super(BoardQCS9100SVM, self).__init__()
        self.socid = socid
        self.board_num = "qcs9100svm"
        self.cpu = 'CORTEXA53'
        self.ram_start = 0x80000000
        self.smem_addr = 0x900000
        self.smem_addr_buildinfo = 0x9071c0
        self.phys_offset = 0xD0780000
        self.imem_start = 0x14680000
        self.kaslr_addr = 0x146aa6d0
        self.wdog_addr = 0x146aa658
        self.imem_file_name = 'OCIMEM.BIN'
        self.arm_smmu_v12 = True

BoardQCM6490(socid=475)
BoardQCM6490SVM(socid=475)
BoardQCM6490(socid=499)

BoardQCS9100(socid=533)
BoardQCS9100SVM(socid=533)
BoardQCS9100(socid=534)
BoardQCS9100SVM(socid=534)
BoardQCS9100(socid=667)
BoardQCS9100SVM(socid=667)
