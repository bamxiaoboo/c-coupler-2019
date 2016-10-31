/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "runtime_datamodel_algorithm.h"
#include "decomp_info_mgt.h"
#include "memory_mgt.h"
#include "runtime_config_dir.h"
#include "cor_global_data.h"
#include "quick_sort.h"


#define TIME_FORMAT_YYYY                 (0x3C00)
#define TIME_FORMAT_YYYYMM               (0x3F00)
#define TIME_FORMAT_YYYYMMDD             (0x3FC0)
#define TIME_FORMAT_YYYYMMDDHHMMSS       (0x3FFF)
#define TIME_FORMAT_YYYYMMDDSSSSS        (0x3FFE)
#define TIME_FORMAT_MMDDSSSSS            (0x3FE)
#define TIME_FORMAT_MMDDHHMMSS           (0x3FF)
#define TIME_FORMAT_DDSSSSS              (0xFE)
#define TIME_FORMAT_DDHHMMSS             (0xFF)
#define TIME_FORMAT_SSSSS                (0x3E)
#define TIME_FORMAT_HHMMSS               (0x3F)
#define TIME_FORMAT_YYYYMMDDHHMM         (0x3FFC)
#define TIME_FORMAT_MMDDHHMM             (0x3FC)
#define TIME_FORMAT_DDHHMM               (0xFC)
#define TIME_FORMAT_HHMM                 (0x3C)
#define TIME_FORMAT_YYYYMMDDHH           (0x3FF0)
#define TIME_FORMAT_MMDDHH               (0x3F0)
#define TIME_FORMAT_DDHH                 (0xF0)
#define TIME_FORMAT_HH                   (0x30)


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
    last_read_full_time = -1;
    netcdf_file_object = NULL;
    change_file_timer = NULL;
    field_read_handler = NULL;
    fields_allocated = false;
    write_grid_name = false;
    IO_file_name_keyword[0] = '\0';
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
    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the type (such as \"datamodel_read\" and \"datamodel_write\") for the runtime data model algorithm \"%s\"", algorithm_cfg_name);
    line_p = line;
    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(datamodel_type, &line_p), "Please specify the type (such as \"datamodel_read\" and \"datamodel_write\") for the runtime data model algorithm \"%s\"", algorithm_cfg_name);
    EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(datamodel_type, "datamodel_write") || words_are_the_same(datamodel_type, "datamodel_read") || words_are_the_same(datamodel_type, "datamodel_check"), 
                     "The type of runtime data model algorithm must be \"datamodel_write\", \"datamodel_read\" or \"datamodel_check\". Please check configuration file \"%s\"", algorithm_cfg_name);
    if (words_are_the_same(datamodel_type, "datamodel_write"))
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(write_type, &line_p) && (words_are_the_same(write_type, "inst") || words_are_the_same(write_type, "aver")), 
                     "For the data model for writing data, please specify the type of the data for writing: \"aver\" (average) or \"inst\" (instantaneous). Please verify the configuration file \"%s\"", algorithm_cfg_name);

    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(IO_file_type, fp_cfg), "Please specify the file type (such as \"netcdf\") for the runtime data model algorithm \"%s\"", algorithm_cfg_name);
    EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(IO_file_type, FILE_TYPE_NETCDF), "Only the file type \"netcdf\" is supported for runtime data model algorithm currently. Please verify configuration file \"%s\". For the support of more file types, please contact liuli-cess@tsinghua.edu.cn", algorithm_cfg_name);
    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the timer for triggering the execution of the runtime data model algorithm \"%s\"", algorithm_cfg_name);
    line_p = line;
    io_timer = new Coupling_timer(&line_p, algorithm_cfg_name);
    if (words_are_the_same(datamodel_type, "datamodel_write")) {
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the timer for changing file name for data writing by the runtime data model algorithm \"%s\".", algorithm_cfg_name);
        line_p = line;
        change_file_timer = new Coupling_timer(&line_p, algorithm_cfg_name);
    }
    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(fields_cfg_file_name, fp_cfg), "Please specify the configuration file of fields for the runtime data model algorithm \"%s\"", algorithm_cfg_name);
    FILE *fp_tmp = open_config_file(fields_cfg_file_name, RUNTIME_DATAMODEL_ALG_DIR);
    fclose(fp_tmp);

    if (words_are_the_same(datamodel_type, "datamodel_read")) {
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the field read handler for the runtime data model algorithm \"%s\" that is for reading fields from input data files", algorithm_cfg_name);
        field_read_handler = datamodel_field_read_handler_mgr->get_a_handler(line);
    }

    if (get_next_line(line, fp_cfg))
        strcpy(IO_file_name_keyword, line);

    fclose(fp_cfg);

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
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(comp_names[i], &line_p), "Please specify the component name for the %dth field in the configuration file \"%s\"", i+1, fields_cfg_file_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(field_names[i], &line_p), "Please specify the field name for the %dth field in the configuration file \"%s\"", i+1, fields_cfg_file_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(field_local_decomp_names[i], &line_p), "Please specify the parallel decomposition name for the %dth field in the configuration file \"%s\"", i+1, fields_cfg_file_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(field_grid_names[i], &line_p), "Please specify the grid name for the %dth field in the configuration file \"%s\"", i+1, fields_cfg_file_name);
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_integer_attr(&line_p, buf_marks[i]), "Please verify or specify the buffer label (an integer) for the %dth field in the configuration file \"%s\"", i+1, fields_cfg_file_name);
        if (words_are_the_same(datamodel_type, "datamodel_write")) {
            if (words_are_the_same(write_type, "aver"))
                average_mark[i] = true;
        }
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(datamodel_field->field_name_in_IO_file, &line_p), "Please specify the variable name in the data file for the %dth field in the configuration file \"%s\"", i+1, fields_cfg_file_name);
        if (words_are_the_same(datamodel_type, "datamodel_write") || words_are_the_same(datamodel_type, "datamodel_read")) {
            if (words_are_the_same(datamodel_type, "datamodel_write"))
                EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(datamodel_field->field_datatype_IO_file, &line_p), "For the runtime data model algorithm \"%s\" for writing data, please specify the data type in IO file of field \"%s\"\n", algorithm_cfg_name, field_names[i]);
            /* NOTE!!! The data type should be determined automatically in C-Coupler2 */
            else EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(datamodel_field->field_datatype_IO_file, &line_p), "For the runtime data model algorithm \"%s\" for reading data, please specify the data type of field \"%s\" in C-Coupler\n", algorithm_cfg_name, field_names[i]);
            get_data_type_size(datamodel_field->field_datatype_IO_file);
            strcpy(datamodel_field->cfg_info_remain_line, line_p);
        }        
    }
    fclose(fp_cfg);

    if (words_are_the_same(datamodel_type, "datamodel_write")) {
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
            EXECUTION_REPORT(REPORT_LOG,-1, true, "Datamodel algorithm \"%s\" will write grid name into the names of grid dimensions", algorithm_cfg_name);
    }
}


void Runtime_datamodel_algorithm::allocate_one_field(int field_indx)
{
    if (datamodel_fields[field_indx]->field_data_mem != NULL)
        return;

    if (words_are_the_same(datamodel_type, "datamodel_write")) {
        datamodel_fields[field_indx]->field_data_mem = alloc_mem(comp_names[field_indx], field_local_decomp_names[field_indx], field_grid_names[field_indx], field_names[field_indx], NULL, buf_marks[field_indx], true, fields_cfg_file_name);
        add_runtime_datatype_transformation(datamodel_fields[field_indx]->field_data_mem, true, io_timer, fields_cfg_file_name);
    }
    else if (words_are_the_same(datamodel_type, "datamodel_read")) {        
        datamodel_fields[field_indx]->field_data_mem = alloc_mem(comp_names[field_indx], field_local_decomp_names[field_indx], field_grid_names[field_indx], field_names[field_indx], datamodel_fields[field_indx]->field_datatype_IO_file, buf_marks[field_indx], false, fields_cfg_file_name);
    }
    strcpy(datamodel_fields[field_indx]->field_data_mem->get_field_data()->get_grid_data_field()->field_name_in_IO_file, datamodel_fields[field_indx]->field_name_in_IO_file);
    if (words_are_the_same(datamodel_type, "datamodel_write")) {
        check_application_io_datatype_consistency(field_names[field_indx], datamodel_fields[field_indx]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, datamodel_fields[field_indx]->field_datatype_IO_file);
        if ((words_are_the_same(datamodel_fields[field_indx]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE) || words_are_the_same(datamodel_fields[field_indx]->field_data_mem->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT))
            && words_are_the_same(datamodel_fields[field_indx]->field_datatype_IO_file, DATA_TYPE_SHORT)) {
            char *line_p = datamodel_fields[field_indx]->cfg_info_remain_line;
            EXECUTION_REPORT(REPORT_ERROR,-1, get_next_double_attr(&line_p, datamodel_fields[field_indx]->add_offset), "Please specify or verify the offset value for transforming the data representations between float and short, for the %dth field in the configuration file \"%s\"", field_indx, algorithm_cfg_name);
            EXECUTION_REPORT(REPORT_ERROR,-1, get_next_double_attr(&line_p, datamodel_fields[field_indx]->scale_factor), "Please specify or verify the scale factor value for transforming the data representations between float and short, for the %dth field in the configuration file \"%s\"", field_indx, algorithm_cfg_name);
            datamodel_fields[field_indx]->have_scale_factor = true;
        }
    }    
}


