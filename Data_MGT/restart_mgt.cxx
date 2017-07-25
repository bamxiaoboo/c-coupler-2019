/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "restart_mgt.h"


Restart_buffer_container::Restart_buffer_container(const char *comp_full_name, const char *buf_type, const char *keyword, const char *buffer_content, long buffer_content_iter)
{
	strcpy(this->comp_full_name, comp_full_name);
	strcpy(this->buf_type, buf_type);
	strcpy(this->keyword, keyword);
	this->buffer_content = buffer_content;
	this->buffer_content_iter = buffer_content_iter;
}


Restart_buffer_container::Restart_buffer_container(const char *array_buffer, long &buffer_content_iter)
{
	long total_size, str_size;


	read_data_from_array_buffer(&total_size, sizeof(long), array_buffer, buffer_content_iter);
	buffer_content = load_string(NULL, str_size, array_buffer, buffer_content_iter);
	this->buffer_content_iter = str_size;
	load_string(keyword, str_size, array_buffer, buffer_content_iter);
	load_string(buf_type, str_size, array_buffer, buffer_content_iter);
	load_string(comp_full_name, str_size, array_buffer, buffer_content_iter);
	EXECUTION_REPORT(REPORT_ERROR, -1, total_size == strlen(comp_full_name) + strlen(buf_type) + strlen(keyword) + sizeof(long)*3 + this->buffer_content_iter, "Restart_buffer_container::Restart_buffer_container: wrong format of restart data file");
}


char *Restart_buffer_container::load_string(char *str, long &str_size, const char *array_buffer, long &buffer_content_iter)
{
	char *local_str = str;
	

	read_data_from_array_buffer(&str_size, sizeof(long), array_buffer, buffer_content_iter);
	if (local_str == NULL)
		local_str = new char [str_size+1];
	read_data_from_array_buffer(local_str, str_size, array_buffer, buffer_content_iter);
	local_str[str_size] = '\0';

	return local_str;
}


void Restart_buffer_container::dump_string(const char *str, long str_size, char **array_buffer, long &buffer_max_size, long &buffer_content_size)
{
	if (str_size == -1)
		str_size = strlen(str);
	write_data_into_array_buffer(str, str_size, array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&str_size, sizeof(long), array_buffer, buffer_max_size, buffer_content_size);
}


void Restart_buffer_container::dump(char **array_buffer, long &buffer_max_size, long &buffer_content_size)
{
	dump_string(comp_full_name, -1, array_buffer, buffer_max_size, buffer_content_size);
	dump_string(buf_type, -1, array_buffer, buffer_max_size, buffer_content_size);
	dump_string(keyword, -1, array_buffer, buffer_max_size, buffer_content_size);
	dump_string(buffer_content, buffer_content_iter, array_buffer, buffer_max_size, buffer_content_size);
	long total_size = strlen(comp_full_name) + strlen(buf_type) + strlen(keyword) + sizeof(long)*3 + buffer_content_iter;
	write_data_into_array_buffer(&total_size, sizeof(long), array_buffer, buffer_max_size, buffer_content_size);
}


Restart_mgt::Restart_mgt(int comp_id)
{ 
	this->comp_id = comp_id; 
	last_timer_on_full_time = -1;
}


Restart_mgt::~Restart_mgt()
{
	clean();
}


void Restart_mgt::clean()
{
	for (int i = 0; i < restart_buffer_containers.size(); i ++)
		delete restart_buffer_containers[i];
	restart_buffer_containers.clear();
}


void Restart_mgt::do_restart_read(const char *file_name, const char *annotation)
{
	int local_proc_id = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_id, "in Restart_mgt::do_restart");


	if (local_proc_id == 0) {
		FILE *restart_fp = fopen(file_name, "r");
		EXECUTION_REPORT(REPORT_ERROR, comp_id, restart_fp != NULL, "Error happens when trying to read restart data at the model code with the annotation \"%s\": the data file \"%s\" does not exist. Please verify.", annotation, file_name);
		fseek(restart_fp, 0, SEEK_END);
		long buffer_content_iter = ftell(restart_fp);
		char *array_buffer = new char [buffer_content_iter];
		fseek(restart_fp, 0, SEEK_SET);
		fread(array_buffer, buffer_content_iter, 1, restart_fp);
		int num_restart_buffer_containers;
		read_data_from_array_buffer(&num_restart_buffer_containers, sizeof(int), array_buffer, buffer_content_iter);
		for (int i = 0; i < num_restart_buffer_containers; i ++) {
			EXECUTION_REPORT(REPORT_ERROR, comp_id, buffer_content_iter > 0, "Software error in Restart_mgt::do_restart_read: wrong organization of restart data file");
			restart_buffer_containers.push_back(new Restart_buffer_container(array_buffer, buffer_content_iter));
		}
		EXECUTION_REPORT(REPORT_ERROR, comp_id, buffer_content_iter == 0, "Software error in Restart_mgt::do_restart_read: wrong organization of restart data file");
	}
}


void Restart_mgt::do_restart_write(const char *annotation)
{
	char *array_buffer;
	long buffer_max_size, buffer_content_size;
	int local_proc_id = comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_id, "in Restart_mgt::do_restart");
	const char *comp_full_name = comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Restart_mgt::do_restart")->get_full_name();
	time_mgr = components_time_mgrs->get_time_mgr(comp_id);
	long current_full_time = time_mgr->get_current_full_time();


	if (time_mgr->is_restart_timer_on()) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, current_full_time != last_timer_on_full_time, "Error happens when the component model tries to write restart data files: the corresponding API \"CCPL_do_restart_write\" has been called more than once in the same time step. Please verify the model code with the annotation \"%s\"");
		last_timer_on_full_time = current_full_time;
		if (local_proc_id == 0) {
			array_buffer = NULL;
			time_mgr->write_time_mgt_into_array(&array_buffer, buffer_max_size, buffer_content_size);
			restart_buffer_containers.push_back(new Restart_buffer_container(comp_full_name, RESTART_BUF_TYPE_TIME, "local time manager", array_buffer, buffer_content_size));
			write_into_file();
		}
	}
}


void Restart_mgt::write_into_file()
{
	char *array_buffer = NULL;
	long buffer_max_size, buffer_content_size;
	int temp_int;
	char restart_file_name[NAME_STR_SIZE];
	Comp_comm_group_mgt_node *comp_node = comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"in Restart_mgt::write_into_file");
	FILE *restart_file;
	

	for (int i = restart_buffer_containers.size()-1; i >= 0; i --)
		restart_buffer_containers[i]->dump(&array_buffer, buffer_max_size, buffer_content_size);
	temp_int = restart_buffer_containers.size();
	write_data_into_array_buffer(&temp_int, sizeof(int), &array_buffer, buffer_max_size, buffer_content_size);

	int date = last_timer_on_full_time/(long)100000;
	int second = last_timer_on_full_time%(long)100000;
	sprintf(restart_file_name, "%s/%s.r.%08d-%05d", comp_node->get_working_dir(), comp_node->get_comp_full_name(), date, second);
	restart_file = fopen(restart_file_name, "w+");
	EXECUTION_REPORT(REPORT_ERROR, comp_id, restart_file != NULL, "Failed to open the file \"%s\" for writing restart data", restart_file_name);
	fwrite(array_buffer, buffer_content_size, 1, restart_file);
	fclose(restart_file);
	delete [] array_buffer;
	clean();

	do_restart_read(restart_file_name, "in internal test");
}


