/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "runtime_datamodel_algorithm.h"
#include "decomp_info_mgt.h"
#include "memory_mgt.h"
#include "runtime_config_dir.h"
#include "cor_global_data.h"


template <class T> static bool differ_two_arrays_accurately(const T *array1, const T *array2, const int array_size)
{
    bool array_different; 
    int i;


    array_different = false;
    for (i = 0; i < array_size; i ++)
        if (array1[i] != array2[i]) {
			printf("Wrong: %lf vs %lf at %d\n", array1[i], array2[i], i);
            array_different = true;
        }

    return array_different;
}


template <class T> static bool differ_two_arrays_with_error(const T *array1, const T *array2, const int array_size, const int error_level)
{
    bool array_different; 
    double max_error;
    int i;


    array_different = false;
    
    for (i = 0, max_error = 1.0; i < error_level; i ++)
        max_error *= 0.1;
    
    for (i = 0; i < array_size; i ++) {
        if (fabs(array2[i]) >= 1e-28) {
            if (fabs(array1[i]/array2[i] - 1) > max_error)
                array_different = true;
        }
        else 
            if(fabs(array1[i] - array2[i]) > 1e-28)
                array_different = true;
    }

    return array_different;
}


Runtime_datamodel_algorithm::Runtime_datamodel_algorithm(const char * cfg_file_name)
{
    netcdf_file_object = NULL;
    change_file_timer = NULL;
	fields_allocated = false;
	write_grid_name = false;
	strcpy(algorithm_cfg_name, cfg_file_name);

	generate_algorithm_info_from_cfg_file();
}


