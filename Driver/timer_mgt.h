/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef COUPLING_TIME_MGT
#define COUPLING_TIME_MGT

#define FREQUENCY_UNIT_SECONDS          "seconds"
#define FREQUENCY_UNIT_DAYS             "days"
#define FREQUENCY_UNIT_MONTHS           "months"
#define FREQUENCY_UNIT_YEARS            "years"


#include "common_utils.h"
#include <vector>


class Timer_mgt;


struct Comps_transfer_time_info
{
    int remote_comp_id;
    int remote_comp_frequency;
    long remote_comp_time;
    long local_comp_time;
    int counter;
};


class Coupling_timer
{
    private:
        friend class Timer_mgt;
        char frequency_unit[NAME_STR_SIZE];
        int frequency_count;
        int delay_count;
        
    public:
        Coupling_timer(const char*, int, int, const char*);
        Coupling_timer(char**, const char*);
		Coupling_timer(Coupling_timer*);
        ~Coupling_timer() {}
        bool is_timer_on();
};


class Timer_mgt
{
    private:
        int start_year;
        int start_month;
        int start_day;
        int start_second;
        int previous_year;   
        int previous_month;
        int previous_day; 
        int previous_second; 
        int current_year;   
        int current_month;
        int current_day; 
        int current_second;
		int reference_year;
		int reference_month;
		int reference_day;
        int stop_year;
        int stop_month;
        int stop_day;
        int stop_second;
        int stop_latency_seconds;
        int sec_per_step; 
        int current_num_elapsed_day;
        int current_step_id;
        long num_total_steps;
        bool leap_year_on;
        Coupling_timer *restart_timer;
        std::vector<Comps_transfer_time_info*> comps_transfer_time_infos;

    public:
        Timer_mgt(int, int, int, int, int, bool, int, const char*, int, int);
        ~Timer_mgt();
        void advance_coupling_step();
        int get_current_year() { return current_year; }
        int get_current_month() { return current_month; }
        int get_current_day() { return current_day; }
        int get_current_hour() { return current_second /3600; }
        int get_current_minute() { return (current_second % 3600) / 60; }
        int get_current_second() { return current_second; }
        int get_comp_frequency() { return sec_per_step; }
		int get_stop_year() { return stop_year; }
		int get_stop_month() { return stop_month; }
		int get_stop_day() { return stop_day; }
		int get_stop_second() { return stop_second; }
        void set_restart_time(long, long);
        bool is_timer_on(char *, int, int);
        bool check_is_coupled_run_finished();
        bool check_is_coupled_run_restart_time();
        double get_double_current_calendar_time(int);
        float get_float_current_calendar_time(int);
        long get_start_full_time();
        long get_previous_full_time();
        long get_current_full_time();
        int get_current_date();
        int get_current_num_time_step();
        long get_num_total_step() { return num_total_steps; }
        int get_comp_stop_latency_seconds() { return stop_latency_seconds; }
		int get_current_num_days_in_year();
        void check_timer_format(const char*, int, int, const char*);
        Comps_transfer_time_info *allocate_comp_transfer_time_info(int);
		bool check_time_consistency_between_components(long);
        long calculate_elapsed_day(int, int, int);
		void get_elapsed_days_from_start_date(int*, int*);
		void get_elapsed_days_from_reference_date(int*, int*);
		void get_current_time(int&, int&, int&, int&, int);
		void reset_timer();
		bool check_is_time_legal(int, int, int, int, const char*);
		bool get_is_leap_year_on() { return leap_year_on; }
};


extern int num_days_of_month_of_nonleap_year[];
extern int num_days_of_month_of_leap_year[];


#endif
