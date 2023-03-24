/* irq.c - Interrupt Request operations
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/pci/msi.h"
#include "pciemu.h"
#include "irq.h"

/* -----------------------------------------------------------------------------
 *  Private
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_irq_init_msi: IRQ initialization in MSI mode
 *
 * Initialize the prefered MSI mode if the host is able to handle MSI.
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 * @errp: pointer to indicate errors
 */
static inline void pciemu_irq_init_msi(PCIEMUDevice *dev, Error **errp)
{
    if (msi_init(&dev->pci_dev, 0, PCIEMU_HW_IRQ_CNT, true, false, errp)) {
        qemu_log_mask(LOG_GUEST_ERROR, "MSI Init Error\n");
        return;
    }
}

/**
 * pciemu_irq_init_intx: IRQ initialization in PIN-IRQ mode
 *
 * Use classic PIN-IRQ assertion mode in case the host is not able
 * to handle MSI. This is disabled automatically by msi_init() if
 * MSI initialization was successful.
 *
 * 0 <= irq_num <= 3, with INTA = 0, INTB = 1, INTC = 2, INTD = 3
 *
 * However, when using pci_set_irq, QEMU calls pci_intx, which
 * performs the following to get the INTX :
 *   pci_get_byte(pci_dev->config + PCI_INTERRUPT_PIN) - 1;
 *
 * Thus, we need to add 1 to whatever INTX we decide to use.
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 * @errp: pointer to indicate errors
 */
static inline void pciemu_irq_init_intx(PCIEMUDevice *dev, Error **errp)
{
    uint8_t *pci_conf = dev->pci_dev.config;
    pci_config_set_interrupt_pin(pci_conf, PCIEMU_HW_IRQ_INTX + 1);
}

/**
 * pciemu_irq_raise_intx: Raise the IRQ if MSI is disabled
 *
 * @dev: Instance of PCIEMUDevice object
 */
static inline void pciemu_irq_raise_intx(PCIEMUDevice *dev)
{
    dev->irq.status.pin.raised = true;
    pci_set_irq(&dev->pci_dev, 1);
}

/**
 * pciemu_irq_raise_msi: Raise the IRQ if MSI is enabled
 *
 * @dev: Instance of PCIEMUDevice object
 * @vector: the IRQ vector being raised
 */
static inline void pciemu_irq_raise_msi(PCIEMUDevice *dev, unsigned int vector)
{
    if (vector >= PCIEMU_IRQ_MAX_VECTORS)
        return;
    MSIVector *msi_vector = &dev->irq.status.msi.msi_vectors[vector];

    msi_vector->raised = true;
    msi_notify(&dev->pci_dev, vector);
}

/**
 * pciemu_irq_lower_intx: Lower the IRQ if MSI is disabled
 *
 * @dev: Instance of PCIEMUDevice object
 */
static inline void pciemu_irq_lower_intx(PCIEMUDevice *dev)
{
    dev->irq.status.pin.raised = false;
    pci_set_irq(&dev->pci_dev, 0);
}

/**
 * pciemu_irq_lower_msi: Lower the IRQ if MSI is enabled
 *
 * @dev: Instance of PCIEMUDevice object
 * @vector: the IRQ vector being lowered
 */
static inline void pciemu_irq_lower_msi(PCIEMUDevice *dev, unsigned int vector)
{
    if (vector >= PCIEMU_IRQ_MAX_VECTORS)
        return;
    MSIVector *msi_vector = &dev->irq.status.msi.msi_vectors[vector];
    if (!msi_vector->raised)
        return;
    msi_vector->raised = false;
}

/* -----------------------------------------------------------------------------
 *  Public
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_irq_raise: Raise the IRQ
 *
 * Used to raise an interrupt.
 *
 * @dev: Instance of PCIEMUDevice object being used
 * @vector: the IRQ vector being raised
 */
void pciemu_irq_raise(PCIEMUDevice *dev, unsigned int vector)
{
    /* If no MSI available on host, we should fallback to pin IRQ assertion */
    if (!msi_enabled(&dev->pci_dev)) {
        pciemu_irq_raise_intx(dev);
        return;
    }
    /* MSI is available */
    pciemu_irq_raise_msi(dev, vector);
}

/**
 * pciemu_irq_lower: Lower the IRQ
 *
 * Used to receive the ACK from the kernel module (device driver).
 *
 * @dev: Instance of PCIEMUDevice object being used
 * @vector: the IRQ vector being lowered
 */
void pciemu_irq_lower(PCIEMUDevice *dev, unsigned int vector)
{
    /* If no MSI available on host, we should fallback to pin IRQ assertion */
    if (!msi_enabled(&dev->pci_dev)) {
        pciemu_irq_lower_intx(dev);
        return;
    }
    /* MSI is available */
    pciemu_irq_lower_msi(dev, vector);
}

/**
 * pciemu_irq_reset: IRQ reset
 *
 * Basically resets (lowers) all IRQ vectors
 *
 * @dev: Instance of PCIEMUDevice object being reset
 */
void pciemu_irq_reset(PCIEMUDevice *dev)
{
    for (int i = PCIEMU_HW_IRQ_VECTOR_START; i <= PCIEMU_HW_IRQ_VECTOR_END; ++i)
        pciemu_irq_lower(dev, i);
}

/**
 * pciemu_irq_init: IRQ initialization
 *
 * Initializes the IRQ block for the instantiated PCIEMUDevice object.
 * Note that we receive a pointer for a PCIEMUDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily get the parent PCIDevice.
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 * @errp: pointer to indicate errors
 */
void pciemu_irq_init(PCIEMUDevice *dev, Error **errp)
{
    /* configure line based interrupt if fallback is needed */
    pciemu_irq_init_intx(dev, errp);
    /* try to confingure MSI based interrupt (preferred) */
    pciemu_irq_init_msi(dev, errp);
}

/**
 * pciemu_irq_fini: IRQ finalization
 *
 * Finalizes the IRQ block for the instantiated PCIEMUDevice object.
 * Note that we receive a pointer for a PCIEMUDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily get the parent PCIDevice.
 *
 * @dev: Instance of PCIEMUDevice object being finalized
 */
void pciemu_irq_fini(PCIEMUDevice *dev)
{
    pciemu_irq_reset(dev);
    msi_uninit(&dev->pci_dev);
}
