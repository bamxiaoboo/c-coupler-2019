/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file is initially finished by Mr. Yufeng Zhou,
  *  and then upgraded and merged into CoR by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "remap_operator_spline_1D.h"
#include "remap_common_utils.h"
#include "cor_global_data.h"
#include <string.h>
#include <math.h>
#include <assert.h>


void Remap_operator_spline_1D::solve_aperiodic_tridiagonal_system(double *a, double *b, double *c, double *f, int n)
{
	double y;


	for (int i = 1; i < n; i++) {
		y = a[i]/b[i-1];
		b[i] -= c[i-1]*y;
		f[i] -= f[i-1]*y;
	}

	f[n-1] /= b[n-1];
	for (int i = n-2; i >=0; i--)
		f[i] = (f[i]-f[i+1]*c[i])/b[i];
}


void Remap_operator_spline_1D::solve_periodic_tridiagonal_system(double *a, double *b, double *c, double *f, int n)
{
	double y;
	int i;


	temp_array_column[0] = a[0];
	temp_array_row[0] = c[n-1];
	for (i = 1; i < n-2; i++) {
		y = a[i]/b[i-1];
		b[i] -= c[i-1]*y;
		temp_array_column[i] = -temp_array_column[i-1]*y;
		f[i] -= f[i-1]*y;
		y = temp_array_row[i-1]/b[i-1];
		temp_array_row[i] = -c[i-1]*y;
		b[n-1] -= temp_array_column[i-1]*y;
		f[n-1] -= f[i-1]*y;
	}	

	y = a[n-2]/b[n-3];
	b[n-2] -= c[n-3]*y;
	c[n-2] -= temp_array_column[n-3]*y;
	f[n-2] -= f[n-3]*y;
	y = temp_array_row[n-3]/b[n-3];
	a[n-1] -= c[n-3]*y;
	b[n-1] -= temp_array_column[n-3]*y;
	f[n-1] -= f[n-3]*y;

	y = a[n-1]/b[n-2];
	b[n-1] -= c[n-2]*y;
	f[n-1] -= f[n-2]*y;
	
	// back substitution
	f[n-1] /= b[n-1];
	b[n-1] = 1.0;
	f[n-2] -= f[n-1]*c[n-2]/b[n-1];
	f[n-2] /= b[n-2];
	b[n-2] = 1.0;
	for (i = n-3; i >= 0; i--) {
		f[i] = f[i]-c[i]*f[i+1]-temp_array_column[i]*f[n-1];
		f[i] /= b[i];
		b[i] = 1.0;
	}
}


