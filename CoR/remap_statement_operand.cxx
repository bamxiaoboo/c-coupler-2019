/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "remap_statement_operand.h"
#include "cor_global_data.h"
#include <string.h>


template <class T> void initialize_data_buf_to_fill_value(T *data_buf, long size, T fill_value)
{
    for (long i = 0; i < size; i ++)
        data_buf[i] = fill_value;
}


long get_interchange_index(long index_iter_src,
                     long index_iter_interchange,
                     long *sub_grid_sizes_src, 
                     long *sub_grid_sizes_interchange, 
                     long *sub_grid_indexes_src,
                     long *sub_grid_indexes_interchange,
                     int *index_interchange_table, 
                     int num_sub_grids)
{
    int i;
    long iter;
    static long interchange_first_dim_total_index_base;

    if (index_iter_src == 0) {
        interchange_first_dim_total_index_base = 1;
        for (i = 0; i < index_interchange_table[0]; i ++)
            interchange_first_dim_total_index_base *= sub_grid_sizes_interchange[i];
        index_iter_interchange = -interchange_first_dim_total_index_base;
    }

    if (sub_grid_indexes_src[0] != 0)
        index_iter_interchange += interchange_first_dim_total_index_base;
    else
        for (iter = 1, index_iter_interchange = 0, i = 0; i < num_sub_grids; i ++) {
            index_iter_interchange += sub_grid_indexes_interchange[i]*iter;
            iter *= sub_grid_sizes_interchange[i];
        }

    for (i = 0; i < num_sub_grids; i ++) {
        sub_grid_indexes_src[i] ++;
        sub_grid_indexes_interchange[index_interchange_table[i]] ++;
        if (sub_grid_indexes_src[i] == sub_grid_sizes_src[i]) {
            sub_grid_indexes_src[i] = 0;
            sub_grid_indexes_interchange[index_interchange_table[i]] = 0;
        }
        else break;
    }

    return index_iter_interchange;
}


template <class T> void interchange_array_data(int num_sized_sub_grids_src,
                                                    long *sub_grid_indexes_src,
                                                    long *sub_grid_indexes_interchange,
                                                    long *sub_grid_sizes_src,
                                                    long *sub_grid_sizes_interchange,
                                                    int *index_interchange_table,
                                                    T *data_src,
                                                    T *data_interchange,
                                                    long array_size,
                                                    int num_point_per_cell)
{
    int i;
    long index_iter_src, index_iter_interchange;


    for (i = 0; i < num_sized_sub_grids_src; i ++) {
        sub_grid_indexes_src[i] = 0;
        sub_grid_indexes_interchange[i] = 0;
    }
    for (index_iter_src = 0, index_iter_interchange = 0; index_iter_src < array_size; index_iter_src ++) {
        index_iter_interchange = get_interchange_index(index_iter_src,
                                                       index_iter_interchange,
                                                       sub_grid_sizes_src, 
                                                       sub_grid_sizes_interchange, 
                                                       sub_grid_indexes_src,
                                                       sub_grid_indexes_interchange,
                                                       index_interchange_table, 
                                                       num_sized_sub_grids_src);
        for (i = 0; i < num_point_per_cell; i ++)
            data_interchange[index_iter_interchange*num_point_per_cell+i] = data_src[index_iter_src*num_point_per_cell+i];
    }

    for (i = 0; i < num_sized_sub_grids_src; i ++)
        EXECUTION_REPORT(REPORT_ERROR, sub_grid_indexes_src[i] == 0 && sub_grid_indexes_interchange[i] == 0,
                     "remap software error2 when interchanging field data\n");
}


Remap_data_field::Remap_data_field()
{
    data_buf = NULL;
    field_name_in_application[0] = '\0';
    field_name_in_IO_file[0] = '\0';
    data_type_in_application[0] = '\0';
    data_type_in_IO_file[0] = '\0';
    required_data_size = 0;
    read_data_size = 0;
    have_fill_value = false;
    fill_value = 0;
}


