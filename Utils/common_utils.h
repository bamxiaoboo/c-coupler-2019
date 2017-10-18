/***************************************************************
  *  Copyright (c) 2017, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef COMMON_UTILS
#define COMMON_UTILS

#define NAME_STR_SIZE    512


#include <stdio.h>
#include <string.h>


extern bool get_next_line(char *, FILE *);
extern bool get_next_attr(char *, char **);
extern bool get_next_integer_attr(char **, int&);
extern bool get_next_double_attr(char **line, double&);
extern bool is_end_of_file(FILE *);
extern void write_string_into_array_buffer(const char*, long, char**, long&, long&);
extern void write_data_into_array_buffer(const void*, long, char **, long&, long&);
extern bool read_data_from_array_buffer(void*, long, const char*, long &, bool);
extern void check_for_coupling_registration_stage(int, int, bool, const char *);
extern void common_checking_for_grid_registration(int, const char *, const char *, int, const char *);
extern void check_for_ccpl_managers_allocated(int, const char *);
extern bool are_two_coord_arrays_same(double *, double *, int, int);
extern void check_API_parameter_string_length(int, int, int, const char *, const char *, const char *);
extern void check_XML_attribute_value_string_length(int, int, const char *, const char *, const char *, int);
extern bool is_string_decimal_number(const char *);


template <typename T1, typename T2> void transform_datatype_of_arrays(const T1 *src_array, T2 *dst_array, long num_local_cells)
{
	for (long i = 0; i < num_local_cells; i ++)
		dst_array[i] = (T2) src_array[i];
}


extern void transform_datatype_of_arrays(const char *, char *, const char *, const char *, long);


template <typename T1, typename T2, typename T3> void arrays_multiplication_template(T1 *src_array1, T2 *src_array2, T3 *dst_array, int array_size)
{
	for (int i = 0; i < array_size; i ++)
		dst_array[i] = ((T3)(src_array1[i])) * ((T3)(src_array2[i]));
}


template <typename T1, typename T2, typename T3> void arrays_division_template(T1 *src_array, T2 *divisor, T3 *dst_array, int array_size)
{
	for (int i = 0; i < array_size; i ++)
		if (divisor[i] != (T2) 0)
			dst_array[i] = ((T3)(src_array[i])) / ((T3)(divisor[i]));
}


template <typename T> bool are_array_values_between_boundaries_kernel(const T *data_array, int array_size, T lower_bound, T upper_bound, T missing_value, bool has_missing_value)
{	
	for (int i = 0; i < array_size; i ++) {
		if (has_missing_value && data_array[i] == missing_value)
			continue;
		if (lower_bound < upper_bound) {
			if (data_array[i] < lower_bound || data_array[i] > upper_bound)
				return false;
		}
		else {
			if (data_array[i] < lower_bound && data_array[i] > upper_bound)
				return false;		
		}
	}

	return true;
}


template <typename T> bool are_array_values_between_boundaries(const char *data_type, const T *data_array, int array_size, T lower_bound, T upper_bound, T missing_value, bool has_missing_value)
{
	if (array_size <= 0)
		return true;

	if (strcmp(data_type, "integer") == 0)
		return are_array_values_between_boundaries_kernel((const int *) data_array, (int) array_size, (int) lower_bound, (int) upper_bound, (int) missing_value, has_missing_value);
	else if (strcmp(data_type, "real4") == 0)
		return are_array_values_between_boundaries_kernel((const float *) data_array, (float) array_size, (float) lower_bound, (float) upper_bound, (float) missing_value, has_missing_value);	
	else if (strcmp(data_type, "real8") == 0)
		return are_array_values_between_boundaries_kernel((const double *) data_array, (double) array_size, (double) lower_bound, (double) upper_bound, (double) missing_value, has_missing_value);

	return false;
}


template <typename T> int is_array_in_sorting_order(T *array, int array_size)    // 0 for non-sorting order; 1 for ascending order; 2 for descending order
{
	int i;

	i = 1;
	while (i < array_size && array[i-1] <= array[i])
		i ++;
	if (i == array_size) {
		if (array[0] == array[array_size-1])
			return 0;
		return 1;
	}

	i = 1;
	while (i < array_size && array[i-1] >= array[i])
		i ++;
	if (i == array_size)
		return 2;

	return 0;
}



#endif
