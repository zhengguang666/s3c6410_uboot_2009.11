#include <common.h>
#include <movi.h>
#include <s3c6410.h>
#include <asm/io.h>

void movi_bl2_copy(void)
{
	writel(readl(HM_CONTROL4) | (0x3 << 16), HM_CONTROL4);
	CopyMMCtoMem(0, MOVI_BL2_POS, MOVI_BL2_BLKCNT, 
                 (uint *)CONFIG_SYS_PHY_UBOOT_BASE, 0);
}

