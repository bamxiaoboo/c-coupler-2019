/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include "fields_gather_scatter_mgt.h"
#include "global_data.h"


template <class T> void Gather_scatter_rearrange_info::rearrange_gather_data(T *buf_src, T *buf_dst, int num_cells_in_fully_decomp)
{
    long i, j, k, m, num_data_in_each_level, num_local_cells, rearrange_indexes_start;
    T *tmp_buf_src, *tmp_buf_dst;


    for (m = 0; m < num_local_procs; m ++) {
		num_data_in_each_level = counts[m] / num_levels;
		num_local_cells = num_data_in_each_level / num_points_in_each_cell;
		rearrange_indexes_start = displs[m] / num_levels / num_points_in_each_cell;
		for (k = 0; k < num_levels; k ++) 
			for (i = 0; i < num_local_cells; i ++) {
				if (rearrange_indexes[rearrange_indexes_start+i] < 0)
					continue;
				tmp_buf_src = buf_src + displs[m] + k*num_data_in_each_level + i*num_points_in_each_cell;
				tmp_buf_dst = buf_dst + rearrange_indexes[rearrange_indexes_start+i]*num_points_in_each_cell + k*num_cells_in_fully_decomp*num_points_in_each_cell;
				for (j = 0; j < num_points_in_each_cell; j ++)
					tmp_buf_dst[j] = tmp_buf_src[j];
			}
    }
}


template <class T> void Gather_scatter_rearrange_info::rearrange_scatter_data(T *buf_src, T *buf_dst, int num_cells_in_fully_decomp)
{
    long i, j, k, m, num_data_in_each_level, num_local_cells, rearrange_indexes_start;
    T *tmp_buf_src, *tmp_buf_dst;


    for (m = 0; m < num_local_procs; m ++) {
		num_data_in_each_level = counts[m] / num_levels;
		num_local_cells = num_data_in_each_level / num_points_in_each_cell;
		rearrange_indexes_start = displs[m] / num_levels / num_points_in_each_cell;
		for (k = 0; k < num_levels; k ++)
			for (i = 0; i < num_local_cells; i ++) {
				if (rearrange_indexes[rearrange_indexes_start+i] < 0)
					continue;
				tmp_buf_src = buf_src + rearrange_indexes[rearrange_indexes_start+i]*num_points_in_each_cell + k*num_cells_in_fully_decomp*num_points_in_each_cell; 
                tmp_buf_dst = buf_dst + displs[m] + k*num_data_in_each_level + i*num_points_in_each_cell;
				for (j = 0; j < num_points_in_each_cell; j ++)
					tmp_buf_dst[j] = tmp_buf_src[j];
			}
    }  
}


