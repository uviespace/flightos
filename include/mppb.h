/**
 * @file include/mppb.h
 */

#ifndef _MPPB_H_
#define _MPPB_H_



#define MPPB_NOC_DMA_BASE_ADDR	0x30100000

#define MPPB_SDRAM_AHB_BASE	0x40000000
#define MPPB_SDRAM_AHB_SIZE	0x0FFFFFFF

#define MPPB_SDRAM_NOC_BASE	0x50000000
#define MPPB_SDRAM_NOC_SIZE	0x0FFFFFFF

#define MPPB_SRAM_NOC_BASE	0x30000000
#define MPPB_SRAM_NOC_SIZE	0x0003FFFF

/* get the IRL for a channel, on the MPPB, this is equivalent to the channel
 * number on the secondary interrupt controller
 */
#define MPPB_NOC_GET_DMA_IRL(chan) (chan)



#endif /* _MPPB_H_ */
