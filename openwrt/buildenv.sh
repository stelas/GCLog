#!/bin/bash

# Setup buildroot
# ---------------
# 1. Download OpenWrt SDK, ex. version 18.06.2 for ar71xx architecture:
#    https://downloads.openwrt.org/releases/18.06.2/targets/ar71xx/generic/openwrt-sdk-18.06.2-ar71xx-generic_gcc-7.3.0_musl.Linux-x86_64.tar.xz
# 2. Execute "source buildenv.sh"

mkdir openwrt && tar -xaf openwrt-sdk*.tar.xz --strip-components 1 -C openwrt/

export STAGING_DIR=$(realpath ./openwrt/staging_dir)
export CC=`find "$STAGING_DIR"/toolchain*/bin/*openwrt-linux-gcc -print`

echo 'Build environment set, now run "make".'
