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
 *    .        Empty
 *    -        Partially filled
 *    =        Full, no summary node
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

static unsigned long start_addr;	/* start address */

int badblock = 0;
int emptyblock = 0;
int partialblock = 0;
int fullblock = 0;
int summaryblock = 0;

static void print_block(unsigned long block_num,
			int bad, int sum, int erase_block_size, int good_data)
{
	if (bad) {
		printf("B");
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
				printf("=");
				fullblock++;
			}
		}
	}
	if (block_num % 80 == 79)
		printf("\n");
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
	int justbb = 0;
	int totalblocks = 0;
	float totalsize = 0; 

	if (argc < 2) {
		printf("Usage: mtd_check [-ib] /dev/mtdX\n");
		printf(" where X is the flash device partition number\n");
		printf("  options:\n");
		printf("     -i output information on the partition and exit\n");
		printf("     -b just output number of bad blocks on the partition and exit\n");
		exit(0);
		}

	for (i = 1; i < argc; i++)
    	{
        	if (argv[i][0] == '-')
        	{
             		if (argv[i][1] == 'i')
                 		justinfo = 1;
             		else if (argv[i][1] == 'b')
				justbb = 1;
			else
             			{
                 		printf("Invalid option %c.\n",argv[i][1]);
				printf("Use -i for nand info only\n");
				printf("Use -b for number of bad blocks only\n");
                 		return 2;
             			}
		}
		else
			/* Open MTD device */
			if ((fd = open(argv[i], O_RDONLY)) == -1) {
				perror("Can't open device");
				exit(1);
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


	printf("Flash type of %s is %d (", argv[i-1], meminfo.type);
		switch (meminfo.type) {
			case 0:
				printf("Absent!!!)\n");
				exit(2);
			case 1:
				printf("MTD_RAM)\n");
				break;
			case 2:
				printf("MTD_ROM)\n");
				break;
			case 3:
				printf("MTD_NORFLASH)\n");
				break;
			case 4:
				printf("MTD_NANDFLASH)\n");
				break;
			case 6:
				printf("MTD_DATAFLASH)\n");
				break;
			case 7:
				printf("MTD_UBIVOLIME)\n");
				break;
			case 8:
				printf("MTD_MLCNANDFLASH)\n");
				break;
		}

				
			
	printf("Flash flags are 0x%x -", meminfo.flags);
	if (meminfo.flags&0x400)
		printf(" MTD_WRITEABLE");
	if (meminfo.flags&0x800)
		printf(" MTD_BIT_WRITEABLE");
	if (meminfo.flags&0x1000)
		printf(" MTD_NO_ERASE");
	if (meminfo.flags&0x2000)
		printf(" MTD_POWERUP_LOCK");
	printf("\n");
		

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 128) &&
	    !(meminfo.oobsize == 64) &&
	    !(meminfo.oobsize == 32) &&
	    !(meminfo.oobsize == 16) && !(meminfo.oobsize == 8)) {
		fprintf(stderr, "Unknown type of flash (not normal NAND)\n");
		close(fd);
		exit(1);
	}


	/* Read the real oob length */
	end_addr = meminfo.size;
	bs = meminfo.writesize;

	printf("Block size %u, page size %u, OOB size %u\n", meminfo.erasesize,
	       bs, meminfo.oobsize);
	printf("%lu bytes, %lu blocks\n", end_addr,
	       end_addr / meminfo.erasesize);
	printf("ECC Stats -  Corrected: %d   Failed: %d  Bad Blocks: %d \n\n", eccinfo.corrected, eccinfo.failed,eccinfo.badblocks);

	if (justinfo == 1){
		close(fd);
		exit(0);
	}
	printf
	    ("B Bad block; . Empty; - Partially filled; = Full; S has a JFFS2 summary node\n\n");

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
			    meminfo.erasesize, j + 1);
	}
	free(block_buf);
	printf("\n\n");
	totalblocks = emptyblock + partialblock + fullblock + badblock + summaryblock;
	totalsize = (float) ((totalblocks*bs)/1024); 
	printf("Summary %s:\n",argv[i-1]);
	printf("Total Blocks: %d  Total Size: %.1f KB\n", totalblocks, totalsize);
	printf("Empty Blocks: %d, Full Blocks: %d, Partially Full: %d, Bad Blocks: %d\n", emptyblock,fullblock,partialblock,badblock);
	return 0;
}

