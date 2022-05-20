# mtd_check

## About
mtd_check is a small utility to display information on the flash mtd devices on armv7l and aarch64 based Asuswrt routers.
If you are not sure what your version is, you can use this command:
```
$ uname -m
```


## Installation

Using your preferred SSH client/terminal, copy and paste the following command, then press Enter:

/usr/sbin/curl --retry 3 "https://raw.githubusercontent.com/JGrana01/mtd_check/master/mtd_check_install" -o "/jffs/scripts/mtd_check_install" && chmod 0755 /jffs/scripts/mtd_check_install && /jffs/scripts/mtd_check_install

mtd_check_install will make sure you have Entware installed and that your kernel is an armv7l or aarch64 version. If not, it will not install the appropriate binary (in /opt/bin) and exit.
The mtd_check_install scripts will stay in /jffs/scripts (it's small) and can be used to re-install/update the mtd_check binary.
At some point I will likely update the script to check for updates. For now, it's very simple.

## Usage

Mtd_check runs from the command line.

$ mtd_check /dev/mtd[0-X] [-i] [-b]

The -i option just displays the Flash type, Block size, page size and OOB size along with the total number of bytes and blocks on the mtd partition.
The -b option only reports the number of bad blocks on the partition. This can be useful for sh/bash scripts to monitor mtd partitions for potential growing bad blocks.

**Note: mtd_check only works with the mtd character devices (i.e. /dev/mtd0, /dev/mtd9, etc.) _not_ the block devices (/dev/mtdblock0, /dev/mtdblock9, etc.). It also will not report any information for ubi formatted mtd partitions.**

One way  to see the available mtd partitions is to cat /proc/mtd:
```
$ cat /proc/mtd
dev:    size   erasesize  name
mtd0: 051c0000 00020000 "rootfs"
mtd1: 051c0000 00020000 "rootfs_update"
mtd2: 00800000 00020000 "data"
mtd3: 00100000 00020000 "nvram"
mtd4: 05700000 00020000 "image_update"
mtd5: 05700000 00020000 "image"
mtd6: 00520000 00020000 "bootfs"
mtd7: 00520000 00020000 "bootfs_update"
mtd8: 00100000 00020000 "misc3"
mtd9: 03f00000 00020000 "misc2"
mtd10: 00800000 00020000 "misc1"
mtd11: 04d23000 0001f000 "rootfs_ubifs"
```
Note that mtd11 (on an AX88U) is a ubi formatted partition and is not supported

Without the -i or -b options, mtd_check will walk all the blocks showing their state:

- **B**&nbsp; &nbsp; &nbsp;Bad block
- **\.**&nbsp; &nbsp; &nbsp;Empty
- **\-**&nbsp; &nbsp; &nbsp;Partially filled
- **\=**&nbsp; &nbsp; &nbsp;Full
- **s**&nbsp; &nbsp; &nbsp;partial with summry node
- **S**&nbsp; &nbsp; &nbsp;has a JFFS2 summary node

Something like this:

```
$ mtd_check /dev/mtd0
Flash type of /dev/mtd0 is 4 (MTD_NANDFLASH)
Flash flags are 400
Block size 131072, page size 2048, OOB size 64
99614720 bytes, 760 blocks
B Bad block; . Empty; - Partially filled; = Full; S has a JFFS2 summary node
-----------===========================------------==============================
=======B========================================================================
================================================================================
================================================================================
=========================================================================B======
================================================================================
================================================================================
=============================================================B==================
=========================================-===========---------------------------
----------------------------------------
Summary blocks: 0
Summary /dev/mtd0:
Total Blocks: 760  Total Size: 1520.0 KB
Empty Blocks: 0, Full Blocks: 666, Partially Full: 91, Bad Blocks: 3
```
## Uninstall

To remove mtd_check, remove the installer and binary:
```
$ rm /jffs/scripts/mtd_check_install
$ rm /opt/bin/mtd_check
```


