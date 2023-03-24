/* mmio.fake.h - MMIO fake functions header
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef PCIEMU_MMIO_FAKE_H
#define PCIEMU_MMIO_FAKE_H

#include "fff_config.h"

#include "mmio.h"

DECLARE_FAKE_VOID_FUNC(pciemu_mmio_init, PCIEMUDevice *, Error **);
DECLARE_FAKE_VOID_FUNC(pciemu_mmio_fini, PCIEMUDevice *);
DECLARE_FAKE_VOID_FUNC(pciemu_mmio_reset, PCIEMUDevice *);


#endif /* PCIEMU_MMIO_FAKE_H */
