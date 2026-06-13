# SPDX-License-Identifier: GPL-2.0
# COPYRIGHT(C) 2024 Samsung Electronics Co., Ltd. All Right Reserved.
dtstree         := $(srctree)/$(src)
trg_boards      += $(patsubst $(dtstree)/%.dts,%.dtb, $(wildcard $(dtstree)/*.dts))
trg_boards      += $(patsubst $(dtstree)/%.dts,%.dtbo, $(wildcard $(dtstree)/*.dts))
dtb-y    += $(trg_boards)
