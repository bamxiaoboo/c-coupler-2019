/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "original_grid_mgt.h"


Original_grid_info::Original_grid_info(int comp_id, int grid_id, const char *grid_name, const char *annotation, Remap_grid_class *original_CoR_grid)
{
	this->comp_id = comp_id;
	this->grid_id = grid_id;
	this->original_CoR_grid = original_CoR_grid;
	strcpy(this->grid_name, grid_name);
	annotation_mgr->add_annotation(this->grid_id, "grid_registration", annotation);
	generate_remapping_grids();
}


const char *Original_grid_info::get_annotation()
{
	return annotation_mgr->get_annotation(this->grid_id, "grid_registration");
}


void Original_grid_info::generate_remapping_grids()
{
	Remap_grid_class *lon_sub_grid = NULL, *lat_sub_grid = NULL, *leaf_grids[256];
	int num_leaf_grids;

	
	H2D_sub_CoR_grid = NULL;
	V1D_sub_CoR_grid = NULL;
	T1D_sub_CoR_grid = NULL;
	H2D_sub_grid_order = -1;
	V1D_sub_grid_order = -1;
	T1D_sub_grid_order = -1;
	
	if (original_CoR_grid->get_num_dimensions() == 2)
		EXECUTION_REPORT(REPORT_ERROR, -1, original_CoR_grid->get_is_sphere_grid(), "Software error in Original_grid_info::generate_remapping_grids: not a sphere grid");
	if (original_CoR_grid->get_num_dimensions() == 1)
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(original_CoR_grid->get_coord_label(),COORD_LABEL_LEV) || words_are_the_same(original_CoR_grid->get_coord_label(),COORD_LABEL_TIME), "Software error in Original_grid_info::generate_remapping_grids: not a vertical grid or not a time grid");
	H2D_sub_CoR_grid = original_CoR_grid->get_sphere_sub_grid();
	
	original_CoR_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, original_CoR_grid);
	for (int i = 0; i < num_leaf_grids; i ++) {
		if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LON)) {
			lon_sub_grid = leaf_grids[i];
			H2D_sub_grid_order = i;
		}
		else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LAT))
			lat_sub_grid = leaf_grids[i];
		else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LEV)) {
			V1D_sub_CoR_grid = leaf_grids[i];
			V1D_sub_grid_order = i;
		}
		else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_TIME)) {
			T1D_sub_CoR_grid = leaf_grids[i];
			T1D_sub_grid_order = i;
		}
	}

	if (H2D_sub_CoR_grid == NULL && (lon_sub_grid != NULL || lat_sub_grid != NULL)) {
		EXECUTION_REPORT(REPORT_ERROR, -1, lon_sub_grid != NULL && lat_sub_grid != NULL, "Software error in Original_grid_info::generate_remapping_grids: wrong sub grids for a sphere grid");
		leaf_grids[0] = lon_sub_grid;
		leaf_grids[1] = lat_sub_grid;
		char H2D_grid_name[NAME_STR_SIZE];
		sprintf(H2D_grid_name, "H2D_grid_for_%s", grid_name);
		H2D_sub_CoR_grid = new Remap_grid_class(H2D_grid_name, 2, leaf_grids, 0);
	}	

	EXECUTION_REPORT(REPORT_ERROR, -1, H2D_sub_CoR_grid != NULL || V1D_sub_CoR_grid != NULL || T1D_sub_CoR_grid != NULL, "Software error in Original_grid_info::generate_remapping_grids: empty grid");
}


bool Original_grid_info::is_V1D_sub_grid_after_H2D_sub_grid()
{
	if (H2D_sub_CoR_grid == NULL || V1D_sub_CoR_grid == NULL)
		return true;

	return H2D_sub_grid_order < V1D_sub_grid_order;
}


Original_grid_mgt::Original_grid_mgt()
{
	original_grids.clear();
	sprintf(CoR_script_name, "%s/CCPL_grid.cor", comp_comm_group_mgt_mgr->get_config_root_comp_dir());
	FILE *fp = fopen(CoR_script_name, "r");
	if (fp == NULL)
		CoR_script_name[0] = '\0';
	else fclose(fp);
	if (strlen(CoR_script_name) != 0)
		CoR_grids = new Remap_mgt(CoR_script_name);
	else CoR_grids = new Remap_mgt(NULL);
}


