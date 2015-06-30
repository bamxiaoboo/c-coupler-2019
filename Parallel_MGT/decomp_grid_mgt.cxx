/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "decomp_grid_mgt.h"
#include "cor_cpl_interface.h"


Decomp_grid_info::Decomp_grid_info(const char *decomp_name, Remap_grid_class *original_grid)
{
    Decomp_info *decomp;
    Remap_grid_class *decomp_info_grid, *decomp_2D_grid;
    Remap_grid_class *leaf_grids[256], *sub_grids[256];
    int num_leaf_grids, num_sub_grids, i, j, decomp_leaf_grid_indexes[2];
	char decomp_grid_name[256];


    EXECUTION_REPORT(REPORT_LOG, true, "generate decomposition grid (%s %s)", decomp_name, original_grid->get_grid_name());

    this->original_grid = original_grid;
    strcpy(this->decomp_name, decomp_name);

	if (words_are_the_same(decomp_name, "NULL")) {
		decomp_grid = original_grid;
		return;
	}
	
    decomp = decomps_info_mgr->search_decomp_info(decomp_name);
    decomp_info_grid = remap_grid_manager->search_remap_grid_with_grid_name(decomp->get_grid_name());

	if (decomp->get_num_local_cells() == 0) {
		this->decomp_grid = NULL;
		return;
	}

    if (this->original_grid->get_is_sphere_grid()) {
        EXECUTION_REPORT(REPORT_ERROR, decomp_info_grid == original_grid, "%s and %s (the grid of parallel decomposition %s) are not the same grid when generating decomp grid",
		                 original_grid->get_grid_name(), decomp->get_grid_name(), decomp_name);
		EXECUTION_REPORT(REPORT_LOG, true, "generate decomposition sphere grid (%s %s) with size %d", decomp_name, original_grid->get_grid_name(), decomp->get_num_local_cells());
        this->decomp_grid = this->original_grid->generate_decomp_grid(decomp->get_local_cell_global_indx(), decomp->get_num_local_cells(), decomp_name);
    }
    else {
		if (original_grid->has_grid_coord_label(COORD_LABEL_LON) || original_grid->has_grid_coord_label(COORD_LABEL_LAT))
	        EXECUTION_REPORT(REPORT_ERROR, remap_grid_manager->search_remap_grid_with_grid_name(decomp->get_grid_name())->is_subset_of_grid(original_grid),
    	                     "grid %s is not a superset of %s (the grid of parallel decomposition %s) are not the same grid when generating decomp grid",
			                 original_grid->get_grid_name(), decomp->get_grid_name(), decomp_name);
        original_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, original_grid);
		for (i = 0, j = 0; i < num_leaf_grids; i ++)
			if (leaf_grids[i]->is_subset_of_grid(decomp_info_grid))
				decomp_leaf_grid_indexes[j++] = i;
		EXECUTION_REPORT(REPORT_ERROR, decomp_leaf_grid_indexes[1]-decomp_leaf_grid_indexes[0] == 1, "C-Coupler error in Decomp_grid_info::Decomp_grid_info\n");
        for (i = 0, num_sub_grids = 0; i < num_leaf_grids; i ++) {
			if (leaf_grids[i] == NULL)
				continue;
			if (leaf_grids[i]->is_subset_of_grid(decomp_info_grid)) {
				for (j = i+1; j < num_leaf_grids; j ++)
					if (leaf_grids[j]->is_subset_of_grid(decomp_info_grid))
						leaf_grids[j] = NULL;
				decomp_2D_grid = decomp_grids_mgr->search_decomp_grid_info(decomp_name, remap_grid_manager->search_remap_grid_with_grid_name(decomp->get_grid_name()), false)->get_decomp_grid();
				sub_grids[num_sub_grids++] = decomp_2D_grid;
			}
			else sub_grids[num_sub_grids++] = leaf_grids[i]->duplicate_grid(leaf_grids[i]); 
        }
		sprintf(decomp_grid_name, "DECOMP_GRID_%s", original_grid->get_grid_name());
        this->decomp_grid = new Remap_grid_class(decomp_grid_name, num_sub_grids, sub_grids, 0);
		this->decomp_grid->set_decomp_name(decomp_name);
		this->decomp_grid->set_original_grid(original_grid);
		EXECUTION_REPORT(REPORT_LOG, true, "the size of decomp grid %s is %ld %d", this->decomp_grid->get_grid_name(), this->decomp_grid->get_grid_size(), num_sub_grids);
		EXECUTION_REPORT(REPORT_LOG, true, "the size of decomp 2D grid %s is %ld vs %d", decomp_2D_grid->get_grid_name(), decomp_2D_grid->get_grid_size(), decomp->get_num_local_cells());
		if (original_grid->has_grid_coord_label(COORD_LABEL_LEV))
			this->decomp_grid->generate_3D_grid_decomp_sigma_values(original_grid, decomp_2D_grid, decomp->get_local_cell_global_indx(), decomp->get_num_local_cells());
    }
}