Gather_scatter_rearrange_info::Gather_scatter_rearrange_info(Field_mem_info *local_field)
{
    int num_local_cells, i;
	int *local_cell_global_indx;


    counts = NULL;
    displs = NULL;
    mpibuf = NULL;
    rearrange_indexes = NULL;
    global_field_mem = NULL;
    strcpy(original_decomp_name, local_field->get_decomp_name());
    sprintf(grid_name, local_field->get_grid_name());
	if (words_are_the_same(local_field->get_decomp_name(), "NULL"))
		strcpy(fully_decomp_name, local_field->get_decomp_name());
	else sprintf(fully_decomp_name, "AUTO_FULL_%s_decomp", decomps_info_mgr->search_decomp_info(local_field->get_decomp_name())->get_grid_name());
    sprintf(data_type, local_field->get_field_data()->get_grid_data_field()->data_type_in_application);
    num_local_procs = compset_communicators_info_mgr->get_num_procs_in_comp(compset_communicators_info_mgr->get_current_comp_id());

    if (words_are_the_same(local_field->get_decomp_name(), "NULL")) {
        has_global_field = false;
        return;
    }

    has_global_field = true;
    num_local_cells = decomps_info_mgr->search_decomp_info(original_decomp_name)->get_num_local_cells();
	if (num_local_cells > 0) {
		num_levels = local_field->get_field_data()->get_coord_value_grid()->get_grid_size() / num_local_cells;
	    num_points_in_each_cell = local_field->get_field_data()->get_grid_data_field()->required_data_size / num_local_cells / num_levels;
	    EXECUTION_REPORT(REPORT_ERROR, num_points_in_each_cell > 0, "C-Coupler software error in Gather_scatter_rearrange_info\n");
		EXECUTION_REPORT(REPORT_ERROR, local_field->get_field_data()->get_coord_value_grid()->get_grid_size()%num_local_cells == 0, 
			         "c_coupler error in Gather_scatter_rearrange_info::Gather_scatter_rearrange_info\n");		
		EXECUTION_REPORT(REPORT_LOG, true, "num of levels and num of points in each cell in Gather_scatter_rearrange_info are %d and %d", num_levels, num_points_in_each_cell);
	}
	else {
		num_levels = -1;
		num_points_in_each_cell = -1;
	}

    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
        counts = new int [num_local_procs];
        displs = new int [num_local_procs];
    }

	MPI_Gather(&num_levels, 1, MPI_INT, counts, 1, MPI_INT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
	MPI_Gather(&num_points_in_each_cell, 1, MPI_INT, displs, 1, MPI_INT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());

    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
		for (i = 0; i < num_local_procs; i ++) {
			if (counts[i] != -1)
				num_levels = counts[i];
			if (displs[i] != -1)
				num_points_in_each_cell = displs[i];
		}
		EXECUTION_REPORT(REPORT_ERROR, num_levels != -1 && num_points_in_each_cell != -1, "parallel decomposition %s is empty, which is not allowed in C-Coupler");
		for (i = 0; i < num_local_procs; i ++)
			EXECUTION_REPORT(REPORT_ERROR, (counts[i] == -1 || num_levels == counts[i]) && (displs[i] == -1 || num_points_in_each_cell == displs[i]), "C-Coupler software error2 in Gather_scatter_rearrange_info\n");
    }
	MPI_Bcast(&num_levels, 1, MPI_INT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());	
	MPI_Bcast(&num_points_in_each_cell, 1, MPI_INT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
	
    MPI_Gather(&num_local_cells, 1, MPI_INT, counts, 1, MPI_INT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
        for (i = 0, num_total_cells = 0; i < num_local_procs; i ++) {
            displs[i] = num_total_cells;
            num_total_cells += counts[i];
        }
        rearrange_indexes = new int [num_total_cells];
    }
	if (num_local_cells > 0)
		local_cell_global_indx = new int [num_local_cells];
	for (i = 0; i < num_local_cells; i ++)
		local_cell_global_indx[i] = decomps_info_mgr->search_decomp_info(original_decomp_name)->get_local_cell_global_indx()[i];
    MPI_Gatherv(local_cell_global_indx, num_local_cells, MPI_INT, rearrange_indexes, counts, displs, MPI_INT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
    EXECUTION_REPORT(REPORT_LOG, true, "generate gather scatter info for (%s %s %s)", original_decomp_name, grid_name, data_type);
    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
        for (i = 0; i < num_local_procs; i ++) {
            displs[i] *= num_points_in_each_cell*num_levels;
            counts[i] *= num_points_in_each_cell*num_levels;
        }
        mpibuf = new char [num_total_cells*num_points_in_each_cell*num_levels*get_data_type_size(data_type)];
        decomps_info_mgr->generate_fully_decomp(fully_decomp_name, decomps_info_mgr->search_decomp_info(original_decomp_name)->get_grid_name());
		EXECUTION_REPORT(REPORT_LOG, true, "allocate global field for gather/scatter");
		global_field_mem = alloc_full_grid_mem(local_field->get_comp_name(), fully_decomp_name, grid_name, local_field->get_field_name(), local_field->get_field_data()->get_grid_data_field()->data_type_in_application, 0, false, "  C-Coupler error  ");
    }
	if (num_local_cells > 0)
		delete [] local_cell_global_indx;
}


bool Gather_scatter_rearrange_info::match(const char *decomp_name, const char *grid_name, const char *data_type)
{
    return words_are_the_same(this->original_decomp_name, decomp_name) && words_are_the_same(this->grid_name, grid_name) && words_are_the_same(this->data_type, data_type);
}


