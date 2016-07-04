#include "global_data.h"
#include "runtime_merge_algorithm.h"
#include "runtime_config_dir.h"
#include "execution_report.h"


Runtime_merge_algorithm::Runtime_merge_algorithm(const char *cfg_name)
{
	FILE *fp_cfg;
    char line[NAME_STR_SIZE], *line_p;

	
	strcpy(algorithm_cfg_name, cfg_name);
	fp_cfg = open_config_file(algorithm_cfg_name, RUNTIME_COMMON_ALG_DIR);
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the timer for the runtime merge algorithm \"%s\"", cfg_name);
	line_p = line;
    timer = new Coupling_timer(&line_p, cfg_name);
	fclose(fp_cfg);
}


Runtime_merge_algorithm::~Runtime_merge_algorithm()
{
	for (int i = 0; i < fields_for_weight.size(); i ++)
		if (fields_for_weight[i]->get_data_buf() != data_buffers_for_weight[i])
			delete [] data_buffers_for_weight[i];
	for (int i = 0; i < fields_for_input.size(); i ++)
		if (fields_for_input[i]->get_data_buf() != data_buffers_for_input[i])
			delete [] data_buffers_for_input[i];
	for (int i = 0; i < fields_for_output.size(); i ++)
		if (fields_for_output[i]->get_data_buf() != data_buffers_for_output[i])
			delete [] data_buffers_for_output[i];
}


template <class T1, class T2> void Runtime_merge_algorithm::transform_data_type(T1 *src, T2 *dst, long size)
{
	for (long i = 0; i < size; i ++)
		dst[i] = (T2) src[i];
}