void Runtime_datamodel_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
    if (fields_allocated)
        return;

    if (words_are_the_same(datamodel_type, "datamodel_write")) {
        for (int i = 0; i < datamodel_fields.size(); i ++) {
            if (!average_mark[i])
                continue;
            if (datamodel_fields[i]->field_data_mem != NULL)
                continue;
            if (memory_manager->search_last_define_field(comp_names[i], field_local_decomp_names[i], field_grid_names[i], field_names[i], buf_marks[i], false, fields_cfg_file_name) == NULL)
                continue;
            allocate_one_field(i);
            datamodel_fields[i]->field_data_mem = add_one_field_for_cumulate_average(datamodel_fields[i]->field_data_mem, io_timer);
            EXECUTION_REPORT(REPORT_LOG,-1, true, "automatically average field \"%s\" in runtime data algorithm \"%s\"", field_names[i], algorithm_cfg_name);
        }
    }

    if (is_algorithm_in_kernel_stage && !io_timer->is_timer_on())
        return;
    
    fields_allocated = true;

    for (int i = 0; i < datamodel_fields.size(); i ++)
        allocate_one_field(i);

    if (words_are_the_same(datamodel_type, "datamodel_read")) {
        for (int i = 0; i < datamodel_fields.size(); i ++)
            field_read_info_indexes.push_back(field_read_handler->register_read_info_for_a_field(datamodel_fields[i]->field_data_mem, NULL));
    }    
}


