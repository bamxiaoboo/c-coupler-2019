/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef RUNTIME_PROCESS_MGT
#define RUNTIME_PROCESS_MGT


#include "common_utils.h" 
#include "Runtime_Algorithm_Basis.h"
#include "external_algorithm_mgt.h"
#include <vector>


struct Runtime_algorithm_mgt
{
    char algorithm_cfg_name[NAME_STR_SIZE];
   	char algorithm_type[NAME_STR_SIZE];
    Runtime_algorithm_basis *runtime_algorithm_object;
};


class Runtime_procedure_mgt
{
	private: 
	    char procedure_name[NAME_STR_SIZE];
		std::vector<Runtime_algorithm_mgt*> runtime_algorithms;
		bool has_executed;

	public: 
		Runtime_procedure_mgt(const char*);
		~Runtime_procedure_mgt() {}
		bool match(const char*);
		void add_runtime_algorithm(Runtime_algorithm_mgt*);
		void execute(const char*);
		void write_restart_fields();
};


class Runtime_process_mgt
{
    private:
        std::vector<Runtime_algorithm_mgt*> runtime_algorithms;
        std::vector<Runtime_procedure_mgt*> runtime_procedures;

        void add_runtime_algorithm(const char*, const char*);
		Runtime_procedure_mgt *search_coupling_procedure(const char*);

    public:
        Runtime_process_mgt() {}
        ~Runtime_process_mgt();
        void add_runtime_algorithms(const char*);
        void add_runtime_procedures(const char*);
        void execute_coupling_procedure(const char*, const char*);
        void write_restart_fields();
		void register_model_algorithm(const char*, Model_algorithm);
};


#endif

