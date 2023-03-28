/* pciemu_unittest.c - Unit tests for pciemu
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
#include "pciemu_mmio.fake.h"

#include "../src/hw/pciemu/pciemu.c"

DEFINE_FFF_GLOBALS;

TEST(pciemu_register_types, "Test registration of PCIEMU device type")
{
    pciemu_register_types();
    EXPECT_EQ(type_register_static_fake.call_count, 1,
              "Should call type_register_static once");
}

TEST(pciemu_device_init, "Test initialization of PCIEMU device")
{
    PCIDevice pci_dev = { .name = "pciemu_test" };
    Error *e = NULL;
    pciemu_device_init(&pci_dev, &e);
    EXPECT_EQ(pciemu_irq_init_fake.call_count, 1, "Should init irq once");
    EXPECT_EQ(pciemu_dma_init_fake.call_count, 1, "Should init dma once");
    EXPECT_EQ(pciemu_mmio_init_fake.call_count, 1, "Should init mmio once");
}

TEST(pciemu_device_fini, "Test finalization of PCIEMU device")
{
    PCIDevice pci_dev = { .name = "pciemu_test" };
    pciemu_device_fini(&pci_dev);
    EXPECT_EQ(pciemu_irq_fini_fake.call_count, 1, "Should fini irq once");
    EXPECT_EQ(pciemu_dma_fini_fake.call_count, 1, "Should fini dma once");
    EXPECT_EQ(pciemu_mmio_fini_fake.call_count, 1, "Should fini mmio once");
}

TEST(pciemu_reset, "Test reset of PCIEMU device")
{
    PCIEMUDevice dev = { .pci_dev = { .name = "pciemu_test" } };
    pciemu_reset(&dev);
    EXPECT_EQ(pciemu_irq_reset_fake.call_count, 1, "Should reset irq once");
    EXPECT_EQ(pciemu_dma_reset_fake.call_count, 1, "Should reset dma once");
    EXPECT_EQ(pciemu_mmio_reset_fake.call_count, 1, "Should reset mmio once");
}

TEST_MAIN()