void Runtime_datamodel_algorithm::datamodel_read()
{    
    if (last_read_full_time == timer_mgr->get_current_full_time())
        return;
    
    for (int i = 0; i < datamodel_fields.size(); i++)
        field_read_handler->update_one_field(field_read_info_indexes[i]);

    last_read_full_time = timer_mgr->get_current_full_time();

    EXECUTION_REPORT(REPORT_LOG,-1, true, "finish field read for runtime data model algorithm \"%s\"", algorithm_cfg_name);
    for (int i = 0; i < datamodel_fields.size(); i++)
        datamodel_fields[i]->field_data_mem->check_field_sum();
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
        else EXECUTION_REPORT(REPORT_ERROR,-1, false, "Data type is not supported when checking in data model");
        
        if (is_arrays_different)
            EXECUTION_REPORT(REPORT_LOG,-1, true, "check (%s, \"%s\") different", IO_file_name, datamodel_fields[i]->field_name_in_IO_file);
        else
            EXECUTION_REPORT(REPORT_LOG,-1, true, "check (%s, \"%s\") OK", IO_file_name, datamodel_fields[i]->field_name_in_IO_file);
        datamodel_fields[i]->field_data_mem->use_field_values(fields_cfg_file_name);
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
        sprintf(full_file_name, "%s.%s.%s.%s.%04d%04d%05d.nc", compset_communicators_info_mgr->get_current_case_name(), compset_communicators_info_mgr->get_current_comp_name(), 
                        IO_file_name_keyword, write_type, current_full_time/((long)1000000000), current_full_time%((long)1000000000)/100000, current_full_time%100000);
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
            datamodel_fields[i]->field_data_mem->use_field_values(fields_cfg_file_name);
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


Datamodel_field_read_handler::Datamodel_field_read_handler(const char *cfg_name)
{
    FILE *fp_cfg;
    char line[NAME_STR_SIZE * 16], *line_p, time_offset_str[NAME_STR_SIZE], time_offset_format[NAME_STR_SIZE], report_hint[NAME_STR_SIZE], str[NAME_STR_SIZE];
    int date, second_in_day, i, j, num_stars = 0, time_offset;
    long full_time_format;


    pos_time_interval_left = -1;
    pos_time_interval_right = -1;
    default_time_pos = 2;
    id_time_format_time_offset = -1;
    time_offset = 0;
    num_time_field = 0;
    period = -1;
    
    strcpy(handler_name, cfg_name);
    fp_cfg = open_config_file(cfg_name, RUNTIME_DATAMODEL_ALG_DIR);    
    
    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg),  "The pattern of names of the files for data reading must be specified in field read handler \"%s\"", cfg_name);
    line_p = line;
    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(str, &line_p), "The pattern of names of the files for data reading must be specified in field read handler \"%s\"", cfg_name);
    for (i = strlen(str)-1; i >= 0; i --)
        if (str[i] == '/')
            break;
    if (i == -1)
        file_dir[0] = '\0';
    else {
        strncpy(file_dir, str, i+1);
        file_dir[i+1] = '\0';
    }
    strcpy(file_name_pattern, str+i+1);
    for (i = 0; i < strlen(file_dir); i ++)
        EXECUTION_REPORT(REPORT_ERROR,-1, file_dir[i] != '*', "The directory intra the file name pattern for the field read handler \"%s\" has \"*\", which is not allowed. Please verify.", cfg_name);
    for (i = 0, num_stars = 0; i < strlen(file_name_pattern); i ++)
        if (file_name_pattern[i] == '*')
            num_stars ++;
    EXECUTION_REPORT(REPORT_ERROR,-1, num_stars < 2, "The file name intra the file name pattern for the field read handler \"%s\" can have at most one \"*\". Please verify.", cfg_name);
        
    if (!get_next_attr(time_format_type_in_file_name, &line_p)) {
        time_format_type_in_file_name[0] = '\0';
        id_time_format_in_file_name = -1;
        EXECUTION_REPORT(REPORT_ERROR,-1, num_stars == 0, "For the field read handler \"%s\", as the specified file name pattern contains \"*\", the time format for the file name must be specified. Please verify.", cfg_name);
    }
    else {
        id_time_format_in_file_name = check_time_format(time_format_type_in_file_name, "in the name of input data files");
        EXECUTION_REPORT(REPORT_ERROR,-1, num_stars == 1, "For the field read handler \"%s\", as the specified file name pattern does not contain \"*\", the time format for the file name must not be specified. Please verify the file name pattern or the time format.", cfg_name);
    }

    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the period for the field read handler \"%s\". The period must be one of \"none\", \"year\", \"month\", and \"day\"", cfg_name);
    EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(line, "none") || words_are_the_same(line, "year") || words_are_the_same(line, "month") || words_are_the_same(line, "day"), 
                     "The period of a field read handler must be one of \"none\", \"year\", \"month\", and \"day\". Please verify the configuration file \"%s\"", cfg_name);
    if (words_are_the_same(line, "none"))
        period = 0;
    else if (words_are_the_same(line, "year")) {
        period = 1;
        EXECUTION_REPORT(REPORT_ERROR,-1, !timer_mgr->get_is_leap_year_on(), "For the simulation using cyclic yearly input data, leap year must be disabled. Please check the configuration file \"%s\"", cfg_name);
    }
    else if (words_are_the_same(line, "month"))
        period = 2;
    else period = 3;

    if (period == 0) {
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the time offset of field read handler \"%s\". Please specify \"0\" if there is no time offset", cfg_name);
        line_p = line;
        EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(time_offset_str, &line_p),    "C-Coupler error in Datamodel_field_read_handler::Datamodel_field_read_handler");
        if (words_are_the_same(time_offset_str, "0")) {
            time_offset = 0;
            id_time_format_time_offset = -1;
        }
        else {
            EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(time_offset_format, &line_p), "Please specify the format of time offset when the time offset is not \"0\" in field read handler \"%s\"", cfg_name);
            id_time_format_time_offset = check_time_format(time_offset_format, "time offset");
            EXECUTION_REPORT(REPORT_ERROR,-1, id_time_format_time_offset != TIME_FORMAT_HHMMSS && id_time_format_time_offset != TIME_FORMAT_SSSSS && 
                             id_time_format_time_offset != TIME_FORMAT_YYYYMMDDSSSSS && id_time_format_time_offset != TIME_FORMAT_YYYYMMDDHHMMSS &&
                             id_time_format_time_offset != TIME_FORMAT_MMDDHH && id_time_format_time_offset != TIME_FORMAT_MMDDHHMM &&
                             id_time_format_time_offset != TIME_FORMAT_MMDDHHMMSS && id_time_format_time_offset != TIME_FORMAT_MMDDSSSSS &&
                             id_time_format_time_offset != TIME_FORMAT_DDHH && id_time_format_time_offset != TIME_FORMAT_DDHHMM &&
                             id_time_format_time_offset != TIME_FORMAT_DDHHMMSS && id_time_format_time_offset != TIME_FORMAT_DDSSSSS, 
                             "The time format \"%s\" is legal but not supported for time offset. Please check the field read handler \"%s\"", time_offset_format, handler_name);        
            if (time_offset_str[0] == '-')
                EXECUTION_REPORT(REPORT_ERROR,-1, get_time_from_string(time_offset_str+1, time_offset_format, id_time_format_time_offset, strlen(time_offset_str+1), false, date, second_in_day),
                                 "The time offset \"%s\" specified for field read handler \"%s\" is wrong (does not match the specified format). Please check.", time_offset_str, handler_name);
            else EXECUTION_REPORT(REPORT_ERROR,-1, get_time_from_string(time_offset_str, time_offset_format, id_time_format_time_offset, strlen(time_offset_str), false, date, second_in_day),
                                 "The time offset  \"%s\" specified for field read handler \"%s\" is wrong (does not match the specified format). Please check.", time_offset_str, handler_name);
            time_offset = date;
            if (id_time_format_time_offset == TIME_FORMAT_YYYY)
                time_offset = time_offset+101;
            if (id_time_format_time_offset == TIME_FORMAT_YYYYMM)
                time_offset = time_offset+1;
            if (time_offset_str[0] == '-')
                time_offset = -time_offset;
        }
        EXECUTION_REPORT(REPORT_LOG,-1, true, "The time offset specified in field read handler \"%s\" is %d", handler_name, time_offset);
        if (time_offset != 0)
            EXECUTION_REPORT(REPORT_ERROR,-1, timer_mgr->check_is_time_legal(abs(time_offset)/10000, (abs(time_offset)%10000/100), abs(time_offset)%100, 0, NULL),
                             "The time offset specified for the field read handler \"%s\" is not legal. Please check.", handler_name);
        if (time_offset != 0)
            offset_num_elapsed_day = timer_mgr->calculate_elapsed_day((abs(time_offset))/10000, ((abs(time_offset))%10000)/100, ((abs(time_offset))%100));
        else offset_num_elapsed_day = 0;
        if (time_offset < 0)
            offset_num_elapsed_day = -offset_num_elapsed_day;
    }
    else offset_num_elapsed_day = 0;
    
    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the number of time fields (the fields with time information such as date and second) in the input data files of field read handler \"%s\"", cfg_name);
    line_p = line;
    EXECUTION_REPORT(REPORT_ERROR,-1, get_next_integer_attr(&line_p, num_time_field), "Please verify the number of time fields (the fields with time information such as date and second) in the input data files of field read handler \"%s\". \"%s\" is a not a number (integer).", cfg_name, line);
    EXECUTION_REPORT(REPORT_LOG,-1, true, "The number of time fields specified in field read handler \"%s\" is %d", handler_name, num_time_field);
    EXECUTION_REPORT(REPORT_ERROR,-1, num_time_field >= 0 && num_time_field <= 16, "The number of time fields (the fields with time information such as date and second) in the input data files of field read handler \"%s\" must be between 0 and 16. Please check.", cfg_name);
    if (num_time_field > 0) {
        for (i = 0; i < num_time_field; i ++) {
            EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "The time fields specified in the configuration file of field read handler \"%s\" is not enough (does not match the specified number of time fields)", cfg_name);
            line_p = line;
            EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(time_field_name[i], &line_p),  "C-Coupler error in Datamodel_field_read_handler::Datamodel_field_read_handler");
            EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(time_field_format[i], &line_p),  "The format for time field \"%s\" is not specified in the configuration file of field read handler \"%s\". Please check", time_field_name[i], cfg_name);
            sprintf(report_hint, "time field \"%s\"", time_field_name[i]);
            time_field_format_id[i] = check_time_format(time_field_format[i], report_hint);
        }
        for (i = 0; i < num_time_field; i ++)
            for (j = i+1; j < num_time_field; j ++)
                EXECUTION_REPORT(REPORT_ERROR,-1, (time_field_format_id[i]&time_field_format_id[j]) == 0, "The format of two time fields (\"%s\" and \"%s\") in input data files specified for runtime datamodel algorithm \"%s\" have overlaps. Please verify.", time_field_name[i], time_field_name[j], handler_name);
        full_time_format = 0;
        for (i = 0; i < num_time_field; i ++)
            full_time_format = (full_time_format | time_field_format_id[i]);
        EXECUTION_REPORT(REPORT_ERROR,-1, full_time_format==TIME_FORMAT_HHMMSS || full_time_format==TIME_FORMAT_SSSSS || full_time_format==TIME_FORMAT_YYYY || full_time_format==TIME_FORMAT_YYYYMM || 
                         full_time_format==TIME_FORMAT_YYYYMMDD || full_time_format==TIME_FORMAT_YYYYMMDDHHMMSS || full_time_format==TIME_FORMAT_YYYYMMDDSSSSS || full_time_format==TIME_FORMAT_MMDDHH ||
                         full_time_format==TIME_FORMAT_MMDDHHMM || full_time_format==TIME_FORMAT_MMDDHHMMSS || full_time_format==TIME_FORMAT_MMDDSSSSS || full_time_format==TIME_FORMAT_HH || 
                         full_time_format==TIME_FORMAT_HHMM || full_time_format==TIME_FORMAT_DDHH || full_time_format==TIME_FORMAT_DDHHMM || full_time_format==TIME_FORMAT_DDHHMMSS || 
                         full_time_format == TIME_FORMAT_DDSSSSS || full_time_format==TIME_FORMAT_YYYYMMDDHH || full_time_format==TIME_FORMAT_YYYYMMDDHHMM,
                         "The format of time fields in input data files specified for field read handler \"%s\" is not supported. Please verify.", handler_name);    
    }
	else full_time_format = id_time_format_in_file_name;
	if (period == 0)
		EXECUTION_REPORT(REPORT_ERROR,-1, full_time_format == TIME_FORMAT_YYYY || full_time_format == TIME_FORMAT_YYYYMM || full_time_format == TIME_FORMAT_YYYYMMDD ||
		                 full_time_format == TIME_FORMAT_YYYYMMDDHH || full_time_format == TIME_FORMAT_YYYYMMDDHHMM || full_time_format == TIME_FORMAT_YYYYMMDDHHMMSS ||
		                 full_time_format == TIME_FORMAT_YYYYMMDDSSSSS, "The time information for the field read handler \"%s\" does not contain the number of years. Therefore the handler cannot be cyclic. Please verify", handler_name);

    if (get_next_line(line, fp_cfg)) {
        EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(line, "start") || words_are_the_same(line, "middle") || words_are_the_same(line, "end"), 
                         "The position of default time must be one of \"start\", \"middle\" and \"end\". Please check the configuration of field read handler \"%s\"", handler_name);
        if (words_are_the_same(line, "start"))
            default_time_pos = 1;
        else if (words_are_the_same(line, "middle"))
            default_time_pos = 2;
        else default_time_pos = 3;
    }

    initialize_time_filename_map();
}


int Datamodel_field_read_handler::register_read_info_for_a_field(Field_mem_info *field_instance, Remap_weight_of_strategy_class *remap_weights)
{
    Field_read_info read_info;
    int buf_mark_left = 100,  buf_mark_right = 200, buf_mark_readin = 300;

    
    if (remap_weights != NULL)
        EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(field_instance->get_grid_name(),remap_weights->get_data_grid_dst()->get_grid_name()), 
                         "C-Coupler error in Datamodel_field_read_handler::register_read_info_for_a_field");
    EXECUTION_REPORT(REPORT_ERROR,-1, remap_weights == NULL, "Data interpolation in field read function of runtime data model algorithm is not supported currently. Please contact liuli-cess@tsinghua.edu.cn");

    for (int i = 0; i < fields_read_info.size(); i ++)
        if (fields_read_info[i].output_field_instance == field_instance) {
            fields_read_info[i].reference_count ++;
            return i;
        }

    read_info.remap_algorithm_for_readin_left = NULL;
    read_info.remap_algorithm_for_readin_right = NULL;
    read_info.last_pos_time_interval_left = -1;
    read_info.last_pos_time_interval_right = -1;
    read_info.last_read_full_time = -1;
    read_info.reference_count = 1;
    read_info.output_field_instance = field_instance;
    char *data_type = field_instance->get_field_data()->get_grid_data_field()->data_type_in_application;
    read_info.temp_field_before_time_remap_left = alloc_mem(field_instance->get_comp_name(), field_instance->get_decomp_name(), field_instance->get_grid_name(), field_instance->get_field_name(), data_type, buf_mark_left, false, "  C-Coupler error  ");
    read_info.temp_field_before_time_remap_right = alloc_mem(field_instance->get_comp_name(), field_instance->get_decomp_name(), field_instance->get_grid_name(), field_instance->get_field_name(), data_type, buf_mark_right, false, "  C-Coupler error  ");
    if (remap_weights == NULL)
        read_info.temp_field_readin = alloc_mem(field_instance->get_comp_name(), field_instance->get_decomp_name(), field_instance->get_grid_name(), field_instance->get_field_name(), data_type, buf_mark_readin, false, "  C-Coupler error  ");
    else {
        EXECUTION_REPORT(REPORT_ERROR,-1, false, "data remapping in field read handler is not supported currently.");
        /* Note that remapping is not supported because the default parallel decomposition is not good */
    }
    strcpy(read_info.temp_field_before_time_remap_left->get_field_data()->get_grid_data_field()->field_name_in_IO_file, field_instance->get_field_data()->get_grid_data_field()->field_name_in_IO_file);
    strcpy(read_info.temp_field_before_time_remap_right->get_field_data()->get_grid_data_field()->field_name_in_IO_file, field_instance->get_field_data()->get_grid_data_field()->field_name_in_IO_file);
    strcpy(read_info.temp_field_readin->get_field_data()->get_grid_data_field()->field_name_in_IO_file, field_instance->get_field_data()->get_grid_data_field()->field_name_in_IO_file);    

    fields_read_info.push_back(read_info);
    return fields_read_info.size()-1;
}


