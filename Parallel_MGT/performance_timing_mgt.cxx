/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include <string.h>
#include "performance_timing_mgt.h"
#include "global_data.h"


Performance_timing_unit::Performance_timing_unit(int unit_type, int unit_behavior, int unit_int_keyword, const char *unit_char_keyword)
{
	check_timing_unit(unit_type, unit_behavior, unit_int_keyword, unit_char_keyword);
	this->previous_time = -1.0;
	this->total_time = 0,0;
	this->unit_type = unit_type;
	this->unit_behavior = unit_behavior;
	this->unit_int_keyword = unit_int_keyword;
	if (unit_type == TIMING_TYPE_COMPUTATION)
		strcpy(this->unit_char_keyword, unit_char_keyword);
	else this->unit_char_keyword[0] = '\0';
}


void Performance_timing_unit::check_timing_unit(int unit_type, int unit_behavior, int unit_int_keyword, const char *unit_char_keyword)
{
	EXECUTION_REPORT(REPORT_ERROR, unit_type == TIMING_TYPE_COMMUNICATION || unit_type == TIMING_TYPE_IO || unit_type == TIMING_TYPE_COMPUTATION,
		             "C-Coupler error in checking unit_type in match_timing_unit");
	if (unit_type == TIMING_TYPE_COMMUNICATION)
		EXECUTION_REPORT(REPORT_ERROR, unit_behavior == TIMING_COMMUNICATION_RECV || unit_behavior == TIMING_COMMUNICATION_SEND || unit_behavior == TIMING_COMMUNICATION_SENDRECV,
		                 "C-Coupler error in checking unit_behavior in match_timing_unit");
	if (unit_type == TIMING_TYPE_IO)
		EXECUTION_REPORT(REPORT_ERROR, unit_behavior == TIMING_IO_INPUT || unit_behavior == TIMING_IO_OUTPUT || unit_behavior == TIMING_IO_RESTART,
		                 "C-Coupler error in checking unit_behavior in match_timing_unit");
}


bool Performance_timing_unit::match_timing_unit(int unit_type, int unit_behavior, int unit_int_keyword, const char *unit_char_keyword)
{
	check_timing_unit(unit_type, unit_behavior, unit_int_keyword, unit_char_keyword);

	if (unit_type == TIMING_TYPE_COMMUNICATION)
		return unit_type == this->unit_type && unit_behavior == this->unit_behavior && unit_int_keyword == this->unit_int_keyword;
	if (unit_type == TIMING_TYPE_IO)
		return unit_type == this->unit_type && unit_behavior == this->unit_behavior;
	if (unit_type == TIMING_TYPE_COMPUTATION)
		return unit_type == this->unit_type && words_are_the_same(unit_char_keyword,this->unit_char_keyword);

	return false;
}


void Performance_timing_unit::timing_start()
{
	EXECUTION_REPORT(REPORT_ERROR, previous_time == -1.0, "C-Coupler or model error in starting performance timing: timing unit has not been stoped");
	wtime(&previous_time);
}


void Performance_timing_unit::timing_stop()
{
	double current_time;


	EXECUTION_REPORT(REPORT_ERROR, previous_time != -1.0, "C-Coupler or model error in stopping performance timing: timing unit has not been started");
	wtime(&current_time);
	if (current_time >= previous_time)
		total_time += current_time - previous_time;
	previous_time = -1.0;
}


void Performance_timing_unit::timing_output()
{
	double all_process_sum_time;
	int num_procs = compset_communicators_info_mgr->get_num_procs_in_comp(compset_communicators_info_mgr->get_current_comp_id());

	
    MPI_Allreduce(&total_time, &all_process_sum_time, 1, MPI_DOUBLE, MPI_SUM, compset_communicators_info_mgr->get_current_comp_comm_group());
	if (compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group() == 0) {
		if (unit_type == TIMING_TYPE_IO) {
			if (unit_behavior == TIMING_IO_INPUT) 
				printf("%s spends %lf seconds for reading input data file in each process on average\n", compset_communicators_info_mgr->get_current_comp_name(), all_process_sum_time/num_procs);
			if (unit_behavior == TIMING_IO_OUTPUT) 
				printf("%s spends %lf seconds for writing output data file in each process on average\n", compset_communicators_info_mgr->get_current_comp_name(), all_process_sum_time/num_procs);
			if (unit_behavior == TIMING_IO_RESTART)
				printf("%s spends %lf seconds for writing restart file in each process on average\n", compset_communicators_info_mgr->get_current_comp_name(), all_process_sum_time/num_procs);
		}
		if (unit_type == TIMING_TYPE_COMMUNICATION) {
			if (unit_behavior == TIMING_COMMUNICATION_RECV)
				printf("%s spends %lf seconds for waiting for receiving data from %s in each process on average\n", compset_communicators_info_mgr->get_current_comp_name(), 
				       all_process_sum_time/num_procs, compset_communicators_info_mgr->get_comp_name_by_id(unit_int_keyword));
			if (unit_behavior == TIMING_COMMUNICATION_SEND)
				printf("%s spends %lf seconds for waiting for sending data to %s in each process on average\n", compset_communicators_info_mgr->get_current_comp_name(), 
				       all_process_sum_time/num_procs, compset_communicators_info_mgr->get_comp_name_by_id(unit_int_keyword));
			if (unit_behavior == TIMING_COMMUNICATION_SENDRECV)
				printf("%s spends %lf seconds for data communication for data remapping in each process on average\n", compset_communicators_info_mgr->get_current_comp_name(), all_process_sum_time/num_procs);
		}
		if (unit_type == TIMING_TYPE_COMPUTATION)
			printf("%s spends %lf seconds for numerical algorithm %s in each process on average\n", compset_communicators_info_mgr->get_current_comp_name(), 
			        all_process_sum_time/num_procs, unit_char_keyword);
	}
}


int Performance_timing_mgt::search_timing_unit(int unit_type, int unit_behavior, int unit_int_keyword, const char *unit_char_keyword)
{
	int i;
	
	for (i = 0; i < performance_timing_units.size(); i ++)
		if (performance_timing_units[i]->match_timing_unit(unit_type, unit_behavior, unit_int_keyword, unit_char_keyword))
			return i;

	performance_timing_units.push_back(new Performance_timing_unit(unit_type, unit_behavior, unit_int_keyword, unit_char_keyword));
	return performance_timing_units.size() - 1;
}


void Performance_timing_mgt::performance_timing_start(int unit_type, int unit_behavior, int unit_int_keyword, const char *unit_char_keyword)
{
	performance_timing_units[search_timing_unit(unit_type,unit_behavior,unit_int_keyword,unit_char_keyword)]->timing_start();
}


void Performance_timing_mgt::performance_timing_stop(int unit_type, int unit_behavior, int unit_int_keyword, const char *unit_char_keyword)
{
	performance_timing_units[search_timing_unit(unit_type,unit_behavior,unit_int_keyword,unit_char_keyword)]->timing_stop();
}


void Performance_timing_mgt::performance_timing_output()
{
	for (int i = 0; i < performance_timing_units.size(); i ++)
		performance_timing_units[i]->timing_output();
}


Performance_timing_mgt::~Performance_timing_mgt()
{
	for (int i = 0; i < performance_timing_units.size(); i ++)
		delete performance_timing_units[i];
}

