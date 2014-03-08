/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef FIELD_MGT
#define FIELD_MGT

#include "common_utils.h"
#include <vector>


struct field_attr
{
    char field_name[NAME_STR_SIZE];
    char field_long_name[NAME_STR_SIZE];
    char field_unit[NAME_STR_SIZE];
    char field_data_type[NAME_STR_SIZE];          // data type: logical, integer, real, etc
    char field_dim[NAME_STR_SIZE];           // dimension info: scalar, 1D, 2D, 3D, etc
};


class Field_info_mgt
{
private:
    std::vector<field_attr> fields_attr;
	void add_field_info(const char*, const char*, const char*, const char*, const char*);
    
public:
    Field_info_mgt(const char*, const char*);
    ~Field_info_mgt() {}
    const field_attr* search_field_info(const char*);
    int get_field_data_type_size(const field_attr*);
    int get_field_data_type_size(const char*);
    const char *get_field_data_type(const char*);
	int get_field_num_dims(const char*);
	const char *get_field_long_name(const char*);
	const char *get_field_unit(const char*);
};

#endif
