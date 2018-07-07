/*
 * (C) Copyright 2006 DENX Software Engineering
 *
 * Implementation for U-Boot 1.1.6 by Samsung
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

#include <nand.h>
#if defined(CONFIG_S3C6400)
#include <s3c6400.h>
#elif defined(CONFIG_S3C6410)
#include <s3c6410.h>
#else
#error "Only support 6400 and 6410."
#endif

#include <asm/io.h>
#include <asm/errno.h>

#define S3C_NAND_TYPE_UNKNOWN	0x0
#define S3C_NAND_TYPE_SLC	0x1
#define S3C_NAND_TYPE_MLC_4BIT	0x2
#define S3C_NAND_TYPE_MLC_8BIT	0x3
int cur_ecc_mode = 0;
int nand_type = S3C_NAND_TYPE_UNKNOWN;

static struct nand_ecclayout s3c_nand_oob_mlc_128_8bit = {
    .eccbytes = 104,
    .eccpos = {
        24,25,26,27,28,29,30,31,32,33,
        34,35,36,37,38,39,40,41,42,43,
        44,45,46,47,48,49,50,51,52,53,
        54,55,56,57,58,59,60,61,62,63,
        64,65,66,67,68,69,70,71,72,73,
        74,75,76,77,78,79,80,81,82,83,
        84,85,86,87,88,89,90,91,92,93,
        94,95,96,97,98,99,100,101,102,103,
        104,105,106,107,108,109,110,111,112,113,
        114,115,116,117,118,119,120,121,122,123,
        124,125,126,127},
    .oobfree = {
        {.offset = 2,
         .length = 20}}
};

static struct nand_ecclayout s3c_nand_oob_mlc_232_8bit = {
    .eccbytes = 208,
    .eccpos = {
        24,25,26,27,28,29,30,31,32,33,
        34,35,36,37,38,39,40,41,42,43,
        44,45,46,47,48,49,50,51,52,53,
        54,55,56,57,58,59,60,61,62,63,
        64,65,66,67,68,69,70,71,72,73,
        74,75,76,77,78,79,80,81,82,83,
        84,85,86,87,88,89,90,91,92,93,
        94,95,96,97,98,99,100,101,102,103,
        104,105,106,107,108,109,110,111,112,113,
        114,115,116,117,118,119,120,121,122,123,
        124,125,126,127,

        128,129,130,131,132,133,134,135,136,137,
        138,139,140,141,142,143,144,145,146,147,
        148,149,150,151,152,153,154,155,156,157,
        158,159,160,161,162,163,164,165,166,167,
        168,169,170,171,172,173,174,175,176,177,
        178,179,180,181,182,183,184,185,186,187,
        188,189,190,191,192,193,194,195,196,197,
        198,199,200,201,202,203,204,205,206,207,
        208,209,210,211,212,213,214,215,216,217,
        218,219,220,221,222,223,224,225,226,227,
        228,229,230,231
    },
    .oobfree = {
        {.offset = 2,
         .length = 22}}
};

#define MAX_CHIPS	2
static int nand_cs[MAX_CHIPS] = {0, 1};

#ifdef CONFIG_NAND_SPL
#define printf(arg...) do {} while (0)
#endif

/* Nand flash definition values by jsgood */
#ifdef S3C_NAND_DEBUG
/*
 * Function to print out oob buffer for debugging
 * Written by jsgood
 */
static void print_oob(const char *header, struct mtd_info *mtd)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	printf("%s:\t", header);

	for (i = 0; i < 64; i++)
		printf("%02x ", chip->oob_poi[i]);

	printf("\n");
}
#endif /* S3C_NAND_DEBUG */

#ifdef CONFIG_NAND_SPL
static u_char nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	return readb(this->IO_ADDR_R);
}

static void nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i = 0; i < len; i++)
		writeb(buf[i], this->IO_ADDR_W);
}

static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i = 0; i < len; i++)
		buf[i] = readb(this->IO_ADDR_R);
}
#endif

