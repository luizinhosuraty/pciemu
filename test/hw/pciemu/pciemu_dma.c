/* pciemu_dma.c - Unit tests for hw/pciemu/dma.c file
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "unitctest/unitctest.h"
#include "fff/fff.h"
#include "qemu.fake.h"
#include "pciemu_irq.fake.h"
#include "pciemu_mmio.fake.h"

/* include the source file to test static functions */
#include "../src/hw/pciemu/dma.c"

DEFINE_FFF_GLOBALS;

TEST(pciemu_dma_addr_mask, "Test masking of DMA address")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    dev.dma.config.mask = DMA_BIT_MASK(32);
    dma_addr_t addr = 0xaaaaaaaabbbbbbbb;
    dma_addr_t masked = pciemu_dma_addr_mask(&dev, addr);
    EXPECT_EQ(masked, 0xbbbbbbbb, "Should mask on 32 bits");

    dev.dma.config.mask = DMA_BIT_MASK(16);
    masked = pciemu_dma_addr_mask(&dev, addr);
    EXPECT_NEQ(masked, 0xbbbbbbbb, "Should mask on 16 bits");
    EXPECT_EQ(masked, 0xbbbb, "Should mask on 16 bits");
}

TEST(pciemu_dma_inside_device_boundaries, "Test DMA area boundaries")
{
    dma_addr_t addr = PCIEMU_HW_DMA_AREA_START;
    EXPECT_TRUE(pciemu_dma_inside_device_boundaries(addr), "Inside area");

    addr = PCIEMU_HW_DMA_AREA_START + PCIEMU_HW_DMA_AREA_SIZE;
    EXPECT_TRUE(pciemu_dma_inside_device_boundaries(addr), "Inside area");

    addr = PCIEMU_HW_DMA_AREA_START + PCIEMU_HW_DMA_AREA_SIZE + 1;
    EXPECT_FALSE(pciemu_dma_inside_device_boundaries(addr), "Outside area");
}

TEST(pciemu_dma_execute, "Test execution of DMA")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    dev.dma.config.mask = DMA_BIT_MASK(PCIEMU_HW_DMA_ADDR_CAPABILITY);

    dev.dma.config.cmd = PCIEMU_HW_DMA_DIRECTION_TO_DEVICE;
    dma_addr_t src = 0xbeefbeef;
    dev.dma.config.txdesc.src = src;
    dev.dma.config.txdesc.dst = PCIEMU_HW_DMA_AREA_START;
    pciemu_dma_execute(&dev);
    EXPECT_EQ(address_space_rw_fake.call_count, 1,
              "Should call pci_dma_read once");
    EXPECT_EQ(address_space_rw_fake.arg5_val, false,
              "Should perform pci_dma_read");
    EXPECT_EQ(address_space_rw_fake.arg1_val, src,
              "Should perform pci_dma_read from address in txdesc.src");
    EXPECT_EQ(address_space_rw_fake.arg3_val, &dev.dma.buff[0],
              "Should perform pci_dma_read to start of dedicated area");
    EXPECT_EQ(pciemu_irq_raise_fake.call_count, 1, "Should raise irq once");
    EXPECT_EQ(pciemu_irq_raise_fake.arg1_val, PCIEMU_HW_IRQ_DMA_ENDED_VECTOR,
              "Should raise the correct irq");

    RESET_FAKE(pciemu_irq_raise);
    RESET_FAKE(address_space_rw);
    dev.dma.config.cmd = PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE;
    dma_addr_t dst = 0xaaaabbbb;
    dev.dma.config.txdesc.dst = dst;
    dev.dma.config.txdesc.src = PCIEMU_HW_DMA_AREA_START;
    pciemu_dma_execute(&dev);
    EXPECT_EQ(address_space_rw_fake.call_count, 1,
              "Should call pci_dma_write once");
    EXPECT_EQ(address_space_rw_fake.arg1_val, dst,
              "Should perform pci_dma_read to address in txdesc.dst");
    EXPECT_EQ(address_space_rw_fake.arg3_val, &dev.dma.buff[0],
              "Should perform pci_dma_read from start of dedicated area");
    EXPECT_EQ(address_space_rw_fake.arg5_val, true,
              "Should perform pci_dma_write");
    EXPECT_EQ(pciemu_irq_raise_fake.call_count, 1, "Should raise irq once");
    EXPECT_EQ(pciemu_irq_raise_fake.arg1_val, PCIEMU_HW_IRQ_DMA_ENDED_VECTOR,
              "Should raise the correct irq");

    RESET_FAKE(pciemu_irq_raise);
    RESET_FAKE(address_space_rw);
    dev.dma.config.cmd = 0;
    pciemu_dma_execute(&dev);
    EXPECT_EQ(address_space_rw_fake.call_count, 0,
              "Should NOT perform pci_dma_write : wrong cmd");
    EXPECT_EQ(pciemu_irq_raise_fake.call_count, 0, "Should NOT raise irq");
}