void Runtime_datamodel_algorithm::generate_algorithm_info_from_cfg_file()
{
    FILE * fp_cfg;
    char line[NAME_STR_SIZE * 16], *line_p;
    bool has_input_io_file_name;
	int num_fields;
    Datamodel_field_info *datamodel_field;
	int num_leaf_grids, total_num_leaf_grids;
	Remap_grid_class *leaf_grids[16], *total_leaf_grids[1024], *top_grid;
	int i, j, k;


    fp_cfg = open_config_file(algorithm_cfg_name, RUNTIME_DATAMODEL_ALG_DIR);
    get_next_line(line, fp_cfg);
    line_p = line;
    get_next_attr(datamodel_type, &line_p);
    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(datamodel_type, "datamodel_write") || words_are_the_same(datamodel_type, "datamodel_read") ||
                 words_are_the_same(datamodel_type, "datamodel_check"), "the type of data model must be datamodel_write, datamodel_read or datamodel_check\n");
    if (words_are_the_same(datamodel_type, "datamodel_write"))
        EXECUTION_REPORT(REPORT_ERROR, get_next_attr(write_type, &line_p) && 
                     (words_are_the_same(write_type, "inst") || words_are_the_same(write_type, "aver")), 
                     "for the data model of writing, user must specify the writing type: average(aver) or instantaneous(inst)\n");

    get_next_line(IO_file_type, fp_cfg);
    get_next_line(line, fp_cfg);
	line_p = line;
    io_timer = new Coupling_timer(&line_p);
    if (words_are_the_same(datamodel_type, "datamodel_write")) {
        get_next_line(line, fp_cfg);
		line_p = line;
        change_file_timer = new Coupling_timer(&line_p);
    }
    get_next_line(fields_cfg_file_name, fp_cfg);
    IO_file_name[0] = '\0';
    has_input_io_file_name = get_next_line(line, fp_cfg);
    if (!words_are_the_same(datamodel_type, "datamodel_write"))
        EXECUTION_REPORT(REPORT_ERROR, has_input_io_file_name, "please input the io file name for the non-writing data model\n");
    if (has_input_io_file_name)
        strcpy(IO_file_name, line);
    
    fclose(fp_cfg);

    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(IO_file_type,FILE_TYPE_NETCDF), "IO file type %s is not supported in C-Coupler\n", IO_file_type);

	num_fields = get_num_fields_in_config_file(fields_cfg_file_name, RUNTIME_DATAMODEL_ALG_DIR);
	if (words_are_the_same(datamodel_type, "datamodel_write"))
        num_src_fields = num_fields;
	else num_dst_fields = num_fields;

	allocate_basic_data_structure(num_src_fields, num_dst_fields);

	fp_cfg = open_config_file(fields_cfg_file_name, RUNTIME_DATAMODEL_ALG_DIR);
	for (i = 0; i < num_src_fields+num_dst_fields; i ++) {
		datamodel_field = new Datamodel_field_info;
		datamodel_field->have_scale_factor = false;
		datamodel_field->field_data_mem = NULL;
		datamodel_fields.push_back(datamodel_field);
		get_next_line(line, fp_cfg);
		line_p = line;
        get_next_attr(comp_names[i], &line_p);
        get_next_attr(field_names[i], &line_p);
        get_next_attr(field_local_decomp_names[i], &line_p);
        get_next_attr(field_grid_names[i], &line_p);
        buf_marks[i] = get_next_integer_attr(&line_p);
		EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(datamodel_type, "datamodel_read"), "alloc_mem for datamodel_read has not been well supported");
		if (words_are_the_same(datamodel_type, "datamodel_write")) {
			if (words_are_the_same(write_type, "aver"))
				average_mark[i] = true;
		}
        get_next_attr(datamodel_field->field_name_in_IO_file, &line_p);
        if (words_are_the_same(datamodel_type, "datamodel_write")) {
            EXECUTION_REPORT(REPORT_ERROR, get_next_attr(datamodel_field->field_datatype_IO_file, &line_p), "please input the data type in IO file of field %s\n", field_names[i]);
            get_data_type_size(datamodel_field->field_datatype_IO_file);
			strcpy(datamodel_field->cfg_info_remain_line, line_p);
        }		
	}
	fclose(fp_cfg);

	total_num_leaf_grids = 0;
	for (i = 0; i < num_src_fields+num_dst_fields; i ++) {
		if (words_are_the_same(field_grid_names[i], "NULL"))
			continue;
		if (write_grid_name)
			break;
		top_grid = remap_grid_manager->search_remap_grid_with_grid_name(field_grid_names[i]);
		top_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, top_grid);
		for (k = 0; k < num_leaf_grids; k ++) {
			for (j = 0; j < total_num_leaf_grids; j ++) 
				if (total_leaf_grids[j] != leaf_grids[k] && words_are_the_same(total_leaf_grids[j]->get_coord_label(), leaf_grids[k]->get_coord_label())) {
					write_grid_name = true;
					break;
				}
			if (write_grid_name)
				break;
			total_leaf_grids[total_num_leaf_grids++] = leaf_grids[k];
		}
	}
	if (write_grid_name)
		EXECUTION_REPORT(REPORT_LOG, true, "Datamodel algorithm %s will write grid name into grid dimensions", algorithm_cfg_name);
}


void Runtime_datamodel_algorithm::allocate_one_field(int field_indx)
{
	if (datamodel_fields[field_indx]->field_data_mem != NULL)
		return;

	if (words_are_the_same(datamodel_type, "datamodel_write")) {
		datamodel_fields[field_indx]->field_data_mem = alloc_mem(comp_names[field_indx], field_local_decomp_names[field_indx], field_grid_names[field_indx], field_names[field_indx], NULL, buf_marks[field_indx], true);
		add_runtime_datatype_transformation(datamodel_fields[field_indx]->field_data_mem, true, io_timer);
	}
    strcpy(datamodel_fields[field_indx]->field_data_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, datamodel_fields[field_indx]->field_name_in_IO_file);
    if (words_are_the_same(datamodel_type, "datamodel_write")) {
        check_application_io_datatype_consistency(field_names[field_indx], datamodel_fields[field_indx]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, datamodel_fields[field_indx]->field_datatype_IO_file);
	    if ((words_are_the_same(datamodel_fields[field_indx]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE) || words_are_the_same(datamodel_fields[field_indx]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT))
		    && words_are_the_same(datamodel_fields[field_indx]->field_datatype_IO_file, DATA_TYPE_SHORT)) {
		    char *line_p = datamodel_fields[field_indx]->cfg_info_remain_line;
		    datamodel_fields[field_indx]->add_offset = get_next_double_attr(&line_p);
		    datamodel_fields[field_indx]->scale_factor = get_next_double_attr(&line_p);
		    datamodel_fields[field_indx]->have_scale_factor = true;
		}
    }	
}