void Decomp_grid_info::register_decomp_grid_fields()
{
    Remap_grid_class *leaf_grids[256];
    int num_leaf_grids;
    Field_mem_info *local_lats, *local_lons, *local_areas, *local_masks, *local_nlats, *local_nlons;
	

	if (decomp_grid == NULL)
		return;
	
    if (!this->original_grid->get_is_sphere_grid())
        return;

    this->decomp_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, this->decomp_grid);
    local_lats = alloc_mem(decomps_info_mgr->search_decomp_info(decomp_name)->get_model_name(), decomp_name, original_grid->get_grid_name(), LAT_GF, DATA_TYPE_DOUBLE, 0, false, "C-Coupler error");
    local_lons = alloc_mem(decomps_info_mgr->search_decomp_info(decomp_name)->get_model_name(), decomp_name, original_grid->get_grid_name(), LON_GF, DATA_TYPE_DOUBLE, 0, false, "C-Coupler error");
    local_masks = alloc_mem(decomps_info_mgr->search_decomp_info(decomp_name)->get_model_name(), decomp_name, original_grid->get_grid_name(), MASK_GF, DATA_TYPE_BOOL, 0, false, "C-Coupler error");
    local_nlats = alloc_mem(decomps_info_mgr->search_decomp_info(decomp_name)->get_model_name(), decomp_name, original_grid->get_grid_name(), GRID_LATS_GF, DATA_TYPE_INT, 0, false, "C-Coupler error");
    local_nlons = alloc_mem(decomps_info_mgr->search_decomp_info(decomp_name)->get_model_name(), decomp_name, original_grid->get_grid_name(), GRID_LONS_GF, DATA_TYPE_INT, 0, false, "C-Coupler error");
    memcpy(local_lons->get_data_buf(), leaf_grids[0]->get_grid_center_field()->get_grid_data_field()->data_buf, decomp_grid->get_grid_size()*sizeof(double));
    memcpy(local_lats->get_data_buf(), leaf_grids[1]->get_grid_center_field()->get_grid_data_field()->data_buf, decomp_grid->get_grid_size()*sizeof(double));
    memcpy(local_masks->get_data_buf(), decomp_grid->get_grid_mask_field()->get_grid_data_field()->data_buf, decomp_grid->get_grid_size()*sizeof(bool));
    ((int*)local_nlats->get_data_buf())[0] = cpl_get_sphere_grid_subgrid_size(original_grid->get_grid_name(), LAT_GF);
    ((int*)local_nlons->get_data_buf())[0] = cpl_get_sphere_grid_subgrid_size(original_grid->get_grid_name(), LON_GF);
	local_lats->define_field_values(false);
	local_lons->define_field_values(false);
	local_masks->define_field_values(false);
	local_nlats->define_field_values(false);
	local_nlons->define_field_values(false);

	if (decomp_grid->get_area_or_volumn() != NULL) { 
    	local_areas = alloc_mem(decomps_info_mgr->search_decomp_info(decomp_name)->get_model_name(), decomp_name, original_grid->get_grid_name(), AREA_GF, DATA_TYPE_DOUBLE, 0, false, "C-Coupler error");
    	memcpy(local_areas->get_data_buf(), decomp_grid->get_area_or_volumn(), decomp_grid->get_grid_size()*sizeof(double));
		local_areas->define_field_values(false);
	}
}


