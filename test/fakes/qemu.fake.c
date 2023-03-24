/* qemu.fake.c - QEMU fake functions
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "qemu.fake.h"

DEFINE_FAKE_VALUE_FUNC(Type, type_register_static, const TypeInfo *);
DEFINE_FAKE_VALUE_FUNC(ObjectClass *, object_class_dynamic_cast_assert,
                       ObjectClass *, const char *, const char *, int,
                       const char *);
DEFINE_FAKE_VALUE_FUNC(Object *, object_dynamic_cast_assert, Object *,
                       const char *, const char *, int, const char *);
typedef void (*type_init_fn_arg)(void);
DEFINE_FAKE_VOID_FUNC(register_module_init, type_init_fn_arg, module_init_type);

/* from qemu/softmmu/physmem.c
 * pci_dma_read and pci_dma_write are inlined, which after a sequence of calls
 * to other inline functions end up calling address_space_rw
 */
DEFINE_FAKE_VALUE_FUNC(MemTxResult, address_space_rw, AddressSpace *, hwaddr,
                       MemTxAttrs, void *, hwaddr, bool);

DEFINE_FAKE_VALUE_FUNC(size_t, qemu_target_page_size);

/* from qemu/hw/pci/pci.c */
DEFINE_FAKE_VOID_FUNC(pci_set_irq, PCIDevice *, int);

DEFINE_FAKE_VOID_FUNC(pci_register_bar, PCIDevice *, int, uint8_t,
                      MemoryRegion *);

/* from qemu/hw/pci/msi.c */
DEFINE_FAKE_VALUE_FUNC(int, msi_init, struct PCIDevice *, uint8_t, unsigned int,
                       bool, bool, Error **);
DEFINE_FAKE_VALUE_FUNC(bool, msi_enabled, const PCIDevice *);

DEFINE_FAKE_VOID_FUNC(msi_notify, PCIDevice *, unsigned int);

DEFINE_FAKE_VOID_FUNC(msi_uninit, struct PCIDevice *);

/* from qemu/softmmu/memory.c */
DEFINE_FAKE_VOID_FUNC(memory_region_init_io, MemoryRegion *, Object *,
                      const MemoryRegionOps *, void *, const char *, uint64_t);

/* from qemu/util/log.c */
int qemu_loglevel = 0;
DEFINE_FAKE_VOID_FUNC_VARARG(qemu_log, const char *, ...);
