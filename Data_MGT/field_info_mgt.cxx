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


int Field_info_mgt::get_field_num_dims(const char *field_dim)
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

	EXECUTION_REPORT(REPORT_ERROR, false, "%s is undefined description of the number of dimensions of field\n", field_dim);
	return -1;
}


int Field_info_mgt::get_field_data_type_size(const char *field_name)
{
	return get_field_data_type_size(search_field_info(field_name));
}


int Field_info_mgt::get_field_data_type_size(const field_attr *field_info)
{
	return get_data_type_size(field_info->field_data_type);
}


const field_attr *Field_info_mgt::search_field_info(const char *field_name)
{
    for (int i = 0; i < fields_attr.size(); i ++)
        if (words_are_the_same(field_name, fields_attr[i].field_name))
            return &fields_attr[i];

	EXECUTION_REPORT(REPORT_ERROR, false, "%s is an undefined in fields table\n", field_name);
    return NULL;
}


void Field_info_mgt::add_field_info(const char *field_name, const char *field_long_name, const char *field_unit, 
                                    const char *field_data_type, const char *field_dim)
{
	field_attr local_attr;


	strcpy(local_attr.field_name, field_name);
	strcpy(local_attr.field_long_name, field_long_name);
	strcpy(local_attr.field_unit, field_unit);
	strcpy(local_attr.field_data_type, field_data_type);
	strcpy(local_attr.field_dim, field_dim);
	fields_attr.push_back(local_attr);
	EXECUTION_REPORT(REPORT_WARNING, search_field_info(local_attr.field_name) == &(fields_attr[fields_attr.size()-1]), "field %s has been defined more than once\n", local_attr.field_name);
}


Field_info_mgt::Field_info_mgt(const char *shared_fname, const char *private_fname)
{
    char line[NAME_STR_SIZE*16];
    FILE *fp_field;
    char *local_line;
    field_attr local_attr;
    

	add_field_info("lat", "center latitude of grid cells", "degrees north", "real8", "vector");
	add_field_info("lon", "center longitude of grid cells", "degrees east", "real8", "vector");
	add_field_info("mask", "mask of a domain grid", "unitless", "logical", "vector");
	add_field_info("n_grid_lats", "number of different latitudes of a 2D grid", "unitless", "integer", "scalar");
	add_field_info("n_grid_lons", "number of different longitudes of a 2D grid", "unitless", "integer", "scalar");
	add_field_info("n_grid_levels", "number of levels of a 3D grid", "unitless", "integer", "scalar");
	add_field_info("index", "global index after decomposition of component grid", "unitless", "integer", "vector");
	add_field_info("scalar_int", "scalar integer", "unitless", "integer", "scalar");
	add_field_info("input_orbYear", "year (AD) wrt orbital parameters", "unitless", "integer", "scalar");
	add_field_info("input_orbEccen", "eccen of earth orbit", "unitless", "real8", "scalar");
	add_field_info("input_orbObliq", "earth's obliquity", "degree", "real8", "scalar");
	add_field_info("input_orbObliqr", "earth's obliquity", "rad", "real8", "scalar");
	add_field_info("input_orbMvelp", "mv_ocn_srfing vernel equinox of orbit (degrees)", "degrees", "real8", "scalar");
	add_field_info("input_orbMvelpp", "mv_ocn_srfing vernal equinox longitude of perihelion plus pi", "rad", "real8", "scalar");
	add_field_info("input_orbLambm0", "mean lon perihelion @ vernal eq", "rad", "real8", "scalar");

	if (!words_are_the_same(shared_fname, "NULL")) {
	    fp_field = open_config_file(shared_fname);
	    while (get_next_line(line, fp_field)) {
	        local_line = line;
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_name, &local_line));		
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_long_name, &local_line));
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_unit, &local_line));
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_data_type, &local_line));
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_dim, &local_line));
	        fields_attr.push_back(local_attr);
			EXECUTION_REPORT(REPORT_WARNING, search_field_info(local_attr.field_name) == &(fields_attr[fields_attr.size()-1]), "field %s has been defined has been defined more than once\n", local_attr.field_name);
			get_data_type_size(local_attr.field_data_type);
			get_field_num_dims(local_attr.field_dim);
	    }
		fclose(fp_field);
	}

	if (!words_are_the_same(private_fname, "NULL")) {
	    fp_field = open_config_file(private_fname);
	    while (get_next_line(line, fp_field)) {
	        local_line = line;
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_name, &local_line));		
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_long_name, &local_line));
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_unit, &local_line));
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_data_type, &local_line));
	        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(local_attr.field_dim, &local_line));
	        fields_attr.push_back(local_attr);
			EXECUTION_REPORT(REPORT_ERROR, search_field_info(local_attr.field_name) == &(fields_attr[fields_attr.size()-1]), "field %s has been defined twice in field table\n", local_attr.field_name);
			get_data_type_size(local_attr.field_data_type);
			get_field_num_dims(local_attr.field_dim);
	    }
		fclose(fp_field);
	}
}


const char *Field_info_mgt::get_field_data_type(const char *field_name)
{
    return search_field_info(field_name)->field_data_type;
}


void Field_info_mgt::set_field_data_type(const char *field_name, const char *data_type)
{
    strcpy((char*)(search_field_info(field_name)->field_data_type), data_type);
}


const char *Field_info_mgt::get_field_long_name(const char *field_name)
{
    return search_field_info(field_name)->field_long_name;
}


const char *Field_info_mgt::get_field_unit(const char *field_name)
{
    return search_field_info(field_name)->field_unit;
}

