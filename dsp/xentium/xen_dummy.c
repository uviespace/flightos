
#include <xen_wrapper.h>
#include <xen_printf.h>


struct xen_kernel_cfg {
	char *name;
	unsigned long capabilities;
	unsigned long crit_buf_lvl;
};

const struct xen_kernel_cfg _xen_kernel_param  __attribute__((used)) = 
	{	"xen_dummy",
		0xdeadbeef,
		123
	};




int main(void)
{
	int i = 0;


	while (i++ < 10) {
		x_printf("hello there!\n");
		XEN_WAIT_CYCLES(25000000);
	}

	x_printf("bye\n");


#if 0
	/**
	 * stats
	 */

	unsigned int elapsed		    = 0; 
	unsigned int cycles_used 	    = 0;
	unsigned int samples_processed	    = 0;
	unsigned int samples_processed_last = 0;

	unsigned int tmp_critical_words;
	unsigned int uiTmpBufferCnt = 0;
	volatile unsigned int msg = 0;

	unsigned int *p_tmp_critical;
	unsigned int *p_tmp_buffer;
	unsigned int *ptBANK0     = (unsigned int *) p_xen->tcm;
	unsigned int *ptBANK2     = (unsigned int *) p_xen->tcm + ((unsigned int) BANK_SIZE >> 1);
	/default.ld
		struct xen_data *p_xen_data;

	enum {NONE, REQUEST_DATA, STORE_OUTPUT, YIELD} last_action = NONE;



	XEN_WAIT_INIT(p_xen_data);

	p_tmp_buffer = p_xen_data->tmp_buffer_1;

	// 50% fill, Size is in words; 
	// @note: adjust size so this round of processing will always fit
	// @note: we have two options here: check the element fill counter or do it by checking the pointer difference
	// @note: both ways work, but it depends on how an AlgoLib function is constructed (we really don't want to change them if possible)

	tmp_critical_words = p_xen_data->tmp_buffer_1_size >> 1;
	p_tmp_critical     = p_tmp_buffer + tmp_critical_words;

	x_printf("Reducer @Xen %u. Critical tmp buffer state is %u\n", p_xen_data->xen_id, tmp_critical_words);

	XEN_RESET_CYCLES_COUNTER();

	// lead-in: fetch data
	xen_request_data(p_xen_data, ptBANK0, TCM_WORDSIZE);

	last_action = REQUEST_DATA;

	XEN_MSG_LEON();

	while(1)
	{

		XEN_GET_MSG(msg);

		x_printf("msg\n");
		elapsed = XEN_GET_CYCLES_ELAPSED();

		cycles_used += elapsed;

		XEN_RESET_CYCLES_COUNTER();

		samples_processed     += samples_processed_last;
		samples_processed_last = 0;

		switch(last_action) {
		case	YIELD:

			if(msg == XEN_GO)
				uiTmpBufferCnt = 0;		// store was successful, reset

			if(msg == XEN_WAIT) {			// wait, then try to continue, setup new new data request
				XEN_WAIT_CYCLES(XEN_DEFAULT_WAIT);
				xen_request_data(p_xen_data, ptBANK0, TCM_WORDSIZE);
				last_action = REQUEST_DATA;
				break;
			}

			if(uiTmpBufferCnt > 0) {		// try to flush
				xen_req_store_data(p_xen_data, p_tmp_buffer, uiTmpBufferCnt);
				last_action = YIELD;
				break;
			} else {
				XEN_SET_CYCLES_MSG(cycles_used/samples_processed);
				XEN_YIELD();
			}
			break;

		case	STORE_OUTPUT:

			if(msg == XEN_WAIT) {			// retry
				XEN_WAIT_CYCLES(XEN_DEFAULT_WAIT);
				xen_req_store_data(p_xen_data, p_tmp_buffer, uiTmpBufferCnt);
				last_action = STORE_OUTPUT;
				break;
			}

			if(msg == XEN_GO) {			// success, continue processing
				uiTmpBufferCnt = 0;
				xen_request_data(p_xen_data, ptBANK0, TCM_WORDSIZE);
				last_action = REQUEST_DATA;
				break;
			}
			break;

		case	REQUEST_DATA:

			if(msg == XEN_GO) {			// execute kernel function
				uiTmpBufferCnt += execute_this_function(ptBANK0, p_tmp_buffer, TCM_WORDSIZE);
				samples_processed_last=TCM_WORDSIZE;
			}

			if(msg == XEN_WAIT)
				XEN_WAIT_CYCLES(XEN_DEFAULT_WAIT);

			if(msg == XEN_NODATA) {

				XEN_WAIT_CYCLES(XEN_DEFAULT_WAIT);

				if(uiTmpBufferCnt > 0) {	// try to flush
					xen_req_store_data(p_xen_data, p_tmp_buffer, uiTmpBufferCnt);
					last_action = STORE_OUTPUT;
					break;
				} else	{			// offer to yield
					XEN_SET_REQUEST(XEN_I_YIELD);
					last_action = YIELD;
				}
				break;
			}

			if(uiTmpBufferCnt < tmp_critical_words) {	// setup next block request

				xen_request_data(p_xen_data, ptBANK0, TCM_WORDSIZE);
				last_action = REQUEST_DATA;
			}  else {				// output is critical, flush

				xen_req_store_data(p_xen_data, p_tmp_buffer, uiTmpBufferCnt);
				last_action = STORE_OUTPUT;
			}

			break;

		default: 
			break;
		}


		switch(p_xen_data->ctrl_msg)
		{
		case XEN_PLEASEYIELD:

			p_xen_data->ctrl_msg = 0;
			if(uiTmpBufferCnt > 0) {		// flush. takes precendence over other requests	

				xen_req_store_data(p_xen_data, p_tmp_buffer, uiTmpBufferCnt);
				last_action = YIELD;
			} else {
				XEN_SET_CYCLES_MSG(cycles_used/samples_processed);
				XEN_YIELD();
			}

		default:
			break;
		}

		XEN_MSG_LEON();
	}
#endif
	return 0;
}
