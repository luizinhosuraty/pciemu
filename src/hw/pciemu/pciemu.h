/* pciemu.h
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#ifndef PCIEMU_H
#define PCIEMU_H

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "pciemu_hw.h"
#include "dma.h"
#include "irq.h"

#define TYPE_PCIEMU_DEVICE "pciemu"
#define PCIEMU_DEVICE_DESC "PCIEMU Device"
/*
 * Declare the object type for PCIEMUDevice and all boilerplate code
 * See https://qemu.readthedocs.io/en/latest/devel/qom.html for details
 *
 */
OBJECT_DECLARE_TYPE(PCIEMUDevice, PCIEMUDeviceClass, PCIEMU_DEVICE);

/* Struct that defines our class
 */
typedef struct PCIEMUDeviceClass {
    /*
     * OOP hack : Our parent class (PCIDeviceClass) is part of the strutct
     * The idea is to be able to access it from our own class
     */
    PCIDeviceClass parent_class;
} PCIEMUDeviceClass;

typedef struct PCIEMUDevice {
    /*< private >*/
    /* OOP hack : Our parent (PCIDevice) is part of the struct */
    PCIDevice pci_dev;
    /*< public >*/

    /* IRQs */
    IRQStatus irq;

    /* DMAs */
    DMAEngine dma;

    /* Memory Regions */
    MemoryRegion mmio; /* BAR 0 (registers) */

    /* Registers in BAR0 */
    uint64_t reg[PCIEMU_HW_BAR0_REG_CNT];
} PCIEMUDevice;

#endif /* PCIEMU_H */