TEST(pciemu_dma_doorbell_ring, "Test reception of DMA doorbell")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    RESET_FAKE(address_space_rw);
    RESET_FAKE(pciemu_irq_raise);
    dev.dma.config.mask = DMA_BIT_MASK(PCIEMU_HW_DMA_ADDR_CAPABILITY);
    dev.dma.config.cmd = PCIEMU_HW_DMA_DIRECTION_TO_DEVICE;
    dev.dma.config.txdesc.dst = PCIEMU_HW_DMA_AREA_START;
    dev.dma.status = DMA_STATUS_IDLE;
    pciemu_dma_doorbell_ring(&dev);
    EXPECT_EQ(dev.dma.status, DMA_STATUS_IDLE,
              "Should return with IDLE status");
    EXPECT_EQ(address_space_rw_fake.call_count, 1,
              "Should call pci_dma_read once (proxy in pciemu_dma_execute)");
    EXPECT_EQ(pciemu_irq_raise_fake.call_count, 1, "Should raise irq once");

    RESET_FAKE(pciemu_irq_raise);
    dev.dma.status = DMA_STATUS_EXECUTING;
    pciemu_dma_doorbell_ring(&dev);
    EXPECT_EQ(dev.dma.status, DMA_STATUS_EXECUTING,
              "Should do nothing and return with EXECUTING status");
    EXPECT_EQ(pciemu_irq_raise_fake.call_count, 0, "Should not raise irq");
}

TEST(pciemu_dma_config_txdesc_src, "Test configuration of DMA txdesc src")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    dev.dma.status = DMA_STATUS_IDLE;
    dma_addr_t src = 0xbeefbeef;
    pciemu_dma_config_txdesc_src(&dev, src);
    EXPECT_EQ(dev.dma.config.txdesc.src, src, "Should set the value");

    dev.dma.config.txdesc.src = 0xdeadbeef;
    dev.dma.status = DMA_STATUS_EXECUTING;
    pciemu_dma_config_txdesc_src(&dev, src);
    EXPECT_NEQ(dev.dma.config.txdesc.src, src, "Should not set the value");
}

TEST(pciemu_dma_config_txdesc_dst, "Test configuration of DMA txdesc dst")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    dev.dma.status = DMA_STATUS_IDLE;
    dma_addr_t dst = 0xbeefbeef;
    pciemu_dma_config_txdesc_dst(&dev, dst);
    EXPECT_EQ(dev.dma.config.txdesc.dst, dst, "Should set the value");

    dev.dma.config.txdesc.dst = 0xdeadbeef;
    dev.dma.status = DMA_STATUS_EXECUTING;
    pciemu_dma_config_txdesc_len(&dev, dst);
    EXPECT_NEQ(dev.dma.config.txdesc.dst, dst, "Should not set the value");
}

TEST(pciemu_dma_config_txdesc_len, "Test configuration of DMA txdesc len")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    dev.dma.status = DMA_STATUS_IDLE;
    dma_size_t len = 0x100;
    pciemu_dma_config_txdesc_len(&dev, len);
    EXPECT_EQ(dev.dma.config.txdesc.len, len, "Should set the value");

    dev.dma.config.txdesc.len = 0x800;
    dev.dma.status = DMA_STATUS_EXECUTING;
    pciemu_dma_config_txdesc_len(&dev, len);
    EXPECT_NEQ(dev.dma.config.txdesc.len, len, "Should not set the value");
}

TEST(pciemu_dma_config_cmd, "Test configuration of DMA cmd")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    dev.dma.status = DMA_STATUS_IDLE;
    dma_cmd_t cmd = PCIEMU_HW_DMA_DIRECTION_TO_DEVICE;
    pciemu_dma_config_cmd(&dev, cmd);
    EXPECT_EQ(dev.dma.config.cmd, cmd, "Should set the value");

    dev.dma.config.cmd = 0;
    dev.dma.status = DMA_STATUS_EXECUTING;
    pciemu_dma_config_cmd(&dev, cmd);
    EXPECT_NEQ(dev.dma.config.cmd, cmd, "Should not set the value");
}
TEST(pciemu_dma_reset, "Test reset of DMA")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    pciemu_dma_reset(&dev);
    EXPECT_EQ(dev.dma.status, DMA_STATUS_IDLE, "Should have IDLE status");
    EXPECT_EQ(dev.dma.config.txdesc.src, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.txdesc.dst, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.txdesc.len, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.cmd, 0, "Should be initialized to zero");
}

TEST(pciemu_dma_init, "Test initialization of DMA")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    Error *e = NULL;
    pciemu_dma_init(&dev, &e);
    EXPECT_EQ(dev.dma.status, DMA_STATUS_IDLE, "Should have IDLE status");
    EXPECT_EQ(dev.dma.config.txdesc.src, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.txdesc.dst, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.txdesc.len, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.cmd, 0, "Should be initialized to zero");
}

TEST(pciemu_dma_fini, "Test finalization of DMA")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    pciemu_dma_fini(&dev);
    EXPECT_EQ(dev.dma.status, DMA_STATUS_OFF, "Should have OFF status");
    EXPECT_EQ(dev.dma.config.txdesc.src, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.txdesc.dst, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.txdesc.len, 0, "Should be initialized to zero");
    EXPECT_EQ(dev.dma.config.cmd, 0, "Should be initialized to zero");
}

TEST_MAIN()
