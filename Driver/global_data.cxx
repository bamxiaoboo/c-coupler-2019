/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"


char software_name[16] = "CoR";


Compset_communicators_info_mgt *compset_communicators_info_mgr = NULL;
Routing_info_mgt *routing_info_mgr = NULL;
Timer_mgt *timer_mgr = NULL;
Timer_mgt *restart_read_timer_mgr = NULL;
Decomp_info_mgt *decomps_info_mgr = NULL;
Field_info_mgt *fields_info = NULL;
Memory_mgt *memory_manager = NULL;
Runtime_process_mgt *runtime_process_mgr = NULL;
Restart_mgt *restart_mgr = NULL;
Remap_mgt *grid_remap_mgr = NULL;
Fields_gather_scatter_mgt *fields_gather_scatter_mgr = NULL;
Decomp_grid_mgt *decomp_grids_mgr = NULL;
Performance_timing_mgt *performance_timing_mgr = NULL;
External_algorithm_mgt *external_algorithm_mgr = NULL;
Ensemble_mgt *ensemble_mgr = NULL;
Datamodel_field_read_handler_mgt *datamodel_field_read_handler_mgr = NULL;