Original_grid_mgt::~Original_grid_mgt()
{
	delete CoR_grids;
}


Original_grid_info *Original_grid_mgt::search_grid_info(const char *grid_name, int comp_id)
{
	for (int i = 0; i < original_grids.size(); i ++)
		if (words_are_the_same(original_grids[i]->get_grid_name(), grid_name) && original_grids[i]->get_comp_id() == comp_id)
			return original_grids[i];

	return NULL;
}


Original_grid_info *Original_grid_mgt::search_grid_info(int grid_id)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "software error in Original_grid_mgt::search_grid_info based on grid_id");	
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK];
}


void Original_grid_mgt::check_for_grid_definition(int comp_id, const char *grid_name, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, comp_id, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "The component id is wrong when defining a grid \"%s\". Please check the model code with the annotation \"%s\"", grid_name, annotation);
	if (search_grid_info(grid_name, comp_id) != NULL)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "grid \"%s\" has been defined in component \"%s\" before. Please check the model code with the annoation \"%s\" and \"%s\"",
		                 grid_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "C-Coupler code in check_for_grid_definition for getting component management node")->get_comp_name(), search_grid_info(grid_name, comp_id)->get_annotation(), annotation);
}


int Original_grid_mgt::register_H2D_grid_via_data(int comp_id, const char *grid_name, const char *edge_type, const char *coord_unit, const char *cyclic_or_acyclic, const char *data_type, int dim_size1, int dim_size2, int size_center_lon, int size_center_lat, 
	                                               int size_mask, int size_area, int size_vertex_lon, int size_vertex_lat, void *center_lon, void *center_lat, int *mask, void *area, void *vertex_lon, void *vertex_lat, const char *annotation, int API_id)
{
	int data_type_size, grid_size, num_vertex;
	char true_H2D_grid_name[NAME_STR_SIZE], true_lon_grid_name[NAME_STR_SIZE], true_lat_grid_name[NAME_STR_SIZE];
	char coord_label[NAME_STR_SIZE], coord_name[NAME_STR_SIZE];
	Remap_grid_class *CoR_H2D_grid, *CoR_lon_grid, *CoR_lat_grid, *sub_grids[256];
	

	synchronize_comp_processes_for_API(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_h2d_grid_with_data"), "registering an H2D grid", annotation);
	check_and_verify_name_format_of_string_for_API(comp_id, grid_name, API_id, "the C-Coupler grid", annotation);
	check_API_parameter_string(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_h2d_grid_with_data"), "registering an H2D grid", grid_name, "grid_name", annotation);
	check_API_parameter_string(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_h2d_grid_with_data"), "registering an H2D grid", edge_type, "edge_type", annotation);
	check_API_parameter_string(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_h2d_grid_with_data"), "registering an H2D grid", coord_unit, "coord_unit", annotation);
	check_API_parameter_string(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_h2d_grid_with_data"), "registering an H2D grid", cyclic_or_acyclic, "cyclic_or_acyclic", annotation);
	check_API_parameter_string(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_h2d_grid_with_data"), "registering an H2D grid", data_type, "implicit data type", annotation);
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", dim_size1, "dim_size1", annotation);
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", dim_size2, "dim_size2", annotation);
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_center_lon, "implicit array size for center_lon", annotation);	
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_center_lat, "implicit array size for center_lat", annotation);
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_mask, "implicit array size for mask", annotation);	
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_area, "implicit array size for area", annotation);	
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_vertex_lon, "implicit array size for vertex_lon", annotation);	
	check_API_parameter_int(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_vertex_lat, "implicit array size for vertex_lat", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(coord_unit, COORD_UNIT_DEGREES) || words_are_the_same(coord_unit, COORD_UNIT_RADIANS), "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the parameter \"coord_unit\" is not \"degrees\" or \"radians\". Please check the model code related to the annotation \"%s\".", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(cyclic_or_acyclic, "cyclic") || words_are_the_same(cyclic_or_acyclic, "acyclic"), "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the value of parameter \"cyclic_or_acyclic\" is not \"cyclic\" or \"acyclic\". Please check the model code related to the annotation \"%s\".", annotation);
	if (words_are_the_same(data_type, DATA_TYPE_FLOAT))
		data_type_size = sizeof(float);
	else if (words_are_the_same(data_type, DATA_TYPE_DOUBLE))
		data_type_size = sizeof(double);
	else EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "software error in register_h2d_grid_with_data: wrong implicit data type");
	check_API_parameter_data_array(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_center_lon*data_type_size, (const char*)center_lon, "center_lon", annotation);
	check_API_parameter_data_array(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_center_lat*data_type_size, (const char*)center_lat, "center_lat", annotation);
	check_API_parameter_data_array(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_mask*sizeof(int), (const char*)mask, "mask", annotation);
	check_API_parameter_data_array(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_area*data_type_size, (const char*)area, "area", annotation);
	check_API_parameter_data_array(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_vertex_lon*data_type_size, (const char*)vertex_lon, "vertex_lon", annotation);
	check_API_parameter_data_array(comp_id, API_id, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_h2d_grid_with_data"), "registering an H2D grid", size_vertex_lat*data_type_size, (const char*)vertex_lat, "vertex_lat", annotation);
	check_for_grid_definition(comp_id, grid_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, dim_size1 > 3, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the value of the parameter \"dim_size1\" is wrong. It must be larger than 3. Please check the model code related to the annotation \"%s\".", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, dim_size2 == 0 || dim_size2 > 3, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the value of the parameter \"dim_size2\" is wrong. It must be 0 or a positive value larger than 3. Please check the model code related to the annotation \"%s\".", annotation);
	if (dim_size2 == 0) {
		grid_size = dim_size1;
		EXECUTION_REPORT(REPORT_ERROR, comp_id, size_center_lon == grid_size, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"center_lon\" is different from the grid size that is determined by \"dim_size1\". Please check the model code related to the annotation \"%s\".", annotation);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, size_center_lat == grid_size, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"center_lat\" is different from the grid size that is determined by \"dim_size1\". Please check the model code related to the annotation \"%s\".", annotation);
	}
	else {
		grid_size = (dim_size1)*(dim_size2);
		if (size_center_lon == dim_size1)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, size_center_lat == dim_size2, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"center_lat\" is different from \"dim_size2\". Please check the model code related to the annotation \"%s\".", annotation);
		else {			
			EXECUTION_REPORT(REPORT_ERROR, comp_id, size_center_lon == grid_size, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"center_lon\" is different from the grid size that is determined by the product of \"dim_size1\" and \"dim_size2\". Please check the model code related to the annotation \"%s\".", annotation);
			EXECUTION_REPORT(REPORT_ERROR, comp_id, size_center_lat == grid_size, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"center_lat\" is different from the grid size that is determined by the product of \"dim_size1\" and \"dim_size2\". Please check the model code related to the annotation \"%s\".", annotation);
		}	
	}
	if (size_mask != 0)		
		EXECUTION_REPORT(REPORT_ERROR, comp_id, size_mask == grid_size, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"mask\" is different from the grid size. Please check the model code related to the annotation \"%s\".", annotation);
	if (size_area != 0)		
		EXECUTION_REPORT(REPORT_ERROR, comp_id, size_area == grid_size, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"area\" is different from the grid size. Please check the model code related to the annotation \"%s\".", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, (size_vertex_lon == 0 && size_vertex_lat == 0) || (size_vertex_lon != 0 && size_vertex_lat != 0), "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: optional parameters \"vertex_lon\" and \"vertex_lat\" are not set/unset at the same time. Please check the model code related to the annotation \"%s\".", annotation);
	if (size_vertex_lon != 0) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, (size_vertex_lon) % (size_center_lon) == 0, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"vertex_lon\" is not an integral multiple of the array size of \"center_lon\". Please check the model code related to the annotation \"%s\".", annotation);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, (size_vertex_lat) % (size_center_lat) == 0, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"vertex_lat\" is not an integral multiple of the array size of \"center_lat\". Please check the model code related to the annotation \"%s\".", annotation);
		int num_vertex_lon = (size_vertex_lon) / (size_center_lon);
		int num_vertex_lat = (size_vertex_lat) / (size_center_lat);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, num_vertex_lon == num_vertex_lat, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the numbers of vertexes determined by \"vertex_lon\" and \"vertex_lat\" respectively are not the same. Please check the model code related to the annotation \"%s\".", annotation);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, size_vertex_lon > grid_size && size_vertex_lat > grid_size || size_vertex_lon < grid_size && size_vertex_lat < grid_size, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the array size of \"vertex_lon\" and \"vertex_lat\" are not bigger/smaller than the grid size at the same time. Please check the model code related to the annotation \"%s\".", annotation);
		if (size_center_lon == grid_size) 
			EXECUTION_REPORT(REPORT_ERROR, comp_id, num_vertex_lon >= 3, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the number of vertexes is wrong as it is smaller than 3. Please check the model code related to the annotation \"%s\".", annotation);
		else EXECUTION_REPORT(REPORT_ERROR, comp_id ,num_vertex_lon == 2, "Error happens when registering an H2D grid through API CCPL_register_H2D_grid_via_model_data: the number of vertexes is wrong as it is not 2. Please check the model code related to the annotation \"%s\".", annotation);
		num_vertex = num_vertex_lon;
	}
	sprintf(true_H2D_grid_name, "%s%s", grid_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, annotation)->get_full_name());
	sprintf(true_lon_grid_name, "lon_%s", true_H2D_grid_name);
	sprintf(true_lat_grid_name, "lat_%s", true_H2D_grid_name);
	if (dim_size2 == 0) {
		CoR_lon_grid = new Remap_grid_class(true_lon_grid_name, "lon", coord_unit, cyclic_or_acyclic, 0);
		CoR_lat_grid = new Remap_grid_class(true_lat_grid_name, "lat", coord_unit, cyclic_or_acyclic, 0);
	}
	else {
		CoR_lon_grid = new Remap_grid_class(true_lon_grid_name, "lon", coord_unit, cyclic_or_acyclic, dim_size1);
		CoR_lat_grid = new Remap_grid_class(true_lat_grid_name, "lat", coord_unit, cyclic_or_acyclic, dim_size2);		
	}
	sub_grids[0] = CoR_lon_grid;
	sub_grids[1] = CoR_lat_grid;
	CoR_H2D_grid = new Remap_grid_class(true_H2D_grid_name, 2, sub_grids, grid_size);
	if (size_mask > 0)
		CoR_H2D_grid->read_grid_data_from_array("mask", "mask", DATA_TYPE_INT, (const char*)mask, 0);
	if (size_area > 0)
		CoR_H2D_grid->read_grid_data_from_array("area", "area", data_type, (const char*)area, 0);	
	if (size_center_lon == grid_size) {
		CoR_H2D_grid->read_grid_data_from_array("center", "lon", data_type, (const char*)center_lon, 0);
		CoR_H2D_grid->read_grid_data_from_array("center", "lat", data_type, (const char*)center_lat, 0);
		if (size_vertex_lon >0) {
			CoR_H2D_grid->read_grid_data_from_array("vertex", "lon", data_type, (const char*)vertex_lon, num_vertex);
			CoR_H2D_grid->read_grid_data_from_array("vertex", "lat", data_type, (const char*)vertex_lat, num_vertex);		
		}
	}
	else {
		CoR_lon_grid->read_grid_data_from_array("center", "lon", data_type, (const char*)center_lon, 0);
		CoR_lat_grid->read_grid_data_from_array("center", "lat", data_type, (const char*)center_lat, 0);		
		if (size_vertex_lon > 0) {
			CoR_lon_grid->read_grid_data_from_array("vertex", "lon", data_type, (const char*)vertex_lon, num_vertex);
			CoR_lat_grid->read_grid_data_from_array("vertex", "lat", data_type, (const char*)vertex_lat, num_vertex);		
		}
	}
	remap_grid_manager->add_remap_grid(CoR_lon_grid);
	remap_grid_manager->add_remap_grid(CoR_lat_grid);
	remap_grid_manager->add_remap_grid(CoR_H2D_grid);
	CoR_H2D_grid->end_grid_definition_stage(NULL);
	char nc_file_name[NAME_STR_SIZE];
	sprintf(nc_file_name, "%s/H2D_grids/%s.nc", comp_comm_group_mgt_mgr->get_root_working_dir(), CoR_H2D_grid->get_grid_name());
	if (comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_id, "in register_h2d_grid_with_data") == 0) {
		IO_netcdf *netcdf_file_object = new IO_netcdf("H2D_grid_data", nc_file_name, "w", false);
		netcdf_file_object->write_grid(CoR_H2D_grid, false);
		netcdf_file_object->put_global_text("edge_type", edge_type);
		netcdf_file_object->put_global_text("cyclic_or_acyclic", cyclic_or_acyclic);
		delete netcdf_file_object;
	}
	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, annotation, CoR_H2D_grid));
	
	return original_grids[original_grids.size()-1]->get_grid_id();
}


