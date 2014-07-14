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
    FILE * fp_cfg;
    char line[NAME_STR_SIZE * 16], *line_p;
    bool has_input_io_file_name;


    netcdf_file_object = NULL;
    change_file_timer = NULL;

    fp_cfg = open_config_file(cfg_file_name, RUNTIME_DATAMODEL_ALG_DIR);
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
    io_timer = new Coupling_timer(line);
    if (words_are_the_same(datamodel_type, "datamodel_write")) {
        get_next_line(line, fp_cfg);
        change_file_timer = new Coupling_timer(line);
    }
    get_next_line(line, fp_cfg);
    initialize_datamodel(line);
    IO_file_name[0] = '\0';
    has_input_io_file_name = get_next_line(line, fp_cfg);
    if (!words_are_the_same(datamodel_type, "datamodel_write"))
        EXECUTION_REPORT(REPORT_ERROR, has_input_io_file_name, "please input the io file name for the non-writing data model\n");
    if (has_input_io_file_name)
        strcpy(IO_file_name, line);
    
    fclose(fp_cfg);

    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(IO_file_type,FILE_TYPE_NETCDF), "IO file type %s is not supported in C-Coupler\n", IO_file_type);
}


void Runtime_datamodel_algorithm::initialize_datamodel(const char *in_file)
{
	Field_mem_info *pair_field;
    char line[NAME_STR_SIZE * 16];
    char comp_name[NAME_STR_SIZE];
    char field_name[NAME_STR_SIZE];
    char decomp_name[NAME_STR_SIZE];
    char grid_name[NAME_STR_SIZE];
    char IO_datatype[NAME_STR_SIZE];
    int i;
    Datamodel_field_info *datamodel_field;
    char * line_p;
    int buf_type;


    FILE * fp_cfg = open_config_file(in_file, RUNTIME_DATAMODEL_ALG_DIR);

    while (get_next_line(line, fp_cfg)) {
        datamodel_field = new Datamodel_field_info;
        datamodel_field->have_scale_factor = false;
        datamodel_fields.push_back(datamodel_field);
        line_p = line;
        get_next_attr(comp_name, &line_p);
        get_next_attr(field_name, &line_p);
        get_next_attr(decomp_name, &line_p);
        get_next_attr(grid_name, &line_p);
        buf_type = get_next_integer_attr(&line_p);
		EXECUTION_REPORT(REPORT_ERROR, !words_are_the_same(datamodel_type, "datamodel_read"), "alloc_mem for datamodel_read has not been well supported");
        datamodel_field->field_data_mem = alloc_mem(comp_name, decomp_name, grid_name, field_name, NULL, buf_type, true, &pair_field);
		if (words_are_the_same(datamodel_type, "datamodel_read"))
			add_runtime_datatype_transformation(datamodel_field->field_data_mem, true, io_timer);
        get_next_attr(datamodel_field->field_name_in_IO_file, &line_p);
        strcpy(datamodel_field->field_data_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, datamodel_field->field_name_in_IO_file);
        if (words_are_the_same(datamodel_type, "datamodel_write")) {
            EXECUTION_REPORT(REPORT_ERROR, get_next_attr(datamodel_field->field_datatype_IO_file, &line_p), "please input the data type in IO file of field %s\n", field_name);
            get_data_type_size(datamodel_field->field_datatype_IO_file);
            check_application_io_datatype_consistency(field_name, datamodel_field->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, datamodel_field->field_datatype_IO_file);
            if ((words_are_the_same(datamodel_field->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE) || words_are_the_same(datamodel_field->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT))
                && words_are_the_same(datamodel_field->field_datatype_IO_file, DATA_TYPE_SHORT)) {
                datamodel_field->add_offset = get_next_double_attr(&line_p);
                datamodel_field->scale_factor = get_next_double_attr(&line_p);
                datamodel_field->have_scale_factor = true;
            }
        }
    }
    
    fclose(fp_cfg);
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
            fields_gather_scatter_mgr->gather_write_field(netcdf_file_object, datamodel_fields[i]->field_data_mem, true, timer_mgr->get_current_date(), timer_mgr->get_current_second(), false);
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


void Runtime_datamodel_algorithm::run(bool is_alglrithm_in_kernel_stage)
{
    if (is_alglrithm_in_kernel_stage && !io_timer->is_timer_on())
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