void Gather_scatter_rearrange_info::copy_in_local_field_info(Field_mem_info *local_field_mem)
{
    if (global_field_mem == NULL)
        return; 

    global_field_mem->reset_field_name(local_field_mem->get_field_name());
    strcpy(global_field_mem->get_field_data()->get_grid_data_field()->data_type_in_application, local_field_mem->get_field_data()->get_grid_data_field()->data_type_in_application);
    strcpy(global_field_mem->get_field_data()->get_grid_data_field()->data_type_in_IO_file, local_field_mem->get_field_data()->get_grid_data_field()->data_type_in_IO_file);
    strcpy(global_field_mem->get_field_data()->get_grid_data_field()->field_name_in_application, local_field_mem->get_field_data()->get_grid_data_field()->field_name_in_application);
    strcpy(global_field_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, local_field_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file);
    global_field_mem->get_field_data()->get_grid_data_field()->fill_value = local_field_mem->get_field_data()->get_grid_data_field()->fill_value;
    global_field_mem->get_field_data()->get_grid_data_field()->have_fill_value = local_field_mem->get_field_data()->get_grid_data_field()->have_fill_value;
    global_field_mem->get_field_data()->get_grid_data_field()->field_attributes.clear();
    for (int i = 0; i < local_field_mem->get_field_data()->get_grid_data_field()->field_attributes.size(); i ++)
        global_field_mem->get_field_data()->get_grid_data_field()->field_attributes.push_back( local_field_mem->get_field_data()->get_grid_data_field()->field_attributes[i]);

    global_field_mem->get_field_data()->get_grid_data_field()->initialize_to_fill_value();
}