void Remap_operator_spline_1D::set_parameter(const char *parameter_name, const char *parameter_value)
{
    EXECUTION_REPORT(REPORT_ERROR, enable_to_set_parameters, 
                 "the parameter of remap operator object \"%s\" must be set before using it to build remap strategy\n",
                 object_name);
    if (words_are_the_same(parameter_name, "periodic")) {
		EXECUTION_REPORT(REPORT_ERROR, !set_periodic, 
					     "The parameter \"%s\" of the 1D spline remapping operator \"%s\" has been set before. It can not been set more than once",
						 parameter_name, operator_name);
		if (words_are_the_same(parameter_value, "true"))
	        periodic = true;
		else if (words_are_the_same(parameter_value, "false"))
			periodic = false;
		else EXECUTION_REPORT(REPORT_ERROR, false, 
                      "The value of parameter \"%s\" of the 1D spline remapping operator \"%s\" must be \"true\" or \"false\"",
                      parameter_name, operator_name);
		set_periodic = true;
		if (periodic)
			EXECUTION_REPORT(REPORT_ERROR, !set_enable_extrapolation, 
						     "The parameter \"extrapolation\" of the 1D spline remapping operator \"%s\" has been set before. This remapping operator can not be set to periodic",
							 parameter_name, operator_name);
    }
	else if (words_are_the_same(parameter_name, "period")) {
		EXECUTION_REPORT(REPORT_ERROR, set_periodic && periodic, 
					     "The spline_1D remapping operator \"%s\" has not been set to periodic before. Its \"period\" can not be set",
						 operator_name);
		EXECUTION_REPORT(REPORT_ERROR, !set_period,
						 "The parameter \"%s\" of the 1D spline remapping operator \"%s\" has been set before. It can not been set more than once",
						 parameter_name, operator_name);
		sscanf(parameter_value, "%lf", &period);
		set_period = true;
		EXECUTION_REPORT(REPORT_ERROR, period > 0,
						 "The parameter \"%s\" of the 1D spline remapping operator \"%s\" must be bigger than 0",
						 parameter_name, operator_name);
	}
	else if (words_are_the_same(parameter_name, "extrapolation")) {
		EXECUTION_REPORT(REPORT_ERROR, !periodic,
						 "The parameter \"%s\" of the 1D spline remapping operator \"%s\" can not be set when the 1D spline remapping operator is periodic",
						 parameter_name, operator_name);
		if (words_are_the_same(parameter_value, "true"))
	        enable_extrapolation = true;
		else if (words_are_the_same(parameter_value, "false"))
			enable_extrapolation = false;
		else EXECUTION_REPORT(REPORT_ERROR, false, 
                      "The value of parameter \"%s\" of the 1D spline remapping operator \"%s\" must be \"true\" or \"false\"",
                      parameter_name, operator_name);		
		set_enable_extrapolation = true;
	}
	else if (words_are_the_same(parameter_name, "keep_monotonicity")) {
		EXECUTION_REPORT(REPORT_ERROR, !set_keep_monotonicity,
						 "The parameter \"%s\" of the 1D spline remapping operator \"%s\" has been set before. It can not been set more than once",
						 parameter_name, operator_name);
		if (words_are_the_same(parameter_value, "true")) 
			keep_monotonicity = true;
		else if (words_are_the_same(parameter_value, "false"))
			keep_monotonicity = false;
		else EXECUTION_REPORT(REPORT_ERROR, false, 
                      "The value of parameter \"%s\" of the 1D spline remapping operator \"%s\" must be \"none\", \"overall\" or \"fragment\"",
                      parameter_name, operator_name);
		set_keep_monotonicity = true;
	}
    else EXECUTION_REPORT(REPORT_ERROR, false, 
                      "\"%s\" is a illegal parameter of remapping operator \"%s\"\n",
                      parameter_name, operator_name);
}


void Remap_operator_spline_1D::allocate_local_arrays()
{
	coord_values_src = new double [src_grid->get_grid_size()];
	coord_values_dst = new double [dst_grid->get_grid_size()];
	packed_data_values_src = new double [src_grid->get_grid_size()+1];
	useful_src_cells_global_index = new int [src_grid->get_grid_size()+1];
	array_alpha = new double [src_grid->get_grid_size()+1];
	array_mu = new double [src_grid->get_grid_size()+1];
	array_lambda = new double [src_grid->get_grid_size()+1];
	array_h = new double [src_grid->get_grid_size()+1];
	array_d = new double [src_grid->get_grid_size()+1];
	src_cell_index_left = new int [src_grid->get_grid_size()+1];
	src_cell_index_right = new int [src_grid->get_grid_size()+1];
	temp_array_column = new double [src_grid->get_grid_size()+1];
	temp_array_row = new double [src_grid->get_grid_size()+1];
	final_factor1 = new double [dst_grid->get_grid_size()];
	final_factor2 = new double [dst_grid->get_grid_size()];
	final_factor3 = new double [dst_grid->get_grid_size()];
	final_factor4 = new double [dst_grid->get_grid_size()];
	final_factor5 = new double [dst_grid->get_grid_size()];
	data_in_monotonicity_range = new double [dst_grid->get_grid_size()+2];
	dst_cell_indexes_in_monotonicity_ranges = new int [dst_grid->get_grid_size()];
}


Remap_operator_spline_1D::Remap_operator_spline_1D(const char *object_name, int num_remap_grids, Remap_grid_class **remap_grids)
                                       : Remap_operator_basis(object_name, 
                                                              REMAP_OPERATOR_NAME_SPLINE_1D, 
                                                              1, 
                                                              true, 
                                                              false, 
                                                              false, 
                                                              num_remap_grids, 
                                                              remap_grids)
{
	set_periodic = false;
	set_period = false;
    periodic = false;
	enable_extrapolation = false;
	set_enable_extrapolation = false;
	keep_monotonicity = false;
	set_keep_monotonicity = false;
	allocate_local_arrays();
	remap_weights_groups.push_back(new Remap_weight_sparse_matrix(this));
	remap_weights_groups.push_back(new Remap_weight_sparse_matrix(this));
	remap_weights_groups.push_back(new Remap_weight_sparse_matrix(this));
	remap_weights_groups.push_back(new Remap_weight_sparse_matrix(this));
	remap_weights_groups.push_back(new Remap_weight_sparse_matrix(this));
}


