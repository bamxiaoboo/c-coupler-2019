#ifndef RUNTIME_MERGE
#define RUNTIME_MERGE


#include "Runtime_Algorithm_Basis.h"
#include "remap_grid_class.h"
#include "decomp_info_mgt.h"
#include "timer_mgt.h"
#include "memory.h"
#include <vector>


class Runtime_merge_algorithm: public Runtime_algorithm_basis
{
	private:
		Coupling_timer *timer;
		int num_sources;
		Remap_grid_class *grid_for_weights;
		Remap_grid_class *grid_for_merge;
		Decomp_info *decomp_for_weights;
		Decomp_info *decomp_for_merge;
		std::vector<Field_mem_info*> fields_for_weight;
		std::vector<Field_mem_info*> fields_for_input;
		std::vector<Field_mem_info*> fields_for_output;
		std::vector<double*> data_buffers_for_weight;
		std::vector<double*> data_buffers_for_input;
		std::vector<double*> data_buffers_for_output;
		long loop_size[3];
		
		Field_mem_info *get_a_field(char*, bool, int, const char*);
        template <class T1, class T2> void transform_data_type(T1*, T2*, long);

	public:
		Runtime_merge_algorithm(const char*);
		~Runtime_merge_algorithm();
		void run(bool);
		void allocate_src_dst_fields(bool);
};


#endif
