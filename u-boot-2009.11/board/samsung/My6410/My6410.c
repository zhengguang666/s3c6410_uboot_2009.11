/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2008
 * Guennadi Liakhovetki, DENX Software Engineering, <lg@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <netdev.h>
#include <s3c6400.h>

/* ------------------------------------------------------------------------- */
#define DM9000_Tacs	0x1	/* 1clk		address set-up		*/
#define DM9000_Tcos	0x2	/* 2clk		chip selection set-up	*/
#define DM9000_Tacc	0x4	/* 5clk	    access cycle		*/
#define DM9000_Tcoh	0x1	/* 1clk		chip selection hold	*/
#define DM9000_Tah	0x2	/* 2clk		address holding time	*/
#define DM9000_Tacp	0x2	/* 2clk		page mode access cycle	*/
#define DM9000_PMC	0x0	/* normal(1data)page mode configuration	*/

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b"
			  : "=r" (loops) : "0" (loops));
}

/*
 * Miscellaneous platform dependent initialisations
 */

static void dm9000_pre_init(void)
{
    ulong val;
    val = GPNCON_REG;
    val &= ~(3<<14);
    val |= 2<<14;
    GPNCON_REG = val;

	SROM_BW_REG &= ~(0xf << 4);
	SROM_BW_REG |= (1 << 7) | (1 << 6) | (1 << 4);
	SROM_BC1_REG = ((DM9000_Tacs << 28) + (DM9000_Tcos << 24) +
			(DM9000_Tacc << 16) + (DM9000_Tcoh << 12) +
			(DM9000_Tah << 8) + (DM9000_Tacp << 4) + DM9000_PMC);
}

int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	dm9000_pre_init();

	gd->bd->bi_arch_number = MACH_TYPE;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	printf("Board:   MY6410\n");
	return 0;
}
#endif

#ifdef CONFIG_ENABLE_MMU
ulong virt_to_phy_smdk6400(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xc8000000))
		return addr - 0xc0000000 + 0x50000000;
	else
		printf("do not support this address : %08lx\n", addr);

	return addr;
}
#endif

#ifndef CONFIG_SYS_NO_FLASH
ulong board_flash_get_legacy (ulong base, int banknum, flash_info_t *info)
{
	if (banknum == 0) {	/* non-CFI boot flash */
		info->portwidth = FLASH_CFI_16BIT;
		info->chipwidth = FLASH_CFI_BY16;
		info->interface = FLASH_CFI_X16;
		return 1;
	} else
		return 0;
}
#endif

#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_DRIVER_DM9000
	rc = dm9000_initialize(bis);
#endif
	return rc;
}
#endif