static void s3c_nand_select_chip(struct mtd_info *mtd, int chip)
{
	int ctrl = readl(NFCONT);

	switch (chip) {
	case -1:
		ctrl |= 6;
		break;
	case 0:
		ctrl &= ~2;
		break;
	case 1:
		ctrl &= ~4;
		break;
	default:
		return;
	}

	writel(ctrl, NFCONT);
}

/*
 * Hardware specific access to control-lines function
 * Written by jsgood
 */
static void s3c_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
    unsigned int cur;

    if (ctrl & NAND_CTRL_CHANGE) {
        if (ctrl & NAND_NCE) {
            if (cmd != NAND_CMD_NONE) {
                cur = readl(NFCONT);
                cur &= ~NFCONT_CS;
                writel(cur, NFCONT);
            }
        } else {
            cur = readl(NFCONT);
            cur |= NFCONT_CS;
            writel(cur, NFCONT);
        }
    }

    if (cmd != NAND_CMD_NONE) {
        if (ctrl & NAND_CLE)
            writeb(cmd, NFCMMD);
        else if (ctrl & NAND_ALE)
            writeb(cmd, NFADDR);
    }
}

/*
 * Function for checking device ready pin
 * Written by jsgood
 */
static int s3c_nand_device_ready(struct mtd_info *mtdinfo)
{
	return !!(readl(NFSTAT) & NFSTAT_RnB);
}

#ifdef CONFIG_SYS_S3C_NAND_HWECC
/*
 * Function for checking ECCEncDone in NFSTAT
 */
static void s3c_nand_wait_enc(void)
{
    unsigned long timeo = get_timer(0);
    
    timeo += 80;

    while (get_timer(0) < timeo) {
        if (readl(NFSTAT) & NFSTAT_ECCENCDONE)
            break;
    }
}

/*
 * Function for checking ECCDecDone in NFSTAT
 */
static void s3c_nand_wait_dec(void)
{
    unsigned long timeo = get_timer(0);
    
    timeo += 80;

    while (get_timer(0) < timeo) {
        if (readl(NFSTAT) & NFSTAT_ECCDECDONE)
            break;
    }
}

/*
 * Function for checking ECC Busy
 */
static void s3c_nand_wait_ecc_busy(void)
{
    unsigned long timeo = get_timer(0);
    
    timeo += 80;

    while (get_timer(0) < timeo) {
        if (!(readl(NFESTAT0) & NFESTAT0_ECCBUSY))
            break;
    }
}

/*
 * This function is called before encoding ecc codes to ready ecc engine.
 * Written by jsgood
 */
static void s3c_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	u_long nfcont, nfconf;

    cur_ecc_mode = mode;

    nfconf = readl(NFCONF);

#if defined(CONFIG_S3C6400)
    if (nand_type == S3C_NAND_TYPE_SLC)
        nfconf &= ~NFCONF_ECC_MLC;	/* SLC */
    else
        nfconf |= NFCONF_ECC_MLC;	/* MLC */
#elif defined(CONFIG_S3C6410)
    nfconf &= ~(0x3 << 23);

    if (nand_type == S3C_NAND_TYPE_SLC)
        nfconf |= NFCONF_ECC_1BIT;
    else if(nand_type == S3C_NAND_TYPE_MLC_4BIT)
        nfconf |= NFCONF_ECC_4BIT;
    else if(nand_type == S3C_NAND_TYPE_MLC_8BIT)
        nfconf |= NFCONF_ECC_8BIT;
#endif

	writel(nfconf, NFCONF);

    /* Init main ECC & unlock */
    nfcont = readl(NFCONT);
    nfcont |= NFCONT_INITMECC;
    nfcont &= ~NFCONT_MECCLOCK;

    if (nand_type == S3C_NAND_TYPE_MLC_4BIT || nand_type == S3C_NAND_TYPE_MLC_8BIT) {
        if (mode == NAND_ECC_WRITE)
            nfcont |= NFCONT_ECC_ENC;
        else if (mode == NAND_ECC_READ)
            nfcont &= ~NFCONT_ECC_ENC;
    }

    writel(nfcont, NFCONT);
}

/*
 * This function is called immediately after encoding ecc codes.
 * This function returns encoded ecc codes.
 * Written by jsgood
 */
