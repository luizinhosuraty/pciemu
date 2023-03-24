/* mmio.fake.c - MMIO fake functions
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "pciemu_mmio.fake.h"

DEFINE_FAKE_VOID_FUNC(pciemu_mmio_init, PCIEMUDevice *, Error **);
DEFINE_FAKE_VOID_FUNC(pciemu_mmio_fini, PCIEMUDevice *);
DEFINE_FAKE_VOID_FUNC(pciemu_mmio_reset, PCIEMUDevice *);
