/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "original_grid_mgt.h"
#include <netcdf.h>
#include <unistd.h>


Original_grid_info::Original_grid_info(int comp_id, int grid_id, const char *grid_name, const char *annotation, Remap_grid_class *original_CoR_grid)
{
	this->comp_id = comp_id;
	this->grid_id = grid_id;
	this->original_CoR_grid = original_CoR_grid;
	this->bottom_field_variation_type = BOTTOM_FIELD_VARIATION_UNSET;
	this->bottom_field_name[0] = '\0';
	this->bottom_field_id = -1;
	strcpy(this->grid_name, grid_name);
	annotation_mgr->add_annotation(this->grid_id, "grid_registration", annotation);
	generate_remapping_grids();

	if (H2D_sub_CoR_grid != NULL && V1D_sub_CoR_grid == NULL && T1D_sub_CoR_grid == NULL) {
		char nc_file_name[NAME_STR_SIZE];
		sprintf(nc_file_name, "%s/H2D_grids/%s@%s.nc", comp_comm_group_mgt_mgr->get_root_working_dir(), grid_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "Original_grid_info")->get_full_name());
		if (comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_id, "in register_h2d_grid_with_data") == 0) {
			IO_netcdf *netcdf_file_object = new IO_netcdf("H2D_grid_data", nc_file_name, "w", false);
			netcdf_file_object->write_grid(H2D_sub_CoR_grid, false);
			netcdf_file_object->put_global_text("edge_type", "LONLAT");   // to be modified
			Remap_grid_class *leaf_grids[256];
			int num_leaf_grids;
			H2D_sub_CoR_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, H2D_sub_CoR_grid);
			EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(leaf_grids[0]->get_coord_label(), COORD_LABEL_LON), "software error in Original_grid_info::Original_grid_info");
			if (leaf_grids[0]->get_grid_cyclic())					
				netcdf_file_object->put_global_text("cyclic_or_acyclic", "cyclic");
			else netcdf_file_object->put_global_text("cyclic_or_acyclic", "acyclic");
			delete netcdf_file_object;
		}
	}

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


void Original_grid_info::set_unique_bottom_field(int field_id, int type) 
{ 
	EXECUTION_REPORT(REPORT_ERROR, -1, bottom_field_variation_type == BOTTOM_FIELD_VARIATION_UNSET, "Software error in Original_grid_info::set_unique_bottom_field");
	bottom_field_variation_type = type;
	if (type != BOTTOM_FIELD_VARIATION_EXTERNAL) {
		Field_mem_info *field_inst = memory_manager->get_field_instance(field_id);
		strcpy(bottom_field_name, field_inst->get_field_name());
		bottom_field_id = field_id;
		decomp_grids_mgr->search_decomp_grid_info(field_inst->get_decomp_id(), get_original_CoR_grid(), false)->get_decomp_grid()->set_sigma_grid_dynamic_surface_value_field(field_inst->get_field_data());
	}
} 


void Original_grid_info::write_grid_into_array(char **temp_array_buffer, int &buffer_max_size, int &buffer_content_size)
{
	get_original_CoR_grid()->write_grid_into_array(temp_array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&bottom_field_variation_type, sizeof(int), temp_array_buffer, buffer_max_size, buffer_content_size);
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
		EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "Fail to register grid \"%s\" again because it has already been registered before. Please check the model code with the annotation \"%s\" and \"%s\"", grid_name, search_grid_info(grid_name, comp_id)->get_annotation(), annotation);
}


