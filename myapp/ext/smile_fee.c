/**
 * @file   smile_fee.c
 * @author Armin Luntzer (armin.luntzer@univie.ac.at),
 * @date   2020
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief SMILE FEE interface
 *
 * @warn byte order is BIG ENDIAN!
 *
 */



#include <stdio.h>


#include <smile_fee.h>


/**
 * @brief write the contents of an event package to the console
 *
 * @param pkt a struct fee_display_event
 */

void fee_display_event(const struct fee_event_dection *pkt)
{
	ssize_t i, j;


	if (!pkt)
		return;

	printf("\n\tCOL %d ROW %d\n", pkt->col, pkt->row);

	/* as per MSSL-SMILE-SXI-IRD-0001, req. MSSL-IF-91  tbl 8-11,
	 * the upper left pixel is the last pixel in the data
	 * and the lower left pixel is the first pixel in the data
	 */
	for (i = FEE_EV_ROWS - 1; i >= 0; i--) {

		printf("\t");

		for (j = 0; j <  FEE_EV_COLS; j++)
			printf("%05d ", pkt->pix[j + i * FEE_EV_COLS]);

		printf("\n");
	}

	printf("\n");
}















void test_fee_display_event(void)
{
	ssize_t i, j;
	struct fee_event_dection pkt;


	pkt.col = 12;
	pkt.row = 43;

	for (i = FEE_EV_ROWS - 1; i >= 0; i--)
		for (j = 0; j <  FEE_EV_COLS; j++)
			pkt.pix[j + i * FEE_EV_COLS] = j + i * FEE_EV_COLS;

	fee_display_event(&pkt);
}

#if 0
int main(void)
{
	test_fee_display_event();
}
#endif
