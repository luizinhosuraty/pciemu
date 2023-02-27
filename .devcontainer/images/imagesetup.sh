#!/bin/bash
# imagesetup.sh : Setup the pre-installed images
#
# Copyright (c) 2023 Luiz Henrique Suraty Filho <luiz-dev@suraty.com>
#
# SPDX-License-Identifier: GPL-2.0
#

# Repository information
REPOSITORY_DIR=$(git rev-parse --show-toplevel)
REPOSITORY_NAME=$(basename $REPOSITORY_DIR)

imgdir=$REPOSITORY_DIR/.devcontainer/images

imgbackup=https://download.fedoraproject.org/pub/fedora/linux/releases/37/Server/x86_64/images/Fedora-Server-KVM-37-1.7.x86_64.qcow2

# specific for a given fedora qcow2 file
wget -P $imgdir $imgbackup
cat $imgdir/fedora.tar.gz* | tar xzvf - -C $imgdir/
