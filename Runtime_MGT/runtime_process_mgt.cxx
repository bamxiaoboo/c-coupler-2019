/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "runtime_transfer_algorithm.h"
#include "runtime_process_mgt.h"
#include "runtime_remap_algorithm.h"
#include "runtime_common_algorithm.h"
#include "runtime_datamodel_algorithm.h"
#include "runtime_cumulate_average_algorithm.h"
#include "runtime_merge_algorithm.h"
#include "global_data.h"
#include <stdio.h>


#define PROCEDURE_STAGE_INITIALIZE                  "initialize"
#define PROCEDURE_STAGE_FINALIZE                    "finalize"
#define PROCEDURE_STAGE_KERNEL                      "kernel"


Runtime_procedure_mgt::Runtime_procedure_mgt(const char *procedure_name)
{
	strcpy(this->procedure_name, procedure_name);
	has_executed = false;
}


void Runtime_procedure_mgt::add_runtime_algorithm(Runtime_algorithm_mgt *runtime_algorithm)
{
	runtime_algorithms.push_back(runtime_algorithm);
}


void Runtime_procedure_mgt::execute(const char *procedure_stage)
{
	EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(procedure_stage, PROCEDURE_STAGE_INITIALIZE) || words_are_the_same(procedure_stage, PROCEDURE_STAGE_KERNEL) ||
				 words_are_the_same(procedure_stage, PROCEDURE_STAGE_FINALIZE), "the stage mark of procedure %s must be one of initialize, kernel and finalize", procedure_name);

	if (runtime_algorithms.size() == 0)
		return;

	if (!words_are_the_same(compset_communicators_info_mgr->get_running_case_mode(), "initial") && words_are_the_same(procedure_stage, PROCEDURE_STAGE_INITIALIZE)) {
		EXECUTION_REPORT(REPORT_LOG, true, "bypass all algorithms except model algorithms in initialize stage %s in restart run", procedure_name);
		for (int i = 0; i < runtime_algorithms.size(); i ++) {
			if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "normal")) {
				if (runtime_algorithms[i]->runtime_algorithm_object == NULL)
					runtime_algorithms[i]->runtime_algorithm_object = new Runtime_common_algorithm(runtime_algorithms[i]->algorithm_cfg_name);
				if (((Runtime_common_algorithm*) runtime_algorithms[i]->runtime_algorithm_object)->is_model_algorithm()) {
					EXECUTION_REPORT(REPORT_LOG, true, "run model algorithm %s\n", runtime_algorithms[i]->algorithm_cfg_name);
					EXECUTION_REPORT(REPORT_LOG, true, "before run algorithm %d in procedure %s", i, procedure_name);
					runtime_algorithms[i]->runtime_algorithm_object->run(words_are_the_same(procedure_stage, PROCEDURE_STAGE_KERNEL));
					EXECUTION_REPORT(REPORT_LOG, true, "after run algorithm %d in procedure", i, procedure_name);
				}
			}
		}
		return;
	}

    if (words_are_the_same(procedure_stage, PROCEDURE_STAGE_INITIALIZE)) {
        EXECUTION_REPORT(REPORT_ERROR, timer_mgr->get_current_num_time_step() <= 1,
                     "when coupling procedure %s of component %s works as a initialize-stage procedure, it can only execute once and can only execute before kernel loop or at the first iteration of kernel loop\n",
                     procedure_name, compset_communicators_info_mgr->get_current_comp_name());
		EXECUTION_REPORT(REPORT_ERROR, !has_executed, 
			         "when coupling procedure %s of component %s works as a initialize-stage procedure, it must not be executed before\n");
    }
    else if (words_are_the_same(procedure_stage, PROCEDURE_STAGE_FINALIZE))
        EXECUTION_REPORT(REPORT_ERROR, timer_mgr->check_is_coupled_run_finished(),
                     "coupling procedure %s of component %s is a finalize-stage procedure, which can only execute once and can only execute after kernel loop\n",
                     procedure_name, compset_communicators_info_mgr->get_current_comp_name());

	EXECUTION_REPORT(REPORT_LOG, true, "execute runtime procedure %s",  procedure_name);

    for (int i = 0; i < runtime_algorithms.size(); i ++) {
		if (runtime_algorithms[i]->runtime_algorithm_object == NULL) {
			EXECUTION_REPORT(REPORT_LOG, true, "generate %s algorithm %s with id %d in procedure %s", runtime_algorithms[i]->algorithm_type, runtime_algorithms[i]->algorithm_cfg_name, i, procedure_name);
			if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "remap")) 
				runtime_algorithms[i]->runtime_algorithm_object = new Runtime_remap_algorithm(runtime_algorithms[i]->algorithm_cfg_name);
			else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "normal"))
				runtime_algorithms[i]->runtime_algorithm_object = new Runtime_common_algorithm(runtime_algorithms[i]->algorithm_cfg_name);
			else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "data"))
				runtime_algorithms[i]->runtime_algorithm_object = new Runtime_datamodel_algorithm(runtime_algorithms[i]->algorithm_cfg_name);
			else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "transfer")) 
				runtime_algorithms[i]->runtime_algorithm_object = new Runtime_transfer_algorithm(runtime_algorithms[i]->algorithm_cfg_name); 
			else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "averaging"))
				runtime_algorithms[i]->runtime_algorithm_object = new Runtime_cumulate_average_algorithm(runtime_algorithms[i]->algorithm_cfg_name);
			else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "merge_fields"))
				runtime_algorithms[i]->runtime_algorithm_object = new Runtime_merge_algorithm(runtime_algorithms[i]->algorithm_cfg_name);
			else EXECUTION_REPORT(REPORT_ERROR, false, "error type of runtime algorithm %s", runtime_algorithms[i]->algorithm_type);
			EXECUTION_REPORT(REPORT_LOG, true, "Finish generating algorithm %s with id %d", runtime_algorithms[i]->algorithm_cfg_name, i);
		}

		EXECUTION_REPORT(REPORT_LOG, true, "before run algorithm %d in procedure %s", i, procedure_name);
		runtime_algorithms[i]->runtime_algorithm_object->allocate_src_dst_fields(words_are_the_same(procedure_stage, PROCEDURE_STAGE_KERNEL));
		runtime_algorithms[i]->runtime_algorithm_object->transfer_fields_data_type_before_run();
		runtime_algorithms[i]->runtime_algorithm_object->cumulate_average_before_run(words_are_the_same(procedure_stage, PROCEDURE_STAGE_KERNEL));
		runtime_algorithms[i]->runtime_algorithm_object->run(words_are_the_same(procedure_stage, PROCEDURE_STAGE_KERNEL));
		runtime_algorithms[i]->runtime_algorithm_object->transfer_fields_data_type_after_run();
		EXECUTION_REPORT(REPORT_LOG, true, "after run algorithm %d in procedure", i, procedure_name);
    }

	has_executed = true;
}


