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


#endif
