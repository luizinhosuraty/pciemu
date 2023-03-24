/* pciemu_hw.h - Header file describing HW resources
 *
 * This header can be used by different parts :
 *   - the qemu implementation of the device
 *   - the kernel module driving the device (acting as a datasheet)
 *   - potentially the userspace application when accessing HW directly.
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#ifndef PCIEMU_HW_H
#define PCIEMU_HW_H

/* As explained in [1], we are using VENDOR_ID 0x1b36 and DEVICE_ID 0x1100
 * References :
 *  [1] qemu/docs/specs/pci-ids.txt
 *  [2] qemu/include/hw/pci/pci.h
 */
#define PCIEMU_HW_VENDOR_ID 0x1b36
#define PCIEMU_HW_DEVICE_ID 0x1100
#define PCIEMU_HW_REVISION 0x01

/* BAR */
#define PCIEMU_HW_BAR0 0
#define PCIEMU_HW_BAR_CNT 1

/* MMIO - HARDWARE REGISTERS */
#define PCIEMU_HW_BAR0_REG_CNT 4
#define PCIEMU_HW_BAR0_REG_0 0x00
#define PCIEMU_HW_BAR0_REG_1 0x08
#define PCIEMU_HW_BAR0_REG_2 0x10
#define PCIEMU_HW_BAR0_REG_3 0x18

/* MMIO - IRQ */
#define PCIEMU_HW_BAR0_IRQ_0_RAISE 0x20
#define PCIEMU_HW_BAR0_IRQ_0_LOWER 0x28

/* MMIO - DMA configuration */
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_SRC 0x30
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_DST 0x38
#define PCIEMU_HW_BAR0_DMA_CFG_TXDESC_LEN 0x40
#define PCIEMU_HW_BAR0_DMA_CFG_CMD 0x48
#define PCIEMU_HW_BAR0_DMA_DOORBELL_RING 0x50

/* MMIO BAR0 Boundaries */
#define PCIEMU_HW_BAR0_START PCIEMU_HW_BAR0_REG_0
#define PCIEMU_HW_BAR0_END PCIEMU_HW_BAR0_DMA_DOORBELL_RING

/* DMA */
#define PCIEMU_HW_DMA_ADDR_CAPABILITY 32
#define PCIEMU_HW_DMA_AREA_START 0x10000
#define PCIEMU_HW_DMA_AREA_SIZE 0x1000

/* DMA Commands expliciting direction of transfer */
#define PCIEMU_HW_DMA_DIRECTION_TO_DEVICE 0x1
#define PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE 0x2

/* IRQs */
#define PCIEMU_HW_IRQ_CNT 1
#define PCIEMU_HW_IRQ_VECTOR_START 0
#define PCIEMU_HW_IRQ_VECTOR_END 0
#define PCIEMU_HW_IRQ_INTX 0 /* INTA */

/* IRQs for DMA */
#define PCIEMU_HW_IRQ_DMA_ENDED_VECTOR 0
#define PCIEMU_HW_IRQ_DMA_ENDED_ADDR PCIEMU_HW_BAR0_IRQ_0_RAISE
#define PCIEMU_HW_IRQ_DMA_ACK_ADDR PCIEMU_HW_BAR0_IRQ_0_LOWER

#endif /* PCIEMU_HW_H */
