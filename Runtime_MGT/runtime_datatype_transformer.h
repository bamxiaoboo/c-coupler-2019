/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/

#ifndef RUNTIME_DATATYPE_TRANSFORMER
#define RUNTIME_DATATYPE_TRANSFORMER


#include <vector>
#include "memory_mgt.h"
#include "timer_mgt.h"


class Runtime_datatype_transformer
{
	private:
		std::vector<Field_mem_info*> src_fields;
		std::vector<Field_mem_info*> dst_fields;
		std::vector<Coupling_timer*> timers;

	public:
		Runtime_datatype_transformer() {}
		~Runtime_datatype_transformer() {}
		void add_pair_fields(Field_mem_info*, Field_mem_info*, Coupling_timer*);
		void transform_fields_datatype();
};

#endif

