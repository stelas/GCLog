#!/bin/bash

# Setup buildroot
# ---------------
# 1. Download OpenWrt SDK, ex. Barrier Breaker 14.07 for ar71xx architecture:
#    http://downloads.openwrt.org/barrier_breaker/14.07/ar71xx/generic/OpenWrt-SDK-ar71xx-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2
# 2. Extract archive to ~/OpenWrt
# 3. Modify toolchain path if needed
# 4. Execute "source buildenv.sh"

export STAGING_DIR="~/OpenWrt/staging_dir"
export CC="${STAGING_DIR}/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-gcc"

echo 'Build environment set, now run "make".'
