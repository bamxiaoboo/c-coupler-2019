/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file is initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "io_binary.h"
#include "cor_global_data.h"
#include "remap_operator_conserv_2D.h"
#include "remap_operator_bilinear.h"
#include "remap_operator_distwgt.h"
#include "remap_operator_linear.h"
#include "remap_operator_smooth.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


IO_binary::IO_binary(const char *object_name, const char *file_name, const char *format)
{
    strcpy(this->object_name, object_name);
    strcpy(this->file_type, FILE_TYPE_BINARY);
    strcpy(this->file_name, file_name);
    strcpy(this->open_format, format);
    
    if (words_are_the_same(format, "r"))
        fp_binary = fopen(file_name, "r");
    else if (words_are_the_same(format, "w")) 
        fp_binary = fopen(file_name, "w");       
    else EXECUTION_REPORT(REPORT_ERROR, false, "the format of openning binary file must be read or write (\"r\" or \"w\")\n");

    EXECUTION_REPORT(REPORT_ERROR, fp_binary != NULL, "file %s does not exist\n", file_name);

    fclose(fp_binary);
}


IO_binary::~IO_binary()
{
}


void IO_binary::write_grid(Remap_grid_class *associated_grid, bool write_grid_name)
{
    EXECUTION_REPORT(REPORT_ERROR, false, "remap software error1 in using binary io\n");    
}


void IO_binary::write_grided_data(Remap_grid_data_class *grided_data, bool write_grid_name, int date, int  datesec, bool is_restart_field)
{
    EXECUTION_REPORT(REPORT_ERROR, false, "remap software error2 in using binary io\n");    
}


void IO_binary::write_field_data(Remap_grid_data_class *field_data, 
                                Remap_grid_class *interchange_grid,
                                bool is_grid_data, 
                                const char *grid_field_type, 
                                int dim_ncid_num_vertex,
                                bool write_grid_name)
{
    EXECUTION_REPORT(REPORT_ERROR, false, "remap software error3 in using binary io\n");    
}


void IO_binary::read_data(Remap_data_field *read_data_field)
{
    EXECUTION_REPORT(REPORT_ERROR, false, "remap software error4 in using binary io\n");
}


long IO_binary::get_dimension_size(const char *dim_name)
{
    EXECUTION_REPORT(REPORT_ERROR, false, "can not read size from binary file\n");
    return -1;
}


void IO_binary::write_remap_weights(Remap_weight_of_strategy_class *remap_weights)
{
	char *flat_array;
	long array_size;


    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(open_format, "w"), "can not write to binary file %s: %s, whose open format is not write\n", object_name, file_name);
    EXECUTION_REPORT(REPORT_ERROR, remap_weights != NULL, "remap software error1 in write_remap_weights binary\n");
    
    if (execution_phase_number == 1) {
		remap_weights->write_remap_weights_into_array(&flat_array, array_size, true);
        fp_binary = fopen(file_name, "w+");
		fwrite(flat_array, array_size, 1, fp_binary);
		delete [] flat_array;
        fclose(fp_binary);
    }
}


void IO_binary::read_remap_weights(Remap_weight_of_strategy_class *remap_weights, Remap_strategy_class *remap_strategy)
{    
	long array_size;
	char *input_array;
	

    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(open_format, "r"), "can not read binary file %s: %s, whose open format is not read\n", object_name, file_name);
    EXECUTION_REPORT(REPORT_ERROR, remap_weights != NULL, "remap software error1 in read_remap_weights binary\n");

    if (execution_phase_number == 1) {
        fp_binary = fopen(file_name, "r"); 
		fseek(fp_binary, 0, SEEK_END);
		long array_size = ftell(fp_binary);
		char *input_array = new char [array_size];
		fseek(fp_binary, 0, SEEK_SET);
		fread(input_array, array_size, 1, fp_binary);
		remap_weights->read_remap_weights_from_array(input_array, array_size, true, NULL);
		delete [] input_array;
		fclose(fp_binary);
    }
}