Remap_operator_spline_1D::~Remap_operator_spline_1D()
{
	delete [] coord_values_src;
	delete [] coord_values_dst;
	delete [] packed_data_values_src;
	delete [] useful_src_cells_global_index;
	delete [] array_alpha;
	delete [] array_mu;
	delete [] array_lambda;
	delete [] array_h;
	delete [] array_d;
	delete [] src_cell_index_left;
	delete [] src_cell_index_right;
	delete [] temp_array_column;
	delete [] temp_array_row;
	delete [] final_factor1;
	delete [] final_factor2;
	delete [] final_factor3;
	delete [] final_factor4;
	delete [] final_factor5;
	delete [] data_in_monotonicity_range;
	delete [] dst_cell_indexes_in_monotonicity_ranges;
}


void Remap_operator_spline_1D::compute_remap_weights_of_one_dst_cell(long cell_index_dst)
{
}


void Remap_operator_spline_1D::calculate_remap_weights()
{
	int i, j;
	bool ascending_order, src_cell_mask, dst_cell_mask;
	long temp_long_value;


	clear_remap_weight_info_in_sparse_matrix();

	for (i = 0; i < dst_grid->get_grid_size(); i ++)
		get_cell_center_coord_values_of_dst_grid(i, &coord_values_dst[i]);
	for (i = 0; i < src_grid->get_grid_size(); i ++)
		get_cell_center_coord_values_of_src_grid(i, &coord_values_src[i]);

	ascending_order = coord_values_src[0] < coord_values_src[1];
	for (i = 1; i < src_grid->get_grid_size() - 1; i ++) 
		EXECUTION_REPORT(REPORT_ERROR,ascending_order == coord_values_src[i] < coord_values_src[i+1], 
						 "the center coordinate values corresponding to the 1D grid %s are not sorted into ascending or descending order",
						 src_grid->get_grid_name());
	ascending_order = coord_values_dst[0] < coord_values_dst[1];
	for (i = 1; i < dst_grid->get_grid_size() - 1; i ++) 
		EXECUTION_REPORT(REPORT_ERROR, ascending_order == coord_values_dst[i] < coord_values_dst[i+1], 
						 "the center coordinate values corresponding to the 1D grid %s are not sorted into ascending or descending order",
						 dst_grid->get_grid_name());

	if (periodic) {
		EXECUTION_REPORT(REPORT_ERROR,fabs(coord_values_src[0]-coord_values_dst[src_grid->get_grid_size()-1]) < period, 
						 "The variation of center coordinate values corresponding to the 1D grid %s is larger than one period",
						 src_grid->get_grid_name());
		EXECUTION_REPORT(REPORT_ERROR,fabs(coord_values_dst[0]-coord_values_dst[dst_grid->get_grid_size()-1]) < period, 
						 "The variation of center coordinate values corresponding to the 1D grid %s is larger than one period",
						 dst_grid->get_grid_name());
	}
	
	num_useful_src_cells = 0;
	ascending_order = coord_values_src[0] < coord_values_src[1];
	if (ascending_order) {
		for (i = 0; i < src_grid->get_grid_size(); i ++) {
			get_cell_mask_of_src_grid(i, &src_cell_mask);
			if (src_cell_mask) {
				get_cell_center_coord_values_of_src_grid(i, &coord_values_src[num_useful_src_cells]);
				useful_src_cells_global_index[num_useful_src_cells++] = i;
			}
		}	
	}
	else {
		for (i = src_grid->get_grid_size()-1; i >= 0; i --) {
			get_cell_mask_of_src_grid(i, &src_cell_mask);
			if (src_cell_mask) {
				get_cell_center_coord_values_of_src_grid(i, &coord_values_src[num_useful_src_cells]);
				useful_src_cells_global_index[num_useful_src_cells++] = i;
			}
		}
	}

	if (num_useful_src_cells == 0)
		return;

	if (!periodic)
		EXECUTION_REPORT(REPORT_ERROR, num_useful_src_cells > 2, "Less than three source cells for 1D spline interpolation are not enough");
	else EXECUTION_REPORT(REPORT_ERROR, num_useful_src_cells > 1, "Less than two source cells for 1D spline interpolation are not enough"); 
		
	if (periodic) {
		coord_values_src[num_useful_src_cells] = coord_values_src[0] + period;
		useful_src_cells_global_index[num_useful_src_cells] = useful_src_cells_global_index[0];
		array_size_src = num_useful_src_cells + 1;
	}
	else array_size_src = num_useful_src_cells;

	for (i = 0; i < array_size_src-1; i ++)
		array_h[i] = coord_values_src[i+1]-coord_values_src[i];

	for (i = 1; i < array_size_src-1; i ++) {
		array_mu[i] = array_h[i-1]/(array_h[i-1]+array_h[i]);
		array_lambda[i] = 1.0-array_mu[i];
	}

	if (!periodic) {
		array_lambda[0] = 0.0;
		array_mu[array_size_src-1] = 0.0;
	} else {
		array_lambda[array_size_src-1] = array_h[0]/(array_h[array_size_src-2]+array_h[0]);
		array_mu[array_size_src-1] = 1.0-array_lambda[array_size_src-1];
	}

	for (i = 0; i < dst_grid->get_grid_size(); i ++) {
		src_cell_index_left[i] = -1;
		src_cell_index_right[i] = -1;
		get_cell_mask_of_dst_grid(i, &dst_cell_mask);
		if (!dst_cell_mask)
			continue;
		search_src_cells_around_dst_cell(coord_values_dst[i], 0, array_size_src-1, src_cell_index_left[i], src_cell_index_right[i]);
		for (j = 0; j < array_size_src && coord_values_src[j] <= coord_values_dst[i]; j ++);
		if (j > 0 && j < array_size_src)
			EXECUTION_REPORT(REPORT_ERROR, src_cell_index_left[i] == j-1 && src_cell_index_right[i] == j, "error error3\n"); 
		if ((src_cell_index_left[i] == -1 || src_cell_index_right[i] == -1) && !enable_extrapolation)
			continue;
		if (src_cell_index_right[i] == -1) {
			src_cell_index_right[i] = src_cell_index_left[i];
			src_cell_index_left[i] --;
		}
		if (src_cell_index_left[i] == -1) {
			src_cell_index_left[i] = src_cell_index_right[i];
			src_cell_index_right[i] ++;
		}
	    final_factor1[i] = pow(coord_values_src[src_cell_index_right[i]]-coord_values_dst[i], 3.0)/(6.0*array_h[src_cell_index_left[i]]);
		final_factor2[i] = pow(coord_values_dst[i]-coord_values_src[src_cell_index_left[i]], 3.0)/(6.0*array_h[src_cell_index_left[i]]);
		final_factor3[i] = array_h[src_cell_index_left[i]]*array_h[src_cell_index_left[i]]/6.0;
		final_factor4[i] = (coord_values_src[src_cell_index_right[i]]-coord_values_dst[i])/array_h[src_cell_index_left[i]];
		final_factor5[i] = (coord_values_dst[i]-coord_values_src[src_cell_index_left[i]])/array_h[src_cell_index_left[i]];
	}

	for (i = 0; i < array_size_src; i ++) {
		add_remap_weights_to_sparse_matrix((long*)(&array_mu[i]), useful_src_cells_global_index[i], array_lambda+i, 1, 0);
		add_remap_weights_to_sparse_matrix((long*)(&coord_values_src[i]), temp_long_value, array_h+i, 1, 1);
	}
	for (i = 0; i < dst_grid->get_grid_size(); i ++) {
		add_remap_weights_to_sparse_matrix((long*)(&final_factor1[i]), *((long*)(&final_factor2[i])), final_factor3+i, 1, 2);
		add_remap_weights_to_sparse_matrix((long*)(&final_factor4[i]), *((long*)(&final_factor5[i])), coord_values_dst+i, 1, 3);
		temp_long_value = src_cell_index_left[i];
		add_remap_weights_to_sparse_matrix(&temp_long_value, src_cell_index_right[i], final_factor5+i, 1, 4);
	}
}


