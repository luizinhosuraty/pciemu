/* pciemu.c - A PCIe device emulator.
 *
 * The pciemu can be considered as a tutorial/template for the creation
 * of virtual PCIe devices in QEMU.
 *
 * It has basic functionalities :
 *   - MMIO (Memory Mapped I/O) capabilities to access device registers/memory
 *   - DMA to and from a dedicated device buffer area
 *   - IRQ generation to inform the conclusion of DMA
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "pciemu.h"
#include "pciemu_hw.h"
#include "dma.h"
#include "irq.h"
#include "mmio.h"

/* -----------------------------------------------------------------------------
 *  Internal functions
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_reset: resets the PCIEMUDevice
 *
 * @dev: Instance of PCIEMUDevice object being reset
 */
static void pciemu_reset(PCIEMUDevice *dev)
{
    pciemu_irq_reset(dev);
    pciemu_dma_reset(dev);
    pciemu_mmio_reset(dev);
}

/* -----------------------------------------------------------------------------
 *  Object related functions
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_device_init: Device initialization
 *
 * Initializes the newly instantiated PCIEMUDevice object. This can be seen
 * as a constructor of the PCIEMUDevice.
 * Note that we receive a pointer for a PCIDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily cast to a PCIEMUDevice.
 *
 * @pci_dev: Instance of PCIDevice object being initialized
 * @errp: pointer to indicate errors
 */
static void pciemu_device_init(PCIDevice *pci_dev, Error **errp)
{
    PCIEMUDevice *dev = PCIEMU_DEVICE(pci_dev);
    pciemu_irq_init(dev, errp);
    pciemu_dma_init(dev, errp);
    pciemu_mmio_init(dev, errp);
}

/**
 * pciemu_device_fini: Device finalization
 *
 * Finalizes the instantiated PCIEMUDevice object. This can be seen
 * as a destructor of the PCIEMUDevice.
 * Note that we receive a pointer for a PCIDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily cast to a PCIEMUDevice.
 *
 * @pci_dev: Instance of PCIDevice object being initialized
 */
static void pciemu_device_fini(PCIDevice *pci_dev)
{
    PCIEMUDevice *dev = PCIEMU_DEVICE(pci_dev);
    pciemu_irq_fini(dev);
    pciemu_dma_fini(dev);
    pciemu_mmio_fini(dev);
}

/**
 * pciemu_device_reset: Device reset
 *
 * Note that we receive a pointer for a DeviceState, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily cast to a PCIEMUDevice.
 *
 * @dev: Instance of DeviceState object being reset
 */
static void pciemu_device_reset(DeviceState *dev)
{
    pciemu_reset(PCIEMU_DEVICE(dev));
}

/* -----------------------------------------------------------------------------
 *  Class related functions
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_class_init: Class initialization
 *
 * Initializes the class PCIEMUDeviceClass.
 * Note that we receive a pointer for an ObjectClass, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily cast to a PCIEMUDeviceClass.
 *
 * @klass: ObjectClass being initialized
 * @class_data: the data passed during initialization
 */
static void pciemu_class_init(ObjectClass *klass, void *class_data)
{
    DeviceClass *device_class = DEVICE_CLASS(klass);
    PCIDeviceClass *pci_device_class = PCI_DEVICE_CLASS(klass);

    pci_device_class->realize = pciemu_device_init;
    pci_device_class->exit = pciemu_device_fini;
    pci_device_class->vendor_id = PCIEMU_HW_VENDOR_ID;
    pci_device_class->device_id = PCIEMU_HW_DEVICE_ID;
    pci_device_class->revision = PCIEMU_HW_REVISION;
    pci_device_class->class_id = PCI_CLASS_OTHERS;

    set_bit(DEVICE_CATEGORY_MISC, device_class->categories);
    device_class->desc = PCIEMU_DEVICE_DESC;
    device_class->reset = pciemu_device_reset;
}

/* -----------------------------------------------------------------------------
 *  Declaration, definition and registration of type information
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_info: Description of the new pciemu type
 *
 * Specifies the pciemu type with information about PCIEMUDevice object and
 * how to initialize the class (PCIEMUDeviceClass), the parent of this object
 * and the interfaces it eventually implements.
 * This is information is used to create the ObjectClass and Object instance
 * of the PCIEMU device.
 *
 * Check [1] for more information on the QEMU Object Model (QOM).
 *
 * References:
 *  [1] https://qemu.readthedocs.io/en/latest/devel/qom.html
 *
 */
static const TypeInfo pciemu_info = {
    .name = TYPE_PCIEMU_DEVICE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCIEMUDevice),
    .class_init = pciemu_class_init,
    .interfaces =
        (InterfaceInfo[]){
            { INTERFACE_PCIE_DEVICE },
            {},
        },
};

/**
 * pciemu_register_types: Register the pciemu type with QOM
 *
 * With the registration of the pciemu type, QEMU can now create the
 * ObjectClass (PCIEMUDeviceClass) and Object (PCIEMUDevice) instance
 * with the correct information on their size, the parent relationship
 * with other instances and how to initialize them.
 *
 * Check [1] for more information.
 *
 * References:
 *  [1] https://qemu.readthedocs.io/en/latest/devel/qom.html
 *
 */
static void pciemu_register_types(void)
{
    type_register_static(&pciemu_info);
}

type_init(pciemu_register_types)
