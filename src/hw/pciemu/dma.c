/* dma.c - Direct Memory Access (DMA) operations
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "dma.h"
#include "irq.h"
#include "pciemu.h"

/* -----------------------------------------------------------------------------
 *  Private
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_dma_addr_mask: Mask the DMA address according to device's capability
 *
 * @dev: Instance of PCIEMUDevice object being used
 * @addr: Address to be masked
 */
static inline dma_addr_t pciemu_dma_addr_mask(PCIEMUDevice *dev,
                                              dma_addr_t addr)
{
    dma_addr_t masked = addr & dev->dma.config.mask;
    if (masked != addr) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "masked (%" PRIx64 ") != addr (%" PRIx64 ") \n", masked,
                      addr);
    }
    return masked;
}

/**
 * pciemu_dma_inside_device_boundaries: Check if addr is inside boundaries
 *
 * @addr: Address to be checked (address in device address space)
 */
static inline bool pciemu_dma_inside_device_boundaries(dma_addr_t addr)
{
    return (PCIEMU_HW_DMA_AREA_START <= addr &&
            addr <= PCIEMU_HW_DMA_AREA_START + PCIEMU_HW_DMA_AREA_SIZE);
}

/**
 * pciemu_dma_execute: Execute the DMA operation
 *
 * Effectively executes the DMA operation according to the configurations
 * in the transfer descriptor.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
static void pciemu_dma_execute(PCIEMUDevice *dev)
{
    DMAEngine *dma = &dev->dma;
    if (dma->config.cmd != PCIEMU_HW_DMA_DIRECTION_TO_DEVICE &&
        dma->config.cmd != PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE)
        return;
    if (dma->config.cmd == PCIEMU_HW_DMA_DIRECTION_TO_DEVICE) {
        /* DMA_DIRECTION_TO_DEVICE
         *   The transfer direction is RAM(or other device)->device.
         *   The content in the bus address dma->config.txdesc.src, which points
         *   to RAM memory (or other device memory), will be copied to address
         *   dst inside the device.
         *   dma->buff is the dedicated area inside the device to receive
         *   DMA transfers. Thus, dst is basically the offset of dma->buff.
         */
        if (!pciemu_dma_inside_device_boundaries(dma->config.txdesc.dst)) {
            qemu_log_mask(LOG_GUEST_ERROR, "dst register out of bounds \n");
            return;
        }
        dma_addr_t src = pciemu_dma_addr_mask(dev, dma->config.txdesc.src);
        dma_addr_t dst = dma->config.txdesc.dst - PCIEMU_HW_DMA_AREA_START;
        int err = pci_dma_read(&dev->pci_dev, src, dma->buff + dst,
                               dma->config.txdesc.len);
        if (err) {
            qemu_log_mask(LOG_GUEST_ERROR, "pci_dma_read err=%d\n", err);
        }
    } else {
        /* DMA_DIRECTION_FROM_DEVICE
         *   The transfer direction is device->RAM (or other device).
         *   This means that the content in the src address inside the device
         *   will be copied to the bus address dma->config.txdesc.dst, which
         *   points to a RAM memory (or other device memory).
         *   dma->buff is the dedicated area inside the device to receive
         *   DMA transfers. Thus, src is basically the offset of dma->buff.
         */
        if (!pciemu_dma_inside_device_boundaries(dma->config.txdesc.src)) {
            qemu_log_mask(LOG_GUEST_ERROR, "src register out of bounds \n");
            return;
        }
        dma_addr_t src = dma->config.txdesc.src - PCIEMU_HW_DMA_AREA_START;
        dma_addr_t dst = pciemu_dma_addr_mask(dev, dma->config.txdesc.dst);
        int err = pci_dma_write(&dev->pci_dev, dst, dma->buff + src,
                                dma->config.txdesc.len);
        if (err) {
            qemu_log_mask(LOG_GUEST_ERROR, "pci_dma_write err=%d\n", err);
        }
    }
    pciemu_irq_raise(dev, PCIEMU_HW_IRQ_DMA_ENDED_VECTOR);
}

/* -----------------------------------------------------------------------------
 *  Public
 * -----------------------------------------------------------------------------
 */