void Remap_operator_spline_1D::search_src_cells_around_dst_cell(double coord_value_dst, int src_index_start, int src_index_end, int &src_cell_index_left, int &src_cell_index_right)
{
	int src_index_mid;


	if (coord_values_src[src_index_start] > coord_value_dst) {
		EXECUTION_REPORT(REPORT_ERROR, !periodic, "software error1: can not find the location of dst cell in original grid");
		src_cell_index_left = -1;
		src_cell_index_right = src_index_start;
		return;
	}

	if (coord_values_src[src_index_end] < coord_value_dst) {
		EXECUTION_REPORT(REPORT_ERROR, !periodic, "software error2: can not find the location of dst cell in original grid");
		src_cell_index_left = src_index_end;
		src_cell_index_right = -1;
		return;
	}

	if (coord_values_src[src_index_start] == coord_value_dst) {
		src_cell_index_left = src_index_start;
		src_cell_index_right = src_index_start + 1;
		return;
	}

	if (coord_values_src[src_index_end] == coord_value_dst) {
		src_cell_index_left = src_index_end-1;
		src_cell_index_right = src_index_end;
		return;
	}

	search_src_cells_around_dst_cell_recursively(coord_value_dst, src_index_start, src_index_end, src_cell_index_left, src_cell_index_right);
}


