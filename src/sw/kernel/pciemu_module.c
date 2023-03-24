/* pciemu_module.c - Kernel module to control the pciemu virtual device.
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include "hw/pciemu_hw.h"
#include "pciemu_module.h"
#include "sw/module/pciemu_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Kernel module to drive the pciemu virutal device");
MODULE_AUTHOR("Luiz Henrique Suraty Filho <luiz-dev@suraty.com>");

static struct class *pciemu_class;

static struct pci_device_id pciemu_id_tbl[] = {
	{ PCI_DEVICE(PCIEMU_HW_VENDOR_ID, PCIEMU_HW_DEVICE_ID) },
	{},
};

MODULE_DEVICE_TABLE(pci, pciemu_id_tbl);

static int pciemu_open(struct inode *inode, struct file *fp)
{
	unsigned int bar = iminor(inode);
	struct pciemu_dev *pciemu_dev =
		container_of(inode->i_cdev, struct pciemu_dev, cdev);
	/* Only BAR 0 operations */
	if (bar != PCIEMU_HW_BAR0)
		return -ENXIO;
	if (pciemu_dev->bar.len == 0)
		return -EIO;
	fp->private_data = pciemu_dev;
	return 0;
}

static int pciemu_mmap(struct file *fp, struct vm_area_struct *vma)
{
	int ret = 0;
	struct pciemu_dev *pciemu_dev = fp->private_data;
	unsigned long pfn = pciemu_dev->bar.start >> PAGE_SHIFT;
	if (vma->vm_end - vma->vm_start > pciemu_dev->bar.len)
		return -EIO;
	ret = io_remap_pfn_range(vma, vma->vm_start, pfn,
				 vma->vm_end - vma->vm_start,
				 vma->vm_page_prot);
	return ret;
}

static long pciemu_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	struct pciemu_dev *pciemu_dev = fp->private_data;
	int pages_pinned = 0;
	int pages_nb_req = 1;
	unsigned long __user vaddr = arg;
	unsigned long ofs = vaddr & ~PAGE_MASK;
	unsigned long len = ((ofs + sizeof(int)) > PAGE_SIZE) ?
				    (PAGE_SIZE - ofs) :
				    sizeof(int);
	dev_dbg(&(pciemu_dev->pdev->dev), "pciemu_ioctl, cmd = %x, addr=%lx\n",
		cmd, vaddr);
	switch (cmd) {
	case PCIEMU_IOCTL_DMA_TO_DEVICE:
		pages_pinned = pin_user_pages_fast(vaddr, pages_nb_req,
						   FOLL_LONGTERM,
						   &pciemu_dev->dma.page);
		if (pages_pinned == pages_nb_req) {
			pciemu_dma_from_host_to_device(
				pciemu_dev, pciemu_dev->dma.page, ofs, len);
		}
		break;
	case PCIEMU_IOCTL_DMA_FROM_DEVICE:
		pages_pinned = pin_user_pages_fast(vaddr, pages_nb_req,
						   FOLL_LONGTERM,
						   &pciemu_dev->dma.page);
		if (pages_pinned == pages_nb_req) {
			pciemu_dma_from_device_to_host(
				pciemu_dev, pciemu_dev->dma.page, ofs, len);
		}
		break;
	default:
		return -ENOTTY;
	}
	return 0;
}

static const struct file_operations pciemu_fops = {
	.owner = THIS_MODULE,
	.open = pciemu_open,
	.mmap = pciemu_mmap,
	.unlocked_ioctl = pciemu_ioctl,
};

static void pciemu_dev_clean(struct pciemu_dev *pciemu_dev)
{
	pciemu_dev->bar.start = 0;
	pciemu_dev->bar.end = 0;
	pciemu_dev->bar.len = 0;
	if (pciemu_dev->bar.mmio)
		pci_iounmap(pciemu_dev->pdev, pciemu_dev->bar.mmio);
}

