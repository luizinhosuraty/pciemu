/* pciemu_ioctl.h - IOCTL definitions
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef _PCIEMU_IOCTL_H_
#define _PCIEMU_IOCTL_H_

#define PCIEMU_IOCTL_MAGIC 0xE1

#define PCIEMU_IOCTL_DMA_TO_DEVICE _IOW(PCIEMU_IOCTL_MAGIC, 1, void *)
#define PCIEMU_IOCTL_DMA_FROM_DEVICE _IOR(PCIEMU_IOCTL_MAGIC, 2, void *)

#endif /* _PCIEMU_IOCTL_H_ */
