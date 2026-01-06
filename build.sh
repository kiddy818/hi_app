#!/usr/bin/bash

# source  /home/sshw/.local/00_help_script/armv8_gnu_env.sh
# CROSS=/mnt/datafs/ub24-fs/cross-compile/aarch64-v01c01-linux-gnu-gcc/bin/aarch64-v01c01-linux-gnu- 

SENSOR0_TYPE=OV_OS08A20_MIPI_8M_30FPS_12BIT make -f Makefile_SDK2

# fit to imx415? sc450?
# SENSOR0_TYPE=OV_OS08A20_MIPI_8M_30FPS_12BIT CROSS=/mnt/datafs/ub24-fs/cross-compile/aarch64-v01c01-linux-gnu-gcc/bin/aarch64-v01c01-linux-gnu- make