bool Runtime_procedure_mgt::match(const char *procedure_name)
{
	return words_are_the_same(this->procedure_name, procedure_name);
}


void Runtime_procedure_mgt::write_restart_fields()
{
    EXECUTION_REPORT(REPORT_LOG, true, "runtime process manager write restart fields\n");

	EXECUTION_REPORT(REPORT_LOG, true, "runtime process manager find kernel procedure for writing restart fields\n");

    for (int i = 0; i < runtime_algorithms.size(); i ++)
		if (runtime_algorithms[i]->runtime_algorithm_object != NULL)
            if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "averaging")) {
				EXECUTION_REPORT(REPORT_LOG, true, "runtime process manager find averaging algorithm for writing restart fields\n");
				((Runtime_cumulate_average_algorithm*) (runtime_algorithms[i]->runtime_algorithm_object))->write_restart_fields();
			}
}


void Runtime_process_mgt::add_runtime_algorithms(const char *algorithms_cfg_file)
{
    char line[NAME_STR_SIZE];
    char alg_type[NAME_STR_SIZE];
    char alg_name[NAME_STR_SIZE];
    char *line_p;
    FILE *tmp_cfg_fp;


    if (strcmp(algorithms_cfg_file, "NULL") != 0) {
        tmp_cfg_fp = open_config_file(algorithms_cfg_file);
        while(get_next_line(line, tmp_cfg_fp)) {
            line_p = line;
            get_next_attr(alg_type, &line_p);
            get_next_attr(alg_name, &line_p);
            add_runtime_algorithm(alg_name, alg_type);
        }
        fclose(tmp_cfg_fp);   
    }
}


