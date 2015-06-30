/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <mpi.h>
#include "timer_mgt.h"
#include "global_data.h"


#define SECONDS_PER_DAY                     86400
#define NUM_MONTH_PER_YEAR                  12
#define NUM_DAYS_PER_NONLEAP_YEAR           365
#define NUM_DAYS_PER_LEAP_YEAR              366


int elapsed_days_on_start_of_month_of_nonleap_year[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
int elapsed_days_on_start_of_month_of_leap_year[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
int num_days_of_month_of_nonleap_year[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int num_days_of_month_of_leap_year[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


Coupling_timer::Coupling_timer(const char *freq_unit, int freq_count, int del_count, const char *cfg_name)
{
    strcpy(frequency_unit, freq_unit);
    frequency_count = freq_count;
    delay_count = del_count;
    timer_mgr->check_timer_format(frequency_unit, frequency_count, delay_count, cfg_name);
}


Coupling_timer::Coupling_timer(char **line, const char *cfg_name)
{
    get_next_attr(frequency_unit, line);
    get_next_integer_attr(line, frequency_count);
    get_next_integer_attr(line, delay_count);
    timer_mgr->check_timer_format(frequency_unit, frequency_count, delay_count, cfg_name);
}


Coupling_timer::Coupling_timer(Coupling_timer *existing_timer)
{
	frequency_count = existing_timer->frequency_count;
	delay_count = existing_timer->delay_count;
	strcpy(frequency_unit, existing_timer->frequency_unit);
}


bool Coupling_timer::is_timer_on()
{
    return timer_mgr->is_timer_on(frequency_unit, frequency_count, delay_count);
}


bool Timer_mgt::check_is_time_legal(int year, int month, int day, int second, const char *report_label)
{
	if (report_label != NULL) {
		EXECUTION_REPORT(REPORT_ERROR, year >= 0, "the %s year of simulation run can not be negative", report_label);
		EXECUTION_REPORT(REPORT_ERROR, second >=0 && second <= 86400, "the %s second of simulation run must between 0 and 86400", report_label);
		EXECUTION_REPORT(REPORT_ERROR, second%sec_per_step == 0 && (86400-second)%sec_per_step == 0, "%s second of simulation run must integer multiple of the time step", report_label);
    	EXECUTION_REPORT(REPORT_ERROR, month >= 1 && month <= 12, "the %s month must be between 1 and 12\n", report_label);
		if (leap_year_on && (((year%4) == 0 && (year%100) != 0) || (year%400) == 0))
        	EXECUTION_REPORT(REPORT_ERROR, day >= 1 && day <= num_days_of_month_of_leap_year[month-1], "the %s day must be between 1 and %d\n", report_label, num_days_of_month_of_leap_year[month-1]);
		else EXECUTION_REPORT(REPORT_ERROR, day >= 1 && day <= num_days_of_month_of_nonleap_year[month-1], "the %s day must be between 1 and %d\n", report_label, num_days_of_month_of_nonleap_year[month-1]);
		return true;
	}
	else {
		if (!(year >= 0) || !(second >=0 && second <= 86400) || !(month >= 1 && month <= 12))
			return false;
		if (leap_year_on && (((year%4) == 0 && (year%100) != 0) || (year%400) == 0)) {
			if (!(day >= 1 && day <= num_days_of_month_of_leap_year[month-1]))
				return false;
		}
		else {
			if (!(day >= 1 && day <= num_days_of_month_of_nonleap_year[month-1]))
				return false;
		}
		return true;
	}
}


Timer_mgt::Timer_mgt(int start_date, int start_second, int stop_date, int stop_second, int reference_date, bool leap_year_on, int cpl_step, const char *rest_freq_unit, int rest_freq_count, int stop_latency_seconds)
{
    int steps_per_day;
    long rest_freq_seconds;


    start_year = start_date / 10000;
    start_month = (start_date%10000) / 100;
    start_day = start_date % 100;
    this->start_second = start_second;
    stop_year = stop_date / 10000;
    stop_month = (stop_date%10000) / 100;
    stop_day = stop_date % 100;
    this->stop_second = stop_second;
    reference_year = reference_date / 10000;
    reference_month = (reference_date%10000) / 100;
    reference_day = reference_date % 100;
    sec_per_step = cpl_step;
    previous_year = start_year;
    previous_month = start_month;
    previous_day = start_day;
    previous_second = start_second;
    current_year = start_year;
    current_month = start_month;
    current_day = start_day;
    current_second = start_second;
    current_step_id = 0;
    this->stop_latency_seconds = stop_latency_seconds;
    this->leap_year_on = leap_year_on;

    EXECUTION_REPORT(REPORT_ERROR, sec_per_step>0 && (SECONDS_PER_DAY%sec_per_step)==0, "The number of seconds per day is not a multiple of the number of seconds per step\n");
    EXECUTION_REPORT(REPORT_ERROR, stop_latency_seconds%sec_per_step == 0, "the latency seconds of stopping must be integer multiple of the number of seconds per step\n");
	check_is_time_legal(start_year, start_month, start_day, start_second, "start");
	check_is_time_legal(stop_year, stop_month, stop_day, stop_second, "stop");
//	check_is_time_legal(reference_year, reference_month, reference_day, 0, "feference");  This check is disabled due to MASNUM2

    steps_per_day = SECONDS_PER_DAY / sec_per_step;
    current_num_elapsed_day = calculate_elapsed_day(start_year,start_month,start_day);
    num_total_steps = (calculate_elapsed_day(stop_year,stop_month,stop_day)-current_num_elapsed_day)*steps_per_day + (stop_second-start_second)/sec_per_step;
    EXECUTION_REPORT(REPORT_ERROR, num_total_steps > 0, "the end simulation time must be after the start simulation time\n");

    timer_mgr = this;
    restart_timer = NULL;
    restart_timer = new Coupling_timer(rest_freq_unit, rest_freq_count, 0, "C-Coupler error");
    if (words_are_the_same(rest_freq_unit, FREQUENCY_UNIT_YEARS))
        rest_freq_seconds = NUM_DAYS_PER_NONLEAP_YEAR*86400*rest_freq_count;
    else if (words_are_the_same(rest_freq_unit, FREQUENCY_UNIT_MONTHS))
        rest_freq_seconds = 28*86400*rest_freq_count;
    else if (words_are_the_same(rest_freq_unit, FREQUENCY_UNIT_DAYS))
        rest_freq_seconds = 86400*rest_freq_count;
    else rest_freq_seconds = rest_freq_count;
    EXECUTION_REPORT(REPORT_ERROR, rest_freq_seconds > stop_latency_seconds,  "the time interval of restart writing must be larger than the delay of ending component\n");

	EXECUTION_REPORT(REPORT_ERROR, check_time_consistency_between_components(get_start_full_time()), "the start date of all components are not consistent\n");
}


Timer_mgt::~Timer_mgt()
{
    delete restart_timer;
}


void Timer_mgt::reset_timer()
{
	EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"), 
		             "the model timer cannot be reset when run type is not initial\n");

	current_year = start_year;
	current_month = start_month;
	current_day = start_day;
	current_second = start_second;
	current_step_id = 0;
	
	current_num_elapsed_day = calculate_elapsed_day(start_year,start_month,start_day);
}


int Timer_mgt::get_current_num_days_in_year()
{
	if (leap_year_on && (((current_year%4) == 0 && (current_year%100) != 0) || (current_year%400) == 0))
		return elapsed_days_on_start_of_month_of_leap_year[current_month-1] + current_day;
	return elapsed_days_on_start_of_month_of_nonleap_year[current_month-1] + current_day;
}


long Timer_mgt::calculate_elapsed_day(int year, int month, int day)
{
	int num_leap_year;


	check_is_time_legal(year, month, day, 0, "(at calculate_elapsed_day)");

	if (!leap_year_on)
	    return year*NUM_DAYS_PER_NONLEAP_YEAR + elapsed_days_on_start_of_month_of_nonleap_year[month-1] + day - 1;

	num_leap_year = (year-1)/4 - (year-1)/100 + (year-1)/400;

	if (year > 0)
		num_leap_year ++;   // year 0 is a leap year

	if (((year%4) == 0 && (year%100) != 0) || (year%400) == 0)
		return year*NUM_DAYS_PER_NONLEAP_YEAR + num_leap_year + elapsed_days_on_start_of_month_of_leap_year[month-1] + day - 1;

	return year*NUM_DAYS_PER_NONLEAP_YEAR + num_leap_year + elapsed_days_on_start_of_month_of_nonleap_year[month-1] + day - 1;
}


void Timer_mgt::advance_coupling_step()
{
    int num_remain_days, i, num_days_in_current_month;
 

	previous_year = current_year;
	previous_month = current_month;
	previous_day = current_day;
	previous_second = current_second;

    current_second += sec_per_step;
    current_step_id ++;
    if (current_second == SECONDS_PER_DAY) {
        current_second = 0;
        current_num_elapsed_day ++;
		if (leap_year_on && (((current_year%4) == 0 && (current_year%100) != 0) || (current_year%400) == 0)) 
			num_days_in_current_month = num_days_of_month_of_leap_year[current_month-1];
		else num_days_in_current_month = num_days_of_month_of_nonleap_year[current_month-1];
		current_day ++;
		if (current_day > num_days_in_current_month) {
			current_month ++;
			current_day = 1;
		}
		if (current_month > 12) {
			current_month = 1;
			current_year ++;
		}
    }
	
    EXECUTION_REPORT(REPORT_LOG, true, "current time is %d-%d-%d-%d", current_year, current_month, current_day, current_second);
	EXECUTION_REPORT(REPORT_LOG, true, "current step id is %d", current_step_id);
}


double Timer_mgt::get_double_current_calendar_time(int shift_second)
{
	double calday;

	
	EXECUTION_REPORT(REPORT_ERROR, shift_second>=-86400 && shift_second<= 86400, "The shift seconds for calculating calendar time must be between -86400 and 86400");

	if (leap_year_on && (((current_year%4) == 0 && (current_year%100) != 0) || (current_year%400) == 0)) {
		calday = elapsed_days_on_start_of_month_of_leap_year[current_month-1] + current_day + ((double)(current_second+shift_second))/SECONDS_PER_DAY;
		if (calday > (NUM_DAYS_PER_LEAP_YEAR+1))
			calday = calday - (NUM_DAYS_PER_LEAP_YEAR+1);
	}
	else {
		calday = elapsed_days_on_start_of_month_of_nonleap_year[current_month-1] + current_day + ((double)(current_second+shift_second))/SECONDS_PER_DAY;
		if (calday > (NUM_DAYS_PER_NONLEAP_YEAR+1))
			calday = calday - (NUM_DAYS_PER_NONLEAP_YEAR+1);
	}

	return calday;
}


float Timer_mgt::get_float_current_calendar_time(int shift_second)
{
    return (float) get_double_current_calendar_time(shift_second);
}


bool Timer_mgt::is_timer_on(char *frequency_unit, int frequency_count, int delay_count)
{
    long num_elapsed_time;


    EXECUTION_REPORT(REPORT_ERROR, frequency_count > 0, "C-Coupler software error: the frequency count must be larger than 0\n");

    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_SECONDS)) {
        EXECUTION_REPORT(REPORT_ERROR, (frequency_count%sec_per_step) == 0, "C-Coupler software error: the frequency count of seconds must be integer multiple of coupling step\n");
        num_elapsed_time = (current_num_elapsed_day-calculate_elapsed_day(start_year,start_month,start_day))*SECONDS_PER_DAY+current_second;
    }
    else if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_DAYS)) {
        if (current_second != 0)
            return false;
        num_elapsed_time = current_num_elapsed_day-calculate_elapsed_day(start_year,start_month,start_day);
    }
    else if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_MONTHS)) {
        if (current_second != 0 || current_day != 1)
            return false;
        num_elapsed_time = (current_year-start_year)*NUM_MONTH_PER_YEAR+current_month-start_month;
    }
    else if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_YEARS)) {
        if (current_second != 0 || current_day != 1 || current_month != 1)
            return false;
        num_elapsed_time = current_year-start_year;
    }
    else EXECUTION_REPORT(REPORT_ERROR, false, "C-Coupler software error: frequency unit %s is unsupported\n", frequency_unit);

    return num_elapsed_time >= delay_count && ((num_elapsed_time-delay_count)%frequency_count) == 0;
}


