/* pciemu_dma.c - pciemu virtual device DMA operations
 *
 * These are functions that map BARs inside the kernel module and 
 * access them directly from the kernel module.
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/dma-mapping.h>
#include "pciemu_module.h"
#include "hw/pciemu_hw.h"

static void pciemu_dma_struct_init(struct pciemu_dma *dma, size_t ofs,
				   size_t len, enum dma_data_direction drctn)
{
	dma->offset = ofs;
	dma->len = len;
	dma->direction = drctn;
}

int pciemu_dma_from_host_to_device(struct pciemu_dev *pciemu_dev,
				   struct page *page, size_t ofs, size_t len)
{
	struct pci_dev *pdev = pciemu_dev->pdev;
	void __iomem *mmio = pciemu_dev->bar.mmio;
	pciemu_dma_struct_init(&pciemu_dev->dma, ofs, len, DMA_TO_DEVICE);
	pciemu_dev->dma.dma_handle =
		dma_map_page(&(pdev->dev), page, pciemu_dev->dma.offset,
			     pciemu_dev->dma.len, pciemu_dev->dma.direction);
	if (dma_mapping_error(&(pdev->dev), pciemu_dev->dma.dma_handle))
		return -ENOMEM;
	dev_dbg(&(pdev->dev), "dma_handle_from = %llx\n",
		(unsigned long long)pciemu_dev->dma.dma_handle);
	dev_dbg(&(pdev->dev), "cmd = %x\n", PCIEMU_HW_DMA_DIRECTION_TO_DEVICE);
	iowrite32((u32)pciemu_dev->dma.dma_handle,
		  mmio + PCIEMU_HW_BAR0_DMA_CFG_TXDESC_SRC);
	iowrite32(PCIEMU_HW_DMA_AREA_START,
		  mmio + PCIEMU_HW_BAR0_DMA_CFG_TXDESC_DST);
	iowrite32(pciemu_dev->dma.len,
		  mmio + PCIEMU_HW_BAR0_DMA_CFG_TXDESC_LEN);
	iowrite32(PCIEMU_HW_DMA_DIRECTION_TO_DEVICE,
		  mmio + PCIEMU_HW_BAR0_DMA_CFG_CMD);
	iowrite32(1, mmio + PCIEMU_HW_BAR0_DMA_DOORBELL_RING);
	dev_dbg(&(pdev->dev), "done host->device...\n");
	return 0;
}

int pciemu_dma_from_device_to_host(struct pciemu_dev *pciemu_dev,
				   struct page *page, size_t ofs, size_t len)
{
	struct pci_dev *pdev = pciemu_dev->pdev;
	void __iomem *mmio = pciemu_dev->bar.mmio;
	pciemu_dma_struct_init(&pciemu_dev->dma, ofs, len, DMA_FROM_DEVICE);
	pciemu_dev->dma.dma_handle =
		dma_map_page(&(pdev->dev), page, pciemu_dev->dma.offset,
			     pciemu_dev->dma.len, pciemu_dev->dma.direction);
	if (dma_mapping_error(&(pdev->dev), pciemu_dev->dma.dma_handle))
		return -ENOMEM;
	dev_dbg(&(pdev->dev), "dma_handle_to = %llx\n",
		(unsigned long long)pciemu_dev->dma.dma_handle);
	dev_dbg(&(pdev->dev), "cmd = %x\n",
		PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE);
	iowrite32(PCIEMU_HW_DMA_AREA_START,
		  mmio + PCIEMU_HW_BAR0_DMA_CFG_TXDESC_SRC);
	iowrite32((u32)pciemu_dev->dma.dma_handle,
		  mmio + PCIEMU_HW_BAR0_DMA_CFG_TXDESC_DST);
	iowrite32(pciemu_dev->dma.len,
		  mmio + PCIEMU_HW_BAR0_DMA_CFG_TXDESC_LEN);
	iowrite32(PCIEMU_HW_DMA_DIRECTION_FROM_DEVICE,
		  mmio + PCIEMU_HW_BAR0_DMA_CFG_CMD);
	iowrite32(1, mmio + PCIEMU_HW_BAR0_DMA_DOORBELL_RING);
	dev_dbg(&(pdev->dev), "done device->host...\n\n");
	return 0;
}
