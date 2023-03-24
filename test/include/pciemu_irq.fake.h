/* irq.fake.h - IRQ fake functions header
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef PCIEMU_IRQ_FAKE_H
#define PCIEMU_IRQ_FAKE_H

#include "fff_config.h"

#include "irq.h"

DECLARE_FAKE_VOID_FUNC(pciemu_irq_init, PCIEMUDevice *, Error **);
DECLARE_FAKE_VOID_FUNC(pciemu_irq_fini, PCIEMUDevice *);
DECLARE_FAKE_VOID_FUNC(pciemu_irq_reset, PCIEMUDevice *);
DECLARE_FAKE_VOID_FUNC(pciemu_irq_raise, PCIEMUDevice *, unsigned int);
DECLARE_FAKE_VOID_FUNC(pciemu_irq_lower, PCIEMUDevice *, unsigned int);

#endif /* PCIEMU_IRQ_FAKE_H */