void Timer_mgt::set_restart_time(long start_full_time, long restart_full_time)
{
    long restart_date_value;
    long tmp_date_value;


	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "restart"))
	    EXECUTION_REPORT(REPORT_ERROR, get_start_full_time() == start_full_time, "the start time read from restart file is different from current setting\n");
	EXECUTION_REPORT(REPORT_ERROR, timer_mgr->check_time_consistency_between_components(start_full_time), "the start date of all components in the restart run (restart) is different\n");
	EXECUTION_REPORT(REPORT_ERROR, timer_mgr->check_time_consistency_between_components(restart_full_time), "the restart date of all components in the restart run (restart) is different\n");
    current_second = restart_full_time % 100000;
    current_day = (restart_full_time/100000)%100;
    current_month = (restart_full_time/10000000)%100;
    current_year = restart_full_time/1000000000;
    current_num_elapsed_day = calculate_elapsed_day(current_year,current_month,current_day);
	current_step_id = (current_num_elapsed_day-calculate_elapsed_day(start_year,start_month,start_day)) * (SECONDS_PER_DAY/sec_per_step);
	EXECUTION_REPORT(REPORT_LOG, true, "current step id from restart is %d", current_step_id);
}


bool Timer_mgt::check_is_coupled_run_finished()
{
    EXECUTION_REPORT(REPORT_LOG, true, "check_is_coupled_run_finished %d %ld", current_step_id, num_total_steps);
    return (current_step_id > num_total_steps + (stop_latency_seconds/sec_per_step));
}


