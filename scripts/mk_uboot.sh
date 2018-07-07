#!/bin/sh
rm ../../build_2009/* -r
sync
cd ${SRC_UBOOT}
make My6410_config
make

