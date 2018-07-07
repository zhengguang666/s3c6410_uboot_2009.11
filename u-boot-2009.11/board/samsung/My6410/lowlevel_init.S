/*
 * Memory Setup stuff - taken from blob memsetup.S
 *
 * Copyright (C) 1999 2000 2001 Erik Mouw (J.A.K.Mouw@its.tudelft.nl) and
 *		       Jan-Derk Bakker (J.D.Bakker@its.tudelft.nl)
 *
 * Modified for the Samsung SMDK2410 by
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


#include <config.h>
#include <version.h>

#include <s3c6410.h>

_TEXT_BASE:
	.word	TEXT_BASE

	.globl lowlevel_init
lowlevel_init:
	mov	r12, lr

	ldr	r0, =ELFIN_GPIO_BASE
	ldr     r1, =0x04000000
	str     r1, [r0, #GPFCON_OFFSET]
    ldr     r1, =0x2000
	str     r1, [r0, #GPFDAT_OFFSET]
	
	/* LED on, GPM0~3: LED1~4 */
	ldr	r0, =ELFIN_GPIO_BASE  
	ldr	r1, =0x00001111
	str	r1, [r0, #GPMCON_OFFSET]

	ldr	r1, =0x00000055
	str	r1, [r0, #GPMPUD_OFFSET]

	ldr	r1, =0x000a
	str	r1, [r0, #GPMDAT_OFFSET]

	/* Disable Watchdog */
	ldr	r0, =0x7e000000		@0x7e004000
	orr	r0, r0, #0x4000
	mov	r1, #0
	str	r1, [r0]

	/* External interrupt pending clear */
	ldr	r0, =(ELFIN_GPIO_BASE+EINTPEND_OFFSET)	/*EINTPEND*/
	ldr	r1, [r0]
	str	r1, [r0]

	ldr	r0, =ELFIN_VIC0_BASE_ADDR	@0x71200000
	ldr	r1, =ELFIN_VIC1_BASE_ADDR	@0x71300000

	/* Disable all interrupts (VIC0 and VIC1) */
	mvn	r3, #0x0
	str	r3, [r0, #oINTMSK]
	str	r3, [r1, #oINTMSK]

	/* Set all interrupts as IRQ */
	mov	r3, #0x0
	str	r3, [r0, #oINTMOD]
	str	r3, [r1, #oINTMOD]

	/* Pending Interrupt Clear */
	mov	r3, #0x0
	str	r3, [r0, #oVECTADDR]
	str	r3, [r1, #oVECTADDR]

	/* init system clock */
	bl system_clock_init

	/* for UART */
	bl uart_asm_init

	/* simple init for NAND */
	bl nand_asm_init

	/* Memory subsystem address 0x7e00f120 */
	ldr	r0, =ELFIN_MEM_SYS_CFG

	/* Xm0CSn2 = NFCON CS0, Xm0CSn3 = SROMC CS3 */
	mov	r1, #S3C64XX_MEM_SYS_CFG_NAND  @0x0008
	str	r1, [r0]

	bl	mem_ctrl_asm_init

/* Wakeup support. Don't know if it's going to be used, untested. */
	ldr	r0, =(ELFIN_CLOCK_POWER_BASE + RST_STAT_OFFSET)
	ldr	r1, [r0]
	bic	r1, r1, #0xfffffff7
	cmp	r1, #0x8
	beq	wakeup_reset

	ldr	r0, =ELFIN_UART_BASE
	mov	r1, #'K'
	str	r1, [r0, #UTXH_OFFSET]

1:
	mov	lr, r12
	mov	pc, lr

wakeup_reset:

	/* Clear wakeup status register */
	ldr	r0, =(ELFIN_CLOCK_POWER_BASE + WAKEUP_STAT_OFFSET)
	ldr	r1, [r0]
	str	r1, [r0]

	/* LED test */
	ldr	r0, =ELFIN_GPIO_BASE
	ldr	r1, =0x3000
	str	r1, [r0, #GPNDAT_OFFSET]

	/* Load return address and jump to kernel */
	ldr	r0, =(ELFIN_CLOCK_POWER_BASE + INF_REG0_OFFSET)
	/* r1 = physical address of s3c6400_cpu_resume function */
	ldr	r1, [r0]
	/* Jump to kernel (sleep-s3c6400.S) */
	mov	pc, r1
	nop
	nop

/*
 * system_clock_init: Initialize core clock and bus clock.
 * void system_clock_init(void)
 */
system_clock_init:
	ldr	r0, =ELFIN_CLOCK_POWER_BASE	/* 0x7e00f000 */

#ifdef CONFIG_SYNC_MODE
	ldr	r1, [r0, #OTHERS_OFFSET]
	mov	r2, #0x40
	orr	r1, r1, r2
	str	r1, [r0, #OTHERS_OFFSET]

	nop
	nop
	nop
	nop
	nop

	ldr	r2, =0x80
	orr	r1, r1, r2
	str	r1, [r0, #OTHERS_OFFSET]

check_syncack:
	ldr	r1, [r0, #OTHERS_OFFSET]
	ldr	r2, =0xf00
	and	r1, r1, r2
	cmp	r1, #0xf00
	bne	check_syncack
#else	/* ASYNC Mode */
	nop
	nop
	nop
	nop
	nop

	ldr	r1, [r0, #OTHERS_OFFSET]
	bic	r1, r1, #0xC0
	orr	r1, r1, #0x40
	str	r1, [r0, #OTHERS_OFFSET]

wait_for_async:
	ldr	r1, [r0, #OTHERS_OFFSET]
	and	r1, r1, #0xf00
	cmp	r1, #0x0
	bne	wait_for_async

	ldr	r1, [r0, #OTHERS_OFFSET]
	bic	r1, r1, #0x40
	str	r1, [r0, #OTHERS_OFFSET]
#endif

	mov	r1, #0xff00
	orr	r1, r1, #0xff
	str	r1, [r0, #APLL_LOCK_OFFSET]
	str	r1, [r0, #MPLL_LOCK_OFFSET]
	str	r1, [r0, #EPLL_LOCK_OFFSET]

#if defined(CONFIG_UARTCLK_DOUTMPLL)
	ldr r1, [r0, #CLK_DIV2_OFFSET]
	bic	r1, r1, #0xf0000
	orr	r1, r1, #0x30000
	str	r1, [r0, #CLK_DIV2_OFFSET]
#endif

	/* Set Clock Divider */
	ldr	r1, [r0, #CLK_DIV0_OFFSET]
	bic	r1, r1, #0x30000
	bic	r1, r1, #0xff00
	bic	r1, r1, #0xff
	ldr	r2, =CLK_DIV_VAL
	orr	r1, r1, r2
	str	r1, [r0, #CLK_DIV0_OFFSET]

	ldr	r1, =APLL_VAL
	str	r1, [r0, #APLL_CON_OFFSET]
	ldr	r1, =MPLL_VAL
	str	r1, [r0, #MPLL_CON_OFFSET]

	/* FOUT of EPLL is 24MHz */
	ldr	r1, =0x80200203
	str	r1, [r0, #EPLL_CON0_OFFSET]
	ldr	r1, =0x0
	str	r1, [r0, #EPLL_CON1_OFFSET]

	/* APLL, MPLL, EPLL select to Fout */
	ldr	r1, [r0, #CLK_SRC_OFFSET]
#if defined(CONFIG_UARTCLK_DOUTMPLL)
	ldr	r2, =0x2007
#else
	ldr	r2, =0x7
#endif
	orr	r1, r1, r2
	str	r1, [r0, #CLK_SRC_OFFSET]

	/* wait at least 200us to stablize all clock */
	mov	r1, #0x10000
1:	subs	r1, r1, #1
	bne	1b

	mov	pc, lr

/*
 * uart_asm_init: Initialize UART's pins
 */
uart_asm_init:
	/* set GPIO to enable UART */
	ldr	r0, =ELFIN_GPIO_BASE
	ldr	r1, =0x220022
	str	r1, [r0, #GPACON_OFFSET]
	ldr	r1, =0x2222
	str r1, [r0, #GPBCON_OFFSET]

	ldr	r0, =ELFIN_UART_CONSOLE_BASE		@0x7F005000
	mov	r1, #0x0
	str	r1, [r0, #UFCON_OFFSET]
	str	r1, [r0, #UMCON_OFFSET]

	mov	r1, #0x3                	@was 0.
	str	r1, [r0, #ULCON_OFFSET]

#if defined(CONFIG_UARTCLK_DOUTMPLL)
	ldr	r1, =0xc05			/* UARTCLK SRC = 11 => EXT_UCLK1*/
#else
	ldr	r1, =0x805			/* UARTCLK SRC = 10 => PCLK */
#endif

	str	r1, [r0, #UCON_OFFSET]

#if defined(CONFIG_CLK_400_100_50)
	ldr	r1, =0x1A          /* UARTCLK = 50MHz */
#else
	ldr	r1, =0x22          /* UARTCLK = 66MHz */
#endif
	str	r1, [r0, #UBRDIV_OFFSET]

#if defined(CONFIG_CLK_400_100_50)
	ldr	r1, =0x3          /* UARTCLK = 50MHz */ 
#else
	ldr	r1, =0x1FFF       /* UARTCLK = 66MHz */
#endif
	str	r1, [r0, #UDIVSLOT_OFFSET]

	mov	r1, #'O'
	str	r1, [r0, #UTXH_OFFSET]		

	mov	pc, lr

#ifdef CONFIG_NAND
/*
 * NAND Interface init for MY6410
 */
nand_asm_init:
	ldr	r0, =ELFIN_NAND_BASE
	ldr	r1, [r0, #NFCONF_OFFSET]
	orr	r1, r1, #0x70
	orr	r1, r1, #0x7700
	str	r1, [r0, #NFCONF_OFFSET]

	ldr	r1, [r0, #NFCONT_OFFSET]
	orr	r1, r1, #0x07
	str	r1, [r0, #NFCONT_OFFSET]

	mov	pc, lr
#endif

#ifdef CONFIG_ENABLE_MMU
/*
 * MMU Table for SMDK6400
 */

	/* form a first-level section entry */
.macro FL_SECTION_ENTRY base,ap,d,c,b
	.word (\base << 20) | (\ap << 10) | \
	      (\d << 5) | (1<<4) | (\c << 3) | (\b << 2) | (1<<1)
.endm

.section .mmudata, "a"
	.align 14
	/* the following alignment creates the mmu table at address 0x4000. */
	.globl mmu_table
mmu_table:
	.set __base, 0
	/* 1:1 mapping for debugging */
	.rept 0xA00
	FL_SECTION_ENTRY __base, 3, 0, 0, 0
	.set __base, __base + 1
	.endr

	/* access is not allowed. */
	.rept 0xC00 - 0xA00
	.word 0x00000000
	.endr

	/* 128MB for SDRAM 0xC0000000 -> 0x50000000 */
	.set __base, 0x500
	.rept 0xC80 - 0xC00
	FL_SECTION_ENTRY __base, 3, 0, 1, 1
	.set __base, __base + 1
	.endr

	/* access is not allowed. */
	.rept 0x1000 - 0xc80
	.word 0x00000000
	.endr
#endif