bool Timer_mgt::check_is_coupled_run_restart_time()
{
    return restart_timer->is_timer_on();
}


long Timer_mgt::get_start_full_time()
{
    return (long)start_second + (long)start_day*100000 + (long)start_month*10000000 + (long)start_year*1000000000;
}



long Timer_mgt::get_previous_full_time()
{
    return (long)previous_second + (long)previous_day*100000 + (long)previous_month*10000000 + (long)previous_year*1000000000;
}


long Timer_mgt::get_current_full_time()
{
    return (long)current_second + (long)current_day*100000 + (long)current_month*10000000 + (long)current_year*1000000000;
}


int Timer_mgt::get_current_date()
{
    return (int) (current_year*10000 + current_month*100 + current_day);
}


void Timer_mgt::check_timer_format(const char *frequency_unit, int frequency_count, int delay_count, const char *cfg_name)
{
    bool too_small_end_time;
    bool fit_small_rest_freq;

    
    EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(frequency_unit, FREQUENCY_UNIT_SECONDS) || words_are_the_same(frequency_unit, FREQUENCY_UNIT_DAYS) ||
                 words_are_the_same(frequency_unit, FREQUENCY_UNIT_MONTHS) || words_are_the_same(frequency_unit, FREQUENCY_UNIT_YEARS), 
                 "The frequency unit in timer must be one of \"seconds\", \"days\", \"months\" and \"years\". Please verify the timers in the configuration file %s", cfg_name);
    EXECUTION_REPORT(REPORT_ERROR, frequency_count > 0, "The frquency count in timer must be larger than 0. Please verify the timers in the configuration file %s", cfg_name);
    EXECUTION_REPORT(REPORT_ERROR, delay_count >= 0, "The delay count in timer must be positive. Please verify the timers in the configuration file %s", cfg_name);
    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_SECONDS)) {
        EXECUTION_REPORT(REPORT_ERROR, frequency_count%sec_per_step == 0, "The frequency count in timer must be the integer multiple coupling step when frequency unit is seconds. Please verify the timers in the configuration file %s", cfg_name);
        EXECUTION_REPORT(REPORT_ERROR, delay_count%sec_per_step == 0, "The delay count in timer must be the integer multiple coupling step when frequency unit is seconds. Please verify the timers in the configuration file %s", cfg_name);        
    }
    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_SECONDS))
        too_small_end_time = ((long)num_total_steps)*((long)sec_per_step) <= delay_count;
    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_DAYS))
        too_small_end_time = ((long)num_total_steps)*((long)sec_per_step) <= delay_count*86400;
    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_MONTHS))
        too_small_end_time = ((long)num_total_steps)*((long)sec_per_step) <= delay_count*86400*31;
    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_YEARS))
        too_small_end_time = ((long)num_total_steps)*((long)sec_per_step) <= delay_count*86400*NUM_DAYS_PER_LEAP_YEAR;
    EXECUTION_REPORT(REPORT_ERROR, !too_small_end_time, "The integration time of the simulation should be larger than the coupling delay time <%s, %d>. Please verify the timers in the configuration file %s", frequency_unit, delay_count, cfg_name);

    if (restart_timer == NULL)
        return;
    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_YEARS))
        fit_small_rest_freq = (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_YEARS) && (restart_timer->frequency_count%frequency_count) == 0);
    else if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_MONTHS)) {
        if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_YEARS))
            fit_small_rest_freq = (((restart_timer->frequency_count*12)%frequency_count) == 0);
        else if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_MONTHS))
           fit_small_rest_freq = ((restart_timer->frequency_count%frequency_count) == 0);
        else fit_small_rest_freq = false;
    }
    else if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_DAYS)) {
        if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_YEARS))
            fit_small_rest_freq = (((restart_timer->frequency_count*NUM_DAYS_PER_NONLEAP_YEAR)%frequency_count) == 0);
        else if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_MONTHS))
           fit_small_rest_freq = (frequency_count == 1);
        else if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_DAYS))
           fit_small_rest_freq = ((restart_timer->frequency_count%frequency_count) == 0);
        else if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_SECONDS))
           fit_small_rest_freq = (restart_timer->frequency_count%(frequency_count*86400) == 0);
        else fit_small_rest_freq = false;
    }
    else {
        if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_YEARS))
            fit_small_rest_freq = (((restart_timer->frequency_count*NUM_DAYS_PER_NONLEAP_YEAR*86400)%frequency_count) == 0);
        else if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_MONTHS))
           fit_small_rest_freq = ((86400%frequency_count) == 0);
        else if (words_are_the_same(restart_timer->frequency_unit, FREQUENCY_UNIT_DAYS))
           fit_small_rest_freq = ((restart_timer->frequency_count*86400%frequency_count) == 0);
        else fit_small_rest_freq = ((restart_timer->frequency_count%frequency_count) == 0);
    }

	/* This checker may not be right */
    EXECUTION_REPORT(REPORT_WARNING, fit_small_rest_freq, "the writing of restart data files may be to frequent\n");
}


