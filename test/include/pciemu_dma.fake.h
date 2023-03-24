/* dma.fake.h - DMA fake functions header
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef PCIEMU_DMA_FAKE_H
#define PCIEMU_DMA_FAKE_H

#include "fff_config.h"

#include "dma.h"

DECLARE_FAKE_VOID_FUNC(pciemu_dma_config_txdesc_src, PCIEMUDevice *,
                       dma_addr_t);
DECLARE_FAKE_VOID_FUNC(pciemu_dma_config_txdesc_dst, PCIEMUDevice *,
                       dma_addr_t);
DECLARE_FAKE_VOID_FUNC(pciemu_dma_config_txdesc_len, PCIEMUDevice *,
                       dma_size_t);
DECLARE_FAKE_VOID_FUNC(pciemu_dma_config_cmd, PCIEMUDevice *, dma_cmd_t);
DECLARE_FAKE_VOID_FUNC(pciemu_dma_doorbell_ring, PCIEMUDevice *);
DECLARE_FAKE_VOID_FUNC(pciemu_dma_init, PCIEMUDevice *, Error **);
DECLARE_FAKE_VOID_FUNC(pciemu_dma_fini, PCIEMUDevice *);
DECLARE_FAKE_VOID_FUNC(pciemu_dma_reset, PCIEMUDevice *);

#endif /* PCIEMU_DMA_FAKE_H */