Remap_data_field *Remap_data_field::duplicate_remap_data_field(long data_size, bool copy_data)
{
    Remap_data_field *duplicated_data_field;
    

    duplicated_data_field = new Remap_data_field;
    strcpy(duplicated_data_field->field_name_in_application, this->field_name_in_application);
    strcpy(duplicated_data_field->data_type_in_application, this->data_type_in_application);
    strcpy(duplicated_data_field->field_name_in_IO_file, this->field_name_in_IO_file);
    strcpy(duplicated_data_field->data_type_in_IO_file, this->data_type_in_IO_file);
    duplicated_data_field->read_data_size = this->read_data_size;
    duplicated_data_field->required_data_size = this->required_data_size;
    if (data_size != 0) {
        duplicated_data_field->read_data_size = data_size;
        duplicated_data_field->required_data_size = data_size;
    }
    duplicated_data_field->data_buf = new char [duplicated_data_field->required_data_size*get_data_type_size(duplicated_data_field->data_type_in_application)];
    if (copy_data) {
        EXECUTION_REPORT(REPORT_ERROR, this->required_data_size == duplicated_data_field->required_data_size, "remap software error in duplicate_remap_data_field\n");
        memcpy(duplicated_data_field->data_buf, this->data_buf, duplicated_data_field->required_data_size*get_data_type_size(duplicated_data_field->data_type_in_application));
    }
    else memset(duplicated_data_field->data_buf, 0, duplicated_data_field->required_data_size*get_data_type_size(duplicated_data_field->data_type_in_application));
    for (int i = 0; i < this->field_attributes.size(); i ++)
        duplicated_data_field->field_attributes.push_back(this->field_attributes[i]);

    return duplicated_data_field;
}


void Remap_data_field::interchange_remap_data_field(Remap_data_field *field_data_interchanged, Remap_grid_class *grid_src, Remap_grid_class *grid_interchange)    
{
    int num_sized_sub_grids_src, num_sized_sub_grids_interchange;
    Remap_grid_class *sized_sub_grids_src[256], *sized_sub_grids_interchange[256]; 
    long sub_grid_sizes_src[256], sub_grid_sizes_interchange[256];
    int index_interchange_table[256];
    long sub_grid_indexes_src[256], sub_grid_indexes_interchange[256];
    int i, j;
    int num_point_per_cell;


    EXECUTION_REPORT(REPORT_ERROR, this->required_data_size % grid_src->get_grid_size() == 0,
                 "remap software error in interchange_remap_data_field\n");

    grid_src->get_sized_sub_grids(&num_sized_sub_grids_src, sized_sub_grids_src);
    grid_interchange->get_sized_sub_grids(&num_sized_sub_grids_interchange, sized_sub_grids_interchange);
    grid_src->get_grid_index_interchange_table(grid_interchange, index_interchange_table);
    num_point_per_cell = this->required_data_size / grid_src->get_grid_size();
    
    for (i = 0; i < num_sized_sub_grids_src; i ++)
        sub_grid_sizes_src[i] = sized_sub_grids_src[i]->get_grid_size();
    for (i = 0; i < num_sized_sub_grids_interchange; i ++)
        sub_grid_sizes_interchange[i] = sized_sub_grids_interchange[i]->get_grid_size();

    if (words_are_the_same(this->data_type_in_application, DATA_TYPE_DOUBLE) ||
		words_are_the_same(this->data_type_in_application, DATA_TYPE_LONG))
        interchange_array_data(num_sized_sub_grids_src, 
                           sub_grid_indexes_src, 
                           sub_grid_indexes_interchange, 
                           sub_grid_sizes_src, 
                           sub_grid_sizes_interchange, 
                           index_interchange_table,
                           (double*) this->data_buf,
                           (double*) field_data_interchanged->data_buf,
                           grid_src->get_grid_size(),
                           num_point_per_cell);
    else if (words_are_the_same(this->data_type_in_application, DATA_TYPE_FLOAT) ||
             words_are_the_same(this->data_type_in_application, DATA_TYPE_INT))
        interchange_array_data(num_sized_sub_grids_src, 
                           sub_grid_indexes_src, 
                           sub_grid_indexes_interchange, 
                           sub_grid_sizes_src, 
                           sub_grid_sizes_interchange, 
                           index_interchange_table,
                           (int*) this->data_buf,
                           (int*) field_data_interchanged->data_buf,
                           grid_src->get_grid_size(),
                           num_point_per_cell);
    else if (words_are_the_same(this->data_type_in_application, DATA_TYPE_BOOL) ||
             words_are_the_same(this->data_type_in_application, DATA_TYPE_CHAR))
        interchange_array_data(num_sized_sub_grids_src, 
                           sub_grid_indexes_src, 
                           sub_grid_indexes_interchange, 
                           sub_grid_sizes_src, 
                           sub_grid_sizes_interchange, 
                           index_interchange_table,
                           (char*) this->data_buf,
                           (char*) field_data_interchanged->data_buf,
                           grid_src->get_grid_size(),
                           num_point_per_cell);
    else if (words_are_the_same(this->data_type_in_application, DATA_TYPE_SHORT))
        interchange_array_data(num_sized_sub_grids_src, 
                           sub_grid_indexes_src, 
                           sub_grid_indexes_interchange, 
                           sub_grid_sizes_src, 
                           sub_grid_sizes_interchange, 
                           index_interchange_table,
                           (short*) this->data_buf,
                           (short*) field_data_interchanged->data_buf,
                           grid_src->get_grid_size(),
                           num_point_per_cell);
    else EXECUTION_REPORT(REPORT_ERROR, false, "remap software error in interchange_remap_data_field\n");
}


