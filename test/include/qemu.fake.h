/* qemu.fake.h - QEMU fake functions header
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef QEMU_FAKE_H
#define QEMU_FAKE_H

#include "fff_config.h"

#include "qemu/osdep.h"
#include "qom/object.h"
#include "exec/memory.h"

DECLARE_FAKE_VALUE_FUNC(Type, type_register_static, const TypeInfo *);
DECLARE_FAKE_VALUE_FUNC(ObjectClass *, object_class_dynamic_cast_assert,
                        ObjectClass *, const char *, const char *, int,
                        const char *);
DECLARE_FAKE_VALUE_FUNC(Object *, object_dynamic_cast_assert, Object *,
                        const char *, const char *, int, const char *);
typedef void (*type_init_fn_arg)(void);
DECLARE_FAKE_VOID_FUNC(register_module_init, type_init_fn_arg,
                       module_init_type);

DECLARE_FAKE_VALUE_FUNC(MemTxResult, address_space_rw, AddressSpace *, hwaddr,
                        MemTxAttrs, void *, hwaddr, bool);

DECLARE_FAKE_VALUE_FUNC(size_t, qemu_target_page_size);

DECLARE_FAKE_VOID_FUNC_VARARG(qemu_log, const char *, ...);

DECLARE_FAKE_VOID_FUNC(pci_set_irq, PCIDevice *, int);

DECLARE_FAKE_VOID_FUNC(pci_register_bar, PCIDevice *, int, uint8_t,
                       MemoryRegion *);

DECLARE_FAKE_VALUE_FUNC(int, msi_init, struct PCIDevice *, uint8_t,
                        unsigned int, bool, bool, Error **);

DECLARE_FAKE_VALUE_FUNC(bool, msi_enabled, const PCIDevice *);

DECLARE_FAKE_VOID_FUNC(msi_notify, PCIDevice *, unsigned int);

DECLARE_FAKE_VOID_FUNC(msi_uninit, struct PCIDevice *);

DECLARE_FAKE_VOID_FUNC(memory_region_init_io, MemoryRegion *, Object *,
                       const MemoryRegionOps *, void *, const char *, uint64_t);

#endif /* QEMU_FAKE_H */
