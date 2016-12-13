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



int elapsed_days_on_start_of_month_of_nonleap_year[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
int elapsed_days_on_start_of_month_of_leap_year[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
int num_days_of_month_of_nonleap_year[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int num_days_of_month_of_leap_year[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};



bool common_is_timer_on(const char *frequency_unit, int frequency_count, int lag_count, int current_year, 
	                  int current_month, int current_day, int current_second, int current_num_elapsed_day,
	                  int start_year, int start_month, int start_day, int start_second, int start_num_elapsed_day)
{
    long num_elapsed_time;


    EXECUTION_REPORT(REPORT_ERROR,-1, frequency_count > 0, "C-Coupler software error: the frequency count must be larger than 0\n");

	printf("check %d-%d-%d-%d: %s %d %d\n", current_year, current_month, current_day, current_second, frequency_unit, frequency_count, lag_count);

    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_SECONDS)) {
        num_elapsed_time = ((long)(current_num_elapsed_day-start_num_elapsed_day))*SECONDS_PER_DAY + current_second - start_second;
    }
    else if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_DAYS)) {
        if (current_second != 0)
            return false;
        num_elapsed_time = current_num_elapsed_day-start_num_elapsed_day;
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
    else EXECUTION_REPORT(REPORT_ERROR,-1, false, "C-Coupler software error: frequency unit %s is unsupported\n", frequency_unit);

    return num_elapsed_time >= lag_count && ((num_elapsed_time-lag_count)%frequency_count) == 0;
}


Coupling_timer::Coupling_timer(int comp_id, int timer_id, int *children_timers_id, int num_children_timers, int or_or_and, const char *annotation)
{
	this->timer_id = timer_id;
	this->comp_id = comp_id;
	this->or_or_and = or_or_and;
	EXECUTION_REPORT(REPORT_ERROR, comp_id, num_children_timers > 1, "The parameter of number of children timers must be larger than 1 when defining a complex timer. Please check the model code according to the annotation \"%s\"", annotation);
	EXECUTION_REPORT(REPORT_ERROR, comp_id, or_or_and == 0 || or_or_and == 1, "The value of the parameter of \"OR_or_AND\" is wrong when defining a complex timer. Its value must be 0 (means OR) or 1 (means AND). Please check the model code according to the annotation \"%s\"", annotation);
	for (int i = 0; i < num_children_timers; i ++) {
		EXECUTION_REPORT(REPORT_ERROR, comp_id, timer_mgr2->get_timer(children_timers_id[i]) != NULL, "The id of the %dth timer for defining a new timer is wrong. Please check the model code according to the annotation \"%s\"", i, annotation);
		children.push_back(timer_mgr2->get_timer(children_timers_id[i]));
		EXECUTION_REPORT(REPORT_ERROR, comp_id, children[i]->get_comp_id() == comp_id, "All timers for defining a new timer must belong to the same component. Please check the model code of defining the current timer according to the annotation \"%s\", and the model code corresponding to the %dth timer according to the annotation \"%s\"", i, annotation, annotation_mgr->get_annotation(children[i]->get_timer_id(), "define timer"));
	}
	comp_time_mgr = components_time_mgrs->get_time_mgr(comp_id);
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_time_mgr != NULL, "Software error in Coupling_timer::Coupling_timer, with annotation \"%s\"", annotation);
	annotation_mgr->add_annotation(timer_id, "define timer", annotation);
}


Coupling_timer::Coupling_timer(int comp_id, int timer_id, const char *freq_unit, int freq_count, int del_count, const char *annotation)
{
    strcpy(frequency_unit, freq_unit);
    frequency_count = freq_count;
    lag_count = del_count;
	this->timer_id = timer_id;
	this->comp_id = comp_id;
	this->or_or_and = -1;
	comp_time_mgr = components_time_mgrs->get_time_mgr(comp_id);
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_time_mgr != NULL, "Software error in Coupling_timer::Coupling_timer, with annotation \"%s\"", annotation);
	comp_time_mgr->check_timer_format(frequency_unit, frequency_count, lag_count, annotation);
	annotation_mgr->add_annotation(timer_id, "define timer", annotation);
	if (words_are_the_same(freq_unit, FREQUENCY_UNIT_STEPS)) {
		EXECUTION_REPORT(REPORT_ERROR, -1, comp_time_mgr->get_time_step_in_second() > 0, "Software error in Coupling_timer::Coupling_timer: uninitialized time step");
		strcpy(frequency_unit, FREQUENCY_UNIT_SECONDS);
		frequency_count *= comp_time_mgr->get_time_step_in_second();
		lag_count *= comp_time_mgr->get_time_step_in_second();
	}
}


Coupling_timer::Coupling_timer(const char *freq_unit, int freq_count, int del_count, const char *annotation)
{
    strcpy(frequency_unit, freq_unit);
    frequency_count = freq_count;
    lag_count = del_count;
	timer_id = -1;
	comp_id = -1;
    timer_mgr->check_timer_format(frequency_unit, frequency_count, lag_count, annotation);
}


Coupling_timer::Coupling_timer(char **line, const char *cfg_name)
{
    get_next_attr(frequency_unit, line);
    get_next_integer_attr(line, frequency_count);
    get_next_integer_attr(line, lag_count);
		timer_id = -1;
	comp_id = -1;
    timer_mgr->check_timer_format(frequency_unit, frequency_count, lag_count, cfg_name);
}


Coupling_timer::Coupling_timer(Coupling_timer *existing_timer)
{
	frequency_count = existing_timer->frequency_count;
	lag_count = existing_timer->lag_count;
	strcpy(frequency_unit, existing_timer->frequency_unit);
	timer_id = -1;
}


Coupling_timer::Coupling_timer(int comp_id, int timer_id, Coupling_timer *existing_timer)
{
	frequency_count = existing_timer->frequency_count;
	lag_count = existing_timer->lag_count;
	strcpy(frequency_unit, existing_timer->frequency_unit);
	this->timer_id = timer_id;
	this->comp_id = comp_id;
	comp_time_mgr = components_time_mgrs->get_time_mgr(comp_id);
}


