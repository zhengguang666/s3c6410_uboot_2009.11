#include <config.h>
#if defined(CONFIG_S3C6400)
#include <s3c6400.h>

#elif defined(CONFIG_S3C6410)
#include <s3c6410.h>

#else
#error "Only 6400 and 6410 Soc are supported."
#endif

	.macro addruart, rx
		ldr	\rx, = ELFIN_UART_CONSOLE_BASE
	.endm

	.macro	senduart,rd,rx
		strb 	\rd, [\rx, #UTXH_OFFSET ]
	.endm

	.macro	busyuart, rd, rx
1001:
		@ busy waiting for non fifo
		ldr	\rd, [ \rx, #UTRSTAT_OFFSET ]
		tst	\rd, #UTRSTAT_TXF_EMPTY
		beq	1001b
	.endm

	.macro	waituart,rd, rx
1001:
		@ idle waiting for non fifo
		ldr	\rd, [ \rx, # UTRSTAT_OFFSET ]
		tst	\rd, #UTRSTAT_TXF_EMPTY
		beq	1001b
	.endm