/**
 * pciemu_dma_config_txdesc_src: Configure the source register
 *
 * The source register inside the transfer descriptor (txdesc)
 * describes source of the DMA operation. It can be :
 *  - the bus address pointing to RAM (or other) when direction is "to device"
 *  - the offset inside the DMA memory area when direction is "from device"
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_config_txdesc_src(PCIEMUDevice *dev, dma_addr_t src)
{
    DMAStatus status = qatomic_read(&dev->dma.status);
    if (status == DMA_STATUS_IDLE)
        dev->dma.config.txdesc.src = src;
}

/**
 * pciemu_dma_config_txdesc_dst: Configure the destination register
 *
 * The destination register inside the transfer descriptor (txdesc)
 * describes destination of the DMA operation. It can be :
 *  - the offset inside the DMA memory area when direction is "to device"
 *  - the bus address pointing to RAM (or other) when direction is "from device"
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_config_txdesc_dst(PCIEMUDevice *dev, dma_addr_t dst)
{
    DMAStatus status = qatomic_read(&dev->dma.status);
    if (status == DMA_STATUS_IDLE)
        dev->dma.config.txdesc.dst = dst;
}

/**
 * pciemu_dma_config_txdesc_len: Configure the length register
 *
 * The length register inside the transfer descriptor (txdesc) describes
 * the size of the DMA operation in bytes.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_config_txdesc_len(PCIEMUDevice *dev, dma_size_t size)
{
    DMAStatus status = qatomic_read(&dev->dma.status);
    if (status == DMA_STATUS_IDLE)
        dev->dma.config.txdesc.len = size;
}

/**
 * pciemu_dma_config_cmd: Configure the command register
 *
 * The command register can take the following values (pciemu_hw.h);
 *   - PCIEMU_HW_DMA_DIRECTION_TO_DEVICE - DMA to device memory (dma->buff)
 *   - PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE - DMA from device memory (dma->buff)
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_config_cmd(PCIEMUDevice *dev, dma_cmd_t cmd)
{
    DMAStatus status = qatomic_read(&dev->dma.status);
    if (status == DMA_STATUS_IDLE)
        dev->dma.config.cmd = cmd;
}

/**
 * pciemu_dma_doorbell_ring: Reception of a doorbell
 *
 * When the host (or other device) writes to the doorbell register
 * it is signaling to the DMA engine to start executing the DMA.
 * At this point, it is assumed that the host has already (and properly)
 * configured all necessary DMA engine registers.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_doorbell_ring(PCIEMUDevice *dev)
{
    /* atomic access of dma.status may not be neeeded as the MMIO access
     * will be normally serialized.
     * Though not really necessary, it can show that we need to think of
     * atomic accessing regions, especially if the device is a bit more
     * complex.
     */
    DMAStatus status = qatomic_cmpxchg(&dev->dma.status, DMA_STATUS_IDLE,
                                       DMA_STATUS_EXECUTING);
    if (status == DMA_STATUS_EXECUTING)
        return;
    pciemu_dma_execute(dev);
    qatomic_set(&dev->dma.status, DMA_STATUS_IDLE);
}

/**
 * pciemu_dma_reset: DMA reset
 *
 * Resets the DMA block for the instantiated PCIEMUDevice object.
 * This can be considered a hard reset as we do not wait for the
 * current operation to finish.
 *
 * @dev: Instance of PCIEMUDevice object being used
 */
void pciemu_dma_reset(PCIEMUDevice *dev)
{
    DMAEngine *dma = &dev->dma;
    dma->status = DMA_STATUS_IDLE;
    dma->config.txdesc.src = 0;
    dma->config.txdesc.dst = 0;
    dma->config.txdesc.len = 0;
    dma->config.cmd = 0;

    /* clear the internal buffer */
    memset(dma->buff, 0, PCIEMU_HW_DMA_AREA_SIZE);
}

/**
 * pciemu_dma_init: DMA initialization
 *
 * Initializes the DMA block for the instantiated PCIEMUDevice object.
 * Note that we receive a pointer for a PCIEMUDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily get the parent PCIDevice.
 *
 * @dev: Instance of PCIEMUDevice object being initialized
 * @errp: pointer to indicate errors
 */
void pciemu_dma_init(PCIEMUDevice *dev, Error **errp)
{
    /* Basically reset the DMA engine */
    pciemu_dma_reset(dev);

    /* and set the DMA mask, which does not change */
    dev->dma.config.mask = DMA_BIT_MASK(PCIEMU_HW_DMA_ADDR_CAPABILITY);
}


/**
 * pciemu_dma_fini: DMA finalization
 *
 * Finalizes the DMA block for the instantiated PCIEMUDevice object.
 * Note that we receive a pointer for a PCIEMUDevice, but, due to the OOP hack
 * done by the QEMU Object Model, we can easily get the parent PCIDevice.
 *
 * @dev: Instance of PCIEMUDevice object being finalized
 */
void pciemu_dma_fini(PCIEMUDevice *dev)
{
    pciemu_dma_reset(dev);
    dev->dma.status = DMA_STATUS_OFF;
}
