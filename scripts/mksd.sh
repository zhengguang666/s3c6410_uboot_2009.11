#!/bin/sh
# size: 256KB(BL2) + 8KB(ENV) + 8KB(BL1) = 280KB
# size: 300KB(BL2) + 16KB(ENV) + 8KB(BL1) = 280KB
#dd if=/dev/zero of=/dev/sdb bs=512 seek=7742890 count=596
cp ../../build_2009/u-boot.bin ./
sync
cat u-boot.bin > u-boot_mmc.bin
dd if=u-boot.bin of=u-boot_mmc.bin bs=8k seek=34 count=1
dd if=u-boot_mmc.bin of=$1 bs=512 seek=7742926
sync
rm u-boot_mmc.bin u-boot.bin
sync
