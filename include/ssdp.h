/**
 * @file include/ssdp.h
 */

#ifndef _SSDP_H_
#define _SSDP_H_

#warning "Configuration not yet known, using MPPBv2 as baseline"


#define SSDP_NOC_DMA_BASE_ADDR	0x30100000

#define SSDP_SDRAM_AHB_BASE	0x40000000
#define SSDP_SDRAM_AHB_SIZE	0x0FFFFFFF

#define SSDP_SDRAM_NOC_BASE	0x50000000
#define SSDP_SDRAM_NOC_SIZE	0x0FFFFFFF

#define SSDP_SRAM_NOC_BASE	0x30000000
#define SSDP_SRAM_NOC_SIZE	0x0003FFFF

/* get the IRL for a channel, on the SSDP, this is equivalent to the channel
 * number on the secondary interrupt controller
 */
#define SSDP_NOC_GET_DMA_IRL(chan) (chan)



#endif /* _SSDP_H_ */
