/* pciemu_irq.c - pciemu virtual device IRQ operations
 *
 * These are functions that configure IRQs and the appropriate handlers.
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */
#include "hw/pciemu_hw.h"
#include "pciemu_module.h"
#include <linux/pci.h>

static irqreturn_t pciemu_irq_handler(int irq, void *data)
{
	struct pciemu_dev *pciemu_dev = data;

	dev_dbg(&pciemu_dev->pdev->dev, "irq_handler irq = %d dev = %d\n", irq,
		pciemu_dev->major);

	dma_unmap_page((&pciemu_dev->pdev->dev), pciemu_dev->dma.dma_handle,
		       pciemu_dev->dma.len, pciemu_dev->dma.direction);

	unpin_user_page(pciemu_dev->dma.page);
	/* Must do this ACK, or else the interrupt just keeps firing. */
	iowrite32(1, pciemu_dev->irq.mmio_ack_irq);
	return IRQ_HANDLED;
}

static int pciemu_irq_enable_msi(struct pciemu_dev *pciemu_dev)
{
	int msi_vecs_req;
	int msi_vecs;
	int err;

	/*
	 * Reserve the max msi vectors we might need
	 */
	msi_vecs_req = min_t(int, pci_msi_vec_count(pciemu_dev->pdev),
			     num_online_cpus() + 1);
	dev_dbg(&pciemu_dev->pdev->dev,
		"Trying to enable MSI, requesting %d vectors\n", msi_vecs_req);

	msi_vecs = pci_alloc_irq_vectors(pciemu_dev->pdev, msi_vecs_req,
					 msi_vecs_req, PCI_IRQ_MSI);

	if (msi_vecs < 0) {
		dev_err(&pciemu_dev->pdev->dev,
			"pciemu_irq_enable_msi failed, vectors %d\n", msi_vecs);
		return -ENOSPC;
	}

	if (msi_vecs != msi_vecs_req) {
		pci_free_irq_vectors(pciemu_dev->pdev);
		dev_err(&pciemu_dev->pdev->dev,
			"allocated %d MSI (out of %d requested)\n", msi_vecs,
			msi_vecs_req);
		return -ENOSPC;
	}

	pciemu_dev->irq.irq_num = pci_irq_vector(
		pciemu_dev->pdev, PCIEMU_HW_IRQ_DMA_ENDED_VECTOR);
	if (pciemu_dev->irq.irq_num < 0) {
		pci_free_irq_vectors(pciemu_dev->pdev);
		dev_err(&pciemu_dev->pdev->dev, "vector %d out of range\n",
			PCIEMU_HW_IRQ_DMA_ENDED_VECTOR);
		return -EINVAL;
	}

	err = request_irq(pciemu_dev->irq.irq_num, pciemu_irq_handler,
			  PCIEMU_HW_IRQ_DMA_ENDED_VECTOR,
			  "pciemu_irq_dma_ended", pciemu_dev);
	if (err) {
		dev_err(&pciemu_dev->pdev->dev,
			"failed to request irq %s (%d)\n",
			"pciemu_irq_dma_ended", err);
		pci_free_irq_vectors(pciemu_dev->pdev);
		return err;
	}

	pciemu_dev->irq.mmio_ack_irq =
		pciemu_dev->bar.mmio + PCIEMU_HW_IRQ_DMA_ACK_ADDR;
	return 0;
}

int pciemu_irq_enable(struct pciemu_dev *pciemu_dev)
{
	return pciemu_irq_enable_msi(pciemu_dev);
}