int Datamodel_field_read_handler::check_time_format(const char *time_format, const char *report_hint)
{
    int id_time_format;


    if (words_are_the_same(time_format, "YYYY"))
        id_time_format = TIME_FORMAT_YYYY;
    else if (words_are_the_same(time_format, "YYYYMM") || words_are_the_same(time_format, "YYYY-MM"))
        id_time_format = TIME_FORMAT_YYYYMM;
    else if (words_are_the_same(time_format, "YYYYMMDD") || words_are_the_same(time_format, "YYYY-MM-DD"))
        id_time_format = TIME_FORMAT_YYYYMMDD;
    else if (words_are_the_same(time_format, "YYYYMMDD.SSSSS") || words_are_the_same(time_format, "YYYY-MM-DD.SSSSS") || 
             words_are_the_same(time_format, "YYYYMMDD-SSSSS") || words_are_the_same(time_format, "YYYY-MM-DD-SSSSS") ||
             words_are_the_same(time_format, "YYYYMMDDSSSSS"))
        id_time_format = TIME_FORMAT_YYYYMMDDSSSSS;
    else if (words_are_the_same(time_format, "YYYYMMDD.HHMMSS") || words_are_the_same(time_format, "YYYY-MM-DD.HH-MM-SS") ||
             words_are_the_same(time_format, "YYYYMMDD.HH-MM-SS") || words_are_the_same(time_format, "YYYY-MM-DD.HHMMSS") ||
             words_are_the_same(time_format, "YYYY-MM-DD-HH-MM-SS") || words_are_the_same(time_format, "YYYYMMDDHHMMSS"))
        id_time_format = TIME_FORMAT_YYYYMMDDHHMMSS;
    else if (words_are_the_same(time_format, "SSSSS"))
        id_time_format = TIME_FORMAT_SSSSS;
    else if (words_are_the_same(time_format, "HHMMSS") || words_are_the_same(time_format, "HH-MM-SS") || words_are_the_same(time_format, "HH:MM:SS"))
        id_time_format = TIME_FORMAT_HHMMSS;
    else if (words_are_the_same(time_format, "MMDD.HHMMSS") || words_are_the_same(time_format, "MMDD-HHMMSS") || words_are_the_same(time_format, "MM-DD.HH-MM-SS") ||
             words_are_the_same(time_format, "MM-DD-HH-MM-SS") || words_are_the_same(time_format, "MMDDHHMMSS"))
        id_time_format = TIME_FORMAT_MMDDHHMMSS;
    else if (words_are_the_same(time_format, "MMDD.SSSSS") || words_are_the_same(time_format, "MMDD-SSSSS") || words_are_the_same(time_format, "MM-DD.SSSSS") ||
             words_are_the_same(time_format, "MM-DD-SSSSS") || words_are_the_same(time_format, "MMDDSSSSS"))
        id_time_format = TIME_FORMAT_MMDDSSSSS;
    else if (words_are_the_same(time_format, "DD.HHMMSS") || words_are_the_same(time_format, "DD-HHMMSS") || words_are_the_same(time_format, "DD.HH-MM-SS") ||
             words_are_the_same(time_format, "DD-HH-MM-SS") || words_are_the_same(time_format, "DDHHMMSS"))
        id_time_format = TIME_FORMAT_DDHHMMSS;
    else if (words_are_the_same(time_format, "DD.SSSSS") || words_are_the_same(time_format, "DD-SSSSS") || words_are_the_same(time_format, "DDSSSSS"))
        id_time_format = TIME_FORMAT_DDSSSSS;
    else if (words_are_the_same(time_format, "YYYYMMDD.HHMM") || words_are_the_same(time_format, "YYYY-MM-DD.HH-MM") ||
             words_are_the_same(time_format, "YYYYMMDD.HH-MM") || words_are_the_same(time_format, "YYYY-MM-DD.HHMM") ||
             words_are_the_same(time_format, "YYYY-MM-DD-HH-MM") || words_are_the_same(time_format, "YYYYMMDDHHMM"))
        id_time_format = TIME_FORMAT_YYYYMMDDHHMM;
    else if (words_are_the_same(time_format, "HHMM") || words_are_the_same(time_format, "HH-MM") || words_are_the_same(time_format, "HH:MM"))
        id_time_format = TIME_FORMAT_HHMM;
    else if (words_are_the_same(time_format, "MMDD.HHMM") || words_are_the_same(time_format, "MMDD-HHMM") || words_are_the_same(time_format, "MM-DD.HH-MM") ||
             words_are_the_same(time_format, "MM-DD-HH-MM") || words_are_the_same(time_format, "MMDDHHMM"))
        id_time_format = TIME_FORMAT_MMDDHHMM;
    else if (words_are_the_same(time_format, "DD.HHMM") || words_are_the_same(time_format, "DD-HHMM") || words_are_the_same(time_format, "DD.HH-MM") ||
             words_are_the_same(time_format, "DD-HH-MM") || words_are_the_same(time_format, "DDHHMM"))
        id_time_format = TIME_FORMAT_DDHHMM;
    else if (words_are_the_same(time_format, "YYYYMMDD.HH") || words_are_the_same(time_format, "YYYY-MM-DD.HH") ||
             words_are_the_same(time_format, "YYYYMMDD.HH") || words_are_the_same(time_format, "YYYY-MM-DD.HH") ||
             words_are_the_same(time_format, "YYYY-MM-DD-HH") || words_are_the_same(time_format, "YYYYMMDDHH"))
        id_time_format = TIME_FORMAT_YYYYMMDDHH;
    else if (words_are_the_same(time_format, "HH"))
        id_time_format = TIME_FORMAT_HH;
    else if (words_are_the_same(time_format, "MMDD.HH") || words_are_the_same(time_format, "MMDD-HH") || words_are_the_same(time_format, "MM-DD.HH") ||
             words_are_the_same(time_format, "MM-DD-HH") || words_are_the_same(time_format, "MMDDHH"))
        id_time_format = TIME_FORMAT_MMDDHH;
    else if (words_are_the_same(time_format, "DD.HH") || words_are_the_same(time_format, "DD-HH") || words_are_the_same(time_format, "DDHH"))
        id_time_format = TIME_FORMAT_DDHH;
    else EXECUTION_REPORT(REPORT_ERROR,-1, false, "The time format \"%s\" for \"%s\" of field read handler \"%s\" is incorrect", time_format, report_hint, handler_name);

    return id_time_format;
}