static int s3c_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
				  u_char *ecc_code)
{
    u_long nfcont, nfmecc0, nfmecc1, nfm8ecc0, nfm8ecc1, nfm8ecc2, nfm8ecc3;;

    /* Lock */
    nfcont = readl(NFCONT);
    nfcont |= NFCONT_MECCLOCK;
    writel(nfcont, NFCONT);

    if (nand_type == S3C_NAND_TYPE_SLC) {
        nfmecc0 = readl(NFMECC0);

        ecc_code[0] = nfmecc0 & 0xff;
        ecc_code[1] = (nfmecc0 >> 8) & 0xff;
        ecc_code[2] = (nfmecc0 >> 16) & 0xff;
        ecc_code[3] = (nfmecc0 >> 24) & 0xff;
    } else if(nand_type == S3C_NAND_TYPE_MLC_4BIT){
        if (cur_ecc_mode == NAND_ECC_READ)
            s3c_nand_wait_dec();
        else {
            s3c_nand_wait_enc();

            nfmecc0 = readl(NFMECC0);
            nfmecc1 = readl(NFMECC1);

            ecc_code[0] = nfmecc0 & 0xff;
            ecc_code[1] = (nfmecc0 >> 8) & 0xff;
            ecc_code[2] = (nfmecc0 >> 16) & 0xff;
            ecc_code[3] = (nfmecc0 >> 24) & 0xff;
            ecc_code[4] = nfmecc1 & 0xff;
            ecc_code[5] = (nfmecc1 >> 8) & 0xff;
            ecc_code[6] = (nfmecc1 >> 16) & 0xff;
            ecc_code[7] = (nfmecc1 >> 24) & 0xff;
        }
    }else if(nand_type == S3C_NAND_TYPE_MLC_8BIT){
        if (cur_ecc_mode == NAND_ECC_READ)
            s3c_nand_wait_dec();
        else {
            s3c_nand_wait_enc();

            nfm8ecc0 = readl(NFM8ECC0);
            nfm8ecc1 = readl(NFM8ECC1);
            nfm8ecc2 = readl(NFM8ECC2);
            nfm8ecc3 = readl(NFM8ECC3);

            ecc_code[0] = nfm8ecc0 & 0xff;
            ecc_code[1] = (nfm8ecc0 >> 8) & 0xff;
            ecc_code[2] = (nfm8ecc0 >> 16) & 0xff;
            ecc_code[3] = (nfm8ecc0 >> 24) & 0xff;
            ecc_code[4] = nfm8ecc1 & 0xff;
            ecc_code[5] = (nfm8ecc1 >> 8) & 0xff;
            ecc_code[6] = (nfm8ecc1 >> 16) & 0xff;
            ecc_code[7] = (nfm8ecc1 >> 24) & 0xff;
            ecc_code[8] = nfm8ecc2 & 0xff;
            ecc_code[9] = (nfm8ecc2 >> 8) & 0xff;
            ecc_code[10] = (nfm8ecc2 >> 16) & 0xff;
            ecc_code[11] = (nfm8ecc2 >> 24) & 0xff;
            ecc_code[12] = nfm8ecc3 & 0xff;
        }
    }

    return 0;
}

/*
 * This function determines whether read data is good or not.
 * If SLC, must write ecc codes to controller before reading status bit.
 * If MLC, status bit is already set, so only reading is needed.
 * If status bit is good, return 0.
 * If correctable errors occured, do that.
 * If uncorrectable errors occured, return -1.
 * Written by jsgood
 */
