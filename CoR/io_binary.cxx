/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
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


void IO_binary::write_grid_info(Remap_grid_class *grid, bool consider_area_or_volumn)
{
    long grid_size;
    int grid_num_dimensions, i, id, num_leaf_grids, tmp_int_value;
    Remap_grid_class *leaf_grids[256];
    
    
    grid_size = grid->get_grid_size();
    fwrite(&grid_size, sizeof(long), 1, fp_binary);
    grid_num_dimensions = grid->get_num_dimensions();
    fwrite(&grid_num_dimensions, sizeof(int), 1, fp_binary);
    grid->get_leaf_grids(&num_leaf_grids, leaf_grids, grid);
    for (i = 0; i < num_leaf_grids; i ++) {
        if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LON))
            id = 1;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LAT))
            id = 2;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LEV))
            id = 3;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_TIME))
            id = 4;
        fwrite(&id, sizeof(int), 1, fp_binary);
    }

    if (consider_area_or_volumn) {
        if (grid->get_area_or_volumn() != NULL) {
            tmp_int_value = 1;
            fwrite(&tmp_int_value, sizeof(int), 1, fp_binary);
            fwrite(grid->get_area_or_volumn(), sizeof(double), grid->get_grid_size(), fp_binary);
        }
        else {
            tmp_int_value = 0;
            fwrite(&tmp_int_value, sizeof(int), 1, fp_binary);
        }
    }
}

void IO_binary::write_remap_weights(Remap_weight_of_strategy_class *remap_weights)
{
    Remap_grid_class *remap_grid_src, *remap_grid_dst;
    Remap_grid_class *leaf_grids[256];
    long grid_size, tmp_long_value;
    int num_leaf_grids, i, j, id, grid_num_dimensions, tmp_int_value;
    int num_remap_operator_instances;
    Remap_operator_basis *remap_operator_of_one_instance;
    Remap_weight_of_operator_class *remap_weight_of_operator;
    Remap_weight_sparse_matrix *remap_weights_group;
    char operator_name[256];


    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(open_format, "w"), "can not write to binary file %s: %s, whose open format is not write\n", object_name, file_name);
    EXECUTION_REPORT(REPORT_ERROR, remap_weights != NULL, "remap software error1 in write_remap_weights binary\n");

    remap_grid_src = remap_weights->get_data_grid_src();
    remap_grid_dst = remap_weights->get_data_grid_dst();
    
    if (execution_phase_number == 1) {
        fp_binary = fopen(file_name, "w+");
        write_grid_info(remap_grid_src, true);
        write_grid_info(remap_grid_dst, true);    
        num_remap_operator_instances = remap_weights->get_num_remap_operator_of_weights();
        fwrite(&num_remap_operator_instances, sizeof(int), 1, fp_binary);
        for (i = 0; i < num_remap_operator_instances; i ++) {
            remap_weight_of_operator = remap_weights->get_remap_weight_of_operator(i);
            tmp_long_value = remap_weight_of_operator->get_remap_iter();
            fwrite(&tmp_long_value, sizeof(long), 1, fp_binary);
            remap_operator_of_one_instance = remap_weights->get_remap_operator_of_weights(i);
            memset(operator_name, 0, 256);
            strcpy(operator_name, remap_operator_of_one_instance->get_operator_name());
            fwrite(operator_name, sizeof(char), 256, fp_binary);
            write_grid_info(remap_weight_of_operator->get_field_data_grid_src(), false);
            write_grid_info(remap_weight_of_operator->get_field_data_grid_dst(), false);
            write_grid_info(remap_operator_of_one_instance->get_src_grid(), false);
            write_grid_info(remap_operator_of_one_instance->get_dst_grid(), false);
            tmp_int_value = remap_operator_of_one_instance->get_num_remap_weights_groups();
            fwrite(&tmp_int_value, sizeof(int), 1, fp_binary);
            for (j = 0; j < remap_operator_of_one_instance->get_num_remap_weights_groups(); j ++) {
                remap_weights_group = remap_operator_of_one_instance->get_remap_weights_group(j);
                tmp_long_value = remap_weights_group->get_num_weights();
                fwrite(&tmp_long_value, sizeof(long), 1, fp_binary);
                fwrite(remap_weights_group->get_indexes_src_grid(), sizeof(long), tmp_long_value, fp_binary);
                fwrite(remap_weights_group->get_indexes_dst_grid(), sizeof(long), tmp_long_value, fp_binary);
                fwrite(remap_weights_group->get_weight_values(), sizeof(double), tmp_long_value, fp_binary);
                tmp_long_value = remap_weights_group->get_num_remaped_dst_cells_indexes();
                fwrite(&tmp_long_value, sizeof(long), 1, fp_binary);
                fwrite(remap_weights_group->get_remaped_dst_cells_indexes(), sizeof(long), tmp_long_value, fp_binary);
            }
        }
        fclose(fp_binary);
    }
}