bool Datamodel_field_read_handler::get_time_from_string(const char *str_in_fname, const char *time_format, int id_time_format, int str_size, bool append_default_time, int &date, int &second_in_day)
{
    long time = 0;
    int i;
    int year=0, month=0, day=0, hour=0, minute=0, second=0;


    date = 0;
    second_in_day = 0;

    if (str_size != strlen(time_format))
        return false;

    for (i = 0; i < str_size; i ++) {
        if (time_format[i] == '-' && str_in_fname[i] != '-')
            return false;
        if (time_format[i] == '.' && str_in_fname[i] != '.')
            return false;
        if (time_format[i] == ':' && str_in_fname[i] != ':')
            return false;
        if (time_format[i] == '-' || time_format[i] == '.' || time_format[i] == ':')
            continue;
        if (str_in_fname[i]-'0'< 0 || str_in_fname[i]-'9' > 0) 
            return false;
        time = time*10 + str_in_fname[i]-'0';
    }

    if (id_time_format == TIME_FORMAT_YYYY) {
        year = time;
        if (append_default_time) {
            if (default_time_pos == 1) {
                month = 1;
                day = 1;
            }
            else if (default_time_pos == 3) {
                month = 12;
                day = 31;
                hour = 23;
                minute = 59;
                second = 59;
            }
            else {
                month = 7;
                day = 2;
                if (timer_mgr->get_is_leap_year_on() && (((year%4) == 0 && (year%100) != 0) || (year%400) == 0))
                    hour = 0;
                else hour = 12;
            }
        }
    }
    else if (id_time_format == TIME_FORMAT_YYYYMM) {
        year = time / 100;
        month = time%100;
        if (append_default_time) {
            if (default_time_pos == 1)
                day = 1;
            else {
                if (timer_mgr->get_is_leap_year_on() && (((year%4) == 0 && (year%100) != 0) || (year%400) == 0)) {
                    if (default_time_pos == 2) {
                        day = num_days_of_month_of_leap_year[month-1]/2 + 1;
                        if ((num_days_of_month_of_leap_year[month-1]%2) == 1)
                            hour = 12;
                    }
                    else {
                        day = num_days_of_month_of_leap_year[month-1];
                        hour = 23;
                        minute = 59;
                        second = 59;                        
                    }
                }
                else {
                    if (default_time_pos == 2) {
                        day = num_days_of_month_of_nonleap_year[month-1]/2 + 1;
                        if ((num_days_of_month_of_nonleap_year[month-1]%2) == 1)
                            hour = 12;
                    }
                    else {
                        day = num_days_of_month_of_nonleap_year[month-1];
                        hour = 23;
                        minute = 59;
                        second = 59;                            
                    }
                }
            }
        }
    }
    else if (id_time_format == TIME_FORMAT_YYYYMMDD) {
        year = time / 10000;
        month = (time%10000)/100;
        day = time % 100;
        if (append_default_time) {
            if (default_time_pos == 2)
                hour = 12;
            else if (default_time_pos == 3) {
                hour = 23;
                minute = 59;
                second = 59;                    
            }
        }
    }
    else if (id_time_format == TIME_FORMAT_YYYYMMDDSSSSS) {
        year = time / 1000000000;
        month = (time%1000000000) / 10000000;
        day = (time%10000000) / 100000;
        hour = (time%100000) / 3600;
        minute = ((time%100000)%3600) / 60;
        second = (time%100000) % 60;
    }
    else if (id_time_format == TIME_FORMAT_YYYYMMDDHHMMSS){
        year = time / 10000000000;
        month = (time%10000000000) / 100000000;
        day = (time%100000000) / 1000000;
        hour = (time%1000000) / 10000;
        minute = (time%10000) / 100;
        second = time%100;
    }
    else if (id_time_format == TIME_FORMAT_HHMMSS){
        year = 0;
        month = 0;
        day = 0;
        hour = time / 10000;
        minute = (time%10000) / 100;
        second = time%100;
    }
    else if (id_time_format == TIME_FORMAT_SSSSS) {
        year = 0;
        month = 0;
        day = 0;
        hour = time / 3600;
        minute = (time%3600) / 60;
        second = time % 60;
    }
	else if (id_time_format == TIME_FORMAT_MMDDSSSSS) {
		year = 0;
        month = time / 10000000;
        day = (time%10000000) / 100000;
        hour = (time%100000) / 3600;
        minute = ((time%100000)%3600) / 60;
        second = (time%100000) % 60;		
	}
	else if (id_time_format == TIME_FORMAT_MMDDHHMMSS) {
        year = 0;
        month = time / 100000000;
        day = (time%100000000) / 1000000;
        hour = (time%1000000) / 10000;
        minute = (time%10000) / 100;
        second = time%100;		
	}
	else if (id_time_format == TIME_FORMAT_DDSSSSS) {
		year = 0;
        month = 0;
        day = time / 100000;
        hour = (time%100000) / 3600;
        minute = ((time%100000)%3600) / 60;
        second = (time%100000) % 60;		
	}
	else if (id_time_format == TIME_FORMAT_DDHHMMSS) {
        year = 0;
        month = 0;
        day = time / 1000000;
        hour = (time%1000000) / 10000;
        minute = (time%10000) / 100;
        second = time%100;		
	}
	else if (id_time_format == TIME_FORMAT_YYYYMMDDHHMM) {
        year = time / 100000000;
        month = (time%100000000) / 1000000;
        day = (time%1000000) / 10000;
        hour = (time%10000) / 100;
        minute = (time%100);
        second = 0;
	}
	else if (id_time_format == TIME_FORMAT_MMDDHHMM) {
        year = 0;
        month = time / 1000000;
        day = (time%1000000) / 10000;
        hour = (time%10000) / 100;
        minute = (time%100);
        second = 0;
	}
	else if (id_time_format == TIME_FORMAT_DDHHMM) {
        year = 0;
        month = 0;
        day = time / 10000;
        hour = (time%10000) / 100;
        minute = (time%100);
        second = 0;		
	}
	else if (id_time_format == TIME_FORMAT_HHMM) {
        year = 0;
        month = 0;
        day = 0;
        hour = time / 100;
        minute = (time%100);
        second = 0;		
	}	
	else if (id_time_format == TIME_FORMAT_YYYYMMDDHH) {
        year = time / 1000000;
        month = (time%1000000) / 10000;
        day = (time%10000) / 100;
        hour = (time%100);
        minute = 0;
        second = 0;
	}
	else if (id_time_format == TIME_FORMAT_MMDDHH) {
        year = 0;
        month = time / 10000;
        day = (time%10000) / 100;
        hour = (time%100);
        minute = 0;
        second = 0;
	}
	else if (id_time_format == TIME_FORMAT_DDHH) {
        year = 0;
        month = 0;
        day = time / 100;
        hour = (time%100);
        minute = 0;
        second = 0;
	}
	else if (id_time_format == TIME_FORMAT_HH) {
        year = 0;
        month = 0;
        day = 0;
        hour = (time%100);
        minute = 0;
        second = 0;
	}
    else EXECUTION_REPORT(REPORT_ERROR,-1, false, "C-Coupler error1 in get_time_from_string");

    date = year*10000 + month*100 + day;
    second_in_day = hour*3600+minute*60+second;    
    
    return true;
}


