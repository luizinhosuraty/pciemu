/* pciemu_irq.c - Unit tests for pciemu
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "unitctest/unitctest.h"
#include "fff/fff.h"
#include "qemu.fake.h"

#include "../src/hw/pciemu/irq.c"

DEFINE_FFF_GLOBALS;

TEST(pciemu_irq_init_msi, "Test IRQ initialization in MSI mode")
{
    PCIEMUDevice dev;
    Error *e = NULL;
    pciemu_irq_init_msi(&dev, &e);
    EXPECT_EQ(msi_init_fake.call_count, 1, "Should call once");
    EXPECT_EQ(msi_init_fake.arg1_val, 0,
              "Should not set offset for MSI capability in PCI config");
    EXPECT_EQ(msi_init_fake.arg2_val, PCIEMU_HW_IRQ_CNT,
              "Should set the correct number of MSI vectors");
    EXPECT_EQ(
        msi_init_fake.arg3_val, true,
        "Should make the device capable of sending a 64-bit message addr");
    EXPECT_EQ(msi_init_fake.arg4_val, false,
              "Should not  make the device support per-vector masking");
}

TEST(pciemu_irq_init_intx, "Test IRQ initialization in PIN mode")
{
    PCIDevice pdev;
    uint8_t pci_conf[PCI_CFG_SPACE_EXP_SIZE];
    pdev.config = pci_conf;
    PCIEMUDevice dev;
    dev.pci_dev = pdev;
    Error *e = NULL;
    pciemu_irq_init_intx(&dev, &e);
    EXPECT_EQ(dev.pci_dev.config[PCI_INTERRUPT_PIN], PCIEMU_HW_IRQ_INTX + 1,
              "Should set PCI_INTERRUPT_PIN to INTA ");
}

TEST(pciemu_irq_raise_intx, "Test IRQ raise in PIN mode")
{
    PCIEMUDevice dev;
    pciemu_irq_raise_intx(&dev);
    EXPECT_EQ(pci_set_irq_fake.call_count, 1, "Should call once");
    EXPECT_EQ(pci_set_irq_fake.arg1_val, 1,
              "Should set the level to 1 (raise IRQ)");
    EXPECT_EQ(dev.irq.status.pin.raised, true,
              "Should correctly set the IRQ status");
}

TEST(pciemu_irq_raise_msi, "Test IRQ raise in MSI mode")
{
    PCIEMUDevice dev;
    unsigned int vector = PCIEMU_IRQ_MAX_VECTORS + 1;
    pciemu_irq_raise_msi(&dev, vector);
    EXPECT_EQ(msi_notify_fake.call_count, 0,
              "Should not notify if vector is out of range");

    vector = 0;
    dev.irq.status.msi.msi_vectors[vector].raised = false;
    pciemu_irq_raise_msi(&dev, vector);
    EXPECT_EQ(msi_notify_fake.call_count, 1, "Should notify once");
    EXPECT_EQ(msi_notify_fake.arg1_val, vector,
              "Should notify with correct vector");

    vector = 1;
    RESET_FAKE(msi_notify);
    dev.irq.status.msi.msi_vectors[vector].raised = true;
    pciemu_irq_raise_msi(&dev, vector);
    EXPECT_EQ(msi_notify_fake.call_count, 1, "Should notify once");
    EXPECT_EQ(msi_notify_fake.arg1_val, vector,
              "Should notify with correct vector");
}

TEST(pciemu_irq_lower_intx, "Test lowering IRQ in PIN mode")
{
    RESET_FAKE(pci_set_irq);
    PCIEMUDevice dev;
    pciemu_irq_lower_intx(&dev);
    EXPECT_EQ(pci_set_irq_fake.call_count, 1, "Should call once");
    EXPECT_EQ(pci_set_irq_fake.arg1_val, 0,
              "Should set the level to 0 (lower IRQ)");
    EXPECT_EQ(dev.irq.status.pin.raised, false,
              "Should correctly set the IRQ status");
}

TEST(pciemu_irq_lower_msi, "Test lowering IRQ in MSI mode")
{
    PCIEMUDevice dev;
    unsigned int vector = PCIEMU_IRQ_MAX_VECTORS + 1;
    dev.irq.status.msi.msi_vectors[vector].raised = true;
    pciemu_irq_lower_msi(&dev, vector);
    EXPECT_EQ(dev.irq.status.msi.msi_vectors[vector].raised, true,
              "Should not lower if vector is out of range");

    vector = 0;
    pciemu_irq_lower_msi(&dev, vector);
    EXPECT_EQ(dev.irq.status.msi.msi_vectors[vector].raised, false,
              "Should lower with correct vector");
}

TEST(pciemu_irq_reset, "Test reset of IRQ")
{
    PCIEMUDevice dev;
    pciemu_irq_reset(&dev);
    EXPECT_EQ(
        msi_enabled_fake.call_count,
        PCIEMU_HW_IRQ_VECTOR_END - PCIEMU_HW_IRQ_VECTOR_START + 1,
        "Should call pci_irq_lower for each vector (msi_enabled is proxy)");
}

TEST(pciemu_irq_init, "Test initialization of IRQ")
{
    /* The test is already done with the above tests of the static inline
     * functions called by pciemu_irq_init
     */
    TEST_SKIP();
}

TEST(pciemu_irq_fini, "Test finalization of IRQ")
{
    PCIEMUDevice dev;
    pciemu_irq_fini(&dev);
    EXPECT_EQ(msi_uninit_fake.call_count, 1, "Should call once");
}

TEST_MAIN()