int Original_grid_mgt::register_H2D_grid_via_file(int comp_id, const char *grid_name, const char *data_file_name, const char *annotation)
{
	int rcode, ncfile_id, grid_id;
	int size_center_lon, size_center_lat, size_mask, size_area, size_vertex_lon, size_vertex_lat;
	long dim_lon_size, dim_lat_size, dim_H2D_size, dim_size1, dim_size2;
	char *center_lon, *center_lat, *vertex_lon, *vertex_lat, *area;
	int ndims_for_center_lon, ndims_for_center_lat, ndims_for_vertex_lon, ndims_for_vertex_lat, ndims_for_mask, ndims_for_area;
	int *mask, *dims_for_center_lon, *dims_for_center_lat, *dims_for_vertex_lon, *dims_for_vertex_lat, *dims_for_mask, *dims_for_area;
	char data_type_for_center_lat[NAME_STR_SIZE], data_type_for_center_lon[NAME_STR_SIZE], data_type_for_vertex_lon[NAME_STR_SIZE], data_type_for_vertex_lat[NAME_STR_SIZE], data_type_for_mask[NAME_STR_SIZE], data_type_for_area[NAME_STR_SIZE];
	char edge_type[NAME_STR_SIZE], cyclic_or_acyclic[NAME_STR_SIZE], unit_center_lon[NAME_STR_SIZE], unit_center_lat[NAME_STR_SIZE], unit_vertex_lon[NAME_STR_SIZE], unit_vertex_lat[NAME_STR_SIZE];
	

	synchronize_comp_processes_for_API(comp_id, API_ID_GRID_MGT_REG_H2D_GRID_VIA_FILE, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_H2D_grid_via_file"), "registering an H2D grid", annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_H2D_GRID_VIA_FILE, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_H2D_grid_via_file"), "registering an H2D grid", grid_name, "grid_name", annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_H2D_GRID_VIA_FILE, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_H2D_grid_via_file"), "registering an H2D grid", data_file_name, "data_file_name", annotation);
	check_for_grid_definition(comp_id, grid_name, annotation);

	IO_netcdf *netcdf_file_object = new IO_netcdf("H2D_grid_data", data_file_name, "r", false);
	dim_lon_size = netcdf_file_object->get_dimension_size(COORD_LABEL_LON);
	dim_lat_size = netcdf_file_object->get_dimension_size(COORD_LABEL_LAT);
	dim_H2D_size = netcdf_file_object->get_dimension_size("grid_size");
	netcdf_file_object->read_file_field(COORD_LABEL_LON, (void**)(&center_lon), &ndims_for_center_lon, &dims_for_center_lon, &size_center_lon, data_type_for_center_lon);
	netcdf_file_object->read_file_field(COORD_LABEL_LAT, (void**)(&center_lat), &ndims_for_center_lat, &dims_for_center_lat, &size_center_lat, data_type_for_center_lat);
	netcdf_file_object->read_file_field("vertex_lon", (void**)(&vertex_lon), &ndims_for_vertex_lon, &dims_for_vertex_lon, &size_vertex_lon, data_type_for_vertex_lon);
	netcdf_file_object->read_file_field("vertex_lat", (void**)(&vertex_lat), &ndims_for_vertex_lat, &dims_for_vertex_lat, &size_vertex_lat, data_type_for_vertex_lat);
	netcdf_file_object->read_file_field("area", (void**)(&area), &ndims_for_area, &dims_for_area, &size_area, data_type_for_area);
	netcdf_file_object->read_file_field("mask", (void**)(&mask), &ndims_for_mask, &dims_for_mask, &size_mask, data_type_for_mask);
	if (dim_lon_size > 0 && dim_lat_size > 0 && dim_H2D_size > 0)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, dim_H2D_size == dim_lon_size*dim_lat_size, "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the size of dimension \"grid_size\" is different from the multiple of sizes of dimensions \"lon\" and \"lat\"", grid_name, annotation, data_file_name);
	if (dim_lon_size > 0 && dim_lat_size > 0) {
		dim_size1 = dim_lon_size;
		dim_size2 = dim_lat_size;
	}
	else {
		dim_size1 = dim_H2D_size;
		dim_size2 = 0;
	}

	EXECUTION_REPORT(REPORT_ERROR, comp_id, center_lon != NULL, "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: the longitude value for the center of each grid point is not specified in the data file \"%s\". ", 
		             grid_name, annotation, data_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, center_lat != NULL, "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: the latitude value for the center of each grid point is not specified in the data file \"%s\". ", 
		             grid_name, annotation, data_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, vertex_lon != NULL && vertex_lat != NULL || vertex_lon == NULL && vertex_lat == NULL, "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the longitude and latitude values for each vertex of each grid point must be specified/unspecified at the same time", 
					 grid_name, annotation, data_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(data_type_for_center_lon,data_type_for_center_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the data type of \"lon\" and \"lat\" are not the same", grid_name, annotation, data_file_name);
	if (vertex_lon != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(data_type_for_center_lon,data_type_for_vertex_lon), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the data type of \"vertex_lon\" is different from the data type of \"lon\".", grid_name, annotation, data_file_name);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(data_type_for_center_lon,data_type_for_vertex_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the data type of \"vertex_lat\" is different from the data type of \"lat\".", grid_name, annotation, data_file_name);
	}
	if (area != NULL)		
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(data_type_for_center_lon,data_type_for_vertex_lon), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the data type of \"area\" is different from the data type of \"lon\".", grid_name, annotation, data_file_name);
	if (mask != NULL)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(data_type_for_mask, DATA_TYPE_INT), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the data type of \"mask\" is not \"integer\".", grid_name, annotation, data_file_name);

	netcdf_file_object->get_global_text("edge_type", edge_type, NAME_STR_SIZE);
	netcdf_file_object->get_global_text("cyclic_or_acyclic", cyclic_or_acyclic, NAME_STR_SIZE);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, strlen(edge_type) > 0, "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", \"edge_type\" is not specified as a global attribute", grid_name, annotation, data_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, strlen(cyclic_or_acyclic) > 0, "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", \"cyclic_or_acyclic\" is not specified as a global attribute", grid_name, annotation, data_file_name);

	EXECUTION_REPORT(REPORT_ERROR, comp_id, netcdf_file_object->get_file_field_string_attribute(COORD_LABEL_LON, "unit", unit_center_lon), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: fail to get the unit of \"lon\" from the data file \"%s\"", grid_name, annotation, data_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, netcdf_file_object->get_file_field_string_attribute(COORD_LABEL_LAT, "unit", unit_center_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: fail to get the unit of \"lat\" from the data file \"%s\"", grid_name, annotation, data_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(unit_center_lon,unit_center_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the units of \"lon\" and \"lat\" are different", grid_name, annotation, data_file_name);
	if (vertex_lon != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, netcdf_file_object->get_file_field_string_attribute("vertex_lon", "unit", unit_vertex_lon), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: fail to get the unit of \"vertex_lon\" from the data file \"%s\"", grid_name, annotation, data_file_name);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, netcdf_file_object->get_file_field_string_attribute("vertex_lat", "unit", unit_vertex_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: fail to get the unit of \"vertex_lat\" from the data file \"%s\"", grid_name, annotation, data_file_name);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(unit_center_lon,unit_vertex_lon), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the units of \"lon\" and \"vertex_lon\" are different", grid_name, annotation, data_file_name);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(unit_center_lat,unit_vertex_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the units of \"lat\" and \"vertex_lat\" are different", grid_name, annotation, data_file_name);
	}

	grid_id = register_H2D_grid_via_data(comp_id, grid_name, edge_type, unit_center_lon, cyclic_or_acyclic, data_type_for_center_lon, dim_size1, dim_size2,size_center_lon, size_center_lat,
										 size_mask, size_area, size_vertex_lon, size_vertex_lat, center_lon, center_lat, mask, area, vertex_lon, vertex_lat, annotation, API_ID_GRID_MGT_REG_H2D_GRID_VIA_FILE);

	delete [] center_lon;
	delete [] dims_for_center_lon;
	delete [] center_lat;
	delete [] dims_for_center_lat;
	if (vertex_lon != NULL) {
		delete [] vertex_lon;
		delete [] vertex_lat;
		delete [] dims_for_vertex_lon;
		delete [] dims_for_vertex_lat;
	}
	if (mask != NULL) {
		delete [] mask;
		delete [] dims_for_mask;
	}
	if (area != NULL) {
		delete [] area;
		delete [] dims_for_area;
	}
}


