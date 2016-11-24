/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "decomp_info_mgt.h"
#include "global_data.h"
#include "memory_mgt.h"
#include "cor_global_data.h"
#include "cor_cpl_interface.h"
#include <string.h>
      

Decomp_info::Decomp_info(const char *decomp_name, int decomp_id, int grid_id, int num_local_cells, const int *cell_indexes_in_decomp, const char *annotation, bool registered)
{
	Remap_grid_class *CoR_grid;
	int i;
	

	EXECUTION_REPORT(REPORT_ERROR, -1, original_grid_mgr->is_grid_id_legal(grid_id), "the grid_id for registering decomposition \"%s\" is wrong. Please check the model code related to the annotation \"%s\".", decomp_name, annotation);
	
	this->decomp_id = decomp_id;
	this->grid_id = grid_id;
	this->comp_id = original_grid_mgr->get_comp_id_of_grid(grid_id);
    this->num_global_cells = original_grid_mgr->get_CoR_grid(grid_id)->get_grid_size();
	this->num_local_cells = num_local_cells;
	this->local_cell_global_indx = NULL;
	strcpy(this->decomp_name, decomp_name);
	annotation_mgr->add_annotation(decomp_id, "register decomposition", annotation);
	is_registered = false;
	check_and_verify_name_format_of_string_for_API(comp_id, this->decomp_name, API_ID_DECOMP_MGT_REG_DECOMP, "the parallel decomposition", annotation);
	synchronize_comp_processes_for_API(this->comp_id, API_ID_DECOMP_MGT_REG_DECOMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(this->comp_id,"C-Coupler code in Decomp_info::Decomp_info for getting comm group"), "for register a parallel decomposition of a grid", annotation);
	if (registered)
		comp_comm_group_mgt_mgr->confirm_coupling_configuration_active(comp_id, API_ID_DECOMP_MGT_REG_DECOMP, annotation);
	check_API_parameter_string(this->comp_id, API_ID_DECOMP_MGT_REG_DECOMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(this->comp_id,"C-Coupler code in Decomp_info::Decomp_info for getting comm group"), "for register a parallel decomposition of a grid", decomp_name, "decomp_name", annotation);
	check_API_parameter_string(this->comp_id, API_ID_DECOMP_MGT_REG_DECOMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(this->comp_id,"C-Coupler code in Decomp_info::Decomp_info for getting comm group"), "for register a parallel decomposition of a grid", original_grid_mgr->get_name_of_grid(grid_id), "grid_id (the corresponding grid name)", annotation);	

	CoR_grid = original_grid_mgr->search_grid_info(grid_id)->get_CoR_grid();
	EXECUTION_REPORT(REPORT_ERROR, this->comp_id, CoR_grid->get_is_sphere_grid(), "the grid \"%s\" for registering parallel decomposition \"%s\" is not a H2D grid. Please check the model code related to the annoation \"%s\"",
		             CoR_grid->get_grid_name(), decomp_name, annotation);

    /* the parallel decomposition of coupling process determined by the runtime */
    if (num_local_cells < 0) {
        EXECUTION_REPORT(REPORT_LOG, this->comp_id, true, "generate a dynamic decomposition \"%s\" (the corresponding annotation is \"%s\"", decomp_name);
        int num_local_procs = compset_communicators_info_mgr->get_num_procs_in_comp(compset_communicators_info_mgr->get_current_comp_id());
        int current_proc_local_id = compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group();
        num_local_cells = CoR_grid->get_grid_size() / num_local_procs;
        int local_cell_global_start_index = current_proc_local_id*num_local_cells;
        if (current_proc_local_id < (CoR_grid->get_grid_size() % num_local_procs)) {
            num_local_cells ++;
            local_cell_global_start_index += current_proc_local_id;
        }
        else local_cell_global_start_index += (CoR_grid->get_grid_size() % num_local_procs);
        local_cell_global_indx = new int [num_local_cells];
        for (i = 0; i < num_local_cells; i ++)
            local_cell_global_indx[i] = i+local_cell_global_start_index;
    }
    /* the parallel decomposition of the component models */
    else {
        if (num_local_cells == 0)
            local_cell_global_indx = NULL;
        else {
            EXECUTION_REPORT(REPORT_LOG, this->comp_id, true, "num local cells according to input decomp data is %d", num_local_cells);
            local_cell_global_indx = new int [num_local_cells];
            for (i = 0; i < num_local_cells; i ++) {
				if (cell_indexes_in_decomp[i] <= 0)
					local_cell_global_indx[i] = -1;
				else {
	                EXECUTION_REPORT(REPORT_ERROR, this->comp_id, cell_indexes_in_decomp[i] > 0 && cell_indexes_in_decomp[i] <= CoR_grid->get_grid_size(), 
	                             "the cell index specified for registering parallel decompostion of %s is out of the bound of grid size. Please check the model code with the annotation \"%s\"",
	                             decomp_name, annotation);
	                local_cell_global_indx[i] = cell_indexes_in_decomp[i] - 1;  // -1 because fortran array index starts from 1 but c/c++ starts from 0
				}
            }
        }
    }	
}


