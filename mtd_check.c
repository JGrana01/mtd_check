/*
 *  mtd_check.c
 *
 *  Copyright (C) 2009, 2017 Chris Simmonds (chris@2net.co.uk)
 *  jgrana@rochester.rr.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * It is a verion of Chris Simmonds nand_check program tailored to Asuswrt
 * based routers
 *
 * Read a flash partition and report on block usage: for each eraseblock print
 *    B        Bad block
 *    R        Reserved Bad Block Table
 *    .        Empty
 *    -        Partially filled
 *    *        Full, no summary node
 *    S        Full, summary node
 *
 * This program is based on jffs2dump by Thomas Gleixner
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <asm/types.h>
#include <mtd/mtd-user.h>
#include <mtd/mtd-abi.h>

typedef struct mtd_ecc_stats mtd_ecc_stats_t;

/* taken from linux/jffs2.h */
#define JFFS2_SUM_MAGIC	0x02851885
#define VERSION 0.4

static unsigned long start_addr;	/* start address */

int badblock = 0;
int emptyblock = 0;
int partialblock = 0;
int fullblock = 0;
int summaryblock = 0;

void printsize (int x)
{
	int i;
	static const char *flags = "KMGT";
	printf ("%u ",x);
	for (i = 0; x >= 1024 && flags[i] != '\0'; i++) x /= 1024;
	i--;
	if (i >= 0) printf ("(%u%c)",x,flags[i]);
}

static void print_block(unsigned long block_num,
			int bad, int sum, int erase_block_size, int good_data, int bbcount)
{
	if (bad) {
		if (badblock < bbcount)
			printf("B");
		else printf("R");
		badblock++;
	}
	else {
		if (good_data == 0) {
			printf(".");
			emptyblock++;
	} else if (good_data < (erase_block_size / 2)) {
			if (sum)
				printf("s");
			else {
				printf("-");
				partialblock++;
			}
		} else {
			if (sum) {
				printf("S");
				summaryblock++;
			}
			else {
				printf("*");
				fullblock++;
			}
		}
	}
	if (block_num % 80 == 79)
		printf("\n");
}

static int getregions (int fd,struct region_info_user *regions,int *n)
{
	int i,err;
	err = ioctl (fd,MEMGETREGIONCOUNT,n);
	if (err) return (err);
	if (*n > 0) {
		for (i = 0; i < *n; i++)
		{
			regions[i].regionindex = i;
			err = ioctl (fd,MEMGETREGIONINFO,&regions[i]);
			if (err) return (err);
		}
	}
	else printf("No Region Information on device\n");
	return (0);
}

int printregions(int fd)
{
	int err,i,n;
	static struct region_info_user region[1024];

	err = getregions (fd,region,&n);
	if (err < 0)
	{
		perror ("MEMGETREGIONCOUNT");
		return (1);
	}
	if (n > 0){
	printf ("\n"
			"regions = %d\n"
			"\n",
			n);

		for (i = 0; i < n; i++) {
			printf ("region[%d].offset = 0x%.8x\n"
					"region[%d].erasesize = ",
					i,region[i].offset,i);
			printsize (region[i].erasesize);
			printf ("\nregion[%d].numblocks = %d\n"
					"region[%d].regionindex = %d\n",
					i,region[i].numblocks,
					i,region[i].regionindex);
		}
	}
	return(0);
}

int main(int argc, char **argv)
{
	unsigned long ofs, end_addr = 0;
	int ret, fd, bs;
	mtd_info_t meminfo;
	mtd_ecc_stats_t eccinfo;
	loff_t offset;
	int i,j;
	unsigned char *block_buf;
	int bad_block;
	int summary_info;
	int justinfo = 0;
	int dostrict = 0;
	int justbb = 0;
	int justecc = 0;
	int formtdmon=0;
	int showregions = 0;
	int showall = 0;
	int mtdev = 0;

	if (argc < 2) {
		printf("Usage: mtd_check [-ibervV] /dev/mtdX\n");
		printf(" where X is the flash device number\n");
		printf("  options:\n");
		printf("     -i output information on the device and exit\n");
		printf("     -b just output number of bad blocks on the device and exit\n");
		printf("     -e output ECC information and number of bad blocks on the partition and exit\n");
		printf("     -r display nand regions\n");
		printf("     -s strict mode - omit legacy nand devices without reported OOB blocksize\n");
		printf("     -v verbose - show info, blocks and regions\n");
		printf("     -V show mtd_check version\n");
		exit(0);
		}

	for (i = 1; i < argc; i++)
    	{
        	if (argv[i][0] == '-')
        	{
			switch(argv[i][1])
			{
				case 'b':
					justbb=1;
					break;
				case 'e':
					justecc=1;
					break;
				case 'i':
					justinfo=1;
					break;
				case 'z':
					formtdmon=1;
					break;
				case 'r':
					showregions=1;
					break;
				case 's':
					dostrict=1;
					break;
				case 'v':
					showall=1;
					break;
				case 'V':
					printf("Version: %.1f\n",VERSION);
					exit(0);
				default:
                 			printf("Invalid option %c.\n",argv[i][1]);
					printf("  -i for nand info only\n");
					printf("  -b for number of bad blocks only\n");
					printf("  -r show regions (if any)\n");
					printf("  -s run strict mode (don't allow older nands with 0 oobsize)\n");
					printf("  -e for ECC information and number of bad blocks only\n");
					printf("  -V for mtd_check version\n");
                 			exit(1);
             		}
		}
		else {
			/* Open MTD device */
			if ((fd = open(argv[i], O_RDONLY)) == -1) {
				perror("Can't open device");
				exit(1);
			}
			mtdev=i;
		}
        }

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit(1);
	}
	if (ioctl(fd, ECCGETSTATS, &eccinfo) != 0) {
		perror("ECCGETSTATS");
		close(fd);
		exit(1);
	}

	if (justbb == 1) {
		printf("%d\n",eccinfo.badblocks);
		close(fd);
		exit(0);
	}

	if (justecc == 1) {
		printf("ECC Stats -  Corrected: %d   Failed: %d \n\n", eccinfo.corrected,eccinfo.failed);
		close(fd);
		exit(0);
	}