int Original_grid_mgt::get_CoR_defined_grid(int comp_id, const char *grid_name, const char *CoR_grid_name, const char *annotation)
{
	Remap_grid_class *original_CoR_grid;

	
	check_for_grid_definition(comp_id, grid_name, annotation);
	original_CoR_grid = remap_grid_manager->search_remap_grid_with_grid_name(CoR_grid_name);
	if (original_CoR_grid == NULL)
		if (strlen(CoR_script_name) > 0)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "cannot register the grid \"%s\" because the CoR script \"%s\" does not define a grid named \"%s\". Please check the model code with the annotation \"%s\" or the CoR script",
				             grid_name, CoR_script_name, CoR_grid_name, annotation);	
		else EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "cannot register the grid \"%s\" because the CoR script is not specified. Please verify.",
				              grid_name, CoR_script_name, CoR_grid_name, annotation);	
	EXECUTION_REPORT(REPORT_ERROR, comp_id, original_CoR_grid->format_sub_grids(original_CoR_grid), "Please modify the definition of grid \"%s\" in the CoR script \"%s\". We propose to order the dimensions of the grid into the order such as lon, lat, level and time");
	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, annotation, original_CoR_grid));
	return original_grids[original_grids.size()-1]->get_local_grid_id();
}


bool Original_grid_mgt::is_grid_id_legal(int grid_id) const
{
	int true_grid_id = grid_id & TYPE_ID_SUFFIX_MASK;


	if ((grid_id & TYPE_ID_PREFIX_MASK) != TYPE_GRID_LOCAL_ID_PREFIX)
		return false;

	if (true_grid_id < 0 || true_grid_id >= original_grids.size())
		return false;

	return true;
}