Decomp_info::Decomp_info(const char *decomp_name, const char *model_name, const char *grid_name,
                         int num_local_cells_in_decomp, const int *cell_indexes_in_decomp)
{
    Remap_grid_class *decomp_grid;
    long i;


	EXECUTION_REPORT(REPORT_LOG,-1, true, "generate decomposition %s with %d local cells\n", decomp_name, num_local_cells_in_decomp);
	EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager != NULL, "the grid manager is not initialized because the CoR script is not set");
    decomp_grid = remap_grid_manager->search_remap_grid_with_grid_name(grid_name);
    EXECUTION_REPORT(REPORT_ERROR,-1, decomp_grid != NULL && decomp_grid->get_is_sphere_grid(), 
                 "the grid %s of parallel decomposition %s is not a defined sphere 2D grid with latitude and longitude\n", 
                 grid_name, decomp_name);
    
    strcpy(this->decomp_name, decomp_name);
    strcpy(this->grid_name, grid_name);
    strcpy(this->model_name, model_name);
    this->num_local_cells = num_local_cells_in_decomp;
    local_cell_global_indx = NULL;
	is_registered = false;

    /* the parallel decomposition of coupling process determined by the runtime */
    if (num_local_cells < 0) {
        EXECUTION_REPORT(REPORT_LOG,-1, true, "generate a dynamic decomposition %s", decomp_name);
        int num_local_procs = compset_communicators_info_mgr->get_num_procs_in_comp(compset_communicators_info_mgr->get_current_comp_id());
        int current_proc_local_id = compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group();
        num_local_cells = decomp_grid->get_grid_size() / num_local_procs;
        int local_cell_global_start_index = current_proc_local_id*num_local_cells;
        if (current_proc_local_id < (decomp_grid->get_grid_size() % num_local_procs)) {
            num_local_cells ++;
            local_cell_global_start_index += current_proc_local_id;
        }
        else local_cell_global_start_index += (decomp_grid->get_grid_size() % num_local_procs);
        local_cell_global_indx = new int [num_local_cells];
        for (i = 0; i < num_local_cells; i ++)
            local_cell_global_indx[i] = i+local_cell_global_start_index;
    }
    /* the parallel decomposition of a full grid for gather and scatter */
    else if (num_local_cells == decomp_grid->get_grid_size() && cell_indexes_in_decomp == NULL) {
        EXECUTION_REPORT(REPORT_LOG,-1, true, "generate a full decomposition %s", decomp_name);
        local_cell_global_indx = new int [num_local_cells];
        for (i = 0; i < num_local_cells; i ++)
            local_cell_global_indx[i] = i;
    }
    /* the parallel decomposition of the component models */
    else {
        if (num_local_cells == 0)
            local_cell_global_indx = NULL;
        else {
            EXECUTION_REPORT(REPORT_LOG,-1, true, "num local cells according to input decomp data is %d", num_local_cells);
            local_cell_global_indx = new int [num_local_cells];
            for (i = 0; i < num_local_cells; i ++) {
				if (cell_indexes_in_decomp[i] <= 0)
					local_cell_global_indx[i] = -1;
				else {
	                EXECUTION_REPORT(REPORT_ERROR,-1, cell_indexes_in_decomp[i] > 0 && cell_indexes_in_decomp[i] <= decomp_grid->get_grid_size(), 
	                             "the cell index in parallel decompostion of %s is out of the bound of grid size\n",
	                             decomp_name);
	                local_cell_global_indx[i] = cell_indexes_in_decomp[i] - 1;  // -1 because fortran array index starts from 1 but c/c++ starts from 0
				}
            }
        }
    }
}


