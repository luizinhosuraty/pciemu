/* pciemu_module.h - header file with definitions for pciemu module
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef _PCIEMU_MODULE_H_
#define _PCIEMU_MODULE_H_

#include <linux/pci.h>
#include <linux/cdev.h>

/* forward declaration */
struct pciemu_dev;

struct pciemu_bar {
	u64 start;
	u64 end;
	u64 len;
	void __iomem *mmio;
};

struct pciemu_dma {
	dma_addr_t dma_handle;
	size_t offset;
	size_t len;
	enum dma_data_direction direction;
	struct page *page;
};

struct pciemu_irq {
	void __iomem *mmio_ack_irq;
	int irq_num;
};

struct pciemu_dev {
	struct pci_dev *pdev;
	/* In this simple PCIe device, we only a single BAR (0)
	 * We could have an array of size PCI_STD_NUM_BARS to
	 * hold information about all bars.
	 */
	struct pciemu_bar bar;
	/* Only one IRQ is used in this simple device :
	 *  - IRQ to inform that DMA has finished
	 * We could also have an array here to describe more IRQs
	 */
	struct pciemu_irq irq;

	struct pciemu_dma dma;
	dev_t minor;
	dev_t major;
	struct cdev cdev;
};

int pciemu_dma_from_host_to_device(struct pciemu_dev *pciemu_dev,
				   struct page *page, size_t offset,
				   size_t size);

int pciemu_dma_from_device_to_host(struct pciemu_dev *pciemu_dev,
				   struct page *page, size_t offset,
				   size_t size);

int pciemu_irq_enable(struct pciemu_dev *pciemu_dev);

#endif /* _PCIEMU_MODULE_H_ */
