/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef GLOBAL_DATA
#define GLOBAL_DATA


#include "compset_communicators_info_mgt.h"
#include "decomp_info_mgt.h"
#include "field_info_mgt.h"
#include "memory_mgt.h"
#include "runtime_process_mgt.h"
#include "timer_mgt.h"
#include "routing_info_mgt.h"
#include "restart_mgt.h"
#include "cor_cpl_interface.h"
#include "cor_global_data.h"
#include "fields_gather_scatter_mgt.h"
#include "decomp_grid_mgt.h"
#include "common_utils.h"
#include "execution_report.h"
#include "performance_timing_mgt.h"
#include "external_algorithm_mgt.h"
#include "ensemble_mgt.h"
#include "runtime_datamodel_algorithm.h"


#define C_COUPLER_CONFIG_DIR             "CCPL_configs"


extern char software_name[];


extern Compset_communicators_info_mgt *compset_communicators_info_mgr;
extern Routing_info_mgt *routing_info_mgr;
extern Timer_mgt *timer_mgr;
extern Timer_mgt *restart_read_timer_mgr;
extern Decomp_info_mgt *decomps_info_mgr;
extern Field_info_mgt *fields_info;
extern Memory_mgt *memory_manager;
extern Runtime_process_mgt *runtime_process_mgr;
extern Restart_mgt *restart_mgr;
extern Remap_mgt *grid_remap_mgr;
extern Fields_gather_scatter_mgt *fields_gather_scatter_mgr;
extern Decomp_grid_mgt *decomp_grids_mgr;
extern Performance_timing_mgt *performance_timing_mgr;
extern External_algorithm_mgt *external_algorithm_mgr;
extern Ensemble_mgt *ensemble_mgr;
extern Datamodel_field_read_handler_mgt *datamodel_field_read_handler_mgr;

#endif