void Remap_operator_spline_1D::search_src_cells_around_dst_cell_recursively(double coord_value_dst, int src_index_start, int src_index_end, int &src_cell_index_left, int &src_cell_index_right)
{
	int src_index_mid;


	if (src_index_start == src_index_end-1) {
		src_cell_index_left = src_index_start;
		src_cell_index_right = src_index_end;
		EXECUTION_REPORT(REPORT_ERROR, coord_values_src[src_cell_index_left] <= coord_value_dst && coord_values_src[src_cell_index_right] >= coord_value_dst, 
			             "software error: can not find the location of dst cell in original grid recursively");
		return;
	}
	
	src_index_mid = (src_index_start+src_index_end) / 2;
	if (coord_values_src[src_index_mid] > coord_value_dst)
		search_src_cells_around_dst_cell_recursively(coord_value_dst, src_index_start, src_index_mid, src_cell_index_left, src_cell_index_right);
	else search_src_cells_around_dst_cell_recursively(coord_value_dst, src_index_mid, src_index_end, src_cell_index_left, src_cell_index_right);
}


void Remap_operator_spline_1D::do_remap_values_caculation(double *data_values_src, double *data_values_dst)
{
	int i, j, k, m, start_index_monotonicity_range, end_index_monotonicity_range;
	int original_index1, original_index2, original_index3;
	int num_dst_cells_in_monotonicity_ranges;
	long temp_long_value1, temp_long_value2;
	double temp_double_value;
	double ratio;
	bool check_monotonicity, next_in_same_monotonicity_range;


	array_size_src = remap_weights_groups[0]->get_num_weights();
	if (array_size_src == 0)
		return;
	
	for (i = 0; i < array_size_src; i ++) {
		remap_weights_groups[0]->get_weight((long*)(&array_mu[i]), &temp_long_value1, array_lambda+i, i);
		useful_src_cells_global_index[i] = temp_long_value1;
		remap_weights_groups[1]->get_weight((long*)(&coord_values_src[i]), &temp_long_value2, array_h+i, i);
	}
	for (i = 0; i < dst_grid->get_grid_size(); i ++) {
		remap_weights_groups[2]->get_weight((long*)(&final_factor1[i]), ((long*)(&final_factor2[i])), final_factor3+i, i);
		remap_weights_groups[3]->get_weight((long*)(&final_factor4[i]), ((long*)(&final_factor5[i])), coord_values_dst+i, i);
		remap_weights_groups[4]->get_weight(&temp_long_value1, &temp_long_value2, &temp_double_value, i);
		src_cell_index_left[i] = temp_long_value1;
		src_cell_index_right[i] = temp_long_value2;
	}

	for (i = 0; i < array_size_src; i++)
		array_alpha[i] = 2.0;
	
	for (i = 0; i < array_size_src; i ++)
		packed_data_values_src[i] = data_values_src[useful_src_cells_global_index[i]];

	for (i = 1; i < array_size_src-1; i++)
		array_d[i] = 6.0*((packed_data_values_src[i+1]-packed_data_values_src[i])/array_h[i]-(packed_data_values_src[i]-packed_data_values_src[i-1])/array_h[i-1])/(array_h[i-1]+array_h[i]);

	if (!periodic) {
		array_d[0] = 0.0;
		array_d[array_size_src-1] = 0.0;
		solve_aperiodic_tridiagonal_system(array_mu, array_alpha, array_lambda, array_d, array_size_src);
	}
	else {
		array_lambda[array_size_src-1] = array_h[0]/(array_h[array_size_src-2]+array_h[0]);
		array_mu[array_size_src-1] = 1.0-array_lambda[array_size_src-1];
		array_d[array_size_src-1] = 6.0*((packed_data_values_src[1]-packed_data_values_src[0])/array_h[0]-(packed_data_values_src[array_size_src-1]-packed_data_values_src[array_size_src-2])/array_h[array_size_src-2])/(array_h[0]+array_h[array_size_src-2]);
		solve_periodic_tridiagonal_system(array_mu+1, array_alpha+1, array_lambda+1, array_d+1, array_size_src-1);
		array_d[0]=array_d[array_size_src-1];
	}

	for (i = 0; i < dst_grid->get_grid_size(); i ++) {
		if (src_cell_index_left[i] == -1 || src_cell_index_right[i] == -1)
			continue;
		data_values_dst[i] = array_d[src_cell_index_left[i]]*final_factor1[i];
		data_values_dst[i] += array_d[src_cell_index_right[i]]*final_factor2[i];
		data_values_dst[i] += (data_values_src[src_cell_index_left[i]]-array_d[src_cell_index_left[i]]*final_factor3[i])*final_factor4[i];
		data_values_dst[i] += (data_values_src[src_cell_index_right[i]]-array_d[src_cell_index_right[i]]*final_factor3[i])*final_factor5[i];
	}

	if (keep_monotonicity) {
		for (i = 0, num_dst_cells_in_monotonicity_ranges = 0; i < dst_grid->get_grid_size(); i ++) {
			if (src_cell_index_left[i] == -1 || src_cell_index_right[i] == -1)
				continue;
			if (!(coord_values_dst[i] > coord_values_src[src_cell_index_left[i]] && coord_values_dst[i] < coord_values_src[src_cell_index_right[i]]))
				continue;
			dst_cell_indexes_in_monotonicity_ranges[num_dst_cells_in_monotonicity_ranges ++] = i;
		}
		start_index_monotonicity_range = -1;
		for (i = 0; i < num_dst_cells_in_monotonicity_ranges; i ++) {
			if (start_index_monotonicity_range == -1) {
				j = 0;
				start_index_monotonicity_range = i;
				data_in_monotonicity_range[j++] = data_values_src[src_cell_index_left[dst_cell_indexes_in_monotonicity_ranges[i]]];
			}
			end_index_monotonicity_range = i;
			data_in_monotonicity_range[j++] = data_values_dst[dst_cell_indexes_in_monotonicity_ranges[i]];
			if (i == num_dst_cells_in_monotonicity_ranges - 1)
				next_in_same_monotonicity_range = false;
			else {
				original_index1 = dst_cell_indexes_in_monotonicity_ranges[start_index_monotonicity_range];
				original_index2 = dst_cell_indexes_in_monotonicity_ranges[i+1];
				next_in_same_monotonicity_range = (src_cell_index_left[original_index1] == src_cell_index_left[original_index2] && src_cell_index_right[original_index1] == src_cell_index_right[original_index2]);
			}
			if (!next_in_same_monotonicity_range) {
				original_index1 = dst_cell_indexes_in_monotonicity_ranges[start_index_monotonicity_range];
				original_index2 = dst_cell_indexes_in_monotonicity_ranges[end_index_monotonicity_range];
				EXECUTION_REPORT(REPORT_ERROR, src_cell_index_left[original_index1] == src_cell_index_left[original_index2] && src_cell_index_right[original_index1] == src_cell_index_right[original_index2], 
								 "software error: in keep monotonicity"); 			
				data_in_monotonicity_range[j++] = data_values_src[src_cell_index_right[original_index1]];
				check_monotonicity = true;
				for (k = 0; k < j - 1; k ++)
					if ((data_in_monotonicity_range[k] >= data_in_monotonicity_range[k+1]) != (data_in_monotonicity_range[0] >= data_in_monotonicity_range[j-1])) {
						check_monotonicity = false;
						break;
					}
				if (!check_monotonicity) {
					original_index1 = src_cell_index_left[dst_cell_indexes_in_monotonicity_ranges[start_index_monotonicity_range]];
					original_index2 = src_cell_index_right[dst_cell_indexes_in_monotonicity_ranges[start_index_monotonicity_range]];
					for (k = start_index_monotonicity_range; k <= end_index_monotonicity_range; k ++) {
						original_index3 = dst_cell_indexes_in_monotonicity_ranges[k];
						ratio = (coord_values_dst[original_index3]-coord_values_src[original_index1]) / (coord_values_src[original_index2]-coord_values_src[original_index1]);
						data_values_dst[original_index3] = data_values_src[original_index1]*(1-ratio) + data_values_src[original_index2]*ratio;
					}
				}
				start_index_monotonicity_range = -1;
			}
		}
	}
}