int Original_grid_mgt::register_H2D_grid_via_comp(int comp_id, const char *grid_name, const char *annotation)
{
	char XML_file_name[NAME_STR_SIZE], nc_file_name[NAME_STR_SIZE];
	const char *another_comp_full_name = NULL, *another_comp_grid_name = NULL;
	FILE *tmp_file;
	int line_number;

	
	check_for_grid_definition(comp_id, grid_name, annotation);
	synchronize_comp_processes_for_API(comp_id, API_ID_GRID_MGT_REG_H2D_GRID_VIA_COMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_H2D_grid_via_comp"), "register_H2D_grid_via_comp", annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_H2D_GRID_VIA_COMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_H2D_grid_via_comp"), "registering an H2D grid", grid_name, "grid_name", annotation);

	sprintf(XML_file_name, "%s/redirection_configs/%s.import.redirection.xml", comp_comm_group_mgt_mgr->get_config_all_dir(), comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "in register_H2D_grid_via_comp")->get_full_name());
	tmp_file = fopen(XML_file_name, "r");
	EXECUTION_REPORT(REPORT_ERROR, comp_id, tmp_file != NULL, "Error happens when calling the C-Coupler API \"CCPL_register_H2D_grid_from_another_component\" to register an H2D grid \"%s\": the grid redirection configuration file (\"%s\") does not exist. The API call is at the model code with the annotation \"%s\". ", grid_name, XML_file_name, annotation);
	fclose(tmp_file);

	TiXmlDocument XML_file(XML_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, XML_file.LoadFile(comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_H2D_grid_via_comp")), "Fail to read the XML configuration file \"%s\", because the file is not in a legal XML format. Please check.", XML_file_name);
	TiXmlElement *root_XML_element = XML_file.FirstChildElement();
	TiXmlNode *root_XML_element_node = (TiXmlNode*) root_XML_element;
	for (; root_XML_element_node != NULL; root_XML_element_node = root_XML_element_node->NextSibling()) {
		root_XML_element = root_XML_element_node->ToElement();
		if (words_are_the_same(root_XML_element->Value(),"component_grid_redirection_configuration"))
			break;
	}
	if (root_XML_element_node != NULL) {
		for (TiXmlNode *grid_XML_element_node = root_XML_element->FirstChild(); grid_XML_element_node != NULL; grid_XML_element_node = grid_XML_element_node->NextSibling()) {
			TiXmlElement *grid_XML_element = grid_XML_element_node->ToElement();
			const char *xml_grid_name = get_XML_attribute(comp_id, grid_XML_element, "local_grid_name", XML_file_name, line_number, "grid name of the current component", "the grid redirection configuration file");
			if (words_are_the_same(xml_grid_name, grid_name)) {
				another_comp_full_name = get_XML_attribute(comp_id, grid_XML_element, "another_comp_full_name", XML_file_name, line_number, "the full name of the another component", "the grid redirection configuration file");
				another_comp_grid_name = get_XML_attribute(comp_id, grid_XML_element, "another_comp_grid_name", XML_file_name, line_number, "the grid name of the another component", "the grid redirection configuration file");
				break;
			}
		}	
	}

	EXECUTION_REPORT(REPORT_ERROR, comp_id, another_comp_full_name != NULL, "Error happens when calling the C-Coupler API \"CCPL_register_H2D_grid_from_another_component\" to register an H2D grid \"%s\": the grid redirection configuration file (\"%s\") does not contain the information for this grid. The API call is at the model code with the annotation \"%s\". ", grid_name, XML_file_name, annotation);

	sprintf(nc_file_name, "%s/H2D_grids/%s@%s.nc", comp_comm_group_mgt_mgr->get_root_working_dir(), another_comp_grid_name, another_comp_full_name);
	EXECUTION_REPORT(REPORT_PROGRESS, comp_id, true, "Wait to read NetCDF file \"%s\" to register H2D grid \"%s\"", nc_file_name, grid_name);
	if (comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_id, "in register_H2D_grid_via_comp") == 0) {
		while (true) {
			sleep(1);
			int ncfile_id;
			int rcode = nc_open(nc_file_name, NC_NOWRITE, &ncfile_id);
			char cyclic_or_acyclic[NAME_STR_SIZE];
			if (rcode != NC_NOERR)
				continue;
			rcode = nc_get_att_text(ncfile_id, NC_GLOBAL, "cyclic_or_acyclic", cyclic_or_acyclic);
			if (rcode != NC_NOERR)
				continue;
			nc_close(ncfile_id);
			break;
		}
	}
	synchronize_comp_processes_for_API(comp_id, API_ID_GRID_MGT_REG_H2D_GRID_VIA_COMP, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_H2D_grid_via_comp"), "register_H2D_grid_via_comp", annotation);

	return register_H2D_grid_via_file(comp_id, grid_name, nc_file_name, annotation);
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
	sprintf(true_H2D_grid_name, "%s@%s", grid_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, annotation)->get_full_name());
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
		CoR_H2D_grid->read_grid_data_from_array("center", COORD_LABEL_LON, data_type, (const char*)center_lon, 0);
		CoR_H2D_grid->read_grid_data_from_array("center", COORD_LABEL_LAT, data_type, (const char*)center_lat, 0);
		if (size_vertex_lon >0) {
			CoR_H2D_grid->read_grid_data_from_array("vertex", COORD_LABEL_LON, data_type, (const char*)vertex_lon, num_vertex);
			CoR_H2D_grid->read_grid_data_from_array("vertex", COORD_LABEL_LAT, data_type, (const char*)vertex_lat, num_vertex);		
		}
	}
	else {
		CoR_lon_grid->read_grid_data_from_array("center", COORD_LABEL_LON, data_type, (const char*)center_lon, 0);
		CoR_lat_grid->read_grid_data_from_array("center", COORD_LABEL_LAT, data_type, (const char*)center_lat, 0);		
		if (size_vertex_lon > 0) {
			CoR_lon_grid->read_grid_data_from_array("vertex", COORD_LABEL_LON, data_type, (const char*)vertex_lon, num_vertex);
			CoR_lat_grid->read_grid_data_from_array("vertex", COORD_LABEL_LAT, data_type, (const char*)vertex_lat, num_vertex);		
		}
	}
	remap_grid_manager->add_remap_grid(CoR_lon_grid);
	remap_grid_manager->add_remap_grid(CoR_lat_grid);
	remap_grid_manager->add_remap_grid(CoR_H2D_grid);
	CoR_H2D_grid->end_grid_definition_stage(NULL);
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
	EXECUTION_REPORT(REPORT_ERROR, comp_id, netcdf_file_object->get_file_field_string_attribute(COORD_LABEL_LON, "unit", unit_center_lon) || netcdf_file_object->get_file_field_string_attribute(COORD_LABEL_LON, "units", unit_center_lon), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: fail to get the unit of \"lon\" from the data file \"%s\"", grid_name, annotation, data_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, netcdf_file_object->get_file_field_string_attribute(COORD_LABEL_LAT, "unit", unit_center_lat) || netcdf_file_object->get_file_field_string_attribute(COORD_LABEL_LAT, "units", unit_center_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: fail to get the unit of \"lat\" from the data file \"%s\"", grid_name, annotation, data_file_name);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(unit_center_lon,unit_center_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: in the data file \"%s\", the units of \"lon\" and \"lat\" are different", grid_name, annotation, data_file_name);
	if (vertex_lon != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, netcdf_file_object->get_file_field_string_attribute("vertex_lon", "unit", unit_vertex_lon) || netcdf_file_object->get_file_field_string_attribute("vertex_lon", "units", unit_vertex_lon), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: fail to get the unit of \"vertex_lon\" from the data file \"%s\"", grid_name, annotation, data_file_name);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, netcdf_file_object->get_file_field_string_attribute("vertex_lat", "unit", unit_vertex_lat) || netcdf_file_object->get_file_field_string_attribute("vertex_lat", "units", unit_vertex_lat), "Error happens when registering an H2D grid \"%s\" (the corresponding model code annotation is \"%s\") through API CCPL_register_H2D_grid_via_data_file: fail to get the unit of \"vertex_lat\" from the data file \"%s\"", grid_name, annotation, data_file_name);
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

	return grid_id;
}