Decomp_info::~Decomp_info()
{
	EXECUTION_REPORT(REPORT_LOG,-1, true, "deleting decomposition %s", decomp_name);
    if (local_cell_global_indx != NULL)
        delete [] local_cell_global_indx;
}


void Decomp_info::gen_decomp_grid_data()
{
    int *local_grid_index;
    int i;


    num_global_cells = cpl_get_grid_size(grid_name);
    local_grid_index = (int *) alloc_mem(model_name, decomp_name, grid_name, "index", DATA_TYPE_INT, 0, false, "  C-Coupler error  ")->get_data_buf();
	memory_manager->search_field_via_data_buf(local_grid_index, true)->define_field_values(false);

    for (i = 0; i < num_local_cells; i ++) {
        local_grid_index[i] = local_cell_global_indx[i]+1;
    }
}


void Decomp_info::check_local_cell_global_indx()
{
    for (int i = 0; i < num_local_cells; i ++)
        EXECUTION_REPORT(REPORT_ERROR,-1, local_cell_global_indx[i] >= -1 && local_cell_global_indx[i] < num_global_cells, "C-Coupler error in check_local_cell_global_indx\n");
}


Decomp_info_mgt::~Decomp_info_mgt()
{
    for (int i = 0; i < decomps_info.size(); i ++)
        delete decomps_info[i];
}


void Decomp_info_mgt::add_decomp_from_model_interface(const char *decomp_name, const char *decomp_grid_name, const char *comp_name,
                                                      int num_local_cells_in_decomp, int *cell_indexes_in_decomp)
{    
    for (int j = 0; j < decomps_info.size(); j ++)
        EXECUTION_REPORT(REPORT_ERROR,-1, !words_are_the_same(decomps_info[j]->get_decomp_name(), decomp_name), 
                     "Decomp %s has been defined more than once in component %s\n", decomp_name, compset_communicators_info_mgr->get_current_comp_name());
	if (comp_name == NULL)
	    decomps_info.push_back(new Decomp_info(decomp_name,compset_communicators_info_mgr->get_current_comp_name(),decomp_grid_name,num_local_cells_in_decomp,cell_indexes_in_decomp));
	else decomps_info.push_back(new Decomp_info(decomp_name,comp_name,decomp_grid_name,num_local_cells_in_decomp,cell_indexes_in_decomp));
    decomps_info[decomps_info.size()-1]->gen_decomp_grid_data();
	decomps_info[decomps_info.size()-1]->set_decomp_registered();
}


void Decomp_info_mgt::add_decomps_from_cfg_file(const char *cfg_name)
{
    FILE *file_cfg;
    char line[NAME_STR_SIZE*16];
    char decomp_name[NAME_STR_SIZE];
    char model_name[NAME_STR_SIZE];
    char grid_name[NAME_STR_SIZE];
    char *line_p;
	int i = 0;


    if (words_are_the_same(cfg_name, "NULL"))
        return;
    file_cfg = open_config_file(cfg_name);

    while(get_next_line(line, file_cfg)) {
        line_p = line;
		i ++;
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(decomp_name, &line_p), "Please specify the decomposition name of the %dth parallel decomposition in the configuration file \"%s\".", i, cfg_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(model_name, &line_p), "Please specify the name of the component that owns the %dth parallel decomposition in the configuration file \"%s\".", i, cfg_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(grid_name, &line_p), "Please specify the grid name of the %dth parallel decomposition in the configuration file \"%s\".", i, cfg_name);
        decomps_info.push_back(new Decomp_info(decomp_name,model_name,grid_name,-1,NULL));
        decomps_info[decomps_info.size()-1]->gen_decomp_grid_data();
    }

    fclose(file_cfg);
}