bool Decomp_grid_info::match(const char *decomp_name, Remap_grid_class *original_grid)
{
    return words_are_the_same(decomp_name, this->decomp_name) && this->original_grid == original_grid;
}


Decomp_grid_info::~Decomp_grid_info()
{
	if (decomp_grid != NULL && decomp_grid != original_grid)
	    delete decomp_grid;
}


Decomp_grid_info *Decomp_grid_mgt::search_decomp_grid_info(const char *decomp_name, Remap_grid_class *original_grid, bool diag)
{
    for (int i = 0; i < decomp_grids_info.size(); i ++)
        if (decomp_grids_info[i]->match(decomp_name, original_grid))
            return decomp_grids_info[i];

	if (diag)
		EXECUTION_REPORT(REPORT_ERROR, true, "C-Coupler error in Decomp_grid_mgt::search_decomp_grid_info");
		
    decomp_grids_info.push_back(new Decomp_grid_info(decomp_name, original_grid));
    decomp_grids_info[decomp_grids_info.size()-1]->register_decomp_grid_fields();
    return decomp_grids_info[decomp_grids_info.size()-1];
}


Decomp_grid_mgt::~Decomp_grid_mgt()
{
    for (int i = 0; i < decomp_grids_info.size(); i ++)
        delete decomp_grids_info[i];
}


void Decomp_grid_mgt::check_unique_registered_decomp_for_dynamic_sigma_grid(Remap_grid_class *original_grid, const char *error_string)
{
	Decomp_grid_info *decomp_grids[2];
	int num_decomp_grids = 0, i;
	bool is_dynamic_sigma_grid = false;


	EXECUTION_REPORT(REPORT_ERROR, original_grid->is_sigma_grid(), "C-Coupler error1 in Decomp_grid_mgt::check_unique_registered_decomp_for_dynamic_sigma_grid");

	for (i = 0; i < decomp_grids_info.size(); i ++)
		if (decomp_grids_info[i]->get_original_grid()->is_the_same_grid_with(original_grid) && decomp_grids_info[i]->get_decomp_grid()->get_sigma_grid_dynamic_surface_value_field() != NULL)
			is_dynamic_sigma_grid = true;

	if (!is_dynamic_sigma_grid)
		return;

	for (i = 0; i < decomp_grids_info.size() && num_decomp_grids < 2; i ++)
		if (decomp_grids_info[i]->get_original_grid()->is_the_same_grid_with(original_grid)) {
			EXECUTION_REPORT(REPORT_ERROR, decomps_info_mgr->search_decomp_info(decomp_grids_info[i]->get_decomp_name()) != NULL, "C-Coupler error2 in Decomp_grid_mgt::check_unique_registered_decomp_for_dynamic_sigma_grid");
			if (decomps_info_mgr->search_decomp_info(decomp_grids_info[i]->get_decomp_name())->is_registered_decomp())
				decomp_grids[num_decomp_grids ++] = decomp_grids_info[i];
		}

	EXECUTION_REPORT(REPORT_ERROR, num_decomp_grids <= 1, "Grid %s has at least two registerred decompositions such as %s and %s, %s", original_grid->get_grid_name(),
					 decomp_grids[0]->get_decomp_name(), decomp_grids[1]->get_decomp_name(), error_string);
}

