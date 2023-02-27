#!/bin/bash
# postcreate.sh : Script that run post the creation of the codespace
#
# Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
#
# SPDX-License-Identifier: GPL-2.0
#

# Repository information
REPOSITORY_DIR=$(git rev-parse --show-toplevel)
REPOSITORY_NAME=$(basename $REPOSITORY_DIR)

# Install QEMU dependencies
sudo apt update
sudo apt install -y git libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev \
	            ninja-build netcat libssh-dev libvde-dev libvdeplug-dev \
		    libcap-ng-dev libattr1-dev libslirp-dev

# Initialize submodules
git submodule update --init

# Link QEMU configurations to codespace home
ln -s $PWD/.devcontainer/.qemu_config $HOME/.qemu_config

# Ensure permissions are correct for kvm
sudo chmod 666 /dev/kvm

# Append .qemu_config to bashrc file
echo "source ~/.qemu_config" >> $HOME/.bashrc

# Execute image-related scripts
.devcontainer/images/imagesetup.sh