void Datamodel_field_read_handler::initialize_time_filename_map()
{
    int i, j, k, star_pos_in_fname_pattern = -1;
    int date, second_in_day;
    int last_dim_size, current_dim_size;
    Time_location_in_files time_location;
    std::vector<Time_location_in_files> time_filename_map_unsorted;
    std::vector<Time_location_in_files> time_filename_map_sorted;
    long *full_time, *pos;
    char time_string1[NAME_STR_SIZE], time_string2[NAME_STR_SIZE];


    for (i = 0; i < strlen(file_name_pattern); i ++)
        if (file_name_pattern[i] == '*') {
            EXECUTION_REPORT(REPORT_ERROR,-1, star_pos_in_fname_pattern == -1, "C-Coupler error1 in initialize_time_filename_map");
            star_pos_in_fname_pattern = i;
        }

    if (star_pos_in_fname_pattern != -1) {
        DIR *dir=opendir(file_dir);
		EXECUTION_REPORT(REPORT_ERROR, -1, dir != NULL, "The directory \"%s\" specified in the field read handler \"%s\" cannot be opened. Please verify.", file_dir, handler_name);
        struct dirent *ent;
        while((ent=readdir(dir)) != NULL) {
            int size = strlen(ent->d_name); 
            if(strcmp(ent->d_name + (size - (strlen(file_name_pattern)-star_pos_in_fname_pattern-1)), file_name_pattern+star_pos_in_fname_pattern+1) != 0 ||
                strncmp(ent->d_name, file_name_pattern, star_pos_in_fname_pattern) != 0 || !get_time_from_string(ent->d_name+star_pos_in_fname_pattern, time_format_type_in_file_name, id_time_format_in_file_name, size-strlen(file_name_pattern)+1, true, date, second_in_day))
                continue;
            EXECUTION_REPORT(REPORT_ERROR,-1, timer_mgr->check_is_time_legal(date/10000, (date%10000/100), date%100, second_in_day, NULL),
                             "The time implicitly included in the name of input data file \"%s\" is not legal. Please check.", ent->d_name);            
            time_location.date = date;
            time_location.second_in_day = second_in_day;
            time_location.time_dim_num = -1;
            strcpy(time_location.file_name, file_dir);
            strcat(time_location.file_name, ent->d_name);
            time_filename_map_unsorted.push_back(time_location);
        }
    }
    else {
        EXECUTION_REPORT(REPORT_WARNING, -1, false, "only one input data file \"%s\" is specified for field read handler \"%s\"", file_name_pattern, handler_name);
        time_location.date = -1;
        time_location.second_in_day = -1;
        time_location.time_dim_num = -1;
        strcpy(time_location.file_name, file_dir);
        strcat(time_location.file_name, file_name_pattern);
        time_filename_map_unsorted.push_back(time_location);
    }

    if (time_filename_map_unsorted.size() > 1) {
        full_time = new long [time_filename_map_unsorted.size()];
        pos = new long [time_filename_map_unsorted.size()];
        for (i = 0; i < time_filename_map_unsorted.size(); i ++) {
            full_time[i] = ((long)time_filename_map_unsorted[i].date)*100000 + time_filename_map_unsorted[i].second_in_day;
            pos[i] = i;
        }
        do_quick_sort(full_time, pos, 0, time_filename_map_unsorted.size()-1);
        for (i = 0; i < time_filename_map_unsorted.size(); i ++)
            time_filename_map_sorted.push_back(time_filename_map_unsorted[pos[i]]);
        delete [] full_time;
        delete [] pos;
    }
    else if (time_filename_map_unsorted.size() == 1)
        time_filename_map_sorted.push_back(time_filename_map_unsorted[0]);
    else EXECUTION_REPORT(REPORT_ERROR,-1, false, "None input data file corresponding to field read handler \"%s\" can be found. Please check the file name pattern or the time format.", handler_name);

    if (num_time_field > 0) {
        for (i = 0; i < time_filename_map_sorted.size(); i ++) {
            IO_netcdf *ncfile_handler = new IO_netcdf("tmp_file_handler", time_filename_map_sorted[i].file_name, "r", false);
            void *data_array;
            int *dim_size;
            char data_type[NAME_STR_SIZE];
            int num_dims;
            for (j = 0; j < num_time_field; j ++) {
                ncfile_handler->read_file_field(time_field_name[j], &data_array, &num_dims, &dim_size, data_type);
                printf("read for \"%s\" \"%s\" \"%s\"\n", time_field_name[j], time_filename_map_sorted[i].file_name, data_type);
                fflush(NULL);
                EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(data_type,DATA_TYPE_INT)||words_are_the_same(data_type,DATA_TYPE_CHAR), "The type of field \"%s\" in file \"%s\" must be integer or char", time_field_name[j], time_filename_map_sorted[i].file_name);
                if (words_are_the_same(data_type,DATA_TYPE_INT)) {
                    EXECUTION_REPORT(REPORT_ERROR,-1, num_dims == 1, "The number of dimensions of field \"%s\" in file \"%s\" must 1", time_field_name[j], time_filename_map_sorted[i].file_name);
                    current_dim_size = dim_size[0];
                    if (j == 0) {
                        last_dim_size = dim_size[0];
                        full_time = new long [last_dim_size];
                        for (k = 0; k < last_dim_size; k ++) {
                            full_time[k] = 0;
                        }
                    }
                    EXECUTION_REPORT(REPORT_ERROR,-1, last_dim_size == current_dim_size, "The size of dimension for the time fields in input data file \"%s\" for field read handler \"%s\" do not match each other", time_filename_map_sorted[i].file_name, handler_name);
                    last_dim_size = current_dim_size;
                    for (k = 0; k < dim_size[0]; k ++) {
                        sprintf(time_string2, "%d\0", ((int*)data_array)[k]);
                        if (time_field_format_id[j] == TIME_FORMAT_YYYY)
                            sprintf(time_string1, "%04d\0", ((int*)data_array)[k]);
                        else if (time_field_format_id[j] == TIME_FORMAT_YYYYMM)
                            sprintf(time_string1, "%06d\0", ((int*)data_array)[k]);
                        else if (time_field_format_id[j] == TIME_FORMAT_YYYYMMDD)
                            sprintf(time_string1, "%08d\0", ((int*)data_array)[k]);
                        else if (time_field_format_id[j] == TIME_FORMAT_SSSSS)
                            sprintf(time_string1, "%05d\0", ((int*)data_array)[k]);
                        else if (time_field_format_id[j] == TIME_FORMAT_HHMMSS)
                            sprintf(time_string1, "%06d\0", ((int*)data_array)[k]);
                        else EXECUTION_REPORT(REPORT_ERROR,-1, false, "For the time field in integer data type, the time format \"%s\" is not supported. Please verify the configuration file \"%s\".",
                                              time_field_format[j], handler_name);
                        printf("okok hao: %d : \"%s\"\n", ((int*)data_array)[k], time_string1);
                        EXECUTION_REPORT(REPORT_ERROR,-1, strlen(time_string2) <= strlen(time_string1), "The time in time field \"%s\" in the input data files \"%s\" does not match the time format \"%s\". Please check the field read handler \"%s\" and the time field values in the corresponding date files.",
                                         time_field_name[j], file_name_pattern, time_field_format[j], handler_name);
                        EXECUTION_REPORT(REPORT_ERROR,-1, get_time_from_string(time_string1, time_field_format[j], time_field_format_id[j], strlen(time_field_format[j]), false, date, second_in_day),
                                         "The time in time field \"%s\" in the input data files \"%s\" does not match the time format \"%s\". Please check the field read handler \"%s\" and the time field values in the corresponding date files.",
                                         time_field_name[j], file_name_pattern, time_field_format[j], handler_name);
                        full_time[k] = full_time[k]+((long)date)*100000+second_in_day;
                        printf("okok date and second: %d %d: %d : %ld\n", date, second_in_day, ((int*)data_array)[k], full_time[k]);
                    }
                    for (k = 1; k < dim_size[0]; k ++)
                        EXECUTION_REPORT(REPORT_ERROR,-1, full_time[k-1] <= full_time[k], "The time in field \"%s\" in input data file \"%s\" for field read handler \"%s\" is not sorted in assending order", time_field_name[j], time_filename_map_sorted[i].file_name, handler_name);
                }
                else {
                    EXECUTION_REPORT(REPORT_ERROR,-1, num_dims == 2, "The number of dimensions of field \"%s\" in file \"%s\" must 2", time_field_name[j], time_filename_map_sorted[i].file_name);
                    EXECUTION_REPORT(REPORT_ERROR,-1, false, "The string time field is not supported yet for field read handler. Please contact liuli-cess@tsinghua.edu.cn");
                }    
                delete [] data_array;
                delete [] dim_size;
            }
            for (k = 0; k < last_dim_size; k ++) {
                time_location.date = full_time[k] / 100000;
                time_location.second_in_day = full_time[k] % 100000;
                time_location.time_dim_num = k;
                strcpy(time_location.file_name, time_filename_map_sorted[i].file_name);
                time_filename_map.push_back(time_location);
            }
            delete [] full_time;
            delete ncfile_handler;
        }
    }
    else {
        for (i = 0; i < time_filename_map_sorted.size(); i ++)
            time_filename_map.push_back(time_filename_map_sorted[i]);
    }

    EXECUTION_REPORT(REPORT_ERROR,-1, time_filename_map.size() > 0, "C-Coupler error3 in initialize_time_filename_map");
    if (time_filename_map[0].date == -1)
        EXECUTION_REPORT(REPORT_ERROR,-1, time_filename_map.size() == 1, "C-Coupler error4 in initialize_time_filename_map");
    else for (i = 0; i < time_filename_map.size(); i ++)
        EXECUTION_REPORT(REPORT_ERROR,-1, time_filename_map[i].date != -1, "C-Coupler error5 in initialize_time_filename_map");

    if (period > 0) {
        EXECUTION_REPORT(REPORT_ERROR,-1, time_filename_map.size() > 1, "There is only one time slice of data in the data files specified for the field read handler \"%s\", and therefore the handler should not be cyclic. Please verify the configuration file", handler_name);
        long total_time_interval = (((long)time_filename_map[time_filename_map.size()-1].date)*100000+time_filename_map[time_filename_map.size()-1].second_in_day) -
                                   (((long)time_filename_map[0].date)*100000+time_filename_map[0].second_in_day);
        if (period == 1) {
            EXECUTION_REPORT(REPORT_ERROR,-1, total_time_interval < ((long) 1000000000), "the total time interval is larger than the period of one \"year\" (specified period), which is not allowed. Please check the configuration file \"%s\" and the corresponding data files", handler_name);
            for (i = 0; i < time_filename_map.size(); i ++)
                time_filename_map[i].date = (time_filename_map[i].date%10000);
        }    
        else if (period == 2) {
            EXECUTION_REPORT(REPORT_ERROR,-1, total_time_interval < ((long) 10000000), "the total time interval is larger than the period of one \"month\" (specified period), which is not allowed. Please check the configuration file \"%s\" and the corresponding data files", handler_name);
            for (i = 0; i < time_filename_map.size(); i ++)
                time_filename_map[i].date = 100 + (time_filename_map[i].date%100);            
        }    
        else {
            EXECUTION_REPORT(REPORT_ERROR,-1, total_time_interval < ((long) 100000), "the total time interval is larger than the period of one \"day\" (specified period), which is not allowed. Please check the configuration file \"%s\" and the corresponding data files", handler_name);
            for (i = 0; i < time_filename_map.size(); i ++)
                time_filename_map[i].date = 101;
        }    
        time_filename_map_unsorted.clear();
        for (i = 0; i < time_filename_map.size(); i ++)
            time_filename_map_unsorted.push_back(time_filename_map[i]);
        full_time = new long [time_filename_map_unsorted.size()];
        pos = new long [time_filename_map_unsorted.size()];
        for (i = 0; i < time_filename_map_unsorted.size(); i ++) {
            full_time[i] = ((long)time_filename_map_unsorted[i].date)*100000 + time_filename_map_unsorted[i].second_in_day;
            pos[i] = i;
        }
        do_quick_sort(full_time, pos, 0, time_filename_map_unsorted.size()-1);
        time_filename_map.clear();
        for (i = 0; i < time_filename_map_unsorted.size(); i ++)
            time_filename_map.push_back(time_filename_map_unsorted[pos[i]]);
        delete [] full_time;
        delete [] pos;
        printf("period is set\n");
    }    

    for (i = 0; i < time_filename_map.size(); i ++)
        if (time_filename_map[i].date != -1) {
            date = time_filename_map[i].date;
            second_in_day = time_filename_map[i].second_in_day;
            printf("okok %d\n", time_filename_map[i].date);
            EXECUTION_REPORT(REPORT_ERROR,-1, timer_mgr->check_is_time_legal(date/10000, (date%10000/100), date%100, second_in_day, NULL),
                             "The time in the time fields in the name of input data file \"%s\" is not legal. Please check.", time_filename_map[i].file_name);
            time_filename_map[i].num_elapsed_day = timer_mgr->calculate_elapsed_day(time_filename_map[i].date/10000, (time_filename_map[i].date%10000)/100, time_filename_map[i].date%100);
            printf("num_elapsed_day of %d is %d\n", i, time_filename_map[i].num_elapsed_day);
        }
}


