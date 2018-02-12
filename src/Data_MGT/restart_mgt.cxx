/***************************************************************
  *  Copyright (c) 2017, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "restart_mgt.h"



Restart_buffer_container::Restart_buffer_container(const char *comp_full_name, const char *buf_type, const char *keyword, Restart_mgt *restart_mgr)
{
	strcpy(this->comp_full_name, comp_full_name);
	strcpy(this->buf_type, buf_type);
	strcpy(this->keyword, keyword);
	buffer_max_size = 1000;
	buffer_content = new char [buffer_max_size];
	buffer_content_iter = 0;
	buffer_content_size = 0;
	this->restart_mgr = restart_mgr;
}


Restart_buffer_container::Restart_buffer_container(const char *array_buffer, long &buffer_content_iter, const char *file_name, Restart_mgt *restart_mgr)
{
	long total_size, str_size;


	EXECUTION_REPORT(REPORT_ERROR, -1, read_data_from_array_buffer(&total_size, sizeof(long), array_buffer, buffer_content_iter, file_name == NULL), "Fail to load the restart data file \"%s\": its format is wrong", file_name);
	
	buffer_content = load_string(NULL, str_size, -1, array_buffer, buffer_content_iter, file_name);
	this->buffer_content_iter = str_size;
	this->buffer_content_size = str_size;
	this->restart_mgr = restart_mgr;
	load_string(keyword, str_size, NAME_STR_SIZE, array_buffer, buffer_content_iter, file_name);
	load_string(buf_type, str_size, NAME_STR_SIZE, array_buffer, buffer_content_iter, file_name);
	load_string(comp_full_name, str_size, NAME_STR_SIZE, array_buffer, buffer_content_iter, file_name);
	EXECUTION_REPORT(REPORT_ERROR, -1, total_size == strlen(comp_full_name) + strlen(buf_type) + strlen(keyword) + sizeof(long)*3 + this->buffer_content_iter, "Restart_buffer_container::Restart_buffer_container: wrong format of restart data file");
}


void Restart_buffer_container::dump_in_string(const char *str, long str_size)
{
	dump_string(str, str_size, &buffer_content, buffer_max_size, buffer_content_iter);
	buffer_content_size = buffer_content_iter;
}


void Restart_buffer_container::dump_in_data(const void *data, long size)
{
	write_data_into_array_buffer(data, size, &buffer_content, buffer_max_size, buffer_content_iter);
	buffer_content_size = buffer_content_iter;
}


void Restart_buffer_container::load_restart_data(void *data, long data_size)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, read_data_from_array_buffer(data, data_size, buffer_content, buffer_content_iter, false), "Fail to load the restart data file \"%s\": its format is wrong", restart_mgr->get_input_restart_mgt_info_file());
}


char *Restart_buffer_container::load_restart_string(char *str, long &str_size, long max_size)
{
	return load_string(str, str_size, max_size, buffer_content, buffer_content_iter, restart_mgr->get_input_restart_mgt_info_file());
}


void Restart_buffer_container::dump_out(char **array_buffer, long &buffer_max_size, long &buffer_content_size)
{
	dump_string(comp_full_name, -1, array_buffer, buffer_max_size, buffer_content_size);
	dump_string(buf_type, -1, array_buffer, buffer_max_size, buffer_content_size);
	dump_string(keyword, -1, array_buffer, buffer_max_size, buffer_content_size);
	dump_string(buffer_content, buffer_content_iter, array_buffer, buffer_max_size, buffer_content_size);
	long total_size = strlen(comp_full_name) + strlen(buf_type) + strlen(keyword) + sizeof(long)*3 + buffer_content_iter;
	write_data_into_array_buffer(&total_size, sizeof(long), array_buffer, buffer_max_size, buffer_content_size);
}


bool Restart_buffer_container::match(const char *buf_type, const char *keyword)
{
	return words_are_the_same(this->buf_type, buf_type) && words_are_the_same(this->keyword, keyword);
}


const char *Restart_buffer_container::get_input_restart_mgt_info_file()
{
	return restart_mgr->get_input_restart_mgt_info_file();
}


Restart_mgt::Restart_mgt(Comp_comm_group_mgt_node *comp_node)
{ 
	this->comp_node = comp_node; 
	last_restart_write_full_time = -1;
	last_restart_write_elapsed_time = -1;
	input_restart_mgt_info_file = NULL;
	restart_read_annotation = NULL;
	restart_mgt_info_written = true;
	time_mgr = NULL;
	restart_write_data_file = NULL;
}


Restart_mgt::~Restart_mgt()
{
	clean(true);
	clean(false);
	if (input_restart_mgt_info_file != NULL)
		delete [] input_restart_mgt_info_file;
	if (restart_read_annotation != NULL)
		delete [] restart_read_annotation;
}


int Restart_mgt::get_comp_id() 
{ 
	return comp_node->get_comp_id(); 
}



void Restart_mgt::clean(bool is_write_buffers)
{
	if (is_write_buffers) {
		for (int i = 0; i < restart_write_buffer_containers.size(); i ++)
			delete restart_write_buffer_containers[i];
		restart_write_buffer_containers.clear();
	}
	else {
		for (int i = 0; i < restart_read_buffer_containers.size(); i ++)
			delete restart_read_buffer_containers[i];
		restart_read_buffer_containers.clear();		
	}
}


Restart_buffer_container *Restart_mgt::search_restart_buffer(const char *buf_type, const char *keyword)
{
	for (int i = 0; i < restart_read_buffer_containers.size(); i ++)
		if (restart_read_buffer_containers[i]->match(buf_type, keyword))
			return restart_read_buffer_containers[i];

	return NULL;
}


void Restart_mgt::read_restart_mgt_info(const char *specified_file_name, const char *annotation)
{
	char restart_file_full_name[NAME_STR_SIZE], restart_file_short_name[NAME_STR_SIZE];
			

	if (time_mgr == NULL)
		time_mgr = components_time_mgrs->get_time_mgr(comp_node->get_comp_id());
	
	if ((time_mgr->get_runtype_mark() == RUNTYPE_MARK_INITIAL)) {
		EXECUTION_REPORT(REPORT_PROGRESS, comp_node->get_comp_id(), true, "C-Coupler does not read the restart data file because it is a initial run (the run_type is initial)");
		return;
	}
	
	if (strlen(specified_file_name) == 0) {
		if ((time_mgr->get_runtype_mark() == RUNTYPE_MARK_CONTINUE))
			get_file_name_in_rpointer_file(restart_file_short_name);
		else sprintf(restart_file_short_name, "%s.%s.r.%08d-%05d", time_mgr->get_rest_refcase(), comp_node->get_comp_full_name(), time_mgr->get_rest_refdate(), time_mgr->get_rest_refsecond());
	}
	else strcpy(restart_file_short_name, specified_file_name);
	sprintf(restart_file_full_name, "%s/%s", comp_node->get_working_dir(), restart_file_short_name);
	if (input_restart_mgt_info_file != NULL)
		EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), false, "Error happens when reading the restart data file \"%s\" at the model code with the annotation \"%s\": another restart data file \"%s\" has already been read before at the model code with the annotation \"%s\". Please verify.", restart_file_full_name, annotation, input_restart_mgt_info_file, restart_read_annotation);
	input_restart_mgt_info_file = strdup(restart_file_full_name);
	restart_read_annotation = strdup(annotation);

	if (time_mgr->get_runtype_mark() == RUNTYPE_MARK_CONTINUE || time_mgr->get_runtype_mark() == RUNTYPE_MARK_BRANCH)
		EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), !time_mgr->get_time_has_been_advanced(), "Error happens when reading the restart file \"%s\": cannot advance the model time before reading the restart file. Please check the model code with the annotation \"%s\"", restart_file_full_name, annotation);
	read_restart_mgt_info((time_mgr->get_runtype_mark() == RUNTYPE_MARK_CONTINUE) || (time_mgr->get_runtype_mark() == RUNTYPE_MARK_BRANCH), restart_file_full_name, annotation);
}


void Restart_mgt::read_restart_mgt_info(bool check_existing_data, const char *file_name, const char *annotation)
{
	int local_proc_id = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_node->get_comp_id(), "in Restart_mgt::do_restart");
	char *array_buffer = NULL;
	long buffer_content_iter;


	if (time_mgr == NULL)
		time_mgr = components_time_mgrs->get_time_mgr(comp_node->get_comp_id());

	if (local_proc_id == 0) {
		FILE *restart_fp = fopen(file_name, "r");
		EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), restart_fp != NULL, "Error happens when trying to read restart data at the model code with the annotation \"%s\": the data file \"%s\" does not exist. Please verify.", annotation, file_name);
		fseek(restart_fp, 0, SEEK_END);
		buffer_content_iter = ftell(restart_fp);
		array_buffer = new char [buffer_content_iter];
		fseek(restart_fp, 0, SEEK_SET);
		fread(array_buffer, buffer_content_iter, 1, restart_fp);
	}
	bcast_array_in_one_comp(local_proc_id, &array_buffer, buffer_content_iter, comp_node->get_comm_group());

	int num_restart_buffer_containers;
	EXECUTION_REPORT(REPORT_ERROR, -1, read_data_from_array_buffer(&num_restart_buffer_containers, sizeof(int), array_buffer, buffer_content_iter, false), "Fail to load the restart data file \"%s\": its format is wrong", file_name);
	for (int i = 0; i < num_restart_buffer_containers; i ++) {
		EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), buffer_content_iter > 0, "Software error in Restart_mgt::read_restart_mgt_info: wrong organization of restart data file");
		restart_read_buffer_containers.push_back(new Restart_buffer_container(array_buffer, buffer_content_iter, file_name, this));
	}
	EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), buffer_content_iter == 0, "Software error in Restart_mgt::read_restart_mgt_info: wrong organization of restart data file");
	delete [] array_buffer;

	if ((time_mgr->get_runtype_mark() == RUNTYPE_MARK_CONTINUE) || (time_mgr->get_runtype_mark() == RUNTYPE_MARK_BRANCH)) {
		Restart_buffer_container *time_mgr_restart_buffer = search_restart_buffer(RESTART_BUF_TYPE_TIME, "local time manager");
		EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), time_mgr_restart_buffer != NULL, "Error happens when loading the restart data file \"%s\" at the model code with the annotation \"%s\": this file does not include the data for restarting the time information", file_name, annotation);
		long buffer_size = time_mgr_restart_buffer->get_buffer_content_iter();
		time_mgr->import_restart_data(time_mgr_restart_buffer->get_buffer_content(), buffer_size, file_name, check_existing_data);
	}
}


void Restart_mgt::do_restart_write(const char *annotation, bool bypass_timer)
{
	int local_proc_id = comp_node->get_current_proc_local_id();
	const char *comp_full_name = comp_node->get_full_name();
	long current_full_time;
	Restart_buffer_container *time_mgr_restart_buffer;


	if (time_mgr == NULL)
		time_mgr = components_time_mgrs->get_time_mgr(comp_node->get_comp_id());
	current_full_time = time_mgr->get_current_full_time();

	if (bypass_timer || time_mgr->is_restart_timer_on()) {
		EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), current_full_time != last_restart_write_full_time, "Error happens when the component model tries to write restart data files: the corresponding API \"CCPL_do_restart_write\" has been called more than once in the same time step. Please verify the model code with the annotation \"%s\"");
		last_restart_write_full_time = current_full_time;
		last_restart_write_elapsed_time = time_mgr->get_current_num_elapsed_day()*((long)100000)+time_mgr->get_current_second();
		int date = last_restart_write_full_time/(long)100000;
		int second = last_restart_write_full_time%(long)100000;
		if (local_proc_id == 0) {
			time_mgr_restart_buffer = apply_restart_buffer(comp_full_name, RESTART_BUF_TYPE_TIME, "local time manager");
			time_mgr->write_time_mgt_into_array(time_mgr_restart_buffer->get_buffer_content_ptr(), *(time_mgr_restart_buffer->get_buffer_max_size_ptr()), *(time_mgr_restart_buffer->get_buffer_content_iter_ptr()));
			char restart_data_file_name[NAME_STR_SIZE];
			sprintf(restart_data_file_name, "%s/%s.%s.r.%08d-%05d.nc", comp_node->get_working_dir(), time_mgr->get_case_name(), comp_node->get_comp_full_name(), date, second);
			restart_write_data_file = new IO_netcdf(restart_data_file_name, restart_data_file_name, "w", false);
		}
		inout_interface_mgr->write_into_restart_buffers(comp_node->get_comp_id());
		restart_mgt_info_written = false;
	}
}


void Restart_mgt::write_restart_field_data(Field_mem_info *field_instance, const char *interface_name, const char*label)
{
	Field_mem_info *global_field = fields_gather_scatter_mgr->gather_field(field_instance);

	if (comp_node->get_current_proc_local_id() == 0) {
		sprintf(global_field->get_field_data()->get_grid_data_field()->field_name_in_IO_file, "%s.%s.%s.%13ld", field_instance->get_field_name(), interface_name, label, time_mgr->get_current_full_time());
		EXECUTION_REPORT(REPORT_ERROR, -1, restart_write_data_file != NULL, "Software error in Restart_mgt::write_restart_field_data");
		EXECUTION_REPORT_LOG(REPORT_LOG, comp_node->get_comp_id(), true, "Write variable \"%s\" into restart data file \"%s\"", global_field->get_field_data()->get_grid_data_field()->field_name_in_IO_file, restart_write_data_file->get_file_name());
		restart_write_data_file->write_grided_data(global_field->get_field_data(), true, -1, -1, true);
	}
}


void Restart_mgt::get_file_name_in_rpointer_file(char *restart_file_name)
{
	char rpointer_file_name[NAME_STR_SIZE], line[NAME_STR_SIZE], *line_p;
	FILE *rpointer_file;


	sprintf(rpointer_file_name, "%s/rpointer", comp_node->get_working_dir());
	EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), does_file_exist(rpointer_file_name), "Error happens when try to restart data: file \"%s\" does not exist", rpointer_file_name);
	rpointer_file = fopen(rpointer_file_name, "r");
	get_next_line(line, rpointer_file);
	line_p = line;
	get_next_attr(restart_file_name, &line_p);
	fclose(rpointer_file);
}


void Restart_mgt::write_restart_mgt_into_file()
{
	char *array_buffer = NULL;
	long buffer_max_size, buffer_content_size;
	int temp_int;
	char restart_file_name[NAME_STR_SIZE], rpointer_file_name[NAME_STR_SIZE];
	FILE *restart_file, *rpointer_file;
	

	if (comp_node->get_current_proc_local_id() != 0) {
		clean(true);
		return;
	}

	if (restart_mgt_info_written)
		return;

	if (inout_interface_mgr->is_comp_in_restart_write_window(comp_node->get_comp_id()))
		return;

	restart_mgt_info_written = true;
	
	for (int i = restart_write_buffer_containers.size()-1; i >= 0; i --)
		restart_write_buffer_containers[i]->dump_out(&array_buffer, buffer_max_size, buffer_content_size);
	temp_int = restart_write_buffer_containers.size();
	write_data_into_array_buffer(&temp_int, sizeof(int), &array_buffer, buffer_max_size, buffer_content_size);

	int date = last_restart_write_full_time/(long)100000;
	int second = last_restart_write_full_time%(long)100000;
	sprintf(restart_file_name, "%s/%s.%s.r.%08d-%05d", comp_node->get_working_dir(), time_mgr->get_case_name(), comp_node->get_comp_full_name(), date, second);
	restart_file = fopen(restart_file_name, "w+");
	EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), restart_file != NULL, "Failed to open the file \"%s\" for writing restart data", restart_file_name);
	fwrite(array_buffer, buffer_content_size, 1, restart_file);
	fclose(restart_file);
	delete [] array_buffer;	
	sprintf(rpointer_file_name, "%s/rpointer", comp_node->get_working_dir());
	rpointer_file = fopen(rpointer_file_name, "w+");	
	fprintf(rpointer_file, "%s.%s.r.%08d-%05d\n", time_mgr->get_case_name(), comp_node->get_comp_full_name(), date, second);
	fclose(rpointer_file);
	EXECUTION_REPORT_LOG(REPORT_LOG, comp_node->get_comp_id(), true, "Write restart mgt information into the file \"%s\"", restart_file_name);
	EXECUTION_REPORT(REPORT_ERROR, -1, restart_write_data_file != NULL, "Software error in Restart_mgt::write_restart_mgt_into_file");

	delete restart_write_data_file;
	restart_write_data_file = NULL;

	clean(true);
}


const char *Restart_mgt::get_input_restart_mgt_info_file()
{
	EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), input_restart_mgt_info_file != NULL, "Failed to run the model in a continue or branch run: the restart file has not been read in. Please make sure that the API \"CCPL_do_restart_read\" has been called");
	return input_restart_mgt_info_file;
}


const char *Restart_mgt::get_restart_read_annotation()
{
	EXECUTION_REPORT(REPORT_ERROR, comp_node->get_comp_id(), restart_read_annotation != NULL, "Failed to run the model in a continue or branch run: the restart file has not been read in. Please make sure that the API \"CCPL_do_restart_read\" has been called");
	return restart_read_annotation;
}


Restart_buffer_container *Restart_mgt::apply_restart_buffer(const char *comp_full_name, const char *buf_type, const char *keyword)
{
	Restart_buffer_container *new_restart_buffer = new Restart_buffer_container(comp_full_name, buf_type, keyword, this);
	restart_write_buffer_containers.push_back(new_restart_buffer);

	return new_restart_buffer;
}


bool Restart_mgt::is_in_restart_write_window(long full_time)
{
	if (last_restart_write_elapsed_time == -1)
		return false;

	if (full_time == -1)
		return false;

	return full_time <= last_restart_write_elapsed_time || time_mgr->get_current_num_elapsed_day()*((long)100000)+time_mgr->get_current_second() <= last_restart_write_elapsed_time;
}


bool Restart_mgt::is_in_restart_read_window(long full_time)
{
	if (time_mgr->get_runtype_mark() == RUNTYPE_MARK_INITIAL || time_mgr->get_runtype_mark() == RUNTYPE_MARK_HYBRID)
		return false;

	if (full_time == -1)
		return false;

	return full_time <= time_mgr->get_restart_full_time() || time_mgr->get_current_num_elapsed_day()*((long)100000)+time_mgr->get_current_second() <= time_mgr->get_restart_full_time();
}