int Original_grid_mgt::register_V1D_grid_via_data(int comp_id, const char *grid_name, const char *grid_type, const char *coord_unit, int grid_size, 
	                                              double value1, const double *value2, const double *value3, double value4, const char *annotation)
{
	char full_grid_name[NAME_STR_SIZE];
	Remap_grid_class *CoR_V1D_grid;
	
	
	synchronize_comp_processes_for_API(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_V1D_grid_via_data"), "registering a V1D grid", annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_V1D_grid_via_data"), "registering a V1D grid", grid_name, "grid_name", annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_V1D_grid_via_data"), "registering a V1D grid", grid_type, "grid_type", annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in register_V1D_grid_via_data"), "registering a V1D grid", coord_unit, "coord_unit", annotation);
	check_API_parameter_int(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_V1D_grid_via_data"), "registering a V1D grid", grid_size, "implicit grid size", annotation);
	check_API_parameter_data_array(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_V1D_grid_via_data"), "registering a V1D grid", sizeof(double), (const char*)(&value1), "floating-point parameters", annotation);
	check_API_parameter_data_array(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_V1D_grid_via_data"), "registering a V1D grid", sizeof(double)*grid_size, (const char*)(value2), "floating-point parameters", annotation);
	check_API_parameter_data_array(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_V1D_grid_via_data"), "registering a V1D grid", sizeof(double)*grid_size, (const char*)(value3), "floating-point parameters", annotation);
	check_API_parameter_data_array(comp_id, API_ID_GRID_MGT_REG_V1D_GRID_VIA_MODEL_DATA, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id,"in register_V1D_grid_via_data"), "registering a V1D grid", sizeof(double), (const char*)(&value4), "floating-point parameters", annotation);
	check_for_grid_definition(comp_id, grid_name, annotation);	

	sprintf(full_grid_name, "%s@%s", grid_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in register_V1D_grid_via_data")->get_full_name());
	CoR_V1D_grid = new Remap_grid_class(full_grid_name, COORD_LABEL_LEV, coord_unit, NULL, grid_size);
	if (words_are_the_same(grid_type, "Z grid")) {
		CoR_V1D_grid->read_grid_data_from_array("center", COORD_LABEL_LEV, DATA_TYPE_DOUBLE, (const char*)value2, 0);
	}
	else if (words_are_the_same(grid_type, "SIGMA grid")) {
		CoR_V1D_grid->set_lev_grid_sigma_info(value1, value2, NULL, value4);
	}
	else if (words_are_the_same(grid_type, "HYBRID grid")) {
		CoR_V1D_grid->set_lev_grid_sigma_info(value1, value2, value3, value4);
	}

	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, annotation, CoR_V1D_grid));

	remap_grid_manager->add_remap_grid(CoR_V1D_grid);
	
	return original_grids[original_grids.size()-1]->get_grid_id();	
}


int Original_grid_mgt::register_md_grid_via_multi_grids(int comp_id, const char *grid_name, int sub_grid1_id, int sub_grid2_id, int sub_grid3_id, const char *annotation)
{
	int num_sub_grids = 2, num_H2D_grid = 0, num_V1D_grid = 0, num_T1D_grid = 0;
	char full_grid_name[NAME_STR_SIZE];
	Remap_grid_class *CoR_MD_grid, *sub_grids[3];

	
	MPI_Comm local_comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Original_grid_mgt::register_md_grid_via_multi_grids");
	synchronize_comp_processes_for_API(comp_id, API_ID_GRID_MGT_REG_MD_GRID_VIA_MULTI_GRIDS, local_comm, "registering a multi-dimension grid", annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_MD_GRID_VIA_MULTI_GRIDS, local_comm, "registering a multi-dimension grid", grid_name, "grid_name", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid_mgr->is_grid_id_legal(sub_grid1_id), "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": sub_grid1_id is wrong. Please check the model code related to the annotation \"%s\".", grid_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid_mgr->get_comp_id_of_grid(sub_grid1_id) == comp_id, "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": the component corresponding to the grid with id of sub_grid1_id is different from the current component with the id of comp_id. Please check the model code related to the annotation \"%s\".", grid_name, annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_MD_GRID_VIA_MULTI_GRIDS, local_comm, "for registering a multi-dimension grid", original_grid_mgr->get_name_of_grid(sub_grid1_id), "sub_grid1_id (the corresponding grid name)", annotation);		
	EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid_mgr->is_grid_id_legal(sub_grid2_id), "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": sub_grid2_id is wrong. Please check the model code related to the annotation \"%s\".", grid_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid_mgr->get_comp_id_of_grid(sub_grid2_id) == comp_id, "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": the component corresponding to the grid with id of sub_grid2_id is different from the current component with the id of comp_id. Please check the model code related to the annotation \"%s\".", grid_name, annotation);
	check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_MD_GRID_VIA_MULTI_GRIDS, local_comm, "for registering a multi-dimension grid", original_grid_mgr->get_name_of_grid(sub_grid2_id), "sub_grid2_id (the corresponding grid name)", annotation);	
	sub_grids[0] = original_grid_mgr->get_original_CoR_grid(sub_grid1_id);
	sub_grids[1] = original_grid_mgr->get_original_CoR_grid(sub_grid2_id);
	int have_sub_grid3 = sub_grid3_id != -1? 1: 0;
	check_API_parameter_int(comp_id, API_ID_GRID_MGT_REG_MD_GRID_VIA_MULTI_GRIDS, local_comm, "for registering a multi-dimension grid", have_sub_grid3, "sub_grid3_id (specified or not)", annotation);
	if (sub_grid3_id != -1) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid_mgr->is_grid_id_legal(sub_grid3_id), "the sub_grid3_id for registering a multi-dimension grid \"%s\" through the API \"CCPL_register_MD_grid_via_multi_grids\" is wrong. Please check the model code related to the annotation \"%s\".", grid_name, annotation);		
		check_API_parameter_string(comp_id, API_ID_GRID_MGT_REG_MD_GRID_VIA_MULTI_GRIDS, local_comm, "for registering a multi-dimension grid", original_grid_mgr->get_name_of_grid(sub_grid3_id), "sub_grid3_id (the corresponding grid name)", annotation);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid_mgr->get_comp_id_of_grid(sub_grid3_id) == comp_id, "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": the component corresponding to the grid with id of sub_grid3_id is different from the current component with the id of comp_id. Please check the model code related to the annotation \"%s\".", grid_name, annotation);
		sub_grids[num_sub_grids++] = original_grid_mgr->get_original_CoR_grid(sub_grid3_id);
	}	
	check_for_grid_definition(comp_id, grid_name, annotation);

	for (int i = 0; i < num_sub_grids; i ++)
		if (sub_grids[i]->get_is_sphere_grid())
			num_H2D_grid ++;
		else if (sub_grids[i]->has_grid_coord_label(COORD_LABEL_LEV))
			num_V1D_grid ++;
		else if (sub_grids[i]->has_grid_coord_label(COORD_LABEL_TIME))
			num_T1D_grid ++;
		else EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": grid \"%s\" cannot be used to generate a multi-dimension grid. Please check the model code related to the annotation \"%s\".", grid_name, annotation);

	EXECUTION_REPORT(REPORT_ERROR, comp_id, num_H2D_grid <= 1, "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": more than one H2D grid are used to generate a multi-dimension grid. Please check the model code related to the annotation \"%s\".", grid_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, num_V1D_grid <= 1, "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": more than one V1D grid are used to generate a multi-dimension grid. Please check the model code related to the annotation \"%s\".", grid_name, annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, num_T1D_grid <= 1, "Error happends when calling the API \"CCPL_register_MD_grid_via_multi_grids\" to register a multi-dimension grid \"%s\": more than one T1D grid are used to generate a multi-dimension grid. Please check the model code related to the annotation \"%s\".", grid_name, annotation);
	
	sprintf(full_grid_name, "%s@%s", grid_name, comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"register_md_grid_via_multi_grids")->get_full_name());
	CoR_MD_grid = new Remap_grid_class(full_grid_name, num_sub_grids, sub_grids, 0);
	original_grids.push_back(new Original_grid_info(comp_id, original_grids.size()|TYPE_GRID_LOCAL_ID_PREFIX, grid_name, annotation, CoR_MD_grid));	
	remap_grid_manager->add_remap_grid(CoR_MD_grid);
	
	return original_grids[original_grids.size()-1]->get_grid_id();
}