long Datamodel_field_read_handler::search_current_time_interval(int field_read_info_index)
{
    int date_after_offset, current_num_elapsed_day, map_num_elapsed_day, i, search_start_pos;
    long current_full_time, current_elapsed_time, map_left_elapsed_time_after_offset, map_right_elapsed_time_after_offset;


    current_full_time = timer_mgr->get_current_full_time();
    if (period == 1)
        current_full_time = current_full_time%1000000000;
    else if (period == 2)
        current_full_time = current_full_time%10000000 + (long)10000000;
    else if (period == 3)
        current_full_time = current_full_time%100000 + (long)10100000;

    if (current_full_time == fields_read_info[field_read_info_index].last_read_full_time) {
        pos_time_interval_left = fields_read_info[field_read_info_index].last_pos_time_interval_left;
        pos_time_interval_right = fields_read_info[field_read_info_index].last_pos_time_interval_right;
        return current_full_time;
    }

    fields_read_info[field_read_info_index].last_read_full_time = current_full_time;

    if (time_filename_map[0].date == -1) {
        pos_time_interval_left = 0;
        pos_time_interval_right = -1;
        return current_full_time;
    }
    
    current_num_elapsed_day = timer_mgr->calculate_elapsed_day(current_full_time/1000000000, (current_full_time%1000000000)/10000000, (current_full_time%10000000)/100000);
    current_elapsed_time = ((long)current_num_elapsed_day)*100000 + (current_full_time%100000);
    pos_time_interval_left = -1;
    pos_time_interval_right = -1;
    if (fields_read_info[field_read_info_index].last_pos_time_interval_left == -1)
        search_start_pos = 0;
    else if (fields_read_info[field_read_info_index].last_pos_time_interval_right == -1) {
        EXECUTION_REPORT(REPORT_ERROR,-1, period == 0, "C-Coupler error1 in Datamodel_field_read_handler::search_current_time_interval");
        map_left_elapsed_time_after_offset = ((long)(time_filename_map[time_filename_map.size()-1].num_elapsed_day+offset_num_elapsed_day))*100000 + time_filename_map[time_filename_map.size()-1].second_in_day;
        EXECUTION_REPORT(REPORT_ERROR,-1, map_left_elapsed_time_after_offset <= current_elapsed_time && fields_read_info[field_read_info_index].last_pos_time_interval_right == time_filename_map.size()-1, "C-Coupler error2 in Datamodel_field_read_handler::search_current_time_interval");
        pos_time_interval_left = fields_read_info[field_read_info_index].last_pos_time_interval_left;
        return current_full_time;
    }
    else {
        search_start_pos = fields_read_info[field_read_info_index].last_pos_time_interval_left;
        if (fields_read_info[field_read_info_index].last_pos_time_interval_left > fields_read_info[field_read_info_index].last_pos_time_interval_right)
            search_start_pos = fields_read_info[field_read_info_index].last_pos_time_interval_right;
    }

    for (i = search_start_pos; i < time_filename_map.size(); i ++) {
        map_right_elapsed_time_after_offset = ((long)(time_filename_map[i].num_elapsed_day+offset_num_elapsed_day))*100000 + time_filename_map[i].second_in_day;
        if (i > 0) {
            map_left_elapsed_time_after_offset = ((long)(time_filename_map[i-1].num_elapsed_day+offset_num_elapsed_day))*100000 + time_filename_map[i-1].second_in_day;
            printf("okok search %d %d: %ld %ld %ld\n", i, search_start_pos, map_left_elapsed_time_after_offset, map_right_elapsed_time_after_offset, current_elapsed_time);
            EXECUTION_REPORT(REPORT_ERROR,-1, map_left_elapsed_time_after_offset <= current_elapsed_time, "C-Coupler error3 in Datamodel_field_read_handler::search_current_time_interval");
        }
        if (map_right_elapsed_time_after_offset >= current_elapsed_time)
            break;
    }
    if (i == 0)
        pos_time_interval_right = 0;
    else if (i == time_filename_map.size())
        pos_time_interval_left = time_filename_map.size()-1;
    else {
        pos_time_interval_left = i-1;
        pos_time_interval_right = i;
        map_left_elapsed_time_after_offset = ((long)(time_filename_map[i-1].num_elapsed_day+offset_num_elapsed_day))*100000 + time_filename_map[i-1].second_in_day;
        map_right_elapsed_time_after_offset = ((long)(time_filename_map[i].num_elapsed_day+offset_num_elapsed_day))*100000 + time_filename_map[i].second_in_day;
        EXECUTION_REPORT(REPORT_ERROR,-1, map_right_elapsed_time_after_offset >= current_elapsed_time && map_left_elapsed_time_after_offset <= current_elapsed_time, "C-Coupler error4 in Datamodel_field_read_handler::search_current_time_interval");        
    }    
    if (period > 0) {
        EXECUTION_REPORT(REPORT_ERROR,-1, pos_time_interval_left != -1 || pos_time_interval_right != -1, "C-Coupler error5 in Datamodel_field_read_handler::search_current_time_interval");
        if (pos_time_interval_left == -1)
            pos_time_interval_left = time_filename_map.size()-1;
        if (pos_time_interval_right == -1)
            pos_time_interval_right = 0;
    }

    EXECUTION_REPORT(REPORT_ERROR,-1, ! (pos_time_interval_left == -1 && pos_time_interval_right == -1), "C-Coupler error6 in Datamodel_field_read_handler::search_current_time_interval");

    return current_full_time;
}


