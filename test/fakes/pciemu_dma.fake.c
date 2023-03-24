/* dma.fake.c - DMA fake functions
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */


#include "pciemu_dma.fake.h"

DEFINE_FAKE_VOID_FUNC(pciemu_dma_config_txdesc_src, PCIEMUDevice *, dma_addr_t);
DEFINE_FAKE_VOID_FUNC(pciemu_dma_config_txdesc_dst, PCIEMUDevice *, dma_addr_t);
DEFINE_FAKE_VOID_FUNC(pciemu_dma_config_txdesc_len, PCIEMUDevice *, dma_size_t);
DEFINE_FAKE_VOID_FUNC(pciemu_dma_config_cmd, PCIEMUDevice *, dma_cmd_t);
DEFINE_FAKE_VOID_FUNC(pciemu_dma_doorbell_ring, PCIEMUDevice *);
DEFINE_FAKE_VOID_FUNC(pciemu_dma_init, PCIEMUDevice *, Error **);
DEFINE_FAKE_VOID_FUNC(pciemu_dma_fini, PCIEMUDevice *);
DEFINE_FAKE_VOID_FUNC(pciemu_dma_reset, PCIEMUDevice *);