void Original_grid_mgt::set_3d_grid_bottom_field(int comp_id, int grid_id, int field_id, int static_or_dynamic_or_external, int API_id, const char *API_label, const char *annotation)
{
	Original_grid_info *original_grid = original_grid_mgr->get_original_grid(grid_id);
	Original_grid_info *field_original_grid;
	MPI_Comm local_comm = comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "in Original_grid_mgt::set_3d_grid_bottom_field");


	synchronize_comp_processes_for_API(comp_id, API_id, local_comm, "setting the bottom field of a 3-D grid", annotation);
	check_API_parameter_string(comp_id, API_id, local_comm, "setting the bottom field of a 3-D grid", original_grid->get_grid_name(), "the name of the 3-D grid", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid->is_3D_grid(), "Error happens when calling API \"%s\" to set the bottom field of a 3-D grid \"%s\": this grid is not a 3-D grid. Please check the model code related to the annotation \"%s\".", API_label, original_grid->get_grid_name(), annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, original_grid->get_original_CoR_grid()->is_sigma_grid(), "Error happens when calling API \"%s\" to set the bottom field of the 3-D grid \"%s\": cannot set the bottom field to this grid because its V1D sub grid is not a SIGMA or HYBRID grid. Please check the model code related to the annotation \"%s\".", API_label, original_grid->get_grid_name(), annotation);
	if (original_grid->get_bottom_field_variation_type() != BOTTOM_FIELD_VARIATION_UNSET)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, false, "Error happens when calling API \"%s\" to set the bottom field of the 3-D grid \"%s\": the bottom field has been set before at the model code with the annotation \"%s\" and cannot be set again at the model code with the annotation \"%s\".", API_label, original_grid->get_grid_name(), annotation_mgr->get_annotation(grid_id, "set bottom field"), annotation);
	if (static_or_dynamic_or_external != BOTTOM_FIELD_VARIATION_EXTERNAL) {
		check_API_parameter_field_instance(comp_id, API_id, local_comm, "setting the bottom field of a 3-D grid", field_id, "the bottom field", annotation);
		Field_mem_info *field_inst = memory_manager->get_field_instance(field_id);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, field_inst->get_grid_id() != -1, "Error happens when calling API \"%s\" to set the bottom field of the 3-D grid \"%s\": the bottom field \"%s\" is not on a grid. Please check the model code related to the annotation \"%s\".", API_label, original_grid->get_grid_name(), field_inst->get_field_name(), annotation);
		field_original_grid = original_grid_mgr->get_original_grid(field_inst->get_grid_id());
		EXECUTION_REPORT(REPORT_ERROR, comp_id, field_original_grid->is_H2D_grid(), "Error happens when calling API \"%s\" to set the bottom field of the 3-D grid \"%s\": the grid \"%s\" corresponding to the bottom field \"%s\" is not a H2D grid. Please check the model code related to the annotation \"%s\".", API_label, original_grid->get_grid_name(), field_original_grid->get_grid_name(), field_inst->get_field_name(), annotation);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, field_original_grid->get_original_CoR_grid()->is_subset_of_grid(original_grid->get_original_CoR_grid()), "Error happens when calling API \"%s\" to set the bottom field of the 3-D grid \"%s\": the grid \"%s\" corresponding to the bottom field \"%s\" is not a sub grid of the 3-D grid. Please check the model code related to the annotation \"%s\".", API_label, original_grid->get_grid_name(), field_original_grid->get_grid_name(), field_inst->get_field_name(), annotation);

	}
	annotation_mgr->add_annotation(grid_id, "set bottom field", annotation);
	original_grid->set_unique_bottom_field(field_id, static_or_dynamic_or_external);
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

