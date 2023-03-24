/* irq.h - Interrupt Request operations
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef PCIEMU_IRQ_H
#define PCIEMU_IRQ_H

#include "qemu/osdep.h"
#include "hw/pci/pci.h"

#define PCIEMU_IRQ_MAX_VECTORS 32

/* forward declaration (defined in pciemu.h) to avoid circular reference */
typedef struct PCIEMUDevice PCIEMUDevice;

/* defines a single MSIVector */
typedef struct MSIVector {
    PCIEMUDevice *dev;
    bool raised;
} MSIVector;

typedef struct IRQStatusMSI {
    MSIVector msi_vectors[PCIEMU_IRQ_MAX_VECTORS];
} IRQStatusMSI;

typedef struct IRQStatusPin {
    /* our simple device has only one interrupt, a more complex one
     * would require us to track in some way which event caused the
     * interrupt. Thus, a bool type would not be appropriate.
     * We would probably need to use a masked scheme .
     */
    bool raised;
} IRQStatusPin;

/* IRQ status -> either msi or pin is being used */
typedef struct IRQStatus {
    union {
        IRQStatusMSI msi;
        IRQStatusPin pin;
    } status;
} IRQStatus;

void pciemu_irq_raise(PCIEMUDevice *dev, unsigned int vector);

void pciemu_irq_lower(PCIEMUDevice *dev, unsigned int vector);

void pciemu_irq_reset(PCIEMUDevice *dev);

void pciemu_irq_init(PCIEMUDevice *dev, Error **errp);

void pciemu_irq_fini(PCIEMUDevice *dev);

#endif /* PCIEMU_IRQ_H */
