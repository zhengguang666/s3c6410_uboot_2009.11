#ifndef __MOVI_H__
#define __MOVI_H__

#if defined(CONFIG_S3C6400) || defined(CONFIG_S3C6410) 
#define	TCM_BASE		0x0C004000
#else
# error TCM_BASE is not defined
#endif

/* TCM function for bl2 load */
#if defined(CONFIG_S3C6400)
#define CopyMMCtoMem(a,b,c,d,e)	\
    (((int(*)(uint, ushort, uint *, uint, int))(*((uint *)(TCM_BASE + 0x8))))(a,b,c,d,e))

#elif defined(CONFIG_S3C6410) 
#define CopyMMCtoMem(a,b,c,d,e)	\
    (((int(*)(int, uint, ushort, uint *, int))(*((uint *)(TCM_BASE + 0x8))))(a,b,c,d,e))

#endif

/* size information */
#if defined(CONFIG_S3C6400)
#define SS_SIZE			(4 * 1024)  // Stepping Stone Size
#define eFUSE_SIZE		(2 * 1024)	// 1.5k eFuse, 0.5k reserved
#else
#define SS_SIZE			(8 * 1024)  // Stepping Stone Size 
#define eFUSE_SIZE		(1 * 1024)	// 0.5k eFuse, 0.5k reserved
#endif

/* MMC definitions */
#define MOVI_BLKSIZE		512
#define MOVI_ENV_SIZE		(16 * 1024)  
#define MOVI_TOTAL_BLKCNT	*((volatile unsigned int*)(TCM_BASE - 0x4))
#define MOVI_HIGH_CAPACITY	*((volatile unsigned int*)(TCM_BASE - 0x8))

/* partition information */
#define PART_SIZE_BL		(256 * 1024)

#define MOVI_LAST_BLKPOS	(MOVI_TOTAL_BLKCNT - (eFUSE_SIZE / MOVI_BLKSIZE))
#define MOVI_BL1_BLKCNT		(SS_SIZE / MOVI_BLKSIZE)
#define MOVI_ENV_BLKCNT		(MOVI_ENV_SIZE / MOVI_BLKSIZE)
#define MOVI_BL2_BLKCNT	    (PART_SIZE_BL / MOVI_BLKSIZE)	

#define MOVI_BL2_POS		(MOVI_LAST_BLKPOS - MOVI_BL1_BLKCNT - \
                             MOVI_BL2_BLKCNT - MOVI_ENV_BLKCNT)

#endif /*__MOVI_H__*/