Coupling_timer::Coupling_timer(const char *array_buffer, int &buffer_content_iter, int comp_id)
{
	int num_children;

	
	read_data_from_array_buffer(frequency_unit, NAME_STR_SIZE, array_buffer, buffer_content_iter);
	read_data_from_array_buffer(&frequency_count, sizeof(int), array_buffer, buffer_content_iter);
	read_data_from_array_buffer(&lag_count, sizeof(int), array_buffer, buffer_content_iter);
	read_data_from_array_buffer(&num_children, sizeof(int), array_buffer, buffer_content_iter);
	comp_time_mgr = components_time_mgrs->get_time_mgr(comp_id);
	for (int i = 0; i < num_children; i ++)
		children.push_back(new Coupling_timer(array_buffer, buffer_content_iter, comp_id));
}


void Coupling_timer::write_timer_into_array(char **array_buffer, int &buffer_max_size, int &buffer_content_size)
{
	int num_children = children.size();
	for (int i = num_children-1; i >= 0; i --)
		children[i]->write_timer_into_array(array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&num_children, sizeof(int), array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&lag_count, sizeof(int), array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(&frequency_count, sizeof(int), array_buffer, buffer_max_size, buffer_content_size);
	write_data_into_array_buffer(frequency_unit, NAME_STR_SIZE, array_buffer, buffer_max_size, buffer_content_size);
}


bool Coupling_timer::is_timer_on(int current_year, int current_month, int current_day, int current_second, int current_num_elapsed_day,
	                             int start_year, int start_month, int start_day, int start_second, int start_num_elapsed_day)
{
	return common_is_timer_on(frequency_unit, frequency_count, lag_count, current_year,  
		                      current_month, current_day, current_second, current_num_elapsed_day,
	                          start_year, start_month, start_day, start_second, start_num_elapsed_day);
}


void Coupling_timer::get_time_of_next_timer_on(Time_mgt *time_mgr, int current_year, int current_month, int current_day, int current_second, int current_num_elapsed_days, int time_step_in_second, int &next_timer_num_elapsed_days, int &next_timer_second, bool advance)
{
	if (advance)
		time_mgr->advance_time(current_year, current_month, current_day, current_second, current_num_elapsed_days, time_step_in_second);
	while (!is_timer_on(current_year, current_month, current_day, current_second, current_num_elapsed_days, time_mgr->get_start_year(), 
		                time_mgr->get_start_month(), time_mgr->get_start_day(), time_mgr->get_start_second(), time_mgr->get_start_num_elapsed_day()))	
		time_mgr->advance_time(current_year, current_month, current_day, current_second, current_num_elapsed_days, time_step_in_second);

	next_timer_num_elapsed_days = current_num_elapsed_days;
	next_timer_second = current_second;
}



bool Coupling_timer::is_timer_on()
{
	if (children.size() == 0)
		return comp_time_mgr->is_timer_on(frequency_unit, frequency_count, lag_count);
	else if (or_or_and == 0) { // or
		for (int i = 0; i < children.size(); i ++)
			if (children[i]->is_timer_on())
				return true;
		return false;
	}
	else {  // and
		for (int i = 0; i < children.size(); i ++)
			if (!children[i]->is_timer_on())
				return false;
		return true;	
	}
}


bool Timer_mgt::check_is_legal_timer_id(int timer_id)
{
	if ((timer_id & TYPE_ID_PREFIX_MASK) != TYPE_TIMER_ID_PREFIX)
		return false;

	return (timer_id & TYPE_ID_SUFFIX_MASK) < timers.size();
}


Coupling_timer *Timer_mgt::get_timer(int timer_id)
{
	if (!check_is_legal_timer_id(timer_id))
		return NULL;

	return timers[timer_id&TYPE_ID_SUFFIX_MASK];
}


int Timer_mgt::define_timer(int comp_id, const char *freq_unit, int freq_count, int del_count, const char *annotation)
{
	timers.push_back(new Coupling_timer(comp_id, TYPE_TIMER_ID_PREFIX|timers.size(), freq_unit, freq_count, del_count, annotation));
	return timers[timers.size()-1]->get_timer_id();
}


int Timer_mgt::define_timer(int comp_id, int *timers_id, int num_timers, int or_or_and, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "The component id for defining a timer is wrong. Please check the model code with the annotation \"%s\"", annotation);
	timers.push_back(new Coupling_timer(comp_id, TYPE_TIMER_ID_PREFIX|timers.size(), timers_id, num_timers, or_or_and, annotation));
	return timers[timers.size()-1]->get_timer_id();
}


int Timer_mgt::define_timer(int comp_id, Coupling_timer *existing_timer)
{
	Coupling_timer *new_timer = new Coupling_timer(comp_id, TYPE_TIMER_ID_PREFIX|timers.size(), existing_timer);
	timers.push_back(new_timer);
	return new_timer->get_timer_id();
}


bool Timer_mgt::is_timer_on(int timer_id, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, -1, check_is_legal_timer_id(timer_id), "The timer id is wrong when checking whether a timer is on. Please check the model code with the annotation \"%s\"", annotation);
	return timers[timer_id&TYPE_ID_SUFFIX_MASK]->is_timer_on();
}