void Runtime_merge_algorithm::allocate_src_dst_fields(bool is_algorithm_in_kernel_stage)
{
	int i, j, manner_of_weight_specification, buf_mark;
	double temp_value;
	FILE *fp_cfg, *fp_field;
	char line[NAME_STR_SIZE*3], attr[NAME_STR_SIZE];
	char comp_name[NAME_STR_SIZE], grid_name[NAME_STR_SIZE], field_name[NAME_STR_SIZE], decomp_name[NAME_STR_SIZE];
	char *line_p, *line_p2;
	Field_mem_info *field;
	Remap_grid_class *size_grids1[256], *size_grids2[256];
	int num_size_grids1, num_size_grids2;


	if (is_algorithm_in_kernel_stage && !timer->is_timer_on())
		return;

	if (fields_allocated)
		return;
	fields_allocated = true;

	fp_cfg = open_config_file(algorithm_cfg_name, RUNTIME_COMMON_ALG_DIR);
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the timer for the runtime merge algorithm \"%s\"", algorithm_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the number of sources for the runtime merge algorithm \"%s\"", algorithm_cfg_name);
	line_p = line;	
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_integer_attr(&line_p, num_sources), "The number of sources of a runtime merge algorithm must be an integer. Please verify the configuration file \"%s\"", algorithm_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, num_sources >= 1, "The number of sources of a runtime merge algorithm must cannot be smaller than 1. Please verify the configuration file \"%s\"", algorithm_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the grid and parallel decomposition of the fields for the runtime merge algorithm \"%s\"", algorithm_cfg_name);
	line_p = line;
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(attr, &line_p), "C-Coupler error1 in Runtime_merge_algorithm::allocate_src_dst_fields at runtime merge algorithm \"%s\"", algorithm_cfg_name);
	grid_for_merge = remap_grid_manager->search_remap_grid_with_grid_name(attr);
	if (!words_are_the_same(attr, "NULL"))
		EXECUTION_REPORT(REPORT_ERROR,-1, grid_for_merge != NULL, "The grid of fields (\"%s\") for the runtime merge algorithm \"%s\" does not exist. Please verify the corresponding configuration file", attr, algorithm_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(attr, &line_p), "Please specify the parallel decomposition of the fields for the runtime merge algorithm \"%s\"", algorithm_cfg_name);
	if (!words_are_the_same(attr, "NULL"))
		decomp_for_merge = decomps_info_mgr->search_decomp_info(attr);
	else decomp_for_merge = NULL;
	if (grid_for_merge == NULL || !grid_for_merge->has_grid_coord_label(COORD_LABEL_LON) || !grid_for_merge->has_grid_coord_label(COORD_LABEL_LAT))
		EXECUTION_REPORT(REPORT_ERROR,-1, decomp_for_merge == NULL, "The grid and parallel decomposition of the fields for the runtime merge algorithm \"%s\" do not match with each other. Please verify.", algorithm_cfg_name);
	else if (decomp_for_merge != NULL)
		EXECUTION_REPORT(REPORT_ERROR,-1, remap_grid_manager->search_remap_grid_with_grid_name(decomp_for_merge->get_grid_name())->is_subset_of_grid(grid_for_merge), 
						 "The grid (\"%s\") and parallel decomposition (\"%s\") of the fields for the runtime merge algorithm \"%s\" do not match with each other. Please verify.",  
						 grid_for_merge->get_grid_name(), decomp_for_merge->get_decomp_name(), algorithm_cfg_name);

	for (i = 0; i < num_sources+1; i ++) {
		if (i < num_sources)
			EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the configuration file for the input fields of source %dth for the runtime merge algorithm \"%s\"", i+1, algorithm_cfg_name);
		else EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the configuration file for the output fields for the runtime merge algorithm \"%s\"", algorithm_cfg_name);
		j = 0;
		fp_field = open_config_file(line, RUNTIME_COMMON_ALG_DIR);
		while (get_next_line(attr, fp_field)) {
			field = get_a_field(attr, i<num_sources, j, line);
			j ++;
			if (i < num_sources)
				fields_for_input.push_back(field);
			else fields_for_output.push_back(field);
		}
		fclose(fp_field);
	}

	EXECUTION_REPORT(REPORT_ERROR,-1, fields_for_input.size() > 0, "At least one output field for the runtime merge algorithm \"%s\" must be specified. Please verify.", algorithm_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, fields_for_output.size()*num_sources == fields_for_input.size(), "The configuration files of the input and output fields for the runtime merge algorithm \"%s\" are not consistent in size (number of lines). Please verify.", algorithm_cfg_name);

	for (j = 0; j < fields_for_output.size(); j ++)
		for (i = 0; i < num_sources; i ++) 
			EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(fields_for_input[i*fields_for_output.size()+j]->get_field_name(),fields_for_output[j]->get_field_name()), 
							 "The configuration files of the input and output fields for the runtime merge algorithm \"%s\" are not consistent. Please check the %dth field in each configuration file.", algorithm_cfg_name, j+1);			
		
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the manner (\"immediate data\", \"file data\", or \"internal field data\") for specifying the weight values for the runtime merge algorithm \"%s\"", algorithm_cfg_name);
	if (words_are_the_same(line, "immediate data"))
		manner_of_weight_specification = 1;
	else if (words_are_the_same(line, "file data"))
		manner_of_weight_specification = 2;
	else if (words_are_the_same(line, "internal field data"))
		manner_of_weight_specification = 3;
	else EXECUTION_REPORT(REPORT_ERROR,-1, false, "Please verify the manner (\"immediate data\", \"file data\", or \"internal field data\") for specifying the weight values for the runtime merge algorithm \"%s\". \"%s\" is an illegal manner", algorithm_cfg_name, line);
	if (manner_of_weight_specification != 3) {
		EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), 
						 "Please specify the grid of the weight values for the runtime merge algorithm \"%s\" when the manner of weight value specification is \"immediate data\" or \"file data\"", algorithm_cfg_name);
		grid_for_weights = remap_grid_manager->search_remap_grid_with_grid_name(line);
		if (!words_are_the_same(line, "NULL")) {
			EXECUTION_REPORT(REPORT_ERROR,-1, grid_for_weights != NULL, "The grid of weight values for the runtime merge algorithm \"%s\" does not exist. Please verify the corresponding configuration file", algorithm_cfg_name);
			EXECUTION_REPORT(REPORT_ERROR,-1, !grid_for_weights->has_grid_coord_label(COORD_LABEL_LAT) && !grid_for_weights->has_grid_coord_label(COORD_LABEL_LON),
							 "When the manner of weight value specification is \"immediate data\" or \"file data\", the grid of weight values cannot be a super grid of a horizontal grid. Please verify the corresponding configuration file of the runtime merge algorithm \"%s\"", algorithm_cfg_name);
		}
		decomp_for_weights = NULL;
		for (i = 0; i < num_sources; i ++)
			fields_for_weight.push_back(alloc_mem(compset_communicators_info_mgr->get_current_comp_name(), "NULL", line, "scalar_double", DATA_TYPE_DOUBLE, memory_manager->get_num_fields(), false, algorithm_cfg_name)); 
		for (i = 0; i < num_sources; i ++) {
			EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the weight values of source %dth for the runtime merge algorithm \"%s\"", i+1, algorithm_cfg_name);
			if (manner_of_weight_specification == 1) {
				j = 0;
				line_p = line;
				while(get_next_attr(attr, &line_p)) {
					EXECUTION_REPORT(REPORT_ERROR,-1, j < fields_for_weight[0]->get_size_of_field(),
									 "The number of weight values of source %dth specified for the runtime merge algorithm \"%s\" does not match (more than) the number required. Please verify.", i+1, algorithm_cfg_name);
					line_p2 = attr;
					EXECUTION_REPORT(REPORT_ERROR,-1, get_next_double_attr(&line_p2, temp_value), "The %dth weight value of source %dth specified for the runtime merge algorithm \"%s\" is not a legal double value. Please verify.", j, i+1, algorithm_cfg_name);
					((double*)fields_for_weight[i]->get_data_buf())[j] = temp_value;
					j ++;
				}				
			}
			else {
				fp_field = open_config_file(line, RUNTIME_COMMON_ALG_DIR);
				j = 0;
				while(get_next_line(line, fp_field)) {
					EXECUTION_REPORT(REPORT_ERROR,-1, j < fields_for_weight[0]->get_size_of_field(),
									 "The number of weight values of source %dth specified for the runtime merge algorithm \"%s\" does not match (more than) the number required. Please verify.", i+1, algorithm_cfg_name);
					line_p = line;
					EXECUTION_REPORT(REPORT_ERROR,-1, get_next_double_attr(&line_p, temp_value), "The %dth weight value of source %dth specified for the runtime merge algorithm \"%s\" is not a legal double value. Please verify.", j, i+1, algorithm_cfg_name);
					((double*)fields_for_weight[i]->get_data_buf())[j] = temp_value;
					j ++;
				}
				fclose(fp_field);
			}
			EXECUTION_REPORT(REPORT_ERROR,-1, j == fields_for_weight[0]->get_size_of_field(),
							 "The number of weight values of source %dth specified for the runtime merge algorithm \"%s\" is not match (%d < %ld), less than) the number required. Please verify.", i+1, algorithm_cfg_name, j, fields_for_weight[0]->get_size_of_field());
		}
		for (i = 0; i < num_sources; i ++)
			fields_for_weight[i]->define_field_values(false);
	}
	else {
		for (i = 0; i < num_sources; i ++) {
			EXECUTION_REPORT(REPORT_ERROR,-1, get_next_line(line, fp_cfg), "Please specify the field of the weight values of source %dth for the runtime merge algorithm \"%s\"", i+1, algorithm_cfg_name);
			line_p = line;
			EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(comp_name, &line_p), "Please specify the component name for the weight field of source %dth for the runtime merge algorithm \"%s\"", i+1, algorithm_cfg_name);
			EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(decomp_name, &line_p), "Please specify the decomposition name for the weight field of source %dth for the runtime merge algorithm \"%s\"", i+1, algorithm_cfg_name);
			EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(grid_name, &line_p), "Please specify the grid name for the weight field of source %dth for the runtime merge algorithm \"%s\"", i+1, algorithm_cfg_name);
			EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(field_name, &line_p), "Please specify the field name for the weight field of source %dth for the runtime merge algorithm \"%s\"", i+1, algorithm_cfg_name);
			EXECUTION_REPORT(REPORT_ERROR,-1, get_next_integer_attr(&line_p, buf_mark),  "Please specify the buffer mark for the weight field of source %dth for the runtime merge algorithm \"%s\"", i+1, algorithm_cfg_name); 		
			fields_for_weight.push_back(alloc_mem(comp_name, decomp_name, grid_name, field_name, NULL, buf_mark, true, algorithm_cfg_name));
			if (i == 0) {
				if (words_are_the_same(grid_name, "NULL"))
					grid_for_weights = NULL;
				else grid_for_weights = remap_grid_manager->search_remap_grid_with_grid_name(grid_name);
				if (words_are_the_same(decomp_name, "NULL"))
					decomp_for_weights = NULL;
				else decomp_for_weights = decomps_info_mgr->search_decomp_info(decomp_name);
			}
		}
	}	
	fclose(fp_cfg);

	for (i = 0; i < num_sources-1; i ++) {
		EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(fields_for_weight[i]->get_grid_name(), fields_for_weight[num_sources-1]->get_grid_name()),
						 "The grids of the weight values for the runtime merge algorithm \"%s\" must be the same. Please verify the corresponding configuration files", algorithm_cfg_name);
		EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(fields_for_weight[i]->get_decomp_name(), fields_for_weight[num_sources-1]->get_decomp_name()),
						 "The parallel decompositions of the weight values for the runtime merge algorithm \"%s\" must be the same. Please verify the corresponding configuration files", algorithm_cfg_name);
	}

	if (decomp_for_merge == NULL || decomp_for_weights != NULL)
		EXECUTION_REPORT(REPORT_ERROR,-1, decomp_for_merge == decomp_for_weights, "The parallel decompositions between the fields and weights are not consistent for the runtime merge algorithm \"%s\". Please verify (for example, try to make the parallel decompositions the same).", algorithm_cfg_name);
	if (grid_for_weights != NULL)
		EXECUTION_REPORT(REPORT_ERROR,-1, grid_for_merge != NULL && grid_for_weights->is_subset_of_grid(grid_for_merge), 
						 "The grids between the fields and weights are not consistent for the runtime merge algorithm \"%s\". Please verify (for example, try to make the grid of weights the same with or be a subgrid of the grid of fields).", algorithm_cfg_name);

	for (i = 0; i < fields_for_weight.size(); i ++)
		if (words_are_the_same(fields_for_weight[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE))
			data_buffers_for_weight.push_back((double*)fields_for_weight[i]->get_data_buf());
		else {
			EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(fields_for_weight[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT), "The data type of weight values of source %dth must be \"REAL8(dobule)\" or \"REAL4(float)\". Please verify the configuration files of the runtime merge algorithm \"%s\"/. ", i+1, algorithm_cfg_name);
			data_buffers_for_weight.push_back(new double [fields_for_weight[i]->get_size_of_field()]);
		}
	for (i = 0; i < fields_for_input.size(); i ++)
		if (words_are_the_same(fields_for_input[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE))
			data_buffers_for_input.push_back((double*)fields_for_input[i]->get_data_buf());
		else {
			EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(fields_for_input[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT), "The data type of %dth input fields of source %dth must be \"REAL8(dobule)\" or \"REAL4(float)\". Please verify the configuration files of the runtime merge algorithm \"%s\". ", (i%fields_for_output.size())+1, (i/fields_for_output.size())+1, algorithm_cfg_name);
			data_buffers_for_input.push_back(new double [fields_for_input[i]->get_size_of_field()]);
		}
	for (i = 0; i < fields_for_output.size(); i ++)
		if (words_are_the_same(fields_for_output[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_DOUBLE))
			data_buffers_for_output.push_back((double*)fields_for_output[i]->get_data_buf());
		else {
			EXECUTION_REPORT(REPORT_ERROR,-1, words_are_the_same(fields_for_output[i]->get_field_data()->get_grid_data_field()->data_type_in_application, DATA_TYPE_FLOAT), "The data type of %dth output field of source %dth must be \"REAL8(dobule)\" or \"REAL4(float)\". Please verify the configuration files of the runtime merge algorithm \"%s\". ", (i%fields_for_output.size())+1, (i/fields_for_output.size())+1, algorithm_cfg_name);
			data_buffers_for_output.push_back(new double [fields_for_output[i]->get_size_of_field()]);
		}

	if (grid_for_weights == NULL) {
		loop_size[0] = fields_for_output[0]->get_size_of_field();
		loop_size[1] = 1;
		loop_size[2] = 1;
	}
	else {
		loop_size[0] = 1;
		loop_size[1] = fields_for_weight[0]->get_size_of_field();
		loop_size[2] = 1;
		grid_for_merge->get_sized_sub_grids(&num_size_grids1, size_grids1);
		grid_for_weights->get_sized_sub_grids(&num_size_grids2, size_grids2);
		for (i = 0; i < num_size_grids1; i ++) {
			if (size_grids1[i]->is_similar_grid_with(size_grids2[0]))
				break;
			loop_size[0] *= size_grids1[i]->get_grid_size();
		}
		EXECUTION_REPORT(REPORT_ERROR,-1, i+num_size_grids2 <= num_size_grids1, "C-Coupler error1 in Runtime_merge_algorithm::allocate_src_dst_fields at runtime merge algorithm \"%s\"", algorithm_cfg_name);
		for (j = 0; j < num_size_grids2; j ++) {
			EXECUTION_REPORT(REPORT_ERROR,-1, size_grids1[i]->is_similar_grid_with(size_grids2[j]), "The grids for weight values and fields do not match. Please verify the runtime merge algorithm \"%s\" (please contact liuli-cess@tsinghua.edu.cn if there is any problem).", algorithm_cfg_name);
			i ++;
		}
		for (; i < num_size_grids1; i ++)
			loop_size[2] *= size_grids1[i]->get_grid_size();
		if (decomp_for_merge != NULL) {
			if (loop_size[0] > 1 && size_grids1[0]->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(decomp_for_merge->get_grid_name())))
				loop_size[0] = loop_size[0] / decomp_for_merge->get_num_global_cells() * decomp_for_merge->get_num_local_cells();
			if (loop_size[2] > 1 && size_grids1[num_size_grids1-1]->is_subset_of_grid(remap_grid_manager->search_remap_grid_with_grid_name(decomp_for_merge->get_grid_name())))
				loop_size[2] = loop_size[2] / decomp_for_merge->get_num_global_cells() * decomp_for_merge->get_num_local_cells();
		}
	}

	EXECUTION_REPORT(REPORT_LOG,-1, true, "Loop size for the runtime merge algorithm \"%s\" are %ld, %ld and %ld", algorithm_cfg_name, loop_size[0], loop_size[1], loop_size[2]); 
}


Field_mem_info *Runtime_merge_algorithm::get_a_field(char *line, bool input, int indx, const char *field_cfg_name)
{
	char comp_name[NAME_STR_SIZE], field_name[NAME_STR_SIZE], *line_p;
	int buf_type;


	line_p = line;
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(comp_name, &line_p), "C-Coupler error1 in Runtime_merge_algorithm::get_a_field at runtime merge algorithm \"%s\"", algorithm_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_attr(field_name, &line_p), "Please specify the field name for the %dth field specified in configuration file %s for runtime merge algorithm \"%s\"", indx, field_cfg_name, algorithm_cfg_name);
	EXECUTION_REPORT(REPORT_ERROR,-1, get_next_integer_attr(&line_p, buf_type), "Please specify the buffer mark for the %dth field specified in configuration file %s for runtime merge algorithm \"%s\"", indx, field_cfg_name, algorithm_cfg_name);

	if (input)
		return alloc_mem(comp_name, decomp_for_merge->get_decomp_name(), grid_for_merge->get_grid_name(), field_name, NULL, buf_type, input, algorithm_cfg_name);
	else return alloc_mem(comp_name, decomp_for_merge->get_decomp_name(), grid_for_merge->get_grid_name(), field_name, DATA_TYPE_DOUBLE, buf_type, input, algorithm_cfg_name);
}