static int s3c_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				 u_char *read_ecc, u_char *calc_ecc)
{
    int ret = -1;
    u_long nfestat0, nfestat1, nfmeccdata0, nfmeccdata1, nfmlcbitpt;
    u_long nf8eccerr0, nf8eccerr1, nf8eccerr2, nfmlc8bitpt0, nfmlc8bitpt1;
    u_char err_type;

    if (nand_type == S3C_NAND_TYPE_SLC) {
        /* SLC: Write ECC data to compare */
        nfmeccdata0 = (read_ecc[1] << 16) | read_ecc[0];
        nfmeccdata1 = (read_ecc[3] << 16) | read_ecc[2];
        writel(nfmeccdata0, NFMECCDATA0);
        writel(nfmeccdata1, NFMECCDATA1);

        /* Read ECC status */
        nfestat0 = readl(NFESTAT0);
        err_type = nfestat0 & 0x3;

        switch (err_type) {
        case 0: /* No error */
            ret = 0;
            break;

        case 1: /* 1 bit error (Correctable)
               (nfestat0 >> 7) & 0x7ff	:error byte number
               (nfestat0 >> 4) & 0x7	:error bit number */
            printk("s3c-nand: SLC 1 bit error detected at byte %ld, correcting from "
                   "0x%02x ", (nfestat0 >> 7) & 0x7ff, dat[(nfestat0 >> 7) & 0x7ff]);
            dat[(nfestat0 >> 7) & 0x7ff] ^= (1 << ((nfestat0 >> 4) & 0x7));
            printk("to 0x%02x...OK\n", dat[(nfestat0 >> 7) & 0x7ff]);
            ret = 1;
            break;

        case 2: /* Multiple error */
        case 3: /* ECC area error */
            printk("s3c-nand: SLC ECC uncorrectable error detected ???\n");
            ret = -1;
            break;
        }
    } else if (nand_type == S3C_NAND_TYPE_MLC_4BIT) {
        /* MLC: */
        s3c_nand_wait_ecc_busy();

        nfestat0 = readl(NFESTAT0);
        nfestat1 = readl(NFESTAT1);
        nfmlcbitpt = readl(NFMLCBITPT);

        err_type = (nfestat0 >> 26) & 0x7;

        /* No error, If free page (all 0xff) */
        if ((nfestat0 >> 29) & 0x1) {
            err_type = 0;
        } else {
            /* No error, If all 0xff from 17th byte in oob (in case of JFFS2 format) */
            if (dat) {
                if (dat[17] == 0xff && dat[26] == 0xff && dat[35] == 0xff &&
                    dat[44] == 0xff && dat[54] == 0xff)
                    err_type = 0;
            }
        }

        switch (err_type) {
        case 5: /* Uncorrectable */
            printk("s3c-nand: MLC4 ECC uncorrectable error detected\n");
            ret = -1;
            break;

        case 4: /* 4 bit error (Correctable) */
            dat[(nfestat1 >> 16) & 0x3ff] ^= ((nfmlcbitpt >> 24) & 0xff);

        case 3: /* 3 bit error (Correctable) */
            dat[nfestat1 & 0x3ff] ^= ((nfmlcbitpt >> 16) & 0xff);

        case 2: /* 2 bit error (Correctable) */
            dat[(nfestat0 >> 16) & 0x3ff] ^= ((nfmlcbitpt >> 8) & 0xff);

        case 1: /* 1 bit error (Correctable) */
            printk("s3c-nand: MLC4 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[nfestat0 & 0x3ff] ^= (nfmlcbitpt & 0xff);
            ret = err_type;
            break;

        case 0: /* No error */
            ret = 0;
            break;
        }
    } else if (nand_type == S3C_NAND_TYPE_MLC_8BIT) {
        while (readl(NF8ECCERR0) & (1<<31));
        nf8eccerr0 = readl(NF8ECCERR0);
        nf8eccerr1 = readl(NF8ECCERR1);
        nf8eccerr2 = readl(NF8ECCERR2);
        nfmlc8bitpt0 = readl(NFMLC8BITPT0);
        nfmlc8bitpt1 = readl(NFMLC8BITPT1);
        err_type = (nf8eccerr0 >> 25) & 0xf;

        /* No error, If free page (all 0xff) */
        if ((nf8eccerr0 >> 29) & 0x1)
            err_type = 0;

        switch (err_type) {
        case 9: /* Uncorrectable */
            printk("s3c-nand: MLC8 ECC uncorrectable error detected\n");
            ret = -1;
            break;

        case 8: /* 8 bit error (Correctable) */
            printk("s3c-nand: MLC8 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[(nf8eccerr2 >> 22) & 0x3ff] ^= ((nfmlc8bitpt1 >> 24) & 0xff);

        case 7: /* 7 bit error (Correctable) */
            printk("s3c-nand: MLC8 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[(nf8eccerr2 >> 11) & 0x3ff] ^= ((nfmlc8bitpt1 >> 16) & 0xff);

        case 6: /* 6 bit error (Correctable) */
            printk("s3c-nand: MLC8 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[nf8eccerr2 & 0x3ff] ^= ((nfmlc8bitpt1 >> 8) & 0xff);

        case 5: /* 5 bit error (Correctable) */
            printk("s3c-nand: MLC8 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[(nf8eccerr1 >> 22) & 0x3ff] ^= (nfmlc8bitpt1 & 0xff);

        case 4: /* 4 bit error (Correctable) */
            printk("s3c-nand: MLC8 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[(nf8eccerr1 >> 11) & 0x3ff] ^= ((nfmlc8bitpt0 >> 24) & 0xff);

        case 3: /* 3 bit error (Correctable) */
            printk("s3c-nand: MLC8 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[nf8eccerr1 & 0x3ff] ^= ((nfmlc8bitpt0 >> 16) & 0xff);

        case 2: /* 2 bit error (Correctable) */
            printk("s3c-nand: MLC8 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[(nf8eccerr0 >> 15) & 0x3ff] ^= ((nfmlc8bitpt0 >> 8) & 0xff);

        case 1: /* 1 bit error (Correctable) */
            printk("s3c-nand: MLC8 %d bit(s) error detected, corrected successfully\n", err_type);
            dat[nf8eccerr0 & 0x3ff] ^= (nfmlc8bitpt0 & 0xff);
            ret = err_type;
            break;

        case 0: /* No error */
            ret = 0;
            break;
        }
    }

    return ret;
}

void s3c_nand_write_page_8bit(struct mtd_info *mtd, struct nand_chip *chip,
                              const uint8_t *buf)
{
    int i, eccsize = 512;
    int eccbytes = 13;
    int eccsteps = mtd->writesize / eccsize;
    uint8_t *ecc_calc = chip->buffers->ecccalc;
    uint8_t *p = buf;
    uint32_t *mecc_pos = chip->ecc.layout->eccpos;

    for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
        chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
        chip->write_buf(mtd, p, eccsize);
        chip->ecc.calculate(mtd, p, &ecc_calc[i]);
    }

    for (i = 0; i < eccbytes * (mtd->writesize / eccsize); i++)
        chip->oob_poi[mecc_pos[i]] = ecc_calc[i];

    chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

int s3c_nand_read_page_8bit(struct mtd_info *mtd, struct nand_chip *chip,
                            uint8_t *buf, int page)
{
    int i, stat, eccsize = 512;
    int eccbytes = 13;
    int eccsteps = mtd->writesize / eccsize;
    int col = 0;
    uint8_t *p = buf;
    uint32_t *mecc_pos = chip->ecc.layout->eccpos;

    /* Step1: read whole oob */
    col = mtd->writesize;
    chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
    chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

    col = 0;
    for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
        chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
        chip->ecc.hwctl(mtd, NAND_ECC_READ);
        chip->read_buf(mtd, p, eccsize);
        chip->write_buf(mtd, chip->oob_poi + mecc_pos[0] +
                             ((chip->ecc.steps - eccsteps) * eccbytes), eccbytes);

        chip->ecc.calculate(mtd, 0, 0);
        stat = chip->ecc.correct(mtd, p, 0, 0);

        if (stat == -1)
            mtd->ecc_stats.failed++;

        col = eccsize * ((mtd->writesize / eccsize) + 1 - eccsteps);
    }

    return 0;
}
#endif /* CONFIG_SYS_S3C_NAND_HWECC */

/*
 * Board-specific NAND initialization. The following members of the
 * argument are board-specific (per include/linux/mtd/nand.h):
 * - IO_ADDR_R?: address to read the 8 I/O lines of the flash device
 * - IO_ADDR_W?: address to write the 8 I/O lines of the flash device
 * - hwcontrol: hardwarespecific function for accesing control-lines
 * - dev_ready: hardwarespecific function for  accesing device ready/busy line
 * - enable_hwecc?: function to enable (reset)  hardware ecc generator. Must
 *   only be provided if a hardware ECC is available
 * - eccmode: mode of ecc, see defines
 * - chip_delay: chip dependent delay for transfering data from array to
 *   read regs (tR)
 * - options: various chip options. They can partly be set to inform
 *   nand_scan about special functionality. See the defines for further
 *   explanation
 * Members with a "?" were not set in the merged testing-NAND branch,
 * so they are not set here either.
 */
int board_nand_init(struct nand_chip *nand)
{
	static int chip_n;

	if (chip_n >= MAX_CHIPS)
		return -ENODEV;

	NFCONT_REG = (NFCONT_REG & ~NFCONT_WP) | NFCONT_ENABLE | 0x6;

	nand->IO_ADDR_R		= (void __iomem *)NFDATA;
	nand->IO_ADDR_W		= (void __iomem *)NFDATA;
	nand->cmd_ctrl		= s3c_nand_hwcontrol;
	nand->dev_ready		= s3c_nand_device_ready;
	nand->select_chip	= s3c_nand_select_chip;
	nand->options		= 0;
#ifdef CONFIG_NAND_SPL
	nand->read_byte		= nand_read_byte;
	nand->write_buf		= nand_write_buf;
	nand->read_buf		= nand_read_buf;
#endif

#ifdef CONFIG_SYS_S3C_NAND_HWECC
    int mak_id, dev_id, ceinfo;

	nand->ecc.hwctl		= s3c_nand_enable_hwecc;
	nand->ecc.calculate	= s3c_nand_calculate_ecc;
	nand->ecc.correct	= s3c_nand_correct_data;

	nand->ecc.mode		= NAND_ECC_HW;

	nand->cmd_ctrl(0, NAND_CMD_READID, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	nand->cmd_ctrl(0, 0x00, NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE);
	nand->dev_ready(0);

    mak_id = readb(nand->IO_ADDR_R);
    dev_id = readb(nand->IO_ADDR_R);
    ceinfo = readb(nand->IO_ADDR_R);
    
    switch (mak_id) {
    case NAND_MFR_SAMSUNG:
        switch (dev_id) {
        case 0xd5:
            nand_type = S3C_NAND_TYPE_MLC_8BIT;
            nand->ecc.read_page = s3c_nand_read_page_8bit;
            nand->ecc.write_page = s3c_nand_write_page_8bit;
            nand->ecc.size = 512;
            nand->ecc.bytes = 13;
            switch (ceinfo) {
            case 0x94:  /* K9GAG08U0D, size=2GB, type=MLC, Page=4K */
            case 0x14:  /* K9GAG08U0M, size=2GB, type=MLC, Page=4K */
                nand->ecc.layout = &s3c_nand_oob_mlc_128_8bit;
                break;
            case 0x84:  /* K9GAG08U0E, size=2GB, type=MLC, Page=8K */
                nand->ecc.layout = &s3c_nand_oob_mlc_232_8bit;
                break;
            }
            break;
        case 0xd7:
            nand_type = S3C_NAND_TYPE_MLC_8BIT;
            nand->ecc.read_page = s3c_nand_read_page_8bit;
            nand->ecc.write_page = s3c_nand_write_page_8bit;
            nand->ecc.size = 512;
            nand->ecc.bytes = 13;
            nand->ecc.layout = &s3c_nand_oob_mlc_128_8bit;
            break;
        }
        break;
    case NAND_MFR_MICRON:
        switch (dev_id) {
        case 0x38:      /* MT29F8G08ABACAWP */ 
        case 0x48:      /* MT29F16G08ABACAWP */ 
            nand_type = S3C_NAND_TYPE_MLC_8BIT;
            nand->ecc.read_page = s3c_nand_read_page_8bit;
            nand->ecc.write_page = s3c_nand_write_page_8bit;
            nand->ecc.size = 512;
            nand->ecc.bytes = 13;
            nand->ecc.layout = &s3c_nand_oob_mlc_128_8bit;
            break;
        }
        break;
    }

#else
	nand->ecc.mode		= NAND_ECC_SOFT;
#endif /* ! CONFIG_SYS_S3C_NAND_HWECC */

	nand->priv		= nand_cs + chip_n++;

	return 0;
}
