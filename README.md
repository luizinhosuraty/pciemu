<div align="center">

<h1>pciemu</h1>

![License](https://img.shields.io/github/license/luizinhosuraty/pciemu)
![Build](https://img.shields.io/github/actions/workflow/status/luizinhosuraty/pciemu/linux-ci.yml?label=tests)
 
</div>

pciemu provides an example of PCIe Device Emulation in QEMU.

The idea is to help those willing to explore PCIe devices but do not have access
to a real hardware. Or maybe someone with research ideas for a new PCIe device
or capability who want to easily test those ideas.

Virtual devices can also be used to speed up the development process of a new
PCIe device and create test suites that do not require the existence of the
physical HW.

Please note that pciemu implements a relatively simple device, with the goal
mainly being to provide an initial tutorial on how to write a virtual PCIe device
in QEMU.

## Preparing the environment

There are mainly two ways of building and running : locally or using codespaces.

### Codespaces

Users willing to use codespaces will benefit from scripts properly preparing and
configuring codespaces to be used "out-of-the-box".

Basically, during the instantiation of the codespace, the
["post create" script](.devcontainer/postcreate.sh) will install all
necessary dependencies, ensure configurations are correct and execute the 
[scripts to intialize](.devcontainer/images/imagesetup.sh) the qcow2 disk images.

### Locally

Another option is to build and run locally. 
In this case, configuring the environment can vary depending on the system being
used. Users are encouraged to check the 
["post create" script](.devcontainer/postcreate.sh) to understand all
necessary dependencies and configurations for building and running.

## Setup and Compiling QEMU

The [setup script](setup.sh) will modify build files and configure QEMU to
properly compile the pciemu device along with all other QEMU files. Thus,
once all preparation is finished, run the following:

```bash
$ ./setup.sh
```

Note here for those running locally: though normally not necessary, you may
need to change the arguments of the ```./configure``` command inside the
[setup script](setup.sh) to better translate them to your system requirements.

## Running

There is basically no distinction between running locally or inside a codespace.

Once all dependencies are installed and QEMU is properly compiled, all you have
to do is run QEMU:

```bash
$ qemu_fedora
```

This alias will launch a Virtual Machine (VM) with QEMU in the background with
the correct set of arguments. (see [.qemu_config](.devcontainer/.qemu_config))

Finally, to login into the VM, use the alias:

```bash
$ fedora
```

Check the [image information file](.devcontainer/images/info.txt) for more
details regarding the image files.

### Inside the VM

In order to make the qcow2 files relatively small, the VM images do not come
with several packages installed. A few are very important for the purpose of
compiling the kernel module and userspace program.
Thus, inside the VM, the first order of business is to run the following:

``` bash
sudo dnf install -y make gcc "kernel-devel-uname-r == $(uname -r)"
```

Later, it's all standard kernel module compilation and insertion into the kernel:

```bash
$ cd /hostdir/src/sw/kernel/
$ make
$ sudo insmod pciemu.ko
```

Finally, the userspace program:

```bash
$ cd /hostdir/src/sw/userspace/
$ make
$ ./pciemu_example -h
```

Don't forget that ```lspci``` is your friend when it comes to PCIe devices.