int Original_grid_mgt::get_comp_id_of_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_comp_id_of_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_comp_id();
}


const char *Original_grid_mgt::get_name_of_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_name_of_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_grid_name();
}


Remap_grid_class *Original_grid_mgt::get_original_CoR_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_original_CoR_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_original_CoR_grid();
}


Original_grid_info *Original_grid_mgt::get_original_grid(int grid_id) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_original_CoR_grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK];
}

int Original_grid_mgt::get_grid_size(int grid_id, const char *annotation) const
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "The grid id used for getting grid size is wrong. Please verify the model code corresponding to the annotation \"%s\"", annotation);		
	return get_original_CoR_grid(grid_id)->get_grid_size();
}


int Original_grid_mgt::add_original_grid(int comp_id, const char *grid_name, Remap_grid_class *original_CoR_grid)
{
	Original_grid_info *existing_grid = search_grid_info(grid_name, comp_id);
	if (existing_grid != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, original_CoR_grid == existing_grid->get_original_CoR_grid(), "Software error in Original_grid_mgt::add_original_grid");
		return existing_grid->get_local_grid_id();
	}

	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, "Original_grid_mgt::add_original_grid", original_CoR_grid));
	return original_grids[original_grids.size()-1]->get_local_grid_id();
}


int Original_grid_mgt::get_num_grid_levels(int grid_id)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Software error in Original_grid_mgt::get_num_grid_levels: wrong grid id");		
	if (original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_V1D_sub_CoR_grid() == NULL)
		return 1;

	EXECUTION_REPORT(REPORT_ERROR, -1, original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_V1D_sub_CoR_grid()->get_grid_size() > 0, "Software error in Original_grid_mgt::get_num_grid_levels: wrong size of vertical grid");
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->get_V1D_sub_CoR_grid()->get_grid_size();
}


bool Original_grid_mgt::is_V1D_sub_grid_after_H2D_sub_grid(int grid_id)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, is_grid_id_legal(grid_id), "Original_grid_info::is_V1D_sub_grid_after_H2D_sub_grid: wrong grid id");		
	return original_grids[grid_id&TYPE_ID_SUFFIX_MASK]->is_V1D_sub_grid_after_H2D_sub_grid();
}

