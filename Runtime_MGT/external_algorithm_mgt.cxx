/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"
#include "frac.h"
#include "areafact.h"
#include "flux_epbal.h"
#include "flux_solar.h"
#include "merge_ocn.h"
#include "map_npfix.h"
#include "flux_albo.h"
#include "flux_atmOcn.h"
#include "merge_atm.h"
#include "flux_albi.h"
#include "fst_diag.h"
#include "fields_mult.h"
#include "fields_add.h"
#include "fields_copy.h"
#include "merge_surface_data.h"


External_algorithm_mgt::External_algorithm_mgt()
{
	register_external_algorithm("frac_init_atm", frac_init_atm, NULL);
	register_external_algorithm("frac_init_ocn", frac_init_ocn, NULL);
	register_external_algorithm("frac_set_atm", frac_set_atm, NULL);
	register_external_algorithm("frac_set_ocn", frac_set_ocn, NULL);
	register_external_algorithm("areafact_init", areafact_init, NULL);
	register_external_algorithm("flux_epbal", flux_epbal, NULL);
	register_external_algorithm("flux_solar", flux_solar, NULL);
	register_external_algorithm("merge_ocn", merge_ocn, NULL);
	register_external_algorithm("merge_carbon_flux", merge_carbon_flux, NULL);
	register_external_algorithm("map_npfix", map_npfix, NULL);
	register_external_algorithm("flux_albo", flux_albo, NULL);
	register_external_algorithm("flux_atmOcn", flux_atmOcn, NULL);
	register_external_algorithm("merge_atm", merge_atm, NULL);
	register_external_algorithm("flux_albi", flux_albi, NULL);
	register_external_algorithm("diag_dodiag", diag_dodiag, NULL);
	register_external_algorithm("diag_solar", diag_solar, NULL);
	register_external_algorithm("fields_mult", fields_mult, NULL);
	register_external_algorithm("fields_add", fields_add, NULL);
	register_external_algorithm("field_copy", fields_copy, NULL);
	register_external_algorithm("reset_surface_data", reset_surface_data, NULL);
	register_external_algorithm("merge_one_kind_surface_data", merge_one_kind_surface_data, NULL);
}


External_algorithm_mgt::~External_algorithm_mgt()
{
	for (int i = 0; i < external_algorithms.size(); i ++)
		delete external_algorithms[i];
}


void External_algorithm_mgt::register_external_algorithm(const char *algorithm_name, C_Coupler_algorithm c_coupler_algorithm, Model_algorithm model_algorithm)
{
	External_algorithm_info *external_algorithm;

	
	EXECUTION_REPORT(REPORT_ERROR, (c_coupler_algorithm!=NULL && model_algorithm==NULL) || (c_coupler_algorithm==NULL && model_algorithm!=NULL),
					 "C-Coupler error in input parameters of register_external_algorithm");
	EXECUTION_REPORT(REPORT_ERROR, search_c_coupler_algorithm_pointer(algorithm_name) == NULL && search_model_algorithm_pointer(algorithm_name) == NULL,
					 "Algorithm %s has been registerred before", algorithm_name);

	external_algorithm = new External_algorithm_info;
	external_algorithm->c_coupler_algorithm = c_coupler_algorithm;
	external_algorithm->model_algorithm = model_algorithm;
	strcpy(external_algorithm->algorithm_name, algorithm_name);
	external_algorithms.push_back(external_algorithm);
}


C_Coupler_algorithm External_algorithm_mgt::search_c_coupler_algorithm_pointer(const char *algorithm_name)
{
	for (int i = 0; i < external_algorithms.size(); i ++)
		if (words_are_the_same(external_algorithms[i]->algorithm_name, algorithm_name))
			return external_algorithms[i]->c_coupler_algorithm;
		
	return NULL;
}


Model_algorithm External_algorithm_mgt::search_model_algorithm_pointer(const char *algorithm_name)
{
	for (int i = 0; i < external_algorithms.size(); i ++)
		if (words_are_the_same(external_algorithms[i]->algorithm_name, algorithm_name))
			return external_algorithms[i]->model_algorithm;
		
	return NULL;
}

