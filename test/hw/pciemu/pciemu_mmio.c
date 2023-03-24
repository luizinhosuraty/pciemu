/* pciemu_mmio.c - Unit tests for pciemu MMIO block
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "unitctest/unitctest.h"
#include "fff/fff.h"
#include "qemu.fake.h"
#include "pciemu_dma.fake.h"
#include "pciemu_irq.fake.h"

#include "../src/hw/pciemu/mmio.c"

DEFINE_FFF_GLOBALS;

TEST(pciemu_mmio_valid_access, "Test the check on wheter MMIO access is valid")
{
    hwaddr addr = PCIEMU_HW_BAR0_START;
    unsigned int size = sizeof(uint64_t);
    EXPECT_TRUE(pciemu_mmio_valid_access(addr, size), "addr is inside range");

    addr = PCIEMU_HW_BAR0_END;
    EXPECT_TRUE(pciemu_mmio_valid_access(addr, size), "addr is inside range");

    addr = PCIEMU_HW_BAR0_END + 1;
    EXPECT_FALSE(pciemu_mmio_valid_access(addr, size), "addr is outside range");
}

TEST(pciemu_mmio_read, "Test MMIO read operations")
{
    PCIEMUDevice dev;
    unsigned int size = sizeof(uint64_t);
    uint64_t reg_val;
    uint64_t expect_reg[PCIEMU_HW_BAR0_REG_CNT] = { 0xaa, 0xbb, 0xcc, 0xdd };
    hwaddr reg_addr[PCIEMU_HW_BAR0_REG_CNT] = { PCIEMU_HW_BAR0_REG_0,
                                                PCIEMU_HW_BAR0_REG_1,
                                                PCIEMU_HW_BAR0_REG_2,
                                                PCIEMU_HW_BAR0_REG_3 };

    for (int i = 0; i < PCIEMU_HW_BAR0_REG_CNT; ++i) {
        dev.reg[i] = expect_reg[i];
    }

    for (int i = 0; i < PCIEMU_HW_BAR0_REG_CNT; ++i) {
        reg_val = pciemu_mmio_read(&dev, reg_addr[i], size);
        EXPECT_EQ(reg_val, expect_reg[i], "Should read value properly");
    }

    reg_val = pciemu_mmio_read(&dev, PCIEMU_HW_BAR0_END, size);
    EXPECT_EQ(reg_val, ~0ULL, "Should not return any register value");
}

TEST(pciemu_mmio_write, "Test MMIO write operations")
{
    PCIEMUDevice dev;
    uint64_t val;
    unsigned int size = sizeof(uint64_t);
    hwaddr reg_addr[PCIEMU_HW_BAR0_REG_CNT] = { PCIEMU_HW_BAR0_REG_0,
                                                PCIEMU_HW_BAR0_REG_1,
                                                PCIEMU_HW_BAR0_REG_2,
                                                PCIEMU_HW_BAR0_REG_3 };

    for (int i = 0; i < PCIEMU_HW_BAR0_REG_CNT; ++i) {
        val = 0xbb + i;
        pciemu_mmio_write(&dev, reg_addr[i], val, size);
        EXPECT_EQ(dev.reg[i], val, "Should set value properly");
    }

    pciemu_mmio_write(&dev, PCIEMU_HW_BAR0_IRQ_0_RAISE, val, size);
    EXPECT_EQ(pciemu_irq_raise_fake.call_count, 1, "Should call once");
    EXPECT_EQ(pciemu_irq_raise_fake.arg1_val, 0,
              "Should raise correct irq num");

    pciemu_mmio_write(&dev, PCIEMU_HW_BAR0_IRQ_0_LOWER, val, size);
    EXPECT_EQ(pciemu_irq_lower_fake.call_count, 1, "Should call once");
    EXPECT_EQ(pciemu_irq_lower_fake.arg1_val, 0,
              "Should raise correct irq num");

    pciemu_mmio_write(&dev, PCIEMU_HW_BAR0_DMA_CFG_TXDESC_SRC, val, size);
    EXPECT_EQ(pciemu_dma_config_txdesc_src_fake.call_count, 1,
              "Should call once");
    EXPECT_EQ(pciemu_dma_config_txdesc_src_fake.arg1_val, val,
              "Should call with correct arguments");

    pciemu_mmio_write(&dev, PCIEMU_HW_BAR0_DMA_CFG_TXDESC_DST, val, size);
    EXPECT_EQ(pciemu_dma_config_txdesc_dst_fake.call_count, 1,
              "Should call once");
    EXPECT_EQ(pciemu_dma_config_txdesc_dst_fake.arg1_val, val,
              "Should call with correct arguments");

    pciemu_mmio_write(&dev, PCIEMU_HW_BAR0_DMA_CFG_TXDESC_LEN, val, size);
    EXPECT_EQ(pciemu_dma_config_txdesc_len_fake.call_count, 1,
              "Should call once");
    EXPECT_EQ(pciemu_dma_config_txdesc_len_fake.arg1_val, val,
              "Should call with correct arguments");

    pciemu_mmio_write(&dev, PCIEMU_HW_BAR0_DMA_CFG_CMD, val, size);
    EXPECT_EQ(pciemu_dma_config_cmd_fake.call_count, 1, "Should call once");
    EXPECT_EQ(pciemu_dma_config_cmd_fake.arg1_val, val,
              "Should call with correct arguments");

    pciemu_mmio_write(&dev, PCIEMU_HW_BAR0_DMA_DOORBELL_RING, val, size);
    EXPECT_EQ(pciemu_dma_doorbell_ring_fake.call_count, 1, "Should call once");
}

TEST(pciemu_mmio_reset, "Test reset of MMIO")
{
    PCIEMUDevice dev;
    for (int i = 0; i < PCIEMU_HW_BAR0_REG_CNT; ++i) {
        dev.reg[i] = i + 0xaa;
    }
    pciemu_mmio_reset(&dev);
    for (int i = 0; i < PCIEMU_HW_BAR0_REG_CNT; ++i) {
        EXPECT_EQ(dev.reg[i], 0, "Should have been reset");
    }
}

TEST(pciemu_device_init, "Test initialization of MMIO")
{
    PCIEMUDevice dev;
    Error *e;
    pciemu_mmio_init(&dev, &e);
    EXPECT_EQ(memory_region_init_io_fake.call_count, 1, "Should call once");

    EXPECT_EQ(pci_register_bar_fake.call_count, 1, "Should call once");
    EXPECT_EQ(pci_register_bar_fake.arg1_val, 0,
              "Should use BAR0 as region_num");
    EXPECT_EQ(pci_register_bar_fake.arg2_val, PCI_BASE_ADDRESS_SPACE_MEMORY,
              "Should use PCI_BASE_ADDRESS_SPACE_MEMORY as type");
}

TEST(pciemu_device_fini, "Test finalization of MMIO")
{
    /* For now, we basically call pciemu_mmio_reset. Thus, skip */
    TEST_SKIP();
}

TEST_MAIN()
