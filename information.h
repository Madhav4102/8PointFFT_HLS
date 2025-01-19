#define N 8
#include <hls_stream.h>
#include "ap_int.h"


struct axis_data
{
	ap_uint<64> data;
	ap_uint<1> last;

};
