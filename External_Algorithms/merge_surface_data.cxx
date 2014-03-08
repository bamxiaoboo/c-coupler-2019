
#include "merge_surface_data.h"
#include "execution_report.h"


void merge_one_field_of_surface_data(double *dst, double *src, double *frac, int length)
{
    for(int i = 0; i < length; i++)
        dst[i] = dst[i] + src[i] * frac[i];
}


void reset_surface_data(void **buf_src, void **buf_dst, int *length)
{
	int num_cells = length[0];
	int num_dst_fields = length[2];
	int num_src_fields = length[1];


	EXECUTION_REPORT(REPORT_ERROR, num_src_fields == 0, "the number of input fields of externel function reset_surface_data must be 0");
	for (int j = 0; j < num_dst_fields; j ++)
		for (int i = 0; i < num_cells; i ++)
			((double**) buf_dst)[j][i] = 0.0;
}


void merge_one_kind_surface_data(void **buf_src, void **buf_dst, int *length)
{
	int num_cells = length[0];
	int num_src_fields = length[1];
	int num_dst_fields = length[2];
	double *merge_frac = (double*) buf_src[num_src_fields-1];
	double **merge_input = (double**) buf_src + num_dst_fields;
	double **merge_dst = (double **) buf_dst;


	EXECUTION_REPORT(REPORT_ERROR, num_src_fields == num_dst_fields*2+1, "the number of input fields of externel function merge_one_kind_surface_data must be 2*num_dst_fields+1");
	for (int i = 0; i < num_dst_fields; i ++)
		merge_one_field_of_surface_data(merge_dst[i], merge_input[i], merge_frac, num_cells);
}

