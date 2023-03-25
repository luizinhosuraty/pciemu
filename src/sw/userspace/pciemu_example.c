/* pciemu_example.c - Userspace example on how to use the pciemu device
 *
 * It requires the pciemu kernel module to be loaded to operate correctly.
 * This program basically performs the following :
 *  - opens the device file created by the kernel module
 *  - mmaps the BAR0 to access the device registers
 *  - uses ioctl to DMA to and from its own virtual memory (which are pinned on
 *    the kernel module)
 *
 * Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "hw/pciemu_hw.h"
#include "sw/module/pciemu_ioctl.h"

/* LOGs & co*/
#define KERR "\e[1;31m"
#define KNORM "\e[0m"
#define LOGF(fd, ...) fprintf(fd, __VA_ARGS__)
#define LOG_ERR(...)                \
    LOGF(stderr, KERR __VA_ARGS__); \
    LOGF(stderr, KNORM);
#define LOG(...) LOGF(stdout, __VA_ARGS__)

/* First invalid number (by definition PCI domain numbers are 16 bits long) */
#define PCI_DOMAIN_NUMBER_INVALID (1 << 16)
/* First invalid number (by definition PCI bus numbers are 8 bits long) */
#define PCI_BUS_NUMBER_INVALID (1 << 8)
/* First invalid (by definition PCI device numbers are 5 bits long) */
#define PCI_DEVICE_NUMBER_INVALID (1 << 5)
/* First invalid number (by definition PCI function numbers are 3 bits long) */
#define PCI_FUNCTION_NUMBER_INVALID (1 << 3)

struct context {
    uint64_t *virt_addr;     /* virtual @ of mmaped BAR 0  */
    int fd;                  /* file descriptor of dev file*/
    uint32_t pci_domain_nb;  /* PCI domain number of device (16 bits) */
    uint16_t pci_bus_nb;     /* PCI bus number of device (8 bits)  */
    uint16_t pci_hw_bar_len; /* length of mappable BAR 0   */
    uint8_t pci_hw_regs_nb;  /* number of 64bit device registers */
    uint8_t pci_device_nb;   /* PCI device number of device (5 bits) */
    uint8_t pci_func_nb;     /* PCI function number of device (3 bits) */
    uint8_t verbosity;       /* verbosity level for logs   */
};

static inline void usage(FILE *fd, char **argv)
{
    LOGF(fd, "Usage : %s [-b bus] [-d dom] [-h] [-r regs] [-s slot] [-v]\n",
         argv[0]);
    LOGF(fd, " \t -b bus \n\t\t pci bus number of device\n");
    LOGF(fd, " \t -d dom \n\t\t pci domain number of device\n");
    LOGF(fd, " \t -h \n\t\t display this help message\n");
    LOGF(fd, " \t -r regs \n\t\t number of 64bit device registers\n");
    LOGF(fd, " \t -s slot \n\t\t pci device number of device\n");
    LOGF(fd, " \t -v \n\t\t run on verbose mode\n");
}

static inline void rand_init(void)
{
    time_t t;
    srand((unsigned)time(&t));
}

static inline void print_registers(struct context *ctx, char *msg)
{
    LOG("%sread register contents : \n", msg);
    for (int i = 0; i < ctx->pci_hw_regs_nb; ++i)
        LOG("  virtaddr[%d] = %lu\n", i, ctx->virt_addr[i]);
}

/* use mmaped addresses to write (sequentially) values to PCI registers */
static inline void write_registers_sequential(struct context *ctx)
{
    rand_init();
    LOG("writing to registers ...\n");
    for (int i = 0; i < ctx->pci_hw_regs_nb; ++i)
        ctx->virt_addr[i] = rand() % 1024;
}

static inline void dump_mappings(void)
{
    char cmd[64];
    snprintf(cmd, 64, "cat /proc/%d/maps", getpid());
    system(cmd);
}

static int open_pciemu_dev(struct context *ctx)
{
    char filename[64];

    snprintf(filename, 64, "/dev/pciemu/d%ub%ud%uf%u_bar0", ctx->pci_domain_nb,
             ctx->pci_bus_nb, ctx->pci_device_nb, ctx->pci_func_nb);

    if (ctx->verbosity)
        LOG("filename = %s\n", filename);

    ctx->fd = open(filename, O_RDWR | O_SYNC);
    if (ctx->fd == -1) {
        LOG_ERR("fd failed - file %s\n", filename);
        close(ctx->fd);
    }

    return ctx->fd;
}

/* mmap BAR0 of the pciemu device into virt_addr */
static int mmap_pciemu_bar(struct context *ctx)
{
    ctx->virt_addr =
        (uint64_t *)mmap(NULL, ctx->pci_hw_bar_len, PROT_READ | PROT_WRITE,
                         MAP_SHARED, ctx->fd, 0);

    if (ctx->verbosity)
        LOG("%*p::mmap\n", 20, ctx->virt_addr);

    if (ctx->virt_addr == (void *)-1) {
        LOG_ERR("mmap failed\n");
        return -1;
    }

    if (ctx->verbosity)
        dump_mappings();

    return 0;
}

/* uses the ioctl syscals to perform DMA :
 *     - value of 'a' is DMA'ed into device memory;
 *     - 'b' is the destination of the value DMA'ed from the device memory
 *     - Thus, at the end, b == a;
 */