Remap_data_field::~Remap_data_field()
{
    if (data_buf != NULL)
        delete [] data_buf;
}


void Remap_data_field::read_fill_value()
{
    for (int i = 0; i < field_attributes.size(); i ++)
        if (words_are_the_same(field_attributes[i].attribute_name, FILL_VALUE_LABEL) || words_are_the_same(field_attributes[i].attribute_name, MISS_VALUE_LABEL)) {
            have_fill_value = true;
            if (words_are_the_same(field_attributes[i].attribute_type, DATA_TYPE_FLOAT)) 
                fill_value = (double) ((float*)field_attributes[i].attribute_value)[0];
            else if (words_are_the_same(field_attributes[i].attribute_type, DATA_TYPE_DOUBLE)) 
                fill_value = ((double*) field_attributes[i].attribute_value)[0];
            else if (words_are_the_same(field_attributes[i].attribute_type, DATA_TYPE_INT)) 
                fill_value = (double) ((int*) field_attributes[i].attribute_value)[0];
            else if (words_are_the_same(field_attributes[i].attribute_type, DATA_TYPE_SHORT)) 
                fill_value = (double) ((short*) field_attributes[i].attribute_value)[0];
            else if (words_are_the_same(field_attributes[i].attribute_type, DATA_TYPE_BOOL)) 
                fill_value = (double) ((bool*) field_attributes[i].attribute_value)[0];
            else EXECUTION_REPORT(REPORT_ERROR, false, "the fill value of \"%s\" must be a float, double, int or short value, %s\n", field_name_in_application, field_attributes[i].attribute_type);
        }
}


void Remap_data_field::clean_fill_value()
{
    for (int i = 0; i < field_attributes.size(); i ++)
        if (words_are_the_same(field_attributes[i].attribute_name, FILL_VALUE_LABEL))
            field_attributes.erase(field_attributes.begin()+i);
    for (int i = 0; i < field_attributes.size(); i ++)
        if (words_are_the_same(field_attributes[i].attribute_name, MISS_VALUE_LABEL))
            field_attributes.erase(field_attributes.begin()+i);
}


