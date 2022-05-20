# mtd_check
Program to display information about the NAND  flash on Asuswrt routers
To Install:
/usr/sbin/curl --retry 3 "https://raw.githubusercontent.com/JGrana01/mtd_check/master/mtd_check_install" -o "/jffs/scripts/mtd_check_install" && chmod 0755 /jffs/scripts/mtd_check_install && /jffs/scripts/mtd_check_install

Usage:
$ mtd_check /dev/mtd[0-9] [-i] [-b]

The -i option just displays the Flash type, Block size, page size and OOB size along with the total number of blocks and bytes on the mtd partition.
The -b option only reports the number of bad blocks on the partition

Without the -i or -b options, nand_flash will walk all the blocks showing their state:

B = bad block
. = Empty
- = Partially filled
= = Full
s = partial with summry node
S = has a JFFS2 summary node

Something like this:
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