void Remap_operator_spline_1D::do_src_decomp_caculation(bool *decomp_map_src, const bool *decomp_map_dst)
{
	int i;
	long temp_long_value1, temp_long_value2;
	double temp_double_value;


	array_size_src = remap_weights_groups[0]->get_num_weights();
	if (array_size_src == 0)
		return;
	
	for (i = 0; i < array_size_src; i ++) {
		remap_weights_groups[0]->get_weight(&temp_long_value1, &temp_long_value2, &temp_double_value, i);
		useful_src_cells_global_index[i] = temp_long_value2;
	}
	for (i = 0; i < dst_grid->get_grid_size(); i ++) {
		remap_weights_groups[4]->get_weight(&temp_long_value1, &temp_long_value2, &temp_double_value, i);
		src_cell_index_left[i] = temp_long_value1;
		src_cell_index_right[i] = temp_long_value2;
		if (src_cell_index_left[i] == -1 || src_cell_index_right[i] == -1)
			continue;
		if (decomp_map_dst[i]) {
			decomp_map_src[useful_src_cells_global_index[src_cell_index_left[i]]] = true;
			decomp_map_src[useful_src_cells_global_index[src_cell_index_right[i]]] = true;
		}
	}
}


Remap_operator_basis *Remap_operator_spline_1D::duplicate_remap_operator(bool fully_copy)
{
    Remap_operator_basis *duplicated_remap_operator = new Remap_operator_spline_1D();

	copy_remap_operator_basic_data(duplicated_remap_operator, fully_copy);
	((Remap_operator_spline_1D*) duplicated_remap_operator)->allocate_local_arrays();
	((Remap_operator_spline_1D*) duplicated_remap_operator)->enable_extrapolation = this->enable_extrapolation;
	((Remap_operator_spline_1D*) duplicated_remap_operator)->set_period = this->set_period;
	((Remap_operator_spline_1D*) duplicated_remap_operator)->period = this->period;
	((Remap_operator_spline_1D*) duplicated_remap_operator)->periodic = this->periodic;
	((Remap_operator_spline_1D*) duplicated_remap_operator)->keep_monotonicity = this->keep_monotonicity;
	((Remap_operator_spline_1D*) duplicated_remap_operator)->set_keep_monotonicity = this->set_keep_monotonicity;
	
    return duplicated_remap_operator;
}

Remap_operator_basis *Remap_operator_spline_1D::generate_parallel_remap_operator(Remap_grid_class **decomp_original_grids, int **global_cells_local_indexes_in_decomps)
{
	EXECUTION_REPORT(REPORT_ERROR, false, 
		             "software error: can not generate the parallel remapping operator of the 1D spline remapping algorithm which is only used for vertical grid or time frame in the C-Coupler");	
	return NULL;
}

