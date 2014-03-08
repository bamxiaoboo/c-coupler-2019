/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "global_data.h"


char software_name[16] = "CoR";


Compset_communicators_info_mgt *compset_communicators_info_mgr;
Routing_info_mgt *routing_info_mgr;
Timer_mgt *timer_mgr;
Decomp_info_mgt *decomps_info_mgr;
Field_info_mgt *fields_info;
Memory_mgt *memory_manager;
Runtime_process_mgt *runtime_process_mgr;
Restart_mgt *restart_mgr;
Remap_mgt *grid_remap_manager;
Fields_gather_scatter_mgt *fields_gather_scatter_mgr;
Decomp_grid_mgt *decomp_grids_mgr;
Performance_timing_mgt *performance_timing_mgr;
External_algorithm_mgt *external_algorithm_mgr;