void Runtime_datamodel_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
	if (fields_allocated)
		return;

    for (int i = 0; i < datamodel_fields.size(); i ++) {
		if (!average_mark[i])
			continue;
		if (datamodel_fields[i]->field_data_mem != NULL)
			return;
		if (memory_manager->search_last_define_field(comp_names[i], field_local_decomp_names[i], field_grid_names[i], field_names[i], buf_marks[i], false) == NULL)
			continue;
		allocate_one_field(i);
		datamodel_fields[i]->field_data_mem = add_one_field_for_cumulate_average(datamodel_fields[i]->field_data_mem, io_timer);
		EXECUTION_REPORT(REPORT_LOG, true, "automatically average field %s in runtime data algorithm %s", field_names[i], algorithm_cfg_name);
    }

    if (is_algorithm_in_kernel_stage && !io_timer->is_timer_on())
        return;
	
	fields_allocated = true;

	EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(datamodel_type, "datamodel_read"), "alloc_mem for datamodel_read has not been well supported");

    for (int i = 0; i < datamodel_fields.size(); i ++)
		allocate_one_field(i);
}


void Runtime_datamodel_algorithm::datamodel_read()
{    
    long offset_in_binary_file = 0;
    long field_size;
    long initial_full_time, restart_full_time;
    char full_file_name[NAME_STR_SIZE];
    IO_netcdf *netcdf_file_object;
    

    if (words_are_the_same(IO_file_type, FILE_TYPE_NETCDF)) {
        for (int i = 0; i < datamodel_fields.size(); i++) { 
            sprintf(full_file_name, "%s", IO_file_name);
            netcdf_file_object = new IO_netcdf(full_file_name, full_file_name, "r", true);
            strcpy(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, datamodel_fields[i]->field_name_in_IO_file);
            netcdf_file_object->read_data(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field());
			datamodel_fields[i]->field_data_mem->define_field_values(false);
            delete netcdf_file_object;
        }
    }
}


void Runtime_datamodel_algorithm::datamodel_check()
{
    int field_size;
    bool is_arrays_different;
    long offset_in_binary_file = 0;
    Remap_grid_data_class *temp_field_data;
    char full_file_name[NAME_STR_SIZE];
    IO_netcdf *netcdf_file_object;


    for (int i = 0; i < datamodel_fields.size(); i++) { 
        sprintf(full_file_name, "%s", IO_file_name);
        netcdf_file_object = new IO_netcdf(full_file_name, full_file_name, "r", true);
        temp_field_data = new Remap_grid_data_class("TEMP_FIELD_DATA", datamodel_fields[i]->field_data_mem->get_field_data()->get_coord_value_grid(), 
                                                    netcdf_file_object, datamodel_fields[i]->field_name_in_IO_file); 
        field_size = datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->required_data_size;
        if (words_are_the_same(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_BOOL))
           is_arrays_different = differ_two_arrays_accurately((bool *)temp_field_data->get_grid_data_field()->data_buf, (bool *)datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_buf, field_size);
        else if (words_are_the_same(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_STRING))
           is_arrays_different = differ_two_arrays_accurately((char *)temp_field_data->get_grid_data_field()->data_buf, (char *)datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_buf, field_size);
        else if (words_are_the_same(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_SHORT))
           is_arrays_different = differ_two_arrays_accurately((short *)temp_field_data->get_grid_data_field()->data_buf, (short *)datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_buf, field_size);
        else if (words_are_the_same(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_INT))
           is_arrays_different = differ_two_arrays_accurately((int *)temp_field_data->get_grid_data_field()->data_buf, (int *)datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_buf, field_size);
        else if (words_are_the_same(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_LONG))
           is_arrays_different = differ_two_arrays_accurately((long *)temp_field_data->get_grid_data_field()->data_buf, (long *)datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_buf, field_size);
        else if (words_are_the_same(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT))
           is_arrays_different = differ_two_arrays_accurately((float *)temp_field_data->get_grid_data_field()->data_buf, (float *)datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_buf, field_size);
        else if (words_are_the_same(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE))
           is_arrays_different = differ_two_arrays_accurately((double *)temp_field_data->get_grid_data_field()->data_buf, (double *)datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_buf, field_size);
        else EXECUTION_REPORT(REPORT_ERROR, false, "Data type is not supported when checking in data model");
        
        if (is_arrays_different)
            EXECUTION_REPORT(REPORT_LOG, true, "check (%s, %s) different", IO_file_name, datamodel_fields[i]->field_name_in_IO_file);
        else
            EXECUTION_REPORT(REPORT_LOG, true, "check (%s, %s) OK", IO_file_name, datamodel_fields[i]->field_name_in_IO_file);
		datamodel_fields[i]->field_data_mem->use_field_values();
        delete netcdf_file_object;
        delete temp_field_data;
    }
}