static int pciemu_dev_init(struct pciemu_dev *pciemu_dev, struct pci_dev *pdev)
{
	const unsigned int bar = PCIEMU_HW_BAR0;
	pciemu_dev->pdev = pdev;

	/* Initialize struct with BAR 0 info */
	pciemu_dev->bar.start = pci_resource_start(pdev, bar);
	pciemu_dev->bar.end = pci_resource_end(pdev, bar);
	pciemu_dev->bar.len = pci_resource_len(pdev, bar);
	pciemu_dev->bar.mmio = pci_iomap(pdev, bar, pciemu_dev->bar.len);
	if (!pciemu_dev->bar.mmio) {
		dev_err(&(pdev->dev), "cannot map BAR %u\n", bar);
		pciemu_dev_clean(pciemu_dev);
		return -ENOMEM;
	}
	pci_set_drvdata(pdev, pciemu_dev);
	return 0;
}

static struct pciemu_dev *pciemu_alloc_dev(void)
{
	return kmalloc(sizeof(struct pciemu_dev), GFP_KERNEL);
}

static int pciemu_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int err;
	int mem_bars;
	struct pciemu_dev *pciemu_dev;
	dev_t dev_num;
	struct device *dev;

	/* Allocate the struct that will hold our device state/context*/
	pciemu_dev = pciemu_alloc_dev();
	if (pciemu_dev == NULL) {
		err = -ENOMEM;
		dev_err(&(pdev->dev), "pciemu_alloc_device failed\n");
		goto err_pciemu_alloc;
	}

	/* Enable the PCI device. This will :
	 *  - wake up the device if it was in suspended state,
	 *  - allocate I/O and memory regions of the device (if BIOS did not),
	 *  - allocate an IRQ (if BIOS did not).
	 */
	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&(pdev->dev), "pciemu_enable_device failed\n");
		goto err_pci_enable;
	}

	/* Set the DMA mask */
	err = dma_set_mask_and_coherent(
		&pdev->dev, DMA_BIT_MASK(PCIEMU_HW_DMA_ADDR_CAPABILITY));
	if (err) {
		dev_err(&(pdev->dev), "dma_set_mask_and_coherent\n");
		goto err_dma_set_mask;
	}

	/* Enable DMA (set the bus master bit in the PCI_COMMAND register) */
	pci_set_master(pdev);

	/* verify no other device is already using the same address resource */
	mem_bars = pci_select_bars(pdev, IORESOURCE_MEM);
	if ((mem_bars & (1 << PCIEMU_HW_BAR0)) == 0) {
		dev_err(&(pdev->dev), "pci_select_bars: bar0 not available\n");
		goto err_select_region;
	}
	err = pci_request_selected_regions(pdev, mem_bars,
					   "pciemu_device_bars");
	if (err) {
		dev_err(&(pdev->dev), "pci_request_region: bars being used\n");
		goto err_req_region;
	}

	err = pciemu_dev_init(pciemu_dev, pdev);
	if (err) {
		dev_err(&(pdev->dev), "pciemu_dev_init failed\n");
		goto err_dev_init;
	}

	/* Get device number range (base_minor = bar0 and count = nbr of bars)*/
	err = alloc_chrdev_region(&dev_num, PCIEMU_HW_BAR0, PCIEMU_HW_BAR_CNT,
				  "pciemu");
	if (err) {
		dev_err(&(pdev->dev), "alloc_chrdev_region failed\n");
		goto err_alloc_chrdev;
	}

	/* save minor and major */
	pciemu_dev->minor = MINOR(dev_num);
	pciemu_dev->major = MAJOR(dev_num);

	/* connect cdev with file operations */
	cdev_init(&pciemu_dev->cdev, &pciemu_fops);

	/* add major/min range to cdev */
	err = cdev_add(&pciemu_dev->cdev,
		       MKDEV(pciemu_dev->major, pciemu_dev->minor),
		       PCIEMU_HW_BAR_CNT);
	if (err) {
		dev_err(&(pdev->dev), "cdev_add failed\n");
		goto err_cdev_add;
	}

	/* create /dev/ node via udev */
	dev = device_create(pciemu_class, &pdev->dev,
			    MKDEV(pciemu_dev->major, pciemu_dev->minor),
			    pciemu_dev, "d%xb%xd%xf%x_bar%u",
			    pci_domain_nr(pdev->bus), pdev->bus->number,
			    PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn),
			    PCIEMU_HW_BAR0);
	if (IS_ERR(dev)) {
		err = PTR_ERR(dev);
		dev_err(&(pdev->dev), "device_create failed\n");
		goto err_device_create;
	}

	/* enable IRQs */
	err = pciemu_irq_enable(pciemu_dev);
	if (err) {
		dev_err(&(pdev->dev), "pciemu_irq_enable failed\n");
		goto err_irq_enable;
	}

	dev_info(&(pdev->dev), "pciemu probe - success\n");

	return 0;