bool Time_mgt::check_is_time_legal(int year, int month, int day, int second, const char *report_label)
{
	if (report_label != NULL) {
		EXECUTION_REPORT(REPORT_ERROR,-1, year >= 0, "The time format is wrong: the year of simulation run can not be negative. Please check the model code with the annotation \"%s\"", report_label);
		EXECUTION_REPORT(REPORT_ERROR,-1, second >=0 && second <= SECONDS_PER_DAY, "The time format is wrong: the second of simulation run must between 0 and SECONDS_PER_DAY. Please check the model code with the annotation \"%s\"", report_label);
	   	EXECUTION_REPORT(REPORT_ERROR,-1, month >= 1 && month <= 12, "The time format is wrong: the month must be between 1 and 12. Please check the model code with the annotation \"%s\"", report_label);
		if (leap_year_on && is_a_leap_year(year))
        	EXECUTION_REPORT(REPORT_ERROR,-1, day >= 1 && day <= num_days_of_month_of_leap_year[month-1], "The time format is wrong: the day must be between 1 and %d. Please check the model code with the annotation \"%s\"", num_days_of_month_of_leap_year[month-1], report_label);
		else EXECUTION_REPORT(REPORT_ERROR,-1, day >= 1 && day <= num_days_of_month_of_nonleap_year[month-1], "The time format is wrong: the day must be between 1 and %d. Please check the model code with the annotation \"%s\"", num_days_of_month_of_nonleap_year[month-1], report_label);
		return true;
	}
	else {
		if (!(year >= 0) || !(second >=0 && second <= SECONDS_PER_DAY) || !(month >= 1 && month <= 12))
			return false;
		if (leap_year_on && is_a_leap_year(year)) {
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


Time_mgt::Time_mgt(int comp_id, int start_date, int start_second, int stop_date, int stop_second, int reference_date, bool leap_year_on, int cpl_step, const char *rest_freq_unit, int rest_freq_count, int stop_latency_seconds)
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
    time_step_in_second = cpl_step;
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
	this->comp_id = comp_id;
	this->advance_time_synchronized = false;

    EXECUTION_REPORT(REPORT_ERROR,-1, time_step_in_second>0 && (SECONDS_PER_DAY%time_step_in_second)==0, "The number of seconds per day is not a multiple of the number of seconds per step\n");
    EXECUTION_REPORT(REPORT_ERROR,-1, stop_latency_seconds%time_step_in_second == 0, "the latency seconds of stopping must be integer multiple of the number of seconds per step\n");
	check_is_time_legal(start_year, start_month, start_day, start_second, "start");
	check_is_time_legal(stop_year, stop_month, stop_day, stop_second, "stop");
//	check_is_time_legal(reference_year, reference_month, reference_day, 0, "feference");  This check is disabled due to MASNUM2

    steps_per_day = SECONDS_PER_DAY / time_step_in_second;
    current_num_elapsed_day = calculate_elapsed_day(start_year,start_month,start_day);
	start_num_elapsed_day = current_num_elapsed_day;
	stop_num_elapsed_day = calculate_elapsed_day(stop_year,stop_month,stop_day);
    num_total_steps = (stop_num_elapsed_day-current_num_elapsed_day)*steps_per_day + (stop_second-start_second)/time_step_in_second;
    EXECUTION_REPORT(REPORT_ERROR,-1, num_total_steps > 0, "the end simulation time must be after the start simulation time\n");

    timer_mgr = this;
    restart_timer = NULL;
    restart_timer = new Coupling_timer(rest_freq_unit, rest_freq_count, 0, "C-Coupler error");
    if (words_are_the_same(rest_freq_unit, FREQUENCY_UNIT_YEARS))
        rest_freq_seconds = NUM_DAYS_PER_NONLEAP_YEAR*SECONDS_PER_DAY*rest_freq_count;
    else if (words_are_the_same(rest_freq_unit, FREQUENCY_UNIT_MONTHS))
        rest_freq_seconds = 28*SECONDS_PER_DAY*rest_freq_count;
    else if (words_are_the_same(rest_freq_unit, FREQUENCY_UNIT_DAYS))
        rest_freq_seconds = SECONDS_PER_DAY*rest_freq_count;
    else rest_freq_seconds = rest_freq_count;
    EXECUTION_REPORT(REPORT_ERROR,-1, rest_freq_seconds > stop_latency_seconds,  "the time interval of restart writing must be larger than the delay of ending component\n");

	EXECUTION_REPORT(REPORT_ERROR,-1, check_time_consistency_between_components(get_start_full_time()), "the start date of all components are not consistent\n");
}


Time_mgt::Time_mgt(int comp_id, const char *XML_file_name)
{
	int line_number;

	
	time_step_in_second = -1;
	case_desc[0] = '\0';

	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "Software error in Time_mgt::Time_mgt: wrong component id");
	this->comp_id = comp_id;
	this->restart_timer = NULL;
	this->advance_time_synchronized = false;

	{
		int start_date, stop_date, reference_date, stop_n, rest_freq_count, time_step;
		long num_total_seconds;
		TiXmlDocument XML_file(XML_file_name);
		EXECUTION_REPORT(REPORT_ERROR, -1, XML_file.LoadFile(MPI_COMM_WORLD), "Fail to read XML file \"%s\" with the time information setting. The XML file may not exist or may not be a legal XML file. Please check.", XML_file_name);
		TiXmlElement *XML_element = XML_file.FirstChildElement();
		const char *case_name = XML_element->Attribute("case_name", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, case_name != NULL, "Case name is unset or the format of the XML file is wrong. Please check the XML file \"%s\"", XML_file_name);
		EXECUTION_REPORT(REPORT_ERROR, -1, strlen(case_name) < NAME_STR_SIZE, "The case name set in the XML file \"%s\" is too long. The limit is %d characters. Please check the XML file arround the line_number %d", XML_file_name, NAME_STR_SIZE-1, line_number);
		strcpy(this->case_name, case_name);
		const char *case_desc = XML_element->Attribute("case_description", &line_number);
		if (case_desc != NULL) {
			EXECUTION_REPORT(REPORT_ERROR, -1, strlen(case_desc) < NAME_STR_SIZE, "The description of the current simulation set in the XML file \"%s\" is too long. The limit is %d characters. Please check the XML file arround the line_number %d", XML_file_name, NAME_STR_SIZE-1, line_number);	
			strcpy(this->case_desc, case_desc);
		}
		EXECUTION_REPORT(REPORT_WARNING, -1, case_desc != NULL, "The description of the current simulation is unset or the format of the XML file is wrong. ");
		const char *run_type = XML_element->Attribute("run_type", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, run_type != NULL, "Run type is unset or the format of the XML file is wrong. Please check the XML file \"%s\"", XML_file_name);
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(run_type, "initial") || words_are_the_same(run_type, "continue") || words_are_the_same(run_type, "branch") || words_are_the_same(run_type, "hybrid"),
			             "Run type is wrong. It must be one of the four options: \"initial\", \"continue\", \"branch\" and \"hybrid\". Please check the XML file \"%s\"  arround the line_number %d", XML_file_name, line_number);
		strcpy(this->run_type, run_type);
		// to be added for how to settle the run type. 
		const char *leap_year_string = XML_element->Attribute("leap_year", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, leap_year_string != NULL, "Whether leap year is on or off is unset or the format of the XML file is wrong. Please check the XML file \"%s\"", XML_file_name);
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(leap_year_string, "on") || words_are_the_same(leap_year_string, "off"),
			             "The value of leap year wrong. Its value must be \"on\" or \"off\". Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
		if (words_are_the_same(leap_year_string, "on"))
			leap_year_on = true;
		else leap_year_on = false;
		const char *start_date_string = XML_element->Attribute("start_date", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, start_date_string != NULL, "The start date is unset or the format of the XML file is wrong. Please check the XML file \"%s\"", XML_file_name);
		sscanf(start_date_string, "%d", &start_date);
	    start_year = start_date / 10000;
    	start_month = (start_date%10000) / 100;
    	start_day = start_date % 100;
		EXECUTION_REPORT(REPORT_ERROR, -1, check_is_time_legal(start_year, start_month, start_day, 0, NULL), "The start date specified is a wrong date. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
		const char *start_second_string = XML_element->Attribute("start_second", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, start_second_string != NULL, "The start second is unset or the format of the XML file is wrong. Please check the XML file \"%s\"", XML_file_name);
		sscanf(start_second_string, "%d", &this->start_second);		
		EXECUTION_REPORT(REPORT_ERROR, -1, check_is_time_legal(start_year, start_month, start_day, start_second, NULL), "The start second specified is a wrong second number in a day. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
		current_num_elapsed_day = calculate_elapsed_day(start_year,start_month,start_day);
		current_year = start_year;
		current_month = start_month;
		current_day = start_day;
		current_second = start_second;
		const char *reference_date_string = XML_element->Attribute("reference_date", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, reference_date_string != NULL, "The reference date is unset or the format of the XML file is wrong. Please check the XML file \"%s\"", XML_file_name);
		if (reference_date_string != NULL) {
			sscanf(reference_date_string, "%d", &reference_date);
			reference_year = reference_date / 10000;
			reference_month = (reference_date%10000) / 100;
			reference_day = reference_date % 100;
			EXECUTION_REPORT(REPORT_ERROR, -1, check_is_time_legal(reference_year, reference_month, reference_day, 0, NULL), "The reference date specified is a wrong date. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);			
		}
		else reference_year = reference_month = reference_day = -1;
		const char *rest_freq_unit = XML_element->Attribute("rest_freq_unit", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, rest_freq_unit != NULL, "The time unit for the frequency of writing restart files (rest_freq_unit) is unset or the format of the XML file is wrong. Please check the XML file \"%s\"", XML_file_name);
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(rest_freq_unit, "none") || words_are_the_same(rest_freq_unit, "seconds")|| words_are_the_same(rest_freq_unit, "days") || words_are_the_same(rest_freq_unit, "months") || words_are_the_same(rest_freq_unit, "years"),
			             "The time unit for the frequency of writing restart files (rest_freq_unit) must be one of the following options: \"none\", \"seconds\", \"days\", \"months\", and \"years\". Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
		strcpy(this->rest_freq_unit, rest_freq_unit);
		this->rest_freq_count = 0;
		if (!words_are_the_same(rest_freq_unit, "none")) {
			const char *rest_freq_count_string = XML_element->Attribute("rest_freq_count", &line_number);			
			EXECUTION_REPORT(REPORT_ERROR, -1, rest_freq_count_string != NULL, "The count of time unit for the frequency of writing restart files (rest_freq_count) is unset or the format of the XML file is wrong. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);			
			sscanf(rest_freq_count_string, "%d", &rest_freq_count);
			EXECUTION_REPORT(REPORT_ERROR, -1, rest_freq_count > 0, "The count of time unit for the frequency of writing restart files (rest_freq_count) must be a possitive value. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
			this->rest_freq_count = rest_freq_count;
		}
		const char *stop_option = XML_element->Attribute("stop_option", &line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, stop_option != NULL, "The stop option is unset or the format of the XML file is wrong. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
		EXECUTION_REPORT(REPORT_ERROR, -1, words_are_the_same(stop_option, "date") || words_are_the_same(stop_option, "nseconds") || words_are_the_same(stop_option, "nminutes") || words_are_the_same(stop_option, "nhours") || words_are_the_same(stop_option, "ndays") || words_are_the_same(stop_option, "nmonths") || words_are_the_same(stop_option, "nyears"),
			             "Stop option is wrong. It must be one of the following options: \"date\", \"nseconds\", \"nminutes\", \"nhours\", \"ndays\", \"nmonths\" and \"nyears\". Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
		strcpy(this->stop_option, stop_option);
		if (words_are_the_same(stop_option, "date")) {
			const char *stop_date_string = XML_element->Attribute("stop_date", &line_number);
			EXECUTION_REPORT(REPORT_ERROR, -1, stop_date_string != NULL, "The stop date is unset or the format of the XML file is wrong when the stop option is \"date\". Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);			
			sscanf(stop_date_string, "%d", &stop_date);
			stop_year = stop_date / 10000;
			stop_month = (stop_date%10000) / 100;
			stop_day = stop_date % 100;
			EXECUTION_REPORT(REPORT_ERROR, -1, check_is_time_legal(stop_year, stop_month, stop_day, 0, NULL), "The stop date specified is a wrong date. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
			const char *stop_second_string = XML_element->Attribute("stop_second", &line_number);
			EXECUTION_REPORT(REPORT_ERROR, -1, stop_second_string != NULL, "The stop second is unset or the format of the XML file is wrong when the stop option is \"date\". Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
			sscanf(stop_second_string, "%d", &this->stop_second); 	
			EXECUTION_REPORT(REPORT_ERROR, -1, check_is_time_legal(stop_year, stop_month, stop_day, stop_second, NULL), "The stop second specified is a wrong second number in a day. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
			num_total_seconds = (calculate_elapsed_day(stop_year,stop_month,stop_day)-current_num_elapsed_day)*((long)SECONDS_PER_DAY) + stop_second-start_second;
			EXECUTION_REPORT(REPORT_ERROR,-1, num_total_seconds > 0, "The stop time of simulation is wrong. It must be after the start time. Please check the XML file \"%s\" arround the line_number %d", XML_file_name, line_number);
		}
		else {
			const char *stop_n_string = XML_element->Attribute("stop_n", &line_number);
			EXECUTION_REPORT(REPORT_ERROR, -1, stop_n_string != NULL, "The parameter \"stop_n\" is unset or the format of the XML file is wrong when the stop option is \"%s\". Please check the XML file \"%s\" arround the line_number %d", stop_option, XML_file_name, line_number);
			sscanf(stop_n_string, "%d", &stop_n);
			if (stop_n == -999 || stop_n <= 0)
				stop_year = stop_month = stop_day = stop_second = -1;
			else {
				if (words_are_the_same(stop_option, "nyears")) {
					stop_year = start_year + stop_n;
					stop_month = start_month;
					stop_day = start_day;
					stop_second = start_second;
					if (start_month == 2 && start_day == 29 && !is_a_leap_year(stop_year))
						stop_day = 28;
				}
				else if (words_are_the_same(stop_option, "nmonths")) {
					stop_year = start_year + stop_n/12;
					if (start_month + (stop_n%12) > 12) {
						stop_year ++;
						stop_month = (start_month + (stop_n%12)) - 12;
					}
					else stop_month = (start_month + (stop_n%12));
					stop_day = start_day;
					stop_second = start_second;
					if (is_a_leap_year(stop_year) && num_days_of_month_of_leap_year[stop_month-1] < stop_day)
						stop_day = num_days_of_month_of_leap_year[stop_month-1];
					if (!is_a_leap_year(stop_year) && num_days_of_month_of_nonleap_year[stop_month-1] < stop_day)
						stop_day = num_days_of_month_of_nonleap_year[stop_month-1];					
				}
				else {
					int num_days = 0, num_hours = 0, num_minutes = 0, num_seconds = 0;
					if (words_are_the_same(stop_option, "ndays")) {
						num_days = stop_n;
						num_total_seconds = stop_n * SECONDS_PER_DAY;
					}
					else if (words_are_the_same(stop_option, "nhours")) {
						num_days = stop_n/24;
						num_hours = stop_n % 24;
						num_total_seconds = stop_n * 3600;
					}
					else if (words_are_the_same(stop_option, "nminutes")) {
						num_days = stop_n / 1440;
						num_hours = (stop_n % 1440) / 60;
						num_minutes = stop_n % 60;
						num_total_seconds = stop_n * 60;
					}
					else {
						num_days = stop_n / SECONDS_PER_DAY;
						num_hours = (stop_n % SECONDS_PER_DAY) / 3600;
						num_minutes = (stop_n % 3600) / 60;
						num_seconds = stop_n % 60;
						num_total_seconds = stop_n;
					}
					this->stop_year = -1;
					this->stop_month = -1;
					this->stop_day = -1;
					this->stop_second = -1;
					Time_mgt *cloned_time_mgr = clone_time_mgr(comp_id);
					cloned_time_mgr->set_time_step_in_second(SECONDS_PER_DAY, "C-Coupler creates the time manager of a component");
					for (int i = 0; i < num_days; i ++)
						cloned_time_mgr->advance_model_time("in Time_mgt(...)", false);
					cloned_time_mgr->set_time_step_in_second(3600, "C-Coupler creates the time manager of a component");
					for (int i = 0; i < num_hours; i ++)
						cloned_time_mgr->advance_model_time("in Time_mgt(...)", false);
					cloned_time_mgr->set_time_step_in_second(60, "C-Coupler creates the time manager of a component");
					for (int i = 0; i < num_minutes; i ++)
						cloned_time_mgr->advance_model_time("in Time_mgt(...)", false);
					cloned_time_mgr->set_time_step_in_second(1, "C-Coupler creates the time manager of a component");
					for (int i = 0; i < num_seconds; i ++)
						cloned_time_mgr->advance_model_time("in Time_mgt(...)", false);
					this->stop_year = cloned_time_mgr->current_year;
					this->stop_month = cloned_time_mgr->current_month;
					this->stop_day = cloned_time_mgr->current_day;
					this->stop_second = cloned_time_mgr->current_second;
					delete cloned_time_mgr;
					EXECUTION_REPORT(REPORT_ERROR, -1, num_total_seconds == (calculate_elapsed_day(stop_year,stop_month,stop_day)-current_num_elapsed_day)*((long)SECONDS_PER_DAY) + stop_second-start_second,
					                 "Software error in Time_mgt::Time_mgt: fail to caculate stop time according to stop_n");
				}
			}
			EXECUTION_REPORT(REPORT_PROGRESS, -1, true, "The stop time generated by C-Coupler according to the users' specification is %04d%02d%02d-%05d", stop_year, stop_month, stop_day, stop_second);
			EXECUTION_REPORT(REPORT_ERROR, -1, check_is_time_legal(stop_year,stop_month,stop_day,stop_second,"Software error in Time_mgt::Time_mgt: wrong stop time generated by C-Coupler."));
		}
	}

    previous_year = start_year;
    previous_month = start_month;
    previous_day = start_day;
    previous_second = start_second;
    current_year = start_year;
    current_month = start_month;
    current_day = start_day;
    current_second = start_second;
	current_num_elapsed_day = calculate_elapsed_day(start_year,start_month,start_day);
	start_num_elapsed_day = current_num_elapsed_day;
	stop_num_elapsed_day = calculate_elapsed_day(stop_year,stop_month,stop_day);
	current_step_id = 0;
	num_total_steps = -1;
}


void Time_mgt::build_restart_timer()
{
	if (!words_are_the_same(rest_freq_unit, "none"))
		restart_timer = timer_mgr2->get_timer(timer_mgr2->define_timer(comp_id, rest_freq_unit, rest_freq_count, 0, "C-Coupler define restart timer"));
	printf("information for building restart timer is %s %d\n", rest_freq_unit, rest_freq_count);
}


Time_mgt::~Time_mgt()
{
    delete restart_timer;
}


void Time_mgt::reset_timer()
{
	EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial"), 
		             "the model timer cannot be reset when run type is not initial\n");

	current_year = start_year;
	current_month = start_month;
	current_day = start_day;
	current_second = start_second;
	current_step_id = 0;
	
	current_num_elapsed_day = calculate_elapsed_day(start_year,start_month,start_day);
	start_num_elapsed_day = current_num_elapsed_day;
	stop_num_elapsed_day = calculate_elapsed_day(stop_year,stop_month,stop_day);
}


int Time_mgt::get_current_num_days_in_year()
{
	if (leap_year_on && is_a_leap_year(current_year))
		return elapsed_days_on_start_of_month_of_leap_year[current_month-1] + current_day;
	return elapsed_days_on_start_of_month_of_nonleap_year[current_month-1] + current_day;
}


long Time_mgt::calculate_elapsed_day(int year, int month, int day)
{
	int num_leap_year;


	check_is_time_legal(year, month, day, 0, "(at calculate_elapsed_day)");

	if (!leap_year_on)
	    return year*NUM_DAYS_PER_NONLEAP_YEAR + elapsed_days_on_start_of_month_of_nonleap_year[month-1] + day - 1;

	num_leap_year = (year-1)/4 - (year-1)/100 + (year-1)/400;

	if (year > 0)
		num_leap_year ++;   // year 0 is a leap year

	if (is_a_leap_year(year))
		return year*NUM_DAYS_PER_NONLEAP_YEAR + num_leap_year + elapsed_days_on_start_of_month_of_leap_year[month-1] + day - 1;

	return year*NUM_DAYS_PER_NONLEAP_YEAR + num_leap_year + elapsed_days_on_start_of_month_of_nonleap_year[month-1] + day - 1;
}


void Time_mgt::advance_time(int &current_year, int &current_month, int &current_day, int &current_second, int &current_num_elapsed_day, int time_step_in_second)
{
    int i, num_days_in_current_month;


    current_second += time_step_in_second;
	for (i = 0; i < current_second / SECONDS_PER_DAY; i ++) {
        current_num_elapsed_day ++;
		if (leap_year_on && is_a_leap_year(current_year)) 
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
	current_second = current_second % SECONDS_PER_DAY;	
}


void Time_mgt::advance_model_time(const char *annotation, bool from_external_model)
{
    int i, num_days_in_current_month;
 

	EXECUTION_REPORT(REPORT_ERROR, comp_id, time_step_in_second != -1, "Cannot advance the time of the component \"\%s\" at the model code with the annotation \"%s\", because the time step has not been specified.", 
					 comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "")->get_comp_name(), annotation);
	if (from_external_model && !advance_time_synchronized) {		
		synchronize_comp_processes_for_API(comp_id, API_ID_TIME_MGT_ADVANCE_TIME, comp_comm_group_mgt_mgr->get_comm_group_of_local_comp(comp_id, "C-Coupler code in Time_mgt::advance_model_time"), "advance the time of a component", annotation);
		advance_time_synchronized = true;
	}

	previous_year = current_year;
	previous_month = current_month;
	previous_day = current_day;
	previous_second = current_second;
    current_step_id ++;

	advance_time(current_year, current_month, current_day, current_second, current_num_elapsed_day, time_step_in_second);
	printf("current time is %d   %d   %d   %d   %d  %d\n", current_year, current_month, current_day, current_second, current_num_elapsed_day, current_step_id);
}


double Time_mgt::get_double_current_calendar_time(int shift_second)
{
	double calday;

	
	EXECUTION_REPORT(REPORT_ERROR,-1, shift_second>=-SECONDS_PER_DAY && shift_second<= SECONDS_PER_DAY, "The shift seconds for calculating calendar time must be between -SECONDS_PER_DAY and SECONDS_PER_DAY");

	if (leap_year_on && is_a_leap_year(current_year)) {
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


float Time_mgt::get_float_current_calendar_time(int shift_second)
{
    return (float) get_double_current_calendar_time(shift_second);
}


bool Time_mgt::is_timer_on(const char *frequency_unit, int frequency_count, int lag_count)
{
    long num_elapsed_time;


   	return common_is_timer_on(frequency_unit, frequency_count, lag_count, current_year,  
		                      current_month, current_day, current_second, current_num_elapsed_day,
	                          start_year, start_month, start_day, start_second, start_num_elapsed_day);
}


void Time_mgt::set_restart_time(long start_full_time, long restart_full_time)
{
    long restart_date_value;
    long tmp_date_value;


	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "restart"))
	    EXECUTION_REPORT(REPORT_ERROR,-1, get_start_full_time() == start_full_time, "the start time read from restart file is different from current setting\n");
	EXECUTION_REPORT(REPORT_ERROR,-1, timer_mgr->check_time_consistency_between_components(start_full_time), "the start date of all components in the restart run (restart) is different\n");
	EXECUTION_REPORT(REPORT_ERROR,-1, timer_mgr->check_time_consistency_between_components(restart_full_time), "the restart date of all components in the restart run (restart) is different\n");
    current_second = restart_full_time % 100000;
    current_day = (restart_full_time/100000)%100;
    current_month = (restart_full_time/10000000)%100;
    current_year = restart_full_time/1000000000;
    current_num_elapsed_day = calculate_elapsed_day(current_year,current_month,current_day);
	current_step_id = (current_num_elapsed_day-start_num_elapsed_day) * (SECONDS_PER_DAY/time_step_in_second);
	EXECUTION_REPORT(REPORT_LOG,-1, true, "current step id from restart is %d", current_step_id);
}


bool Time_mgt::check_is_model_run_finished()
{
    EXECUTION_REPORT(REPORT_LOG,-1, true, "check_is_model_run_finished %d %ld", current_step_id, num_total_steps);
	if (num_total_steps == -1)
		return false;
    return current_step_id > num_total_steps;
}


bool Time_mgt::check_is_coupled_run_restart_time()
{
    return restart_timer->is_timer_on();
}


long Time_mgt::get_start_full_time()
{
    return (long)start_second + (long)start_day*100000 + (long)start_month*10000000 + (long)start_year*1000000000;
}



long Time_mgt::get_previous_full_time()
{
    return (long)previous_second + (long)previous_day*100000 + (long)previous_month*10000000 + (long)previous_year*1000000000;
}


long Time_mgt::get_current_full_time()
{
    return (long)current_second + (long)current_day*100000 + (long)current_month*10000000 + (long)current_year*1000000000;
}


int Time_mgt::get_current_date()
{
    return (int) (current_year*10000 + current_month*100 + current_day);
}


void Time_mgt::check_timer_format(const char *frequency_unit, int frequency_count, int lag_count, const char *annotation)
{
	if (time_step_in_second > 0) {
	    EXECUTION_REPORT(REPORT_ERROR, comp_id, words_are_the_same(frequency_unit, FREQUENCY_UNIT_STEPS) || words_are_the_same(frequency_unit, FREQUENCY_UNIT_SECONDS) || words_are_the_same(frequency_unit, FREQUENCY_UNIT_DAYS) ||
	                 words_are_the_same(frequency_unit, FREQUENCY_UNIT_MONTHS) || words_are_the_same(frequency_unit, FREQUENCY_UNIT_YEARS), 
	                 "The frequency unit in timer must be one of \"steps\", \"seconds\", \"days\", \"months\" and \"years\". Please check the model code with the annotation \"%s\"", annotation);
	    EXECUTION_REPORT(REPORT_ERROR, comp_id, frequency_count > 0, "The frquency count in timer must be larger than 0. Please check the model code with the annotation \"%s\"", annotation);
	    if (words_are_the_same(frequency_unit, FREQUENCY_UNIT_SECONDS)) {
	        EXECUTION_REPORT(REPORT_ERROR, comp_id, frequency_count%time_step_in_second == 0, "The frequency count in timer must be a multiple of the time step of the component when the frequency unit is \"seconds\". Please check the model code with the annotation \"%s\"", annotation);
	        EXECUTION_REPORT(REPORT_ERROR, comp_id, lag_count%time_step_in_second == 0, "The lag count in a timer must be a multiple of the time step of the component when the frequency unit is \"seconds\". Please check the model code with the annotation \"%s\"", annotation);        
	    }
		if (lag_count != 0)
			EXECUTION_REPORT(REPORT_ERROR, comp_id, !words_are_the_same(frequency_unit, FREQUENCY_UNIT_MONTHS) && !words_are_the_same(frequency_unit, FREQUENCY_UNIT_YEARS), "The lag count cannot be set when the frequency unit of a timer is \"%s\". Please check the model code with the annotation \"%s\"", frequency_unit, annotation);
	}
}


Comps_transfer_time_info *Time_mgt::allocate_comp_transfer_time_info(int remote_comp_id)
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


bool Time_mgt::check_time_consistency_between_components(long full_time)
{
    int i, num_global_procs;
	bool consistent;
	long *full_time_arrays;


	if (compset_communicators_info_mgr->get_num_components() == 1)
		return true;
	
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Comm_size(compset_communicators_info_mgr->get_global_comm_group(), &num_global_procs) == MPI_SUCCESS);
	
	consistent = true;
	full_time_arrays = new long [num_global_procs];
	EXECUTION_REPORT(REPORT_ERROR,-1, MPI_Allgather(&full_time, 1, MPI_LONG, full_time_arrays, 1, MPI_LONG, compset_communicators_info_mgr->get_global_comm_group()) == MPI_SUCCESS);
	for (i = 1; i < num_global_procs; i ++)
		if (full_time_arrays[i] != full_time_arrays[i-1])
			consistent = false;

    delete [] full_time_arrays;

	return consistent;
}


void Time_mgt::get_elapsed_days_from_start_date(int *num_days, int *num_seconds)
{
	long current_num_elapsed_days, start_num_elapsed_days;
	
	current_num_elapsed_days = calculate_elapsed_day(current_year, current_month, current_day);
	start_num_elapsed_days = calculate_elapsed_day(start_year, start_month, start_day);
	*num_days = current_num_elapsed_days - start_num_elapsed_days;
	*num_seconds = current_second;
}


void Time_mgt::get_elapsed_days_from_reference_date(int *num_days, int *num_seconds)
{
	long current_num_elapsed_days, reference_num_elapsed_days;
	
	current_num_elapsed_days = calculate_elapsed_day(current_year, current_month, current_day);
	reference_num_elapsed_days = calculate_elapsed_day(reference_year, reference_month, reference_day);
	*num_days = current_num_elapsed_days - reference_num_elapsed_days;
	*num_seconds = current_second;
}


void Time_mgt::get_current_time(int &year, int &month, int &day, int &second, int shift_second)
{
	int num_days_in_current_month;

	
	EXECUTION_REPORT(REPORT_ERROR,-1, shift_second>=-SECONDS_PER_DAY && shift_second<= SECONDS_PER_DAY, "The shift seconds for calculating calendar time must be between -SECONDS_PER_DAY and SECONDS_PER_DAY");
	
	year = current_year;
	month = current_month;
	day = current_day;
	second = current_second + shift_second;

    if (second >= SECONDS_PER_DAY) {
		second -= SECONDS_PER_DAY;
		if (leap_year_on && is_a_leap_year(year)) 
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


int Time_mgt::get_current_num_time_step()
{
	if (words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "hybrid") && current_step_id < SECONDS_PER_DAY/time_step_in_second)
		return current_step_id + SECONDS_PER_DAY/time_step_in_second;
    return current_step_id; 
}


Time_mgt *Time_mgt::clone_time_mgr(int comp_id)
{
	Time_mgt *new_time_mgr = new Time_mgt();


	new_time_mgr->start_year = this->start_year;
	new_time_mgr->start_month = this->start_month;
	new_time_mgr->start_day = this->start_day;
	new_time_mgr->start_second = this->start_second;
	new_time_mgr->previous_year = this->previous_year;
	new_time_mgr->previous_month = this->previous_month;
	new_time_mgr->previous_day = this->previous_day;
	new_time_mgr->previous_second = this->previous_second;
	new_time_mgr->current_year = this->current_year;
	new_time_mgr->current_month = this->current_month;
	new_time_mgr->current_day = this->current_day;
	new_time_mgr->current_second = this->current_second;
	new_time_mgr->reference_year = this->reference_year;
	new_time_mgr->reference_month = this->reference_month;
	new_time_mgr->reference_day = this->reference_day;
	new_time_mgr->stop_year = this->stop_year;
	new_time_mgr->stop_month = this->stop_month;
	new_time_mgr->stop_day = this->stop_day;
	new_time_mgr->stop_second = this->stop_second;
	new_time_mgr->time_step_in_second = -1;
	new_time_mgr->current_step_id = 0;
	new_time_mgr->num_total_steps = 0;
	new_time_mgr->stop_latency_seconds = this->stop_latency_seconds;
	new_time_mgr->leap_year_on = this->leap_year_on;
	new_time_mgr->comp_id = comp_id;
	new_time_mgr->current_num_elapsed_day = this->current_num_elapsed_day;
	new_time_mgr->start_num_elapsed_day = this->start_num_elapsed_day;
	new_time_mgr->stop_num_elapsed_day = this->stop_num_elapsed_day;
	new_time_mgr->advance_time_synchronized = false;
	strcpy(new_time_mgr->case_name, this->case_name);
	strcpy(new_time_mgr->case_desc, this->case_desc);
	strcpy(new_time_mgr->run_type, this->run_type);
	strcpy(new_time_mgr->stop_option, this->stop_option);
	strcpy(new_time_mgr->rest_freq_unit, this->rest_freq_unit);
	new_time_mgr->rest_freq_count = this->rest_freq_count;
	new_time_mgr->restart_timer = NULL;

	return new_time_mgr;
}


void Time_mgt::set_time_step_in_second(int time_step_in_second, const char *annotation)
{
	this->time_step_in_second = time_step_in_second;
	EXECUTION_REPORT(REPORT_ERROR, comp_id, time_step_in_second > 0, "The value of the time step is wrong when setting the time step of the component \"%s\". It must be a positive value. Please check the model code with the annotation \"%s\"",
					 comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "get comp name in Time_mgt::set_time_step_in_second")->get_comp_name(), annotation);
	if (stop_year != -1) {
		long total_seconds = (stop_num_elapsed_day-current_num_elapsed_day)*((long)SECONDS_PER_DAY) + stop_second-start_second;
		printf("stop time is %d %d %d %d: %d %d %d %d: %ld : %d VS %d %d\n", stop_year, stop_month, stop_day, stop_second, start_year, start_month, start_day, start_second, total_seconds, current_num_elapsed_day, start_num_elapsed_day, stop_num_elapsed_day);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, total_seconds%((long)time_step_in_second) == 0, "The time step set at model code with the annotation \"%s\" does not match the start time and the stop time of the simulation. Please check the model code and the XML file \"env_run.xml\"", annotation);
		num_total_steps = total_seconds / time_step_in_second;
	}
	else num_total_steps = -1;
	if (restart_timer != NULL) {
		long rest_freq;
		if (words_are_the_same(rest_freq_unit, "days"))
			rest_freq = SECONDS_PER_DAY * rest_freq_count;
		else if (words_are_the_same(rest_freq_unit, "months") || words_are_the_same(rest_freq_unit, "years"))
			rest_freq = SECONDS_PER_DAY;
		else if (words_are_the_same(rest_freq_unit, "seconds"))
			rest_freq = rest_freq_count;
		printf("qiguai %d vs %d: %s %d\n", rest_freq, time_step_in_second, rest_freq_unit, rest_freq_count);
		EXECUTION_REPORT(REPORT_ERROR, comp_id, rest_freq%((long)time_step_in_second) == 0, "The time step set at model code with the annotation \"%s\" does not match the frequency of writing restart data files. Please check the model code and the XML file \"env_run.xml\"", annotation);
	}	
}


bool Time_mgt::is_a_leap_year(int year)
{
	return ((year%4) == 0 && (year%100) != 0) || (year%400) == 0;
}


void Time_mgt::check_consistency_of_current_time(int date, int second, const char *annotation)
{
	EXECUTION_REPORT(REPORT_ERROR, comp_id, date==get_current_date() && second == get_current_second(), "the model time is different from the time managed by the C-Coupler. Please verify the model code according to the annotation \"%s\"", annotation);
}


bool Time_mgt::is_time_out_of_execution(long another_time)
{
	return another_time > ((long)stop_num_elapsed_day)*100000+stop_second;
}


Time_mgt *Components_time_mgt::get_time_mgr(int comp_id)
{
	if (!comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id))
		return NULL;

	if (comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, "C-Coupler native code get time manager")->get_current_proc_local_id() == -1)
		return NULL;

	for (int i = 0; i < components_time_mgrs.size(); i++)
		if (components_time_mgrs[i]->get_comp_id() == comp_id)
			return components_time_mgrs[i];

	return NULL;
}


void Components_time_mgt::define_root_comp_time_mgr(int comp_id, const char *xml_file_name)
{
	components_time_mgrs.push_back(new Time_mgt(comp_id, xml_file_name));
	components_time_mgrs[components_time_mgrs.size()-1]->build_restart_timer();
}



void Components_time_mgt::set_component_time_step(int comp_id, int time_step, const char *annotation)
{
	Time_mgt *time_mgr = get_time_mgr(comp_id);
	if (comp_id, time_mgr->get_time_step_in_second() != -1)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, true, "The time step of the component \"%s\" has been set before (the corresponding model code annotation is \"%s\"). It cannot be set again at the model code with the annotation \"%s\"",
						 comp_comm_group_mgt_mgr->get_global_node_of_local_comp(comp_id, annotation)->get_comp_name(), annotation_mgr->get_annotation(comp_id, "setting time step"), annotation);
	annotation_mgr->add_annotation(comp_id, "setting time step", annotation);
	if (comp_comm_group_mgt_mgr->get_current_proc_id_in_comp(comp_id, "get the local id of the current component in Components_time_mgt::set_component_time_step") == 0)
		EXECUTION_REPORT(REPORT_ERROR, comp_id, time_step > 0, "The value of time step is wrong. It must be a positive value. Please check the model code with the annotation \"%s\"", annotation);
	get_time_mgr(comp_id)->set_time_step_in_second(time_step, annotation);
}


void Components_time_mgt::clone_parent_comp_time_mgr(int comp_id, int parent_comp_id, const char *annotation)
{
	Time_mgt *parent_time_mgr = get_time_mgr(parent_comp_id);
	Time_mgt *new_time_mgr;


	EXECUTION_REPORT(REPORT_ERROR, -1, comp_comm_group_mgt_mgr->is_legal_local_comp_id(comp_id), "Software error in Components_time_mgt::clone_parent_comp_time_mgr: wrong comp_id");
	EXECUTION_REPORT(REPORT_ERROR, comp_id, parent_time_mgr != NULL, "Software error in Components_time_mgt::clone_parent_comp_time_mgr: parent time manager is NULL");
	new_time_mgr = parent_time_mgr->clone_time_mgr(comp_id);
	components_time_mgrs.push_back(new_time_mgr);
	new_time_mgr->build_restart_timer();
}


void Components_time_mgt::advance_component_time(int comp_id, const char *annotation)
{
	Time_mgt *time_mgr = get_time_mgr(comp_id);
	time_mgr->advance_model_time(annotation, true);
    EXECUTION_REPORT(REPORT_LOG, comp_id, true, "The current time is %08d-%05d, and the current number of the time step is %d", time_mgr->get_current_date(), time_mgr->get_current_second(), time_mgr->get_current_step_id());
}


void Components_time_mgt::check_component_current_time(int comp_id, int date, int second, const char *annotation)
{
	Time_mgt *comp_time_mgr = get_time_mgr(comp_id);
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_time_mgr != NULL, "The parameter of component id for checking the current time is wrong. Please check the model code with the annotation of \"%s\"", annotation);
	comp_time_mgr->check_consistency_of_current_time(date, second, annotation);
}


bool Components_time_mgt::is_model_run_ended(int comp_id, const char *annotation)
{
	Time_mgt *comp_time_mgr = get_time_mgr(comp_id);
	EXECUTION_REPORT(REPORT_ERROR, -1, comp_time_mgr != NULL, "The parameter of component id for checking the current time is wrong. Please check the model code with the annotation of \"%s\"", annotation);
	return comp_time_mgr->check_is_model_run_finished();
}