static int ioctl_pciemu(struct context *ctx)
{
    rand_init();
    int a = rand();
    int b = rand();

    LOG("initial values : a = %d  b = %d\n", a, b);

    LOG("dma direction to device, a@ = %p a = %d\n", &a, a);
    if (ioctl(ctx->fd, PCIEMU_IOCTL_DMA_TO_DEVICE, &a)) {
        LOG_ERR("ioctl failed\n");
        return -1;
    }

    LOG("dma direction from device, b@ = %p b = %d\n", &b, b);
    if (ioctl(ctx->fd, PCIEMU_IOCTL_DMA_FROM_DEVICE, &b)) {
        LOG_ERR("ioctl failed\n");
        return -1;
    }

    LOG("final values : a = %d b = %d\n", a, b);
    if (a != b) {
        LOG_ERR("values a and b should be equal after DMAs\n");
        return -2;
    }
    return 0;
}

/* parse arguments, note that some members of ctx are unmutable */
static struct context parse_args(int argc, char **argv)
{
    int op;
    char *endptr;
    struct context ctx;

    /* change domain_nb if necessary */
    ctx.pci_domain_nb = 0;
    ctx.pci_device_nb = PCI_DEVICE_NUMBER_INVALID;
    ctx.pci_bus_nb = PCI_BUS_NUMBER_INVALID;
    /* PCI funciton number is always 0 for pciemu */
    ctx.pci_func_nb = 0;
    /* get the number of registers directly from HW definitions */
    ctx.pci_hw_regs_nb = PCIEMU_HW_BAR0_REG_CNT;
    ctx.pci_hw_bar_len = ctx.pci_hw_regs_nb * sizeof(uint64_t);
    ctx.verbosity = 0;

    while ((op = getopt(argc, argv, "b:d:hr:s:v")) != -1) {
        switch (op) {
        case 'b':
            ctx.pci_bus_nb = strtol(optarg, &endptr, 10);
            if (errno != 0 || optarg == endptr) {
                LOG_ERR("strtol: invalid value (%s) for argument %c\n", optarg,
                        op);
                exit(-1);
            }
            if (ctx.pci_bus_nb >= PCI_BUS_NUMBER_INVALID) {
                LOG_ERR("PCI bus number (%u) out of range ([0, %u])\n",
                        ctx.pci_bus_nb, PCI_BUS_NUMBER_INVALID - 1);
                exit(-1);
            }
            break;
        case 'd':
            ctx.pci_domain_nb = strtol(optarg, &endptr, 10);
            if (errno != 0 || optarg == endptr) {
                LOG_ERR("strtol: invalid value (%s) for argument %c\n", optarg,
                        op);
                exit(-1);
            }
            if (ctx.pci_domain_nb >= PCI_DOMAIN_NUMBER_INVALID) {
                LOG_ERR("PCI domain number (%u) out of range ([0, %u])\n",
                        ctx.pci_domain_nb, PCI_DOMAIN_NUMBER_INVALID - 1);
                exit(-1);
            }
            break;
        case 'h':
            usage(stdout, argv);
            exit(0);
        case 'r':
            ctx.pci_hw_regs_nb = strtol(optarg, &endptr, 10);
            if (errno != 0 || optarg == endptr) {
                LOG_ERR("strtol: invalid value (%s) for argument %c\n", optarg,
                        op);
                exit(-1);
            }
            if (ctx.pci_hw_regs_nb < 1 ||
                ctx.pci_hw_regs_nb > PCIEMU_HW_BAR0_REG_CNT) {
                LOG_ERR("number of registers (%d) out of range ([1 , %d])\n",
                        ctx.pci_hw_regs_nb, PCIEMU_HW_BAR0_REG_CNT);
                exit(-1);
            }
            ctx.pci_hw_bar_len = ctx.pci_hw_regs_nb * sizeof(uint64_t);
            break;
        case 's':
            ctx.pci_device_nb = strtol(optarg, &endptr, 10);
            if (errno != 0 || optarg == endptr) {
                LOG_ERR("strtol: invalid value (%s) for argument %c\n", optarg,
                        op);
                exit(-1);
            }
            if (ctx.pci_device_nb >= PCI_DEVICE_NUMBER_INVALID) {
                LOG_ERR("PCI device number (%u) out of range ([0, %u])\n",
                        ctx.pci_device_nb, PCI_DEVICE_NUMBER_INVALID - 1);
                exit(-1);
            }
            break;
        case 'v':
            ctx.verbosity = 1;
            break;
        default:
            usage(stderr, argv);
            exit(-1);
        }
    }

    if (ctx.pci_device_nb >= PCI_DEVICE_NUMBER_INVALID) {
        LOG_ERR("PCI device (slot) number must be provided. Consider using "
                "lspci.\n");
        exit(-1);
    }

    if (ctx.pci_bus_nb >= PCI_BUS_NUMBER_INVALID) {
        LOG_ERR("PCI bus number must be provided. Consider using lspci.\n");
        exit(-1);
    }

    if (ctx.pci_domain_nb >= PCI_DOMAIN_NUMBER_INVALID) {
        LOG_ERR("PCI domain number must be provided. Consider using lspci.\n");
        exit(-1);
    }

    if (ctx.pci_func_nb >= PCI_FUNCTION_NUMBER_INVALID) {
        LOG_ERR(
            "PCI function number must be provided. Consider using lspci.\n");
        exit(-1);
    }


    return ctx;
}

int main(int argc, char **argv)
{
    struct context ctx = parse_args(argc, argv);

    if (open_pciemu_dev(&ctx) == -1)
        return -1;

    if (mmap_pciemu_bar(&ctx)) {
        close(ctx.fd);
        return -1;
    }

    print_registers(&ctx, "previous ");
    write_registers_sequential(&ctx);
    print_registers(&ctx, "current ");

    if (ioctl_pciemu(&ctx)) {
        close(ctx.fd);
        return -1;
    }

    close(ctx.fd);
    return 0;
}