void Runtime_process_mgt::add_runtime_procedures(const char *procedures_cfg_file)
{
    char line[NAME_STR_SIZE];
    char *line_p;
    FILE *tmp_cfg_fp;
    Runtime_procedure_mgt *runtime_procedure;
	int start_algorithm_id, end_algorithm_id;
	char procedure_name[NAME_STR_SIZE];


    if (!words_are_the_same(procedures_cfg_file, "NULL")) {
        tmp_cfg_fp = open_config_file(procedures_cfg_file);
        while(get_next_line(line, tmp_cfg_fp)) {
            line_p = line;
            get_next_attr(procedure_name, &line_p);
            runtime_procedure = new Runtime_procedure_mgt(procedure_name);
            get_next_integer_attr(&line_p, start_algorithm_id);
            get_next_integer_attr(&line_p, end_algorithm_id);
            EXECUTION_REPORT(REPORT_ERROR, end_algorithm_id >= start_algorithm_id, 
                         "the start algorithm id can not be larger than the end algorithm id in coupling procedure %s of component %s\n", 
                         procedure_name, compset_communicators_info_mgr->get_current_comp_name());
            EXECUTION_REPORT(REPORT_ERROR, start_algorithm_id >= -1, 
                         "the start algorithm id and the end algorithm id can not be smaller than -1 in coupling procedure %s of component %s\n", 
                         procedure_name, compset_communicators_info_mgr->get_current_comp_name());
			if (start_algorithm_id == -1)
				EXECUTION_REPORT(REPORT_ERROR, end_algorithm_id == -1, 
							 "procedure %s of component %s is an empty procedure, its start algorithm id and end algorithm id must be -1", 
							 procedure_name, compset_communicators_info_mgr->get_current_comp_name());
            EXECUTION_REPORT(REPORT_ERROR, end_algorithm_id < (int)runtime_algorithms.size() && start_algorithm_id < (int)runtime_algorithms.size(), 
                         "the start algorithm id and the end algorithm id can not exceed the number of runtime algorithms in coupling procedure %s of component %s\n", 
                         procedure_name, compset_communicators_info_mgr->get_current_comp_name());
			if (start_algorithm_id != -1)
				for (int i = start_algorithm_id; i <= end_algorithm_id; i ++)
					runtime_procedure->add_runtime_algorithm(runtime_algorithms[i]);
            runtime_procedures.push_back(runtime_procedure);
        }
        fclose(tmp_cfg_fp);   
     }
}


Runtime_procedure_mgt *Runtime_process_mgt::search_coupling_procedure(const char *procedure_name)
{
    for (int i = 0; i < runtime_procedures.size(); i ++)
        if (runtime_procedures[i]->match(procedure_name))
            return runtime_procedures[i];
        
    EXECUTION_REPORT(REPORT_WARNING, true, "coupling procedure %s has not been defined in component %s\n",
                 procedure_name, compset_communicators_info_mgr->get_current_comp_name());
	
	return NULL;
}


void Runtime_process_mgt::execute_coupling_procedure(const char *procedure_name, const char *procedure_stage)
{
	Runtime_procedure_mgt *runtime_procedure = search_coupling_procedure(procedure_name);
	if (runtime_procedure != NULL)
		runtime_procedure->execute(procedure_stage);
	else EXECUTION_REPORT(REPORT_ERROR, false, "WARNING: coupling procedure %s has not been set in the corresponding configuration file\n", procedure_name);
}


void Runtime_process_mgt::add_runtime_algorithm(const char *algorithm_cfg_name, const char *algorithm_type)
{
    Runtime_algorithm_mgt *algorithm_container;


    algorithm_container = new Runtime_algorithm_mgt;
    strcpy(algorithm_container->algorithm_cfg_name, algorithm_cfg_name);
    strcpy(algorithm_container->algorithm_type, algorithm_type);
    algorithm_container->runtime_algorithm_object = NULL;

    runtime_algorithms.push_back(algorithm_container);
}


Runtime_process_mgt::~Runtime_process_mgt()
{
    for (int i = 0; i < runtime_procedures.size(); i ++)
        delete runtime_procedures[i];

    for (int i = 0; i < runtime_algorithms.size(); i ++) {
        EXECUTION_REPORT(REPORT_LOG, true, "before deleting %s algorithm", runtime_algorithms[i]->algorithm_type);
        if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "remap"))
            delete (Runtime_remap_algorithm*) (runtime_algorithms[i]->runtime_algorithm_object);
        else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "normal"))
            delete (Runtime_common_algorithm*) (runtime_algorithms[i]->runtime_algorithm_object);
        else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "data")) 
            delete (Runtime_datamodel_algorithm*) (runtime_algorithms[i]->runtime_algorithm_object);
        else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "transfer"))
            delete (Runtime_transfer_algorithm*) (runtime_algorithms[i]->runtime_algorithm_object);
        else if (words_are_the_same(runtime_algorithms[i]->algorithm_type, "averaging"))
            delete (Runtime_cumulate_average_algorithm*) (runtime_algorithms[i]->runtime_algorithm_object);
        delete runtime_algorithms[i];
        EXECUTION_REPORT(REPORT_LOG, true, "after deleting %s algorithm\n", runtime_algorithms[i]->algorithm_type);
    }
}


void Runtime_process_mgt::write_restart_fields()
{
    EXECUTION_REPORT(REPORT_LOG, true, "runtime process manager write restart fields\n");
    for (int i = 0; i < runtime_procedures.size(); i ++)
		runtime_procedures[i]->write_restart_fields();
}


void Runtime_process_mgt::register_model_algorithm(const char *algorithm_name, Model_algorithm model_algorithm)
{
	external_algorithm_mgr->register_external_algorithm(algorithm_name, NULL, model_algorithm);
}