Decomp_info *Decomp_info_mgt::search_decomp_info(const char *decomp_name)
{
    for (int i = 0; i < decomps_info.size(); i ++)
        if (words_are_the_same(decomps_info[i]->get_decomp_name(), decomp_name))
            return decomps_info[i];

    EXECUTION_REPORT(REPORT_ERROR,-1, false, "Decomp %s of comp %s has not been defined\n", decomp_name, compset_communicators_info_mgr->get_current_comp_name());
    return NULL;
}


int Decomp_info_mgt::generate_fully_decomp(int original_decomp_id)
{
	char fully_decomp_name[NAME_STR_SIZE];
	Decomp_info *fully_decomp;
	int *local_cells_global_indexes, num_global_cells, existing_decomp_id;


	sprintf(fully_decomp_name, "fully_decomp_for_%s", get_decomp_info(original_decomp_id)->get_decomp_name());
	existing_decomp_id = search_decomp_info(fully_decomp_name, get_decomp_info(original_decomp_id)->get_comp_id());
	if (existing_decomp_id != -1)
		return get_decomp_info(existing_decomp_id)->get_decomp_id();

	if (comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(get_decomp_info(original_decomp_id)->get_comp_id(), "in Decomp_info_mgt::generate_fully_decomp") == 0) {
		num_global_cells = get_decomp_info(original_decomp_id)->get_num_global_cells();
		local_cells_global_indexes = new int [num_global_cells];
		for (int i = 0; i < num_global_cells; i ++)
			local_cells_global_indexes[i] = i + 1;
		fully_decomp = new Decomp_info(fully_decomp_name, (TYPE_DECOMP_ID_PREFIX|decomps_info.size()), get_decomp_info(original_decomp_id)->get_grid_id(), num_global_cells, local_cells_global_indexes, "fully decomp", false);
		delete [] local_cells_global_indexes;	
	}
	else fully_decomp = new Decomp_info(fully_decomp_name, (TYPE_DECOMP_ID_PREFIX|decomps_info.size()), get_decomp_info(original_decomp_id)->get_grid_id(), 0, NULL, "fully decomp", false);
    decomps_info.push_back(fully_decomp);

    return fully_decomp->get_decomp_id();
}