void Datamodel_field_read_handler::update_one_field(int field_read_info_index)
{
    long current_full_time, current_elapsed_time, current_num_elapsed_day, map_left_elapsed_time_after_offset=-1, map_right_elapsed_time_after_offset=-1;
    double fact_left, fact_right;    


    current_full_time = search_current_time_interval(field_read_info_index);

    if (fields_read_info[field_read_info_index].last_pos_time_interval_left == pos_time_interval_left && fields_read_info[field_read_info_index].last_pos_time_interval_right == pos_time_interval_right) {
        EXECUTION_REPORT(REPORT_LOG,-1, true, "It is unnecessary to read new data from files again. Remapping at linear direction is enough");
    }
    else {
        if (pos_time_interval_left != -1) {
            if (fields_read_info[field_read_info_index].last_pos_time_interval_right == pos_time_interval_left) {
                EXECUTION_REPORT(REPORT_LOG,-1, true, "Datamodel_field_read_handler::update_one_field:  copy from right to left");
                EXECUTION_REPORT(REPORT_ERROR,-1, fields_read_info[field_read_info_index].last_pos_time_interval_right == pos_time_interval_left, "C-Coupler error in Datamodel_field_read_handler::update_one_field");
                memcpy(fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_data_buf(), fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_data_buf(), fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_size_of_field()*get_data_type_size(fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_field_data()->get_grid_data_field()->data_type_in_application));
            }
            else {
                EXECUTION_REPORT(REPORT_LOG,-1, true, "Datamodel_field_read_handler::update_one_field:  read data for left");
                printf("file name is \"%s\": %d %d\n", time_filename_map[pos_time_interval_left].file_name, pos_time_interval_left, time_filename_map.size());
                IO_netcdf *restart_read_nc_file = new IO_netcdf("restart_read_file", time_filename_map[pos_time_interval_left].file_name, "r", false);
                printf("okok1\n");
                fields_gather_scatter_mgr->read_scatter_field(restart_read_nc_file, fields_read_info[field_read_info_index].temp_field_readin, time_filename_map[pos_time_interval_left].time_dim_num);
                printf("okok2\n");                         
                if (fields_read_info[field_read_info_index].remap_algorithm_for_readin_left == NULL)
                    memcpy(fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_data_buf(), fields_read_info[field_read_info_index].temp_field_readin->get_data_buf(), fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_size_of_field()*get_data_type_size(fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_field_data()->get_grid_data_field()->data_type_in_application));
                else EXECUTION_REPORT(REPORT_ERROR,-1, false, "not supported in Datamodel_field_read_handler::update_one_field");
                delete restart_read_nc_file;
            }
        }
        if (pos_time_interval_right != -1) {
            EXECUTION_REPORT(REPORT_LOG,-1, true, "Datamodel_field_read_handler::update_one_field:  read data for right");
            IO_netcdf *restart_read_nc_file = new IO_netcdf("restart_read_file", time_filename_map[pos_time_interval_right].file_name, "r", false);
            fields_gather_scatter_mgr->read_scatter_field(restart_read_nc_file, fields_read_info[field_read_info_index].temp_field_readin, time_filename_map[pos_time_interval_right].time_dim_num);
            EXECUTION_REPORT(REPORT_ERROR,-1, fields_read_info[field_read_info_index].remap_algorithm_for_readin_right == NULL, "not supported in Datamodel_field_read_handler::update_one_field");            
            if (fields_read_info[field_read_info_index].remap_algorithm_for_readin_right == NULL)
                memcpy(fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_data_buf(), fields_read_info[field_read_info_index].temp_field_readin->get_data_buf(), fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_size_of_field()*get_data_type_size(fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_field_data()->get_grid_data_field()->data_type_in_application));
            else EXECUTION_REPORT(REPORT_ERROR,-1, false, "not supported in Datamodel_field_read_handler::update_one_field");
            delete restart_read_nc_file;
        }
    }

    printf("current_full_time is %ld at %s\n", current_full_time, handler_name);
    current_num_elapsed_day = timer_mgr->calculate_elapsed_day(current_full_time/1000000000, (current_full_time%1000000000)/10000000, (current_full_time%10000000)/100000);
    current_elapsed_time = ((long)current_num_elapsed_day)*86400 + (current_full_time%100000);

    printf("wuwu %d %d : %d %d at %s\n", pos_time_interval_left, pos_time_interval_right, time_filename_map[pos_time_interval_left].num_elapsed_day, offset_num_elapsed_day, handler_name);
    if (pos_time_interval_left != -1)        
        map_left_elapsed_time_after_offset = ((long)(time_filename_map[pos_time_interval_left].num_elapsed_day+offset_num_elapsed_day))*86400 + time_filename_map[pos_time_interval_left].second_in_day;
    if (pos_time_interval_right != -1)        
        map_right_elapsed_time_after_offset = ((long)(time_filename_map[pos_time_interval_right].num_elapsed_day+offset_num_elapsed_day))*86400 + time_filename_map[pos_time_interval_right].second_in_day;
    if (period > 0) {
        EXECUTION_REPORT(REPORT_ERROR,-1, pos_time_interval_left != -1 && pos_time_interval_right != -1, "C-Coupler error3 in Datamodel_field_read_handler::update_one_field");
        if (current_elapsed_time > map_right_elapsed_time_after_offset) {
            if (period == 1)
                map_right_elapsed_time_after_offset += 365*86400;
            else if (period == 2)
                map_right_elapsed_time_after_offset += 31*86400;
            else if (period == 3)
                map_right_elapsed_time_after_offset += 86400;
        }
        else if (current_elapsed_time < map_left_elapsed_time_after_offset) {
            if (period == 1)
                map_left_elapsed_time_after_offset -= 365*86400;
            else if (period == 2)
                map_left_elapsed_time_after_offset -= 31*86400;
            else if (period == 3)
                map_left_elapsed_time_after_offset -= 86400;            
        }
        printf("okok %ld %ld: %ld\n", map_left_elapsed_time_after_offset, map_right_elapsed_time_after_offset, current_elapsed_time);
    }

    printf("okokqiguai %ld %ld %ld %d : %d %d : %d\n", map_left_elapsed_time_after_offset, map_right_elapsed_time_after_offset, current_elapsed_time, offset_num_elapsed_day, pos_time_interval_left, pos_time_interval_right, offset_num_elapsed_day);
    if (map_left_elapsed_time_after_offset == -1 || map_right_elapsed_time_after_offset == current_elapsed_time) {
        EXECUTION_REPORT(REPORT_LOG,-1, true, "Copy field from right to dst");
        memcpy(fields_read_info[field_read_info_index].output_field_instance->get_data_buf(), fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_data_buf(), fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_size_of_field()*get_data_type_size(fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_field_data()->get_grid_data_field()->data_type_in_application));
    }
    else if (map_right_elapsed_time_after_offset == -1 || map_left_elapsed_time_after_offset == current_elapsed_time) {
        EXECUTION_REPORT(REPORT_LOG,-1, true, "Copy field from left to dst");
        memcpy(fields_read_info[field_read_info_index].output_field_instance->get_data_buf(), fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_data_buf(), fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_size_of_field()*get_data_type_size(fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_field_data()->get_grid_data_field()->data_type_in_application));
    }
    else {
        fact_left = ((double)(map_right_elapsed_time_after_offset-current_elapsed_time)) / ((double)(map_right_elapsed_time_after_offset-map_left_elapsed_time_after_offset));
        fact_right = ((double)(current_elapsed_time-map_left_elapsed_time_after_offset)) / ((double)(map_right_elapsed_time_after_offset-map_left_elapsed_time_after_offset));
        EXECUTION_REPORT(REPORT_LOG,-1, true, "Remap field with left and right with factors: %lf : %lf", fact_left, fact_right);
        printf("qiguai qiguai: %ld, %ld, %ld  %lf %lf\n", map_right_elapsed_time_after_offset-current_elapsed_time, current_elapsed_time-map_left_elapsed_time_after_offset, map_right_elapsed_time_after_offset-map_left_elapsed_time_after_offset, fact_left, fact_right);
        EXECUTION_REPORT(REPORT_ERROR,-1, fact_left >=0 && fact_left <= 1.0 && fact_right >= 0 && fact_right <= 1.0, "C-Coupler error1 in Datamodel_field_read_handler::update_one_field");
        if (words_are_the_same(fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE)) {
            double *buf_dst = (double*) fields_read_info[field_read_info_index].output_field_instance->get_data_buf();
            double *buf_src_left = (double*) fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_data_buf();
            double *buf_src_right = (double*) fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_data_buf();
            long field_size = fields_read_info[field_read_info_index].output_field_instance->get_size_of_field();
            for (long i = 0; i < field_size; i ++)
                buf_dst[i] = buf_src_left[i]*fact_left + buf_src_right[i]*fact_right;
        }
        else if (words_are_the_same(fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT)) {
            float *buf_dst = (float*) fields_read_info[field_read_info_index].output_field_instance->get_data_buf();
            float *buf_src_left = (float*) fields_read_info[field_read_info_index].temp_field_before_time_remap_left->get_data_buf();
            float *buf_src_right = (float*) fields_read_info[field_read_info_index].temp_field_before_time_remap_right->get_data_buf();
            long field_size = fields_read_info[field_read_info_index].output_field_instance->get_size_of_field();
            for (long i = 0; i < field_size; i ++)
                buf_dst[i] = buf_src_left[i]*fact_left + buf_src_right[i]*fact_right;
        }
        else EXECUTION_REPORT(REPORT_ERROR,-1, false, "C-Coupler error2 in Datamodel_field_read_handler::update_one_field");
    }

    fields_read_info[field_read_info_index].last_pos_time_interval_left = pos_time_interval_left;
    fields_read_info[field_read_info_index].last_pos_time_interval_right = pos_time_interval_right;
    fields_read_info[field_read_info_index].output_field_instance->define_field_values(false);
}


Datamodel_field_read_handler_mgt::~Datamodel_field_read_handler_mgt()
{
    for (int i = 0; i < datamodel_field_read_handlers.size(); i ++)
        delete datamodel_field_read_handlers[i];
}


Datamodel_field_read_handler *Datamodel_field_read_handler_mgt::get_a_handler(const char *handler_name)
{
    EXECUTION_REPORT(REPORT_LOG,-1, true, "require a field read hanler \"%s\"", handler_name);
    
    for (int i = 0; i < datamodel_field_read_handlers.size(); i ++)
        if (words_are_the_same(handler_name, datamodel_field_read_handlers[i]->get_handler_name()))
            return datamodel_field_read_handlers[i];

    datamodel_field_read_handlers.push_back(new Datamodel_field_read_handler(handler_name));
    
    return datamodel_field_read_handlers[datamodel_field_read_handlers.size()-1];
}

