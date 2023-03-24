pciemu_ss = ss.source_set()
pciemu_ss.add(files(
    'dma.c',
    'irq.c',
    'mmio.c',
    'pciemu.c',
))

softmmu_ss.add_all(when: 'CONFIG_PCIEMU', if_true: pciemu_ss)