void IO_binary::read_grid_info(Remap_grid_class *grid, bool consider_area_or_volumn)
{
    long grid_size;
    int grid_num_dimensions, i, gid, rid, num_leaf_grids, tmp_int_value;
    Remap_grid_class *leaf_grids[256];
    double *area_or_volumn;
    
    
    fread(&grid_size, sizeof(long), 1, fp_binary);
    EXECUTION_REPORT(REPORT_ERROR, grid_size == grid->get_grid_size(), "the grid size of %s does not match the binary file\n", grid->get_grid_name());
    fread(&grid_num_dimensions, sizeof(int), 1, fp_binary);
    EXECUTION_REPORT(REPORT_ERROR, grid_num_dimensions == grid->get_num_dimensions(), "the number of dimensions of grid %s does not match the binary file\n", grid->get_grid_name());
    grid->get_leaf_grids(&num_leaf_grids, leaf_grids, grid);
    for (i = 0; i < num_leaf_grids; i ++) {
        if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LON))
            gid = 1;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LAT))
            gid = 2;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LEV))
            gid = 3;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_TIME))
            gid = 4;
        fread(&rid, sizeof(int), 1, fp_binary);
        EXECUTION_REPORT(REPORT_ERROR, gid == rid, "the arrange of coordinate systems of grid %s does not match the binary file\n", grid->get_grid_name());
    }

    if (consider_area_or_volumn) {
        fread(&tmp_int_value, sizeof(int), 1, fp_binary);
        if (tmp_int_value == 1) {
            EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn() != NULL, "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
            area_or_volumn = new double [grid->get_grid_size()];
            fread(area_or_volumn, sizeof(double), grid->get_grid_size(), fp_binary);
            for (long i = 0; i < grid->get_grid_size(); i ++)
                EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn()[i] == area_or_volumn[i], "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
            delete [] area_or_volumn;            
        }
        else EXECUTION_REPORT(REPORT_ERROR, grid->get_area_or_volumn() == NULL, "the area or volumn of grid %s does not match the binary file\n", grid->get_grid_name());
    }
}


void IO_binary::read_remap_operator_instance(Remap_weight_of_strategy_class *remap_weights, 
                                             Remap_grid_class *field_data_grid_src, Remap_grid_class *field_data_grid_dst,
                                             Remap_operator_basis *remap_operator,
                                             long remap_iter)    
{
    Remap_operator_basis *duplicated_remap_operator;
    Remap_grid_class *remap_grids[2];
    Remap_weight_of_operator_class *remap_operator_instance;
    int i, num_remap_weights_groups;
    long num_weights, num_remaped_dst_cells_indexes, *indexes_src_grid, *indexes_dst_grid, *remaped_dst_cells_indexes;
    Remap_weight_sparse_matrix *weight_sparse_matrix;
    double *weight_values;


    remap_grids[0] = remap_operator->get_src_grid();
    remap_grids[1] = remap_operator->get_dst_grid();
    duplicated_remap_operator = remap_operator->duplicate_remap_operator(false);
    fread(&num_remap_weights_groups, sizeof(int), 1, fp_binary);
    for (i = 0; i < num_remap_weights_groups; i ++) {
        fread(&num_weights, sizeof(long), 1, fp_binary);
        indexes_src_grid = new long [num_weights];
        indexes_dst_grid = new long [num_weights];
        weight_values = new double [num_weights];
        fread(indexes_src_grid, sizeof(long), num_weights, fp_binary);
        fread(indexes_dst_grid, sizeof(long), num_weights, fp_binary);
        fread(weight_values, sizeof(double), num_weights, fp_binary);
        fread(&num_remaped_dst_cells_indexes, sizeof(long), 1, fp_binary);
        remaped_dst_cells_indexes = new long [num_remaped_dst_cells_indexes];
        fread(remaped_dst_cells_indexes, sizeof(long), num_remaped_dst_cells_indexes, fp_binary);
        weight_sparse_matrix = new Remap_weight_sparse_matrix(remap_operator, num_weights, indexes_src_grid, indexes_dst_grid, weight_values, num_remaped_dst_cells_indexes, remaped_dst_cells_indexes);
        duplicated_remap_operator->add_weight_sparse_matrix(weight_sparse_matrix);
    }
    
    remap_operator_instance = new Remap_weight_of_operator_class(field_data_grid_src, field_data_grid_dst, remap_iter, remap_operator, duplicated_remap_operator);
    remap_weights->add_weight_of_operator_class(remap_operator_instance);
}