Decomp_info *Decomp_info_mgt::generate_remap_weights_src_decomp(const char *decomp_name_dst, const char *decomp_name_src, const char *remap_weights_name)
{
    Decomp_info *decomp_src, *decomp_dst;
    Decomp_info *decomp_for_remap;
    Remap_weight_of_strategy_class *remap_weights;
    long *decomp_map_src, *decomp_map_dst, *original_map_src;
    int num_local_cells, *local_cell_global_indexes;
    char decomp_name_remap[NAME_STR_SIZE];
	int current_proc_id_computing_node_comp_group, num_procs_computing_node_comp_group;
	int *dst_decomp_size_all_procs_in_computing_node, *displs, *dst_decomp_local_cell_indexes_all_procs_in_computing_node, *current_local_cell_indexes;
	int i, j;


    sprintf(decomp_name_remap, "%s_%s_%s", decomp_name_src, remap_weights_name, decomp_name_dst);
    for (i = 0; i < decomps_info.size(); i ++)
        if (words_are_the_same(decomps_info[i]->get_decomp_name(), decomp_name_remap))
            return decomps_info[i];

    decomp_src = decomps_info_mgr->search_decomp_info(decomp_name_src);
    decomp_dst = decomps_info_mgr->search_decomp_info(decomp_name_dst);
	decomp_src->check_local_cell_global_indx();
	decomp_dst->check_local_cell_global_indx();

	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(compset_communicators_info_mgr->get_computing_node_comp_group(), &num_procs_computing_node_comp_group) == MPI_SUCCESS);
    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_rank(compset_communicators_info_mgr->get_computing_node_comp_group(), &current_proc_id_computing_node_comp_group) == MPI_SUCCESS);
	if (current_proc_id_computing_node_comp_group == 0) {
		dst_decomp_size_all_procs_in_computing_node = new int [num_procs_computing_node_comp_group];
		displs = new int [num_procs_computing_node_comp_group];
	}
	num_local_cells = decomp_dst->get_num_local_cells();
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gather(&num_local_cells, 1, MPI_INT, dst_decomp_size_all_procs_in_computing_node, 1, MPI_INT, 0, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
	if (current_proc_id_computing_node_comp_group == 0) {
		displs[0] = 0;
		for (i = 1; i < num_procs_computing_node_comp_group; i ++)
			displs[i] = displs[i-1] + dst_decomp_size_all_procs_in_computing_node[i-1];
		dst_decomp_local_cell_indexes_all_procs_in_computing_node = new int [displs[num_procs_computing_node_comp_group-1]+dst_decomp_size_all_procs_in_computing_node[num_procs_computing_node_comp_group-1]];
	}	
    EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Gatherv((void*)(decomp_dst->get_local_cell_global_indx()), num_local_cells, MPI_INT, dst_decomp_local_cell_indexes_all_procs_in_computing_node, dst_decomp_size_all_procs_in_computing_node, displs, MPI_INT, 0, compset_communicators_info_mgr->get_computing_node_comp_group()) == MPI_SUCCESS);
	EXECUTION_REPORT(REPORT_LOG,-1, true, "after gathering local cell indexes of processes");
	
    decomp_map_src = new long [decomp_src->get_num_global_cells()];

	if (current_proc_id_computing_node_comp_group == 0) {
		decomp_map_dst = new long [decomp_dst->get_num_global_cells()];
		for (i = 0; i < decomp_dst->get_num_global_cells(); i ++)
			decomp_map_dst[i] = 0;
		for (i = 0; i < num_procs_computing_node_comp_group; i ++) {
			current_local_cell_indexes = dst_decomp_local_cell_indexes_all_procs_in_computing_node + displs[i];
			for (j = 0; j < dst_decomp_size_all_procs_in_computing_node[i]; j ++)
				if (current_local_cell_indexes[j] >= 0)
					decomp_map_dst[current_local_cell_indexes[j]] = (decomp_map_dst[current_local_cell_indexes[j]] | (((long)1)<<i));
		}
		EXECUTION_REPORT(REPORT_LOG,-1, true, "before calculate_src_decomp");
		remap_weights = remap_weights_of_strategy_manager->search_remap_weight_of_strategy(remap_weights_name);
		EXECUTION_REPORT(REPORT_ERROR,-1, remap_weights != NULL, "C-Coupler error in generate_remap_weights_src_decomp: %s is not found", remap_weights_name);
		remap_weights->calculate_src_decomp(remap_grid_manager->search_remap_grid_with_grid_name(decomp_src->get_grid_name()), 
											remap_grid_manager->search_remap_grid_with_grid_name(decomp_dst->get_grid_name()), 
											decomp_map_src, decomp_map_dst);
		EXECUTION_REPORT(REPORT_LOG,-1, true, "after calculate_src_decomp");
	}

	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Bcast(decomp_map_src, decomp_src->get_num_global_cells(), MPI_LONG, 0, compset_communicators_info_mgr->get_computing_node_comp_group())  == MPI_SUCCESS);

    original_map_src = new long [decomp_src->get_num_global_cells()];
    for (long i = 0; i < decomp_src->get_num_global_cells(); i ++)
        original_map_src[i] = 0;

    for (long i = 0; i < decomp_src->get_num_local_cells(); i ++)
		if (decomp_src->get_local_cell_global_indx()[i] >= 0)
	        original_map_src[decomp_src->get_local_cell_global_indx()[i]] = (((long)1)<<current_proc_id_computing_node_comp_group);
    num_local_cells = decomp_src->get_num_local_cells();
    for (long i = 0; i < decomp_src->get_num_global_cells(); i ++)
        if ((decomp_map_src[i]&(((long)1)<<current_proc_id_computing_node_comp_group)) != 0 && (original_map_src[i]&(((long)1)<<current_proc_id_computing_node_comp_group)) == 0)
            num_local_cells ++;
    local_cell_global_indexes = new int [num_local_cells];
    num_local_cells = 0;
    for (long i = 0; i < decomp_src->get_num_local_cells(); i ++)
        local_cell_global_indexes[num_local_cells++] = decomp_src->get_local_cell_global_indx()[i]+1;
    for (long i = 0; i < decomp_src->get_num_global_cells(); i ++)
        if ((decomp_map_src[i]&(((long)1)<<current_proc_id_computing_node_comp_group)) != 0 && (original_map_src[i]&(((long)1)<<current_proc_id_computing_node_comp_group)) == 0)
            local_cell_global_indexes[num_local_cells++] = i+1;

    decomp_for_remap = new Decomp_info(decomp_name_remap, decomp_src->get_model_name(), decomp_src->get_grid_name(), num_local_cells, local_cell_global_indexes);
    decomps_info.push_back(decomp_for_remap);
    decomps_info[decomps_info.size()-1]->gen_decomp_grid_data();

    for (long i = 0; i < decomp_src->get_num_local_cells(); i ++)
        if (decomps_info[decomps_info.size()-1]->get_local_cell_global_indx()[i] != decomp_src->get_local_cell_global_indx()[i])
            EXECUTION_REPORT(REPORT_ERROR,-1, false, "software error in generate_remap_weights_src_decomp");

    delete [] original_map_src;
    delete [] decomp_map_src;
    delete [] local_cell_global_indexes;

	if (current_proc_id_computing_node_comp_group == 0) {
	    delete [] decomp_map_dst;
		delete [] dst_decomp_size_all_procs_in_computing_node;
		delete [] displs;
		delete [] dst_decomp_local_cell_indexes_all_procs_in_computing_node;
	}

    return decomp_for_remap;
}