Comps_transfer_time_info *Timer_mgt::allocate_comp_transfer_time_info(int remote_comp_id)
{
    Comps_transfer_time_info *comps_transfer_time_info = new Comps_transfer_time_info;
    comps_transfer_time_info->remote_comp_id = remote_comp_id;
    comps_transfer_time_info->counter = 0;
    comps_transfer_time_info->remote_comp_frequency = -1;
    comps_transfer_time_info->remote_comp_time = -1;
    comps_transfer_time_info->local_comp_time = -1;
    comps_transfer_time_infos.push_back(comps_transfer_time_info);
    return comps_transfer_time_info;
}


bool Timer_mgt::check_time_consistency_between_components(long full_time)
{
    int i, num_global_procs;
	bool consistent;
	long *full_time_arrays;


	if (compset_communicators_info_mgr->get_num_components() == 1)
		return true;
	
	EXECUTION_REPORT(REPORT_ERROR, MPI_Comm_size(compset_communicators_info_mgr->get_global_comm_group(), &num_global_procs) == MPI_SUCCESS);
	
	consistent = true;
	full_time_arrays = new long [num_global_procs];
	EXECUTION_REPORT(REPORT_ERROR, MPI_Allgather(&full_time, 1, MPI_LONG, full_time_arrays, 1, MPI_LONG, compset_communicators_info_mgr->get_global_comm_group()) == MPI_SUCCESS);
	for (i = 1; i < num_global_procs; i ++)
		if (full_time_arrays[i] != full_time_arrays[i-1])
			consistent = false;

    delete [] full_time_arrays;

	return consistent;
}