void IO_binary::read_remap_weights(Remap_weight_of_strategy_class *remap_weights, Remap_strategy_class *remap_strategy)
{
    Remap_grid_class *field_grid_src, *field_grid_dst, *current_field_grid_src, *current_field_grid_dst;
    Remap_grid_class *leaf_grids_all[256], *leaf_grids_all_sorted[256];
    Remap_grid_class *leaf_grids_remap_operator_src[256], *leaf_grids_remap_operator_dst[256];
    Remap_grid_class *sized_sub_grids[256];
    int i, j, k, num_remap_operator_instances, num_remap_operator, num_leaf_grids_all, num_leaf_grids_remap_operator;
    Remap_operator_basis *remap_operator;
    int coord_system_ids[256], tmp_grid_num_dimensions, num_sized_sub_grids;
    long tmp_grid_size, current_remap_iter, last_remap_iter;
    char operator_name[256], tmp_grid_name[256];
    

    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(open_format, "r"), "can not read binary file %s: %s, whose open format is not read\n", object_name, file_name);
    EXECUTION_REPORT(REPORT_ERROR, remap_weights != NULL, "remap software error1 in read_remap_weights binary\n");

    field_grid_src = remap_weights->get_data_grid_src();
    field_grid_dst = remap_weights->get_data_grid_dst();


    if (execution_phase_number == 1) {
        fp_binary = fopen(file_name, "r"); 

        read_grid_info(field_grid_src, true);
        read_grid_info(field_grid_dst, true);

        fread(&num_remap_operator_instances, sizeof(int), 1, fp_binary);
        num_remap_operator = 0;
        current_field_grid_src = field_grid_src;
        last_remap_iter = -1;
        for (i = 0; i < num_remap_operator_instances; i ++) {
            fread(&current_remap_iter, sizeof(long), 1, fp_binary);
            if (current_remap_iter <= last_remap_iter) {
                num_remap_operator ++;
                current_field_grid_src = current_field_grid_dst;
            }
            last_remap_iter = current_remap_iter;
            remap_operator = remap_strategy->get_remap_operator(num_remap_operator);
            fread(operator_name, sizeof(char), 256, fp_binary);
            EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(operator_name, remap_operator->get_operator_name()),
                         "the remap operator %s does match the binary file, which should be %s\n", 
                         remap_operator->get_operator_name(), operator_name);
            fread(&tmp_grid_size, sizeof(long), 1, fp_binary);
            fread(&tmp_grid_num_dimensions, sizeof(int), 1, fp_binary);
            EXECUTION_REPORT(REPORT_ERROR, tmp_grid_num_dimensions == field_grid_src->get_num_dimensions(), "remap software error2 in read_remap_weights binary\n");
            for (j = 0; j < tmp_grid_num_dimensions; j ++)
                fread(&coord_system_ids[j], sizeof(int), 1, fp_binary);
            current_field_grid_src->get_leaf_grids(&num_leaf_grids_all, leaf_grids_all, current_field_grid_src);
            for (j = 0; j < tmp_grid_num_dimensions; j ++) {
                for (k = 0; k < tmp_grid_num_dimensions; k ++) {
                    if (words_are_the_same(leaf_grids_all[k]->get_coord_label(), COORD_LABEL_LON) && coord_system_ids[j] == 1)
                        break;
                    else if (words_are_the_same(leaf_grids_all[k]->get_coord_label(), COORD_LABEL_LAT) && coord_system_ids[j] == 2) 
                        break;
                    else if (words_are_the_same(leaf_grids_all[k]->get_coord_label(), COORD_LABEL_LEV) && coord_system_ids[j] == 3)
                        break;
                    else if (words_are_the_same(leaf_grids_all[k]->get_coord_label(), COORD_LABEL_TIME) && coord_system_ids[j] == 4)
                        break;
                }
                EXECUTION_REPORT(REPORT_ERROR, k < tmp_grid_num_dimensions, "remap software error3 in read_remap_weights binary\n");
                leaf_grids_all_sorted[j] = leaf_grids_all[k];
            }
            num_sized_sub_grids = 0;
            for (j = 0; j < tmp_grid_num_dimensions; j ++) {
                if (num_sized_sub_grids > 0 && leaf_grids_all_sorted[j]->is_subset_of_grid(sized_sub_grids[num_sized_sub_grids-1]))
                    continue;
                sized_sub_grids[num_sized_sub_grids++] = leaf_grids_all_sorted[j]->get_first_super_grid_of_enable_setting_coord_value();
            }
			current_field_grid_src = remap_grid_manager->search_remap_grid_with_sized_sub_grids(num_sized_sub_grids, sized_sub_grids);
			if (current_field_grid_src == NULL) {
				sprintf(tmp_grid_name, "TMP_GRID_SRC");
				for (j = 0; j < num_sized_sub_grids; j ++)
					sprintf(tmp_grid_name, "%s_%s", tmp_grid_name, sized_sub_grids[j]->get_grid_name());
            	current_field_grid_src = new Remap_grid_class(tmp_grid_name, num_sized_sub_grids, sized_sub_grids, 0);
				remap_grid_manager->add_remap_grid(current_field_grid_src);
			}
            EXECUTION_REPORT(REPORT_ERROR, tmp_grid_size == current_field_grid_src->get_grid_size(), 
                         "the src field grid size of remap operator %s does match the binary file\n",
                         remap_operator->get_object_name());
            remap_operator->get_src_grid()->get_leaf_grids(&num_leaf_grids_remap_operator, leaf_grids_remap_operator_src, remap_operator->get_src_grid());
            remap_operator->get_dst_grid()->get_leaf_grids(&num_leaf_grids_remap_operator, leaf_grids_remap_operator_dst, remap_operator->get_dst_grid());
            for (j = 0; j < num_leaf_grids_remap_operator; j ++)
                for (k = 0; k < num_leaf_grids_all; k ++)
                    if (leaf_grids_all_sorted[k] == leaf_grids_remap_operator_src[j])
                        leaf_grids_all_sorted[k] = leaf_grids_remap_operator_dst[j];
            num_sized_sub_grids = 0;
            for (j = 0; j < tmp_grid_num_dimensions; j ++) {
                if (num_sized_sub_grids > 0 && leaf_grids_all_sorted[j]->is_subset_of_grid(sized_sub_grids[num_sized_sub_grids-1]))
                    continue;
                sized_sub_grids[num_sized_sub_grids++] = leaf_grids_all_sorted[j]->get_first_super_grid_of_enable_setting_coord_value();
            }
			current_field_grid_dst = remap_grid_manager->search_remap_grid_with_sized_sub_grids(num_sized_sub_grids, sized_sub_grids);
			if (current_field_grid_dst == NULL) {
				sprintf(tmp_grid_name, "TMP_GRID_DST");
				for (j = 0; j < num_sized_sub_grids; j ++)
					sprintf(tmp_grid_name, "%s_%s", tmp_grid_name, sized_sub_grids[j]->get_grid_name());
            	current_field_grid_dst = new Remap_grid_class(tmp_grid_name, num_sized_sub_grids, sized_sub_grids, 0);
				remap_grid_manager->add_remap_grid(current_field_grid_dst);
			}
            read_grid_info(current_field_grid_dst, false);
            read_grid_info(remap_operator->get_src_grid(), false);
            read_grid_info(remap_operator->get_dst_grid(), false);
            read_remap_operator_instance(remap_weights, current_field_grid_src, current_field_grid_dst, remap_operator, current_remap_iter);
        }
        EXECUTION_REPORT(REPORT_ERROR, current_field_grid_dst->is_similar_grid_with(field_grid_dst), "remap software error4 in read_remap_weights\n");
    }
}


