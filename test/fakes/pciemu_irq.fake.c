/* irq.fake.c - IRQ fake functions
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "pciemu_irq.fake.h"

DEFINE_FAKE_VOID_FUNC(pciemu_irq_init, PCIEMUDevice *, Error **);
DEFINE_FAKE_VOID_FUNC(pciemu_irq_fini, PCIEMUDevice *);
DEFINE_FAKE_VOID_FUNC(pciemu_irq_reset, PCIEMUDevice *);
DEFINE_FAKE_VOID_FUNC(pciemu_irq_raise, PCIEMUDevice *, unsigned int);
DEFINE_FAKE_VOID_FUNC(pciemu_irq_lower, PCIEMUDevice *, unsigned int);
