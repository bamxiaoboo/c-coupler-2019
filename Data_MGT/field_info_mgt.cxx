/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "field_info_mgt.h"
#include "common_utils.h"
#include "cor_global_data.h"
#include <string.h>


#define FIELD_0_DIM       "scalar"
#define FIELD_2_DIM       "2D"
#define FIELD_3_DIM       "3D"
#define FIELD_4_DIM       "4D"
#define FIELD_VECTOR      "vector"


int Field_info_mgt::get_field_num_dims(const char *field_dim, const char *cfg_name)
{
	if (words_are_the_same(field_dim, FIELD_0_DIM))
		return 0;
	if (words_are_the_same(field_dim, FIELD_2_DIM))
		return 2;
	if (words_are_the_same(field_dim, FIELD_3_DIM))
		return 3;
	if (words_are_the_same(field_dim, FIELD_4_DIM))
		return 4;
	if (words_are_the_same(field_dim, FIELD_VECTOR))
		return 10;

	EXECUTION_REPORT(REPORT_ERROR,-1, false, "\"%s\" is an undefined description of the number of dimensions of field. Please verify the configuration file %s", field_dim, cfg_name);
	return -1;
}


const field_attr *Field_info_mgt::search_field_info(const char *field_name)
{
    for (int i = 0; i < fields_attr.size(); i ++)
        if (words_are_the_same(field_name, fields_attr[i].field_name))
            return &fields_attr[i];

    return NULL;
}


void Field_info_mgt::add_field_info(const char *field_name, const char *field_long_name, const char *field_unit, const char *field_dim, const char *field_type)
{
	field_attr local_attr;


	strcpy(local_attr.field_name, field_name);
	strcpy(local_attr.field_long_name, field_long_name);
	strcpy(local_attr.field_unit, field_unit);
	strcpy(local_attr.field_dim, field_dim);
	strcpy(local_attr.field_type, field_type);
	fields_attr.push_back(local_attr);
	EXECUTION_REPORT(REPORT_WARNING, -1, search_field_info(local_attr.field_name) == &(fields_attr[fields_attr.size()-1]), "field %s has been defined more than once\n", local_attr.field_name);
}


Field_info_mgt::Field_info_mgt()
{
    char line[NAME_STR_SIZE*16];
	char file_name[NAME_STR_SIZE];
    FILE *fp_field;
    char *local_line;
    field_attr local_attr;
	int i;
    

	sprintf(file_name, "%s/public_field_attribute.cfg", comp_comm_group_mgt_mgr->get_config_all_dir());

    fp_field = open_config_file(file_name);
	i = 0;
    while (get_next_line(line, fp_field)) {
        local_line = line;
		i ++;
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(local_attr.field_name, &local_line), "Please specify the name of the %dth field in the configuration file %s.", i, file_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(local_attr.field_long_name, &local_line), "Please specify the long name (description) of the %dth field in the configuration file %s.", i, file_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(local_attr.field_unit, &local_line), "Please specify the unit of the %dth field in the configuration file %s.", i, file_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(local_attr.field_dim, &local_line), "Please specify the number of dimensions of the %dth field in the configuration file %s.", i, file_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(local_attr.field_type, &local_line), "Please specify the type (\"state\" or \"flux\") of the %dth field in the configuration file %s.", i, file_name);
        fields_attr.push_back(local_attr);
		EXECUTION_REPORT(REPORT_WARNING, -1, search_field_info(local_attr.field_name) == &(fields_attr[fields_attr.size()-1]), "field %s has been defined has been defined more than once\n", local_attr.field_name);
		get_field_num_dims(local_attr.field_dim, file_name);
    }
	fclose(fp_field);
}


const char *Field_info_mgt::get_field_long_name(const char *field_name)
{
	if (search_field_info(field_name) == NULL)
		return NULL;
	
    return search_field_info(field_name)->field_long_name;
}


const char *Field_info_mgt::get_field_unit(const char *field_name)
{
	if (search_field_info(field_name) == NULL)
		return NULL;

    return search_field_info(field_name)->field_unit;
}