/* hidden command for mtdmon - much easier to extract data */

	if (formtdmon == 1) {
		printf("%d %d %d \n", eccinfo.badblocks, eccinfo.corrected, eccinfo.failed);
		close(fd);
		exit(0);
	}

	if (showregions == 1){
		printregions(fd);
		exit(0);
	}

	printf("Flash type of %s is %d ", argv[mtdev], meminfo.type);
		switch (meminfo.type) {
			case 0:
				printf("Absent!!!)\n");
				exit(2);
			case 1:
				printf("(RAM)\n");
				break;
			case 2:
				printf("(ROM)\n");
				break;
			case 3:
				printf("(NORFLASH)\n");
				break;
			case 4:
				printf("(NANDFLASH)\n");
				break;
			case 6:
				printf("(DATAFLASH)\n");
				break;
			case 7:
				printf("(UBIVOLIME)\n");
				break;
			case 8:
				printf("(MLCNANDFLASH)\n");
				break;
		}

				
			
	printf("Flash flags are 0x%x -", meminfo.flags);
	if (meminfo.flags&0x400)
		printf(" WRITEABLE");
	if (meminfo.flags&0x800)
		printf(" BIT_WRITEABLE");
	if (meminfo.flags&0x1000)
		printf(" NO_ERASE");
	if (meminfo.flags&0x2000)
		printf(" POWERUP_LOCK");
	printf("\n");

/* can't go much further with ubi volumes, so bail */

	if (meminfo.type == 7) {	
		printf("MTD is UBI Volume\n");
		exit(1);
	}

	/* Make sure device page sizes are valid if running strict mode */
	if (!(meminfo.oobsize == 128) &&
	    !(meminfo.oobsize == 64) &&
	    !(meminfo.oobsize == 32) &&
	    !(meminfo.oobsize == 16) && !(meminfo.oobsize == 8) && (dostrict == 1)) {
		fprintf(stderr, "Unknown type of flash (not normal NAND - oobsize: %d)\n", meminfo.oobsize);
		close(fd);
		exit(1);
	}


	/* Read the real oob length */
	end_addr = meminfo.size;
	bs = meminfo.writesize;

	printf("Block size ");
	printsize(meminfo.erasesize);

/* for potential old NAND devices, if no valid erasesize then bail */

	if (meminfo.erasesize == 0) {
		fprintf(stderr, "Unknown type of flash (not normal NAND - erasesize = 0)\n");
		close(fd);
		exit(1);
	}
	printf("  Page size ");
	printsize(bs);
	printf("  OOB size ");
	printsize(meminfo.oobsize);
	printf("\n");
	printf("Device: ");
	printsize(end_addr);
	printf(" bytes, ");
	printsize(end_addr / meminfo.erasesize);
	printf(" blocks\n");

	if (meminfo.oobsize == 0) {
		printf("Device does not support Bad Block management and/or ECC\n");
		close(fd);
		exit(1);
	}
	printf("ECC Stats -  Corrected: %d   Failed: %d  \n\n", eccinfo.corrected, eccinfo.failed);

	if (justinfo == 1){
		close(fd);
		exit(0);
	}
	if (showall == 1)
		printregions(fd);


	printf
	    ("B Bad block; . Empty; - Partially filled; * Full; R Reserved (BBT); S has a JFFS2 summary node\n\n");

	block_buf = malloc(meminfo.erasesize);
	for (ofs = start_addr; ofs < end_addr; ofs += meminfo.erasesize) {
		/* Read the next erase block */
		if (read(fd, block_buf, meminfo.erasesize) != meminfo.erasesize) {
			perror("read block");
			printf(" ofs=%lu block %lu errno %d\n", ofs,
			       ofs / meminfo.erasesize, errno);
		}

		offset = ofs;
		ret = ioctl(fd, MEMGETBADBLOCK, &offset);
		if (ret > 0) {
			bad_block = 1;
		} else if (ret < 0) {
			perror("MEMGETBADBLOCK");
			printf("ofs=%lx\n", ofs);
			return 1;
		} else {
			bad_block = 0;
			/* See how much of the block contains "data", by
			   scanning backwards to find the first non 0xff byte */
			for (j = (meminfo.erasesize - 1); j >= 0; j--) {
				if (block_buf[j] != 0xff)
					break;
			}
			/* See if there is a summary node at the end of the
			   block. Here, we just check for the summary marker
			   in the last 4 bytes, we don't check that the block
			   it points to is  valid */
			if (*(unsigned long *)
			    (block_buf + meminfo.erasesize -
			     sizeof(unsigned long)) == JFFS2_SUM_MAGIC)
				summary_info = 1;
			else
				summary_info = 0;
		}
		print_block(ofs / meminfo.erasesize, bad_block, summary_info,
			    meminfo.erasesize, j + 1, eccinfo.badblocks);
	}
	free(block_buf);
	printf("\n");
	printf("Summary %s:\n",argv[mtdev]);
	printf("Empty Blocks: %d, Full Blocks: %d, Partially Full: %d, Bad Blocks: %d, Reserved Blocks (BBT) %d\n\n", emptyblock,fullblock,partialblock,eccinfo.badblocks,eccinfo.bbtblocks);
	return 0;
}