void Remap_data_field::set_fill_value(void *given_fill_value)
{
    Remap_field_attribute remap_field_attribute;
    char attribute_value[256];


    read_fill_value();

    if (given_fill_value != NULL) {
        EXECUTION_REPORT(REPORT_ERROR, have_fill_value, "remap software error1 in set_fill_value\n");
        if (words_are_the_same(data_type_in_application, DATA_TYPE_BOOL))
            fill_value = (double)(((bool*)given_fill_value)[0]);
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_LONG))
            fill_value = (double)(((long*)given_fill_value)[0]);
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_INT))
            fill_value = (double)(((int*)given_fill_value)[0]);
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_SHORT))
            fill_value = (double)(((short*)given_fill_value)[0]);
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_FLOAT))
            fill_value = (double)(((float*)given_fill_value)[0]);
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_DOUBLE))
            fill_value = (double)(((double*)given_fill_value)[0]);
        else EXECUTION_REPORT(REPORT_ERROR, false, "remap software error1 in datatype_from_application_to_netcdf in set_fill_value \"%s\"\n", data_type_in_application);  
    }

    if (have_fill_value) {
        clean_fill_value();
        if (words_are_the_same(data_type_in_application, DATA_TYPE_BOOL))
            *((bool*) attribute_value) = (bool) fill_value;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_LONG))
            *((long*) attribute_value) = (long) fill_value;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_INT))
            *((int*) attribute_value) = (int) fill_value;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_SHORT))
            *((short*) attribute_value) = (short) fill_value;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_FLOAT))
            *((float*) attribute_value) = (float) fill_value;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_DOUBLE))
            *((double*) attribute_value) = (double) fill_value;
        else EXECUTION_REPORT(REPORT_ERROR, false, "remap software error2 in datatype_from_application_to_netcdf in set_fill_value \"%s\"\n", data_type_in_application);
    }
    else {
        if (words_are_the_same(data_type_in_application, DATA_TYPE_BOOL))
            *((bool*) attribute_value) = false;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_LONG))
            *((long*) attribute_value) = (long) 0x7FFFFFFFFFFFFFFF;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_INT))
            *((int*) attribute_value) = 0x7FFFFFFF;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_SHORT))
            *((short*) attribute_value) = 0x7FFF;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_FLOAT))
            *((float*) attribute_value) = DEFAULT_FILL_VALUE;
        else if (words_are_the_same(data_type_in_application, DATA_TYPE_DOUBLE))
            *((double*) attribute_value) = DEFAULT_FILL_VALUE;
        else EXECUTION_REPORT(REPORT_ERROR, false, "remap software error2 in datatype_from_application_to_netcdf in set_fill_value \"%s\"\n", data_type_in_application);
    }
	
    strcpy(remap_field_attribute.attribute_name, FILL_VALUE_LABEL);
    strcpy(remap_field_attribute.attribute_type, data_type_in_application);
    remap_field_attribute.attribute_size = 1;
    strncpy(remap_field_attribute.attribute_value, attribute_value, 256);
    field_attributes.push_back(remap_field_attribute);
    strcpy(remap_field_attribute.attribute_name, MISS_VALUE_LABEL);
    field_attributes.push_back(remap_field_attribute);
}