Field_mem_info *Gather_scatter_rearrange_info::gather_field(Field_mem_info *local_field_mem)
{
    if (!has_global_field)
        return local_field_mem;

    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
        global_field_mem->get_field_data()->get_grid_data_field()->initialize_to_fill_value();

    if (get_data_type_size(data_type) == 1) {
        MPI_Gatherv(local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size, MPI_CHAR, 
                    mpibuf, counts, displs, MPI_CHAR, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
            rearrange_gather_data((char*) mpibuf, (char*) global_field_mem->get_data_buf(), decomps_info_mgr->search_decomp_info(fully_decomp_name)->get_num_local_cells());
    }
    else if (get_data_type_size(data_type) == 2) {
        MPI_Gatherv(local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size, MPI_SHORT, 
                    mpibuf, counts, displs, MPI_SHORT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
            rearrange_gather_data((short*) mpibuf, (short*) global_field_mem->get_data_buf(), decomps_info_mgr->search_decomp_info(fully_decomp_name)->get_num_local_cells());
    }
    else if (get_data_type_size(data_type) == 4) {
        MPI_Gatherv(local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size, MPI_INT, 
                    mpibuf, counts, displs, MPI_INT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
            rearrange_gather_data((int*) mpibuf, (int*) global_field_mem->get_data_buf(), decomps_info_mgr->search_decomp_info(fully_decomp_name)->get_num_local_cells());
    }
    else if (get_data_type_size(data_type) == 8) {
        MPI_Gatherv(local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size, MPI_DOUBLE, 
                    mpibuf, counts, displs, MPI_DOUBLE, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
            rearrange_gather_data((double*) mpibuf, (double*) global_field_mem->get_data_buf(), decomps_info_mgr->search_decomp_info(fully_decomp_name)->get_num_local_cells());
    }
    else EXECUTION_REPORT(REPORT_ERROR, false, "C-Coupler error in Gather_scatter_rearrange_info::gather_field\n");

    return global_field_mem;
}


Field_mem_info *Gather_scatter_rearrange_info::get_global_field(Field_mem_info *local_field_mem)
{
    if (!has_global_field)
        return local_field_mem;

    return global_field_mem;
}


void Gather_scatter_rearrange_info::scatter_field(Field_mem_info *local_field_mem)
{
    if (!has_global_field) {
        MPI_Bcast(local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size*get_data_type_size(data_type),
                  MPI_CHAR, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
        return;
    }

    if (get_data_type_size(data_type) == 1) {
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
            rearrange_scatter_data((char*) global_field_mem->get_data_buf(), (char*) mpibuf, decomps_info_mgr->search_decomp_info(fully_decomp_name)->get_num_local_cells());
        MPI_Scatterv(mpibuf, counts, displs, MPI_CHAR, local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size,  
                    MPI_CHAR, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
    }
    else if (get_data_type_size(data_type) == 2) {
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
            rearrange_scatter_data((short*) global_field_mem->get_data_buf(), (short*) mpibuf, decomps_info_mgr->search_decomp_info(fully_decomp_name)->get_num_local_cells());
        MPI_Scatterv(mpibuf, counts, displs, MPI_SHORT, local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size,  
                    MPI_SHORT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
    }
    else if (get_data_type_size(data_type) == 4) {
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
            rearrange_scatter_data((int*) global_field_mem->get_data_buf(), (int*) mpibuf, decomps_info_mgr->search_decomp_info(fully_decomp_name)->get_num_local_cells());
        MPI_Scatterv(mpibuf, counts, displs, MPI_INT, local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size,  
                    MPI_INT, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
    }
    else if (get_data_type_size(data_type) == 8) {
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
            rearrange_scatter_data((double*) global_field_mem->get_data_buf(), (double*) mpibuf, decomps_info_mgr->search_decomp_info(fully_decomp_name)->get_num_local_cells());
        MPI_Scatterv(mpibuf, counts, displs, MPI_DOUBLE, local_field_mem->get_data_buf(), local_field_mem->get_field_data()->get_grid_data_field()->required_data_size,  
                    MPI_DOUBLE, 0, compset_communicators_info_mgr->get_current_comp_comm_group());
    }
    else EXECUTION_REPORT(REPORT_ERROR, false, "C-Coupler error in Gather_scatter_rearrange_info::gather_field\n");
}


Gather_scatter_rearrange_info::~Gather_scatter_rearrange_info()
{
    if (counts != NULL)
        delete [] counts;
    if (displs != NULL)
        delete [] displs;
    if (mpibuf != NULL)
        delete [] mpibuf;
    if (rearrange_indexes != NULL)
        delete [] rearrange_indexes;
}


Gather_scatter_rearrange_info *Fields_gather_scatter_mgt::apply_gather_scatter_rearrange_info(Field_mem_info *local_field)
{
    int i;
    Gather_scatter_rearrange_info *rearrange_info;


    for (i = 0; i < gather_scatter_rearrange_infos.size(); i ++)
        if (gather_scatter_rearrange_infos[i]->match(local_field->get_decomp_name(), local_field->get_grid_name(), local_field->get_field_data()->get_grid_data_field()->data_type_in_application))
            break;

    if (i == gather_scatter_rearrange_infos.size()) {
        rearrange_info = new Gather_scatter_rearrange_info(local_field);
        gather_scatter_rearrange_infos.push_back(rearrange_info);
    }
    else rearrange_info = gather_scatter_rearrange_infos[i];

    rearrange_info->copy_in_local_field_info(local_field);
    return rearrange_info;
}


Field_mem_info *Fields_gather_scatter_mgt::gather_field(Field_mem_info *local_field)
{
    return apply_gather_scatter_rearrange_info(local_field)->gather_field(local_field);
}


void Fields_gather_scatter_mgt::gather_write_field(IO_netcdf *nc_file, Field_mem_info *local_field, bool write_grid_name, int date, int datesec, bool is_restart_field)
{
    Field_mem_info *global_field = gather_field(local_field);
    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
        nc_file->write_grided_data(global_field->get_field_data(), write_grid_name, date, datesec, is_restart_field);
}


void Fields_gather_scatter_mgt::read_scatter_field(IO_netcdf *nc_file, Field_mem_info *local_field, int time_pos)
{
    Gather_scatter_rearrange_info *rearrage_info = apply_gather_scatter_rearrange_info(local_field);
    if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0)
        nc_file->read_data(rearrage_info->get_global_field(local_field)->get_field_data()->get_grid_data_field(), time_pos);
    rearrage_info->scatter_field(local_field);
}


Fields_gather_scatter_mgt::~Fields_gather_scatter_mgt()
{
    for (int i = 0; i < gather_scatter_rearrange_infos.size(); i ++)
        delete gather_scatter_rearrange_infos[i];
}