void Runtime_datamodel_algorithm::datamodel_write()
{
    char full_file_name[NAME_STR_SIZE];
    long field_size;
    FILE *fp_binary;
    long current_full_time = timer_mgr->get_current_full_time();
    int i;
 

    if (strlen(IO_file_name) == 0)
		sprintf(full_file_name, "%s.%s.%s.%04d%04d%05d.nc", compset_communicators_info_mgr->get_current_case_name(), compset_communicators_info_mgr->get_current_comp_name(), 
						write_type, current_full_time/((long)1000000000), current_full_time%((long)1000000000)/100000, current_full_time%100000);
    else sprintf(full_file_name, "%s.nc", IO_file_name);
    if (words_are_the_same(IO_file_type,FILE_TYPE_NETCDF)) {
        if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
            if (netcdf_file_object == NULL) {
                netcdf_file_object = new IO_netcdf(full_file_name, full_file_name, "w", true);
                compset_communicators_info_mgr->write_case_info(netcdf_file_object);
            }
            else if (change_file_timer->is_timer_on()) {
                delete netcdf_file_object;
                netcdf_file_object = new IO_netcdf(full_file_name, full_file_name, "w", true);
                compset_communicators_info_mgr->write_case_info(netcdf_file_object);
            }
        }
        for (i = 0; i < datamodel_fields.size(); i ++) {
            strcpy(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, datamodel_fields[i]->field_name_in_IO_file);
            strcpy(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_IO_file, datamodel_fields[i]->field_datatype_IO_file);
            if (datamodel_fields[i]->have_scale_factor)
                datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->set_scale_factor_and_add_offset(datamodel_fields[i]->scale_factor, datamodel_fields[i]->add_offset);
			datamodel_fields[i]->field_data_mem->check_field_sum();
            fields_gather_scatter_mgr->gather_write_field(netcdf_file_object, datamodel_fields[i]->field_data_mem, write_grid_name, timer_mgr->get_current_date(), timer_mgr->get_current_second(), false);
            datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->clean_scale_factor_and_add_offset_info();
            strcpy(datamodel_fields[i]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_IO_file, "\0");
			datamodel_fields[i]->field_data_mem->use_field_values();
        }
    }
}


Runtime_datamodel_algorithm::~Runtime_datamodel_algorithm()
{
    for (int i = 0; i < datamodel_fields.size(); i ++)
        delete datamodel_fields[i];

    if (netcdf_file_object != NULL)
        delete netcdf_file_object; 
}


void Runtime_datamodel_algorithm::run(bool is_algorithm_in_kernel_stage)
{
    if (is_algorithm_in_kernel_stage && !io_timer->is_timer_on())
        return;

    if (words_are_the_same(datamodel_type, "datamodel_read"))
        datamodel_read();
    else if (words_are_the_same(datamodel_type, "datamodel_write"))
        datamodel_write();
    else if (words_are_the_same(datamodel_type, "datamodel_check"))
        datamodel_check();
}


void Runtime_datamodel_algorithm::change_IO_file_name_for_restart(const char *new_IO_file_name)
{
    for (int i = 0; i < datamodel_fields.size(); i ++)
        strcpy(IO_file_name, new_IO_file_name);
}

