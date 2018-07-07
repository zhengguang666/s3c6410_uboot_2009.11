/*
 *  board/samsung/My6410/nand_cp.c
 *  Copyright (C) 2009-2012 Richard Fan
 *  copy u-boot from nand
 */
#include <common.h> 
#include <linux/mtd/nand.h> 
#include <s3c6410.h>
#include <asm/io.h>

#define NF_CMD(cmd)         writeb(cmd, NFCMMD)             /* 写命令 */
#define NF_ADDR(addr)       writeb(addr, NFADDR)            /* 写地址 */
#define NF_RDDATA8(nand)    readb(NFDATA)                   /* 读8位数据 */
#define NF_ENABLE_CE()      writel(readl(NFCONT) & ~(1<<1), NFCONT) /* 片选使能 */
#define NF_DISABLE_CE()     writel(readl(NFCONT) | (1<<1), NFCONT)  /* 片选禁用 */
#define NF_CLEAR_RB()       writel(readl(NFSTAT) | (1<<4), NFSTAT)  /* 清除就绪/忙位 */
#define NF_DETECT_RB()      do { \
                               while (!(readl(NFSTAT) & (1<<0))); \
                            } while (0)                      /* 等待操作完成 */

static void nandll_reset(void)
{
	NF_ENABLE_CE();             /* 片选使能 */
	NF_CLEAR_RB();              /* 清除就绪/忙位 */
	NF_CMD(NAND_CMD_RESET);     /* 写复位命令 */
	NF_DETECT_RB();             /* 等待复位完成 */
	NF_DISABLE_CE();            /* 片选禁用 */
}

static int nandll_read_page(uchar *buf, ulong addr, int large_block)
{
	int i;
	int page_size = 512;
	
    if (large_block == 1)
        page_size = 2048;
    if (large_block == 2)
        page_size = 4096;
    if(large_block == 3)
        page_size = 8192;
    
	NF_ENABLE_CE();
	NF_CLEAR_RB();
    
    NF_CMD(NAND_CMD_READ0); 
	
	NF_ADDR(0x0);
	if (large_block)
		NF_ADDR(0x0);
	NF_ADDR(addr & 0xff);
	NF_ADDR((addr >> 8) & 0xff);
	NF_ADDR((addr >> 16) & 0xff);
	
    if (large_block)
	    NF_CMD(NAND_CMD_READSTART); 
	
    NF_DETECT_RB();

	for (i = 0; i < page_size; i++) 
		buf[i] =  NF_RDDATA8();

	NF_DISABLE_CE();
	return 0;
}
         
static int nandll_read_blocks(ulong dst_addr, ulong size, int large_block)
{
	int i, page_nr;
	uchar *buf = (uchar *)dst_addr;
	uint page_shift = 9;

    switch (large_block) {
    case 1:
        page_shift = 11;
        break;
    case 2:
        page_shift = 12;
        break;
    case 3:
        page_shift = 13;
        break;
    }

    if (large_block > 1) {
        page_nr = 4 + ((size - 8192)>>page_shift);
        if (size % (1<<page_shift))
            page_nr += 1;

        for (i = 0; i < 4; i++, buf += 2048) 
            nandll_read_page(buf, i, large_block);

        for (i = 4; i < page_nr; i++, buf += (1<<page_shift)) 
            nandll_read_page(buf, i, large_block);
    } else {
        page_nr = size>>page_shift;
        if (size % (1<<page_shift))
            page_nr += 1;

        for(i = 0; i < page_nr; i++, buf += (1<<page_shift)) 
            nandll_read_page(buf, i, large_block);
    }
	return 0;
}

int copy_to_ram_from_nand(void)
{
	int large_block = 0;
	int i, size;
    uint *p_bss_start, *p_armboot_start;
	vu_char mak_id, dev_id, ceinfo;
	
    nandll_reset();

	NF_ENABLE_CE();
	NF_CMD(NAND_CMD_READID); 
	NF_ADDR(0x0);
	
	for (i = 0; i < 200; i++);  /* 短暂延时 */
	
	mak_id = NF_RDDATA8(nand);  /* Maker Code */
	dev_id = NF_RDDATA8(nand);  /* Device Code */
	ceinfo = NF_RDDATA8(nand);  /* Internel Chip Num, Cell Type */

    switch (mak_id) {
    case NAND_MFR_SAMSUNG:
        if (dev_id > 0x80) {   
            large_block = 1;

            switch (dev_id) {
            case 0xd5:      /* K9GAG08U0x */ 
                switch (ceinfo) {
                case 0x94:      /* K9GAG08U0D */ 
                case 0x14:      /* K9GAG08U0M */ 
                    large_block = 2;
                    break;
                case 0x84:      /* K9GAG08U0E */ 
                    large_block = 3;
                    break;
                }
                break;
            case 0xd7:      /* K9LBG08U0D */
                large_block = 2;
                    break;
            }
        }
        break;
    case NAND_MFR_MICRON:
        switch (dev_id) {
        case 0x38:      /* MT29F8G08ABACAWP */ 
        case 0x48:      /* MT29F16G08ABACAWP */ 
            large_block = 2; 
            break;
        }
        break;
    } 

    p_bss_start = (uint*)((uint)&_bss_start - CONFIG_SYS_PHY_UBOOT_BASE);
    p_armboot_start = (uint*)((uint)&_armboot_start - CONFIG_SYS_PHY_UBOOT_BASE);
    size = *p_bss_start - *p_armboot_start; 

	return nandll_read_blocks(CONFIG_SYS_PHY_UBOOT_BASE, size, large_block);
}