int Decomp_info_mgt::search_decomp_info(const char *decomp_name, int comp_id)
{
	printf("search search %s %x\n", decomp_name, comp_id);
	for (int i = 0; i < decomps_info.size(); i ++)
		if (words_are_the_same(decomps_info[i]->get_decomp_name(), decomp_name) && decomps_info[i]->get_comp_id() == comp_id) {
			return decomps_info[i]->get_decomp_id();
		}

	return -1;
}


int Decomp_info_mgt::register_H2D_parallel_decomposition(const char *decomp_name, int grid_id, int num_local_cells, const int *cell_indexes_in_decomp, const char *annotation)
{
	Decomp_info *new_decomp = new Decomp_info(decomp_name, (TYPE_DECOMP_ID_PREFIX|decomps_info.size()), grid_id, num_local_cells, cell_indexes_in_decomp, annotation, true);

	if (search_decomp_info(decomp_name, original_grid_mgr->get_comp_id_of_grid(grid_id)) != -1)
		EXECUTION_REPORT(REPORT_ERROR, new_decomp->get_comp_id(), false, "The parallel decomposition \"%s\" corresponding to grid \"%s\" has been registered before. Please check the model code corresponding to annotations \"%s\" and \"%s\"",
					     decomp_name, original_grid_mgr->get_name_of_grid(grid_id), annotation_mgr->get_annotation(search_decomp_info(decomp_name, original_grid_mgr->get_comp_id_of_grid(grid_id)), "register decomposition"), annotation);

	decomps_info.push_back(new_decomp);

	return new_decomp->get_decomp_id();
} 
	
	
bool Decomp_info_mgt::is_decomp_id_legal(int decomp_id) const
{
	int true_decomp_id = decomp_id & TYPE_ID_SUFFIX_MASK;

	
	if ((decomp_id & TYPE_ID_PREFIX_MASK) != TYPE_DECOMP_ID_PREFIX)
		return false;

	if (true_decomp_id < 0 || true_decomp_id >= decomps_info.size())
		return false;

	return true;
}


int Decomp_info_mgt::get_comp_id_of_decomp(int decomp_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_decomp_id_legal(decomp_id), "Software error when calling Decomp_info_mgt::get_comp_id_of_decomp");
	return decomps_info[decomp_id&TYPE_ID_SUFFIX_MASK]->get_comp_id();
}


Remap_grid_class *Decomp_info_mgt::get_CoR_grid_of_decomp(int decomp_id) const
{	
	EXECUTION_REPORT(REPORT_ERROR, -1, is_decomp_id_legal(decomp_id), "Software error when calling Decomp_info_mgt::get_comp_id_of_decomp");
	return original_grid_mgr->get_CoR_grid(decomps_info[decomp_id&TYPE_ID_SUFFIX_MASK]->get_grid_id());
}


Decomp_info *Decomp_info_mgt::get_decomp_info(int decomp_id)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_decomp_id_legal(decomp_id), "Software error when calling Decomp_info_mgt::get_decomp_info");
	return decomps_info[decomp_id&TYPE_ID_SUFFIX_MASK];
}