err_irq_enable:
	device_destroy(pciemu_class,
		       MKDEV(pciemu_dev->major, pciemu_dev->minor));

err_device_create:
	cdev_del(&pciemu_dev->cdev);

err_cdev_add:
	unregister_chrdev_region(MKDEV(pciemu_dev->major, pciemu_dev->minor),
				 PCIEMU_HW_BAR_CNT);

err_alloc_chrdev:
	pciemu_dev_clean(pciemu_dev);

err_dev_init:
	pci_release_selected_regions(pdev,
				     pci_select_bars(pdev, IORESOURCE_MEM));

err_req_region:
err_select_region:
	pci_clear_master(pdev);

err_dma_set_mask:
	pci_disable_device(pdev);

err_pci_enable:
	kfree(pciemu_dev);

err_pciemu_alloc:
	dev_err(&(pdev->dev), "pciemu_probe failed with error=%d\n", err);
	return err;
}

static void pciemu_remove(struct pci_dev *pdev)
{
	struct pciemu_dev *pciemu_dev = pci_get_drvdata(pdev);
	device_destroy(pciemu_class,
		       MKDEV(pciemu_dev->major, pciemu_dev->minor));
	cdev_del(&pciemu_dev->cdev);
	unregister_chrdev_region(MKDEV(pciemu_dev->major, pciemu_dev->minor),
				 PCIEMU_HW_BAR_CNT);
	pciemu_dev_clean(pciemu_dev);
	pci_clear_master(pdev);
	free_irq(pciemu_dev->irq.irq_num, pciemu_dev);
	pci_free_irq_vectors(pdev);
	pci_release_selected_regions(pdev,
				     pci_select_bars(pdev, IORESOURCE_MEM));
	pci_disable_device(pdev);
	kfree(pciemu_dev);
	dev_info(&(pdev->dev), "pciemu remove - success\n");
}

static struct pci_driver pciemu_pci_driver = {
	.name = "pciemu",
	.id_table = pciemu_id_tbl,
	.probe = pciemu_probe,
	.remove = pciemu_remove,
};

static void pciemu_module_exit(void)
{
	pci_unregister_driver(&pciemu_pci_driver);
	class_destroy(pciemu_class);
	pr_debug("pciemu_module_exit finished successfully\n");
}

static char *pciemu_devnode(struct device *dev, umode_t *mode)
{
	if (mode)
		*mode = 0666;
	return kasprintf(GFP_KERNEL, "pciemu/%s", dev_name(dev));
}
static int __init pciemu_module_init(void)
{
	int err;
	pciemu_class = class_create(THIS_MODULE, "pciemu");
	if (IS_ERR(pciemu_class)) {
		pr_err("class_create error\n");
		err = PTR_ERR(pciemu_class);
		return err;
	}
	pciemu_class->devnode = pciemu_devnode;
	err = pci_register_driver(&pciemu_pci_driver);
	if (err) {
		pr_err("pci_register_driver error\n");
		goto err_pci;
	}
	pr_debug("pciemu_module_init finished successfully\n");
	return 0;
err_pci:
	class_destroy(pciemu_class);
	pr_err("pciemu_module_init failed with err=%d\n", err);
	return err;
}

module_init(pciemu_module_init);
module_exit(pciemu_module_exit);
