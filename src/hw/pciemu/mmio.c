/* mmio.c - Memory Mapped IO operations
 *
 * Official documentation on MMIO and memory operations can be found in :
 *    https://qemu.readthedocs.io/en/latest/devel/memory.html
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#include "qemu/osdep.h"
#include "exec/target_page.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "mmio.h"
#include "irq.h"
#include "pciemu_hw.h"

/* -----------------------------------------------------------------------------
 *  Private
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_mmio_valid_access: Check whether the access is valid
 *
 * The size verification here is not required.
 * (memory_region_access_valid function in QEMU core will filter those out)
 *
 * @addr: address being accessed (relative to the Memory Region)
 * @size: read size in bytes (1, 2, 4, or 8)
 */
static inline bool pciemu_mmio_valid_access(hwaddr addr, unsigned int size)
{
    return (PCIEMU_HW_BAR0_START <= addr && addr <= PCIEMU_HW_BAR0_END);
}

/**
 * pciemu_mmio_read: Callback for read operations
 *
 * Read from the memory region and return the correspondent value.
 * Only valid for regions with READ operations (mostly regiters)
 *
 * @opaque: opaque pointer that points to instantiated object
 * @addr: address being accessed (relative to the Memory Region)
 * @size: read size in bytes (1, 2, 4, or 8)
 */
static uint64_t pciemu_mmio_read(void *opaque, hwaddr addr, unsigned int size)
{
    PCIEMUDevice *dev = opaque;
    uint64_t val = ~0ULL;
    if (!pciemu_mmio_valid_access(addr, size))
        return val;
    switch (addr) {
    case PCIEMU_HW_BAR0_REG_0:
        val = dev->reg[0];
        break;
    case PCIEMU_HW_BAR0_REG_1:
        val = dev->reg[1];
        break;
    case PCIEMU_HW_BAR0_REG_2:
        val = dev->reg[2];
        break;
    case PCIEMU_HW_BAR0_REG_3:
        val = dev->reg[3];
        break;
    }
    return val;
}

/**
 * pciemu_mmio_write: Callback for write operations
 *
 * Write to the memory region.
 * For now, it writes to reg0 regardless of the address.
 *
 * @opaque: opaque pointer that points to instantiated object
 * @addr: address being written (relative to the Memory Region)
 * @val: value to be written
 * @size: write size in bytes (1, 2, 4, or 8)
 */
static void pciemu_mmio_write(void *opaque, hwaddr addr, uint64_t val,
                              unsigned size)
{
    PCIEMUDevice *dev = opaque;
    if (!pciemu_mmio_valid_access(addr, size))
        return;
    switch (addr) {
    case PCIEMU_HW_BAR0_REG_0:
        dev->reg[0] = val;
        break;
    case PCIEMU_HW_BAR0_REG_1:
        dev->reg[1] = val;
        break;
    case PCIEMU_HW_BAR0_REG_2:
        dev->reg[2] = val;
        break;
    case PCIEMU_HW_BAR0_REG_3:
        dev->reg[3] = val;
        break;
    /* Left here for debug purposes only
     * Attempting to raise the IRQ0 when using the default device
     * driver may cause a crash during the unpinning process.
     */
    case PCIEMU_HW_BAR0_IRQ_0_RAISE:
        pciemu_irq_raise(dev, 0);
        break;
    case PCIEMU_HW_BAR0_IRQ_0_LOWER:
        pciemu_irq_lower(dev, 0);
        break;
    case PCIEMU_HW_BAR0_DMA_CFG_TXDESC_SRC:
        pciemu_dma_config_txdesc_src(dev, val);
        break;
    case PCIEMU_HW_BAR0_DMA_CFG_TXDESC_DST:
        pciemu_dma_config_txdesc_dst(dev, val);
        break;
    case PCIEMU_HW_BAR0_DMA_CFG_TXDESC_LEN:
        pciemu_dma_config_txdesc_len(dev, val);
        break;
    case PCIEMU_HW_BAR0_DMA_CFG_CMD:
        pciemu_dma_config_cmd(dev, val);
        break;
    case PCIEMU_HW_BAR0_DMA_DOORBELL_RING:
        pciemu_dma_doorbell_ring(dev);
        break;
    }
}

/* -----------------------------------------------------------------------------
 *  Public
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_mmio_reset: MMIO reset
 *
 * As the mmio block controls the device registers (reg),
 * we just clean them up here.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_mmio_reset(PCIEMUDevice *dev)
{
    for (int i = 0; i < PCIEMU_HW_BAR0_REG_CNT; ++i)
        dev->reg[i] = 0;
}

/**
 * pciemu_mmio_init: MMIO initialization
 *
 * Initializes the MMIO block for the instantiated PCIEMUDevice object.
 * Note that we receive a pointer for a PCIEMUDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily get the parent PCIDevice.
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 * @errp: pointer to indicate errors
 */
void pciemu_mmio_init(PCIEMUDevice *dev, Error **errp)
{
    /* BAR 0 will have memory region described in mmio (pciemu_mmio_ops) */
    /* Keeping the BAR size as the page size of the guest */
    memory_region_init_io(&dev->mmio, OBJECT(dev), &pciemu_mmio_ops, dev,
                          "pciemu-mmio", qemu_target_page_size());
    pci_register_bar(&dev->pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &dev->mmio);
}

/**
 * pciemu_mmio_fini: MMIO finalization
 *
 * Finalizes the MMIO block for the instantiated PCIEMUDevice object.
 * Note that we receive a pointer for a PCIEMUDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily get the parent PCIDevice.
 *
 * @dev: Instance of PCIEMUDevice object being finalized
 */
void pciemu_mmio_fini(PCIEMUDevice *dev)
{
    pciemu_mmio_reset(dev);
}

/**
 * pciemu_mmio_ops: Memory region description
 *
 * Describes the operations and behavior (with callbacks)
 * of the device memory region dedicate for MMIO.
 *
 */
const MemoryRegionOps pciemu_mmio_ops = {
    .read = pciemu_mmio_read,
    .write = pciemu_mmio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
	.min_access_size = 4,
	.max_access_size = 8,
    },
    .impl = {
        .min_access_size = 4,
        .max_access_size = 8,
    },
};