void Remap_data_field::initialize_to_fill_value()
{
    read_fill_value();
    if (!have_fill_value)
        return;

    if (words_are_the_same(data_type_in_application, DATA_TYPE_INT))
        initialize_data_buf_to_fill_value((int*) data_buf, required_data_size, (int) fill_value);
    else if (words_are_the_same(data_type_in_application, DATA_TYPE_SHORT))
        initialize_data_buf_to_fill_value((short*) data_buf, required_data_size, (short) fill_value);
    else if (words_are_the_same(data_type_in_application, DATA_TYPE_FLOAT))
        initialize_data_buf_to_fill_value((float*) data_buf, required_data_size, (float) fill_value);
    else if (words_are_the_same(data_type_in_application, DATA_TYPE_DOUBLE))
        initialize_data_buf_to_fill_value((double*) data_buf, required_data_size, (double) fill_value);
    else if (words_are_the_same(data_type_in_application, DATA_TYPE_BOOL))
        initialize_data_buf_to_fill_value((bool*) data_buf, required_data_size, (bool) fill_value);
    else EXECUTION_REPORT(REPORT_ERROR, false, "remap software error2 in initialize_to_fill_value \"%s\"\n", data_type_in_application);
}


void Remap_data_field::set_field_long_name(const char *long_name)
{
    Remap_field_attribute remap_field_attribute;


    strcpy(remap_field_attribute.attribute_name, "long_name");
    strcpy(remap_field_attribute.attribute_type, DATA_TYPE_CHAR);
    strcpy(remap_field_attribute.attribute_value, long_name);
    remap_field_attribute.attribute_size = strlen(long_name);
    field_attributes.push_back(remap_field_attribute);
}


void Remap_data_field::set_field_unit(const char *unit)
{
    Remap_field_attribute remap_field_attribute;


    strcpy(remap_field_attribute.attribute_name, "units");
    strcpy(remap_field_attribute.attribute_type, DATA_TYPE_CHAR);
    strcpy(remap_field_attribute.attribute_value, unit);
    remap_field_attribute.attribute_size = strlen(unit);
    field_attributes.push_back(remap_field_attribute);
}


void Remap_data_field::set_scale_factor_and_add_offset(double scale_factor, double add_offset)
{
    Remap_field_attribute remap_field_attribute;


    strcpy(remap_field_attribute.attribute_name, "scale_factor");
    strcpy(remap_field_attribute.attribute_type, DATA_TYPE_DOUBLE);
    remap_field_attribute.attribute_size = 1;
    *((double*)remap_field_attribute.attribute_value) = scale_factor;
    field_attributes.push_back(remap_field_attribute);
    strcpy(remap_field_attribute.attribute_name, "add_offset");
    strcpy(remap_field_attribute.attribute_type, DATA_TYPE_DOUBLE);
    remap_field_attribute.attribute_size = 1;
    *((double*)remap_field_attribute.attribute_value) = add_offset;
    field_attributes.push_back(remap_field_attribute);
}


void Remap_data_field::read_scale_factor_and_add_offset(double *scale_factor, double *add_offset)
{
    int i;


    for (i = 0; i < field_attributes.size(); i ++)
        if (words_are_the_same(field_attributes[i].attribute_name, "scale_factor"))
            break;
    EXECUTION_REPORT(REPORT_ERROR, i < field_attributes.size(), "scale factor is not set in field %s\n", field_name_in_application);
    *scale_factor = ((double*)field_attributes[i].attribute_value)[0];

    for (i = 0; i < field_attributes.size(); i ++)
        if (words_are_the_same(field_attributes[i].attribute_name, "add_offset"))
            break;
    EXECUTION_REPORT(REPORT_ERROR, i < field_attributes.size(), "add offset is not set in field %s\n", field_name_in_application);
    *add_offset = ((double*)field_attributes[i].attribute_value)[0];
}


void Remap_data_field::clean_scale_factor_and_add_offset_info()
{
    for (int i = 0; i < field_attributes.size(); i ++)
        if (words_are_the_same(field_attributes[i].attribute_name, "scale_factor"))
            field_attributes.erase(field_attributes.begin()+i);
    for (int i = 0; i < field_attributes.size(); i ++)
        if (words_are_the_same(field_attributes[i].attribute_name, "add_offset"))
            field_attributes.erase(field_attributes.begin()+i);
}
