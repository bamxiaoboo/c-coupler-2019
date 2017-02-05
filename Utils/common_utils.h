/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef COMMON_UTILS
#define COMMON_UTILS

#define NAME_STR_SIZE    1024

#include <stdio.h>

extern bool get_next_line(char *, FILE *);
extern bool get_next_attr(char *, char **);
extern bool get_next_integer_attr(char **, int&);
extern bool get_next_double_attr(char **line, double&);
extern bool is_end_of_file(FILE *);

extern FILE *open_config_file(const char *, const char *);
extern FILE *open_config_file(const char *);
extern int get_num_fields_in_config_file(const char *, const char *);
extern void create_directory(const char*, bool);
extern void write_data_into_array_buffer(const void*, int, char **, int &, int &);
extern void read_data_from_array_buffer(void*, int, const char*, int &);


template <typename T1, typename T2> void transform_datatype_of_arrays(const T1 *src_array, T2 *dst_array, long num_local_cells)
{
	for (long i = 0; i < num_local_cells; i ++)
		dst_array[i] = (T2) src_array[i];
}



#endif
