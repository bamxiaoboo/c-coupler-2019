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


void wtime(double *t)
{
  static int sec = -1;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  if (sec < 0) 
    sec = tv.tv_sec;
  *t = (tv.tv_sec - sec) + 1.0e-6*tv.tv_usec;
}


void report_header(int report_type, int comp_id, bool &condition, char *output_string)
{
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
			else sprintf(output_string+strlen(output_string), "C-Coupler REPORT ERROR: ");
#endif
			break;
		case REPORT_LOG:
			if (line_number > 0 && execution_phase_number < 2)
				sprintf(output_string+strlen(output_string), "CoR REPORT LOG at script line %d: ", line_number);
#ifndef ONLY_CoR
			else sprintf(output_string+strlen(output_string), "C-Coupler REPORT LOG: ");
#endif			
			break;
		case REPORT_WARNING:
			if (line_number > 0 && execution_phase_number < 2)
				sprintf(output_string+strlen(output_string), "CoR REPORT WARNING at script line %d: ", line_number);
#ifndef ONLY_CoR
			else sprintf(output_string+strlen(output_string), "C-Coupler REPORT WARNING: ");
#endif
			break;
		case REPORT_PROGRESS:
			if (line_number > 0 && execution_phase_number < 2)
				sprintf(output_string+strlen(output_string), "CoR REPORT PROGRESS at script line %d: ", line_number);
#ifndef ONLY_CoR
 			else sprintf(output_string+strlen(output_string), "C-Coupler REPORT PROGRESS: ");
#endif
			break;
		default:
			printf("Software error: report type %d is not supported\n", report_type);
			assert(false);
			break;
	}
#ifndef ONLY_CoR
	if (!(line_number > 0 && execution_phase_number < 2) && comp_comm_group_mgt_mgr != NULL) {
		if (comp_id == -1 || !comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id))
			sprintf(output_string+strlen(output_string)-2, " in the root component model corresponding to the executable named \"%s\": ", comp_comm_group_mgt_mgr->get_executable_name());
		else {
			int current_date = components_time_mgrs->get_time_mgr(comp_id)->get_current_date();
			int current_second = components_time_mgrs->get_time_mgr(comp_id)->get_current_second();
			int current_step_id = components_time_mgrs->get_time_mgr(comp_id)->get_current_step_id();
			sprintf(output_string+strlen(output_string)-2, " in the component model \"%s\" corresponding to the executable named \"%s\", at the current simulation time of %08d-%05d (the current step number is %d): ", 
				    comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id,"C-Coupler code in report_header for getting component management node")->get_comp_name(), comp_comm_group_mgt_mgr->get_executable_name(), current_date, current_second, current_step_id);
		}
	}
#endif
}


void report_ender(int report_type, int comp_id, char *output_string)
{
	if (comp_id == -1 || !comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id)) {
		printf("%s\n\n", output_string);
		fflush(NULL);
	}
	else {
		char log_file_name[NAME_STR_SIZE];
		comp_comm_group_mgt_mgr->get_log_file_name(comp_id, log_file_name);
		FILE *log_file = fopen(log_file_name, "a+");
		fprintf(log_file, "%s\n\n", output_string);
		fclose(log_file);
		if (report_type == REPORT_ERROR)
			printf("ERROR happens. Please check the log file \"%s\"\n\n", log_file_name);
	}
	
	if (report_type == REPORT_ERROR)
		assert(false);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition) 
{
	char output_string[NAME_STR_SIZE*4];


	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const void *addr) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, addr);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const void *str2, const char *str3) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1, int value2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, value1, value2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, int value2, const char *str2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, value2, str2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, const char *str2, int value2, long value3) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, str2, value2, value3);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, int value1) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, value1);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, int value1, long value2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, value1, value2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, int value1, const char *str5) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, value1, str5);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, int value1) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, value1);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, const char *str6) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, str6);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, const char *str6, const char *str7) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, str6, str7);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, int value1, const char *str6) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, value1, str6);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, const char *str5, int value1, int value2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, str5, value1, value2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4, int value1, void *addr) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, str4, value1, addr);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, const char *str4) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, str4);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, long value1) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, value1);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, const char *str2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, value1, str2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, const char *str2, const char *str3) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, value1, str2, str3);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, int value1) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, value1);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, int value1, const char *str3) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, value1, str3);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, long value1, int value2, long value3, int value4) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, value1, value2, value3, value4);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, value1);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1, const char *str4) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, value1, str4);

	report_ender(report_type, comp_id, output_string);
}



void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, double value1, double value2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, value1, value2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1, long value2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, str3, value1, value2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, const char *str3, int value1, long value2, long value3) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

	sprintf(output_string+strlen(output_string), str1, str2, str3, value1, value2, value3);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, long value1, long value2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, value1, value2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, const char *str2, long value1, long value2, long value3) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, str2, value1, value2, value3);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, int value2) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, value2);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, int value2, int value3) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, value2, value3);

	report_ender(report_type, comp_id, output_string);
}


void EXECUTION_REPORT(int report_type, int comp_id, bool condition, const char *str1, int value1, int value2, int value3, int value4) 
{
	char output_string[NAME_STR_SIZE*4];
	

	report_header(report_type, comp_id, condition, output_string);

	if (!condition)
		return;

    sprintf(output_string+strlen(output_string), str1, value1, value2, value3, value4);

	report_ender(report_type, comp_id, output_string);
}