void Runtime_merge_algorithm::run(bool is_algorithm_in_kernel_stage)
{
	long i, j, k, m, f, num_fields;
	bool have_illegal_weight;
	double **temp_input_data_buffers, **temp_output_data_buffers;

	
    if (is_algorithm_in_kernel_stage && !timer->is_timer_on()) 
		return;

	for (i = 0; i < fields_for_input.size(); i ++)
		fields_for_input[i]->use_field_values(algorithm_cfg_name);
	for (i = 0; i < fields_for_weight.size(); i ++)
		fields_for_weight[i]->use_field_values(algorithm_cfg_name);

	temp_input_data_buffers = new double *[fields_for_input.size()];
	temp_output_data_buffers = new double *[fields_for_output.size()];
	num_fields = fields_for_output.size();

	for (i = 0; i < fields_for_weight.size(); i ++)
		if (fields_for_weight[i]->get_data_buf() != data_buffers_for_weight[i])
			transform_data_type((float*)fields_for_weight[i]->get_data_buf(), data_buffers_for_weight[i], fields_for_weight[i]->get_size_of_field());

	for (i = 0; i < fields_for_input.size(); i ++)
		if (fields_for_input[i]->get_data_buf() != data_buffers_for_input[i])
			transform_data_type((float*)fields_for_input[i]->get_data_buf(), data_buffers_for_input[i], fields_for_input[i]->get_size_of_field());

	have_illegal_weight = false;
	for (i = 0; i < loop_size[2]; i ++) {
		double sum = 0.0;
		for (j = 0; j < num_sources; j ++) {
			sum += data_buffers_for_weight[j][i];
			if (data_buffers_for_weight[j][i] < 0 || data_buffers_for_weight[j][i] > 1) {
				have_illegal_weight = true;
				break;
			}
		}
		if (sum < 0 || sum > 1.00000000000001)
			have_illegal_weight = true;
		if (have_illegal_weight)
			break;
	}
	EXECUTION_REPORT(REPORT_WARNING, -1, !have_illegal_weight, "Some weight values for the runtime merge algorithm \"%s\" is illegal (smaller than 0.0 or larger than 1.0). Please check.", algorithm_cfg_name);

	for (k = 0; k < loop_size[2]; k ++) {
		if (loop_size[0] > 1) {
			for (j = 0; j < loop_size[1]; j ++) {
				for (f = 0; f < data_buffers_for_input.size(); f ++)
					temp_input_data_buffers[f] = data_buffers_for_input[f]+loop_size[0]*loop_size[1]*k+loop_size[0]*j;
				for (f = 0; f < data_buffers_for_output.size(); f ++)
					temp_output_data_buffers[f] = data_buffers_for_output[f]+loop_size[0]*loop_size[1]*k+loop_size[0]*j;				
				for (i = 0; i < loop_size[0]; i ++) {
					for (f = 0; f < num_fields; f ++) {
						temp_output_data_buffers[f][i] = 0.0;
						for (m = 0; m < num_sources; m ++)
							temp_output_data_buffers[f][i] += temp_input_data_buffers[f+m*num_fields][i]*data_buffers_for_weight[m][j];
					}
				}
			}
		}
		else {
			for (f = 0; f < data_buffers_for_input.size(); f ++)
				temp_input_data_buffers[f] = data_buffers_for_input[f]+loop_size[1]*k;
			for (f = 0; f < data_buffers_for_output.size(); f ++)
				temp_output_data_buffers[f] = data_buffers_for_output[f]+loop_size[1]*k;
			for (j = 0; j < loop_size[1]; j ++) {				
				for (f = 0; f < num_fields; f ++) {
					temp_output_data_buffers[f][i] = 0.0;
					for (m = 0; m < num_sources; m ++)
						temp_output_data_buffers[f][j] += temp_input_data_buffers[f+m*num_fields][j]*data_buffers_for_weight[m][j];
				}
			}			
		}
	}

	EXECUTION_REPORT(REPORT_LOG,-1, true, "finish runtime merge algorithm \"%s\"", algorithm_cfg_name);
	for (i = 0; i < fields_for_output.size(); i ++) {
		if (fields_for_output[i]->get_data_buf() != data_buffers_for_output[i])
			transform_data_type(data_buffers_for_output[i], (float*)fields_for_output[i]->get_data_buf(), fields_for_output[i]->get_size_of_field());
		fields_for_output[i]->define_field_values(false);
		fields_for_output[i]->check_field_sum();
	}

	/*
	printf("result of merge begin\n");
	for (i = 0; i < fields_for_output[0]->get_size_of_field(); i ++)
		printf("result of merge at %ld: %lf vs %lf %lf\n", i, data_buffers_for_output[0][i], data_buffers_for_input[0][i], data_buffers_for_input[fields_for_output.size()][i]);
	printf("result of merge end\n");
	*/

	delete [] temp_input_data_buffers;
	delete [] temp_output_data_buffers;
}