void Timer_mgt::get_elapsed_days_from_start_date(int *num_days, int *num_seconds)
{
	long current_num_elapsed_days, start_num_elapsed_days;
	
	current_num_elapsed_days = calculate_elapsed_day(current_year, current_month, current_day);
	start_num_elapsed_days = calculate_elapsed_day(start_year, start_month, start_day);
	*num_days = current_num_elapsed_days - start_num_elapsed_days;
	*num_seconds = current_second;
}


void Timer_mgt::get_elapsed_days_from_reference_date(int *num_days, int *num_seconds)
{
	long current_num_elapsed_days, reference_num_elapsed_days;
	
	current_num_elapsed_days = calculate_elapsed_day(current_year, current_month, current_day);
	reference_num_elapsed_days = calculate_elapsed_day(reference_year, reference_month, reference_day);
	*num_days = current_num_elapsed_days - reference_num_elapsed_days;
	*num_seconds = current_second;
}


void Timer_mgt::get_current_time(int &year, int &month, int &day, int &second, int shift_second)
{
	int num_days_in_current_month;

	
	EXECUTION_REPORT(REPORT_ERROR, shift_second>=-86400 && shift_second<= 86400, "The shift seconds for calculating calendar time must be between -86400 and 86400");
	
	year = current_year;
	month = current_month;
	day = current_day;
	second = current_second + shift_second;

    if (second >= SECONDS_PER_DAY) {
		second -= SECONDS_PER_DAY;
		if (leap_year_on && (((year%4) == 0 && (year%100) != 0) || (year%400) == 0)) 
			num_days_in_current_month = num_days_of_month_of_leap_year[month-1];
		else num_days_in_current_month = num_days_of_month_of_nonleap_year[month-1];
		day ++;
		if (day > num_days_in_current_month) {
			month ++;
			day = 1;
		}
		if (month > 12) {
			month = 1;
			year ++;
		}
    }
}


int Timer_mgt::get_current_num_time_step()
{
	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "hybrid") && current_step_id < SECONDS_PER_DAY/sec_per_step)
		return current_step_id + SECONDS_PER_DAY/sec_per_step;
    return current_step_id; 
}

