/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef ONLY_CoR
#include "global_data.h"
#endif
#include "execution_report.h"
#include "cor_global_data.h"
#include <assert.h>
#include <stdio.h>
#include <sys/time.h>


FILE *log_file;
char output_string[NAME_STR_SIZE*4];

void wtime(double *t)
{
  static int sec = -1;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (sec < 0) 
    sec = tv.tv_sec;
  *t = (tv.tv_sec - sec) + 1.0e-6*tv.tv_usec;
}


void report_header(int report_type, int comp_id, bool &condition)
{
	if (comp_id == -1 || !comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id))
		log_file = NULL;
	else {
		char log_file_name[NAME_STR_SIZE];
		comp_comm_group_mgt_mgr->get_log_file_name(comp_id, log_file_name);
		log_file = fopen(log_file_name, "a+");
	}

	output_string[0] = '\0';
	
	switch (report_type) {
		case REPORT_ERROR:
			condition = !condition;
			break;
		case REPORT_LOG:
			condition = false;
#ifdef DEBUG_CCPL
			condition = true;
#endif 
#ifdef DEBUG_CoR
			condition = true;
#endif
			break;
		case REPORT_WARNING:
			condition = !condition;
			break;
		case REPORT_PROGRESS:
			condition = true;
			if (line_number > 0 && execution_phase_number == 0)
				condition = false;
			break;
		default:
			printf("report type %d is not support\n", report_type);
			assert(false);
	}

	if (!condition)
		return;

	switch (report_type) {
		case REPORT_ERROR:
			if (line_number > 0 && execution_phase_number < 2)
				sprintf(output_string+strlen(output_string), "CoR REPORT ERROR at script line %d: ", line_number);
#ifndef ONLY_CoR
			else { 
				if (timer_mgr != NULL && compset_communicators_info_mgr != NULL)
					sprintf(output_string+strlen(output_string), "C-Coupler REPORT ERROR in component %s in local process %d at simulation time %ld: ", compset_communicators_info_mgr->get_current_comp_name(), compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group(), timer_mgr->get_current_full_time());
				else sprintf(output_string+strlen(output_string), "C-Coupler REPORT ERROR: ");
			}
#endif
			break;
		case REPORT_LOG:
			if (line_number > 0 && execution_phase_number < 2)
				sprintf(output_string+strlen(output_string), "CoR REPORT LOG at script line %d: ", line_number);
#ifndef ONLY_CoR
			else {
				if (timer_mgr != NULL && compset_communicators_info_mgr != NULL)
					sprintf(output_string+strlen(output_string), "C-Coupler REPORT LOG in component %s in local process %d at simulation time %ld: ", compset_communicators_info_mgr->get_current_comp_name(), compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group(), timer_mgr->get_current_full_time());
				else sprintf(output_string+strlen(output_string), "C-Coupler REPORT LOG: ");
			}
#endif			
			break;
		case REPORT_WARNING:
			if (line_number > 0 && execution_phase_number < 2)
				sprintf(output_string+strlen(output_string), "CoR REPORT WARNING at script line %d: ", line_number);
#ifndef ONLY_CoR
			else {
				if (timer_mgr != NULL && compset_communicators_info_mgr != NULL)
					sprintf(output_string+strlen(output_string), "C-Coupler REPORT WARNING in component %s in local process %d at simulation time %ld: ", compset_communicators_info_mgr->get_current_comp_name(), compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group(), timer_mgr->get_current_full_time());
				else sprintf(output_string+strlen(output_string), "C-Coupler REPORT WARNING: ");
			}
#endif
			break;
		case REPORT_PROGRESS:
			if (line_number > 0 && execution_phase_number < 2)
				sprintf(output_string+strlen(output_string), "CoR REPORT PROGRESS at script line %d: ", line_number);
#ifndef ONLY_CoR
			else {
				if (timer_mgr != NULL && compset_communicators_info_mgr != NULL)
					sprintf(output_string+strlen(output_string), "C-Coupler REPORT PROGRESS in component %s in local process %d at simulation time %ld: ", compset_communicators_info_mgr->get_current_comp_name(), compset_communicators_info_mgr->get_current_proc_id_in_comp_comm_group(), timer_mgr->get_current_full_time());
				else sprintf(output_string+strlen(output_string), "C-Coupler REPORT PROGRESS: ");
			}
#endif
			break;
	}
}


void report_ender(int report_type, int comp_id)
{
	if (log_file == NULL) {
		printf("%s\n", output_string);
		fflush(NULL);
	}
	else {
		fprintf(log_file, "%s\n", output_string);
		fclose(log_file);
	}

	if (report_type == REPORT_ERROR) {
		if (log_file != NULL) {
			char log_file_name[NAME_STR_SIZE];
			comp_comm_group_mgt_mgr->get_log_file_name(comp_id, log_file_name);
			printf("ERROR happens. Please check the log file \"%s\"\n", log_file_name);
		}
		assert(false);
    }
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const void *addr) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, addr);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const void *str2, const char *str3) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1, int value2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, value1, value2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, int value2, const char *str2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, value2, str2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, const char *str2, int value2, long value3) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, str2, value2, value3);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, int value1) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, value1);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, int value1, long value2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, value1, value2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, int value1, const char *str5) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, value1, str5);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, int value1) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, value1);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, const char *str6) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, str6);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, const char *str6, const char *str7) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, str6, str7);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, int value1, const char *str6) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, value1, str6);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, int value1, int value2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, value1, value2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, int value1, void *addr) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, value1, addr);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, str4);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, long value1) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, value1);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, const char *str2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, value1, str2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, const char *str2, const char *str3) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, value1, str2, str3);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, int value1) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, value1);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, int value1, const char *str3) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, value1, str3);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, long value1, int value2, long value3, int value4) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, value1, value2, value3, value4);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, value1);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1, const char *str4) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, value1, str4);

	report_ender(report_type, comp_id);
}



void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, double value1, double value2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, value1, value2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1, long value2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, value1, value2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1, long value2, long value3) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, value1, value2, value3);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, long value1, long value2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, value1, value2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, long value1, long value2, long value3) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, value1, value2, value3);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, int value2) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, value2);

	report_ender(report_type, comp_id);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, int value2, int value3, int value4) 
{
	report_header(report_type, comp_id, condition);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, value2, value3, value4);

	report_ender(report_type, comp_id);
}


