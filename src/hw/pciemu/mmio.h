/* mmio.h - Memory Mapped IO operations
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef PCIEMU_MMIO_H
#define PCIEMU_MMIO_H

#include "pciemu.h"

/* forward declaration (defined in pciemu.h) to avoid circular reference */
typedef struct PCIEMUDevice PCIEMUDevice;

void pciemu_mmio_reset(PCIEMUDevice *dev);

void pciemu_mmio_init(PCIEMUDevice *dev, Error **errp);

void pciemu_mmio_fini(PCIEMUDevice *dev);

extern const MemoryRegionOps pciemu_mmio_ops;

#endif /* PCIEMU_MMIO_H */
