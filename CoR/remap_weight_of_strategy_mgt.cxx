/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "remap_weight_of_strategy_mgt.h"
#include "remap_weight_sparse_matrix.h"
#include "remap_common_utils.h"
#include "parse_special_words.h"
#include "cor_global_data.h"


void Remap_weight_of_strategy_mgt::execute(const char*function, Remap_statement_operand **statement_operands, int num_operands)
{
    int i;
    Remap_weight_of_strategy_class *remap_weights;
    Remap_grid_data_class *field_data_src, *field_data_dst;
    

    if (words_are_the_same(function, FUNCTION_WORD_COMPUTE_REMAP_WEIGHTS)) {
        EXECUTION_REPORT(REPORT_ERROR, num_operands == 4, "function \"%s\" must have one result parameter and three input parameters\n", function);
        check_is_parameter_object_type_remap_weights(function, 0, statement_operands[0], "the remap weights computed");
        check_is_parameter_object_type_remap_scheme(function, 1, statement_operands[1], "the remap scheme corresponding to the remap weights");
        check_is_parameter_object_type_grid(function, 2, statement_operands[2], "the src grid of remap weights (the grid of src field)\n");
        check_is_parameter_object_type_grid(function, 3, statement_operands[3], "the dst grid of remap weights (the grid of dst field)\n");
        remap_weights_of_strategies.push_back(new Remap_weight_of_strategy_class(statement_operands[0]->object->object_name,
                                                                                 statement_operands[1]->object->object_name,
                                                                                 statement_operands[2]->object->object_name,
                                                                                 statement_operands[3]->object->object_name,
                                                                                 NULL, NULL,
                                                                                 false));
    }
    else if (words_are_the_same(function, FUNCTION_WORD_REMAP)){
        EXECUTION_REPORT(REPORT_ERROR, num_operands == 3, "function \"%s\" must have no result parameter and three input parameters\n", function);
        check_is_parameter_object_type_remap_weights(function, 1, statement_operands[0], "the remap weights used for remapping");
        check_is_parameter_object_type_field_data(function, 2, statement_operands[1], "the src field of remapping");
        check_is_parameter_object_type_field_data(function, 3, statement_operands[2], "the dst field of remapping");     
        remap_weights = remap_weights_manager->search_remap_weight_of_strategy(statement_operands[0]->object->object_name);
        field_data_src = remap_field_data_manager->search_remap_field_data(statement_operands[1]->object->object_name);
        field_data_dst = remap_field_data_manager->search_remap_field_data(statement_operands[2]->object->object_name);
        remap_weights->do_remap(field_data_src, field_data_dst);
    }
    else if (words_are_the_same(function, FUNCTION_WORD_READ_REMAP_WEIGHTS)) {
        EXECUTION_REPORT(REPORT_ERROR, num_operands == 6, "function \"%s\" must have one result parameter and five input parameters\n", function);
        check_is_parameter_object_type_remap_weights(function, 0, statement_operands[0], "the remap weights to be read from IO file");
        check_is_parameter_object_type_remap_scheme(function, 1, statement_operands[1], "the remap scheme corresponding to the remap weights");
           check_is_parameter_object_type_grid(function, 2, statement_operands[2], "the src grid of remap weights (the grid of src field)\n");
        check_is_parameter_object_type_grid(function, 3, statement_operands[3], "the dst grid of remap weights (the grid of dst field)\n");
        check_is_parameter_object_type_IO(function, 4, statement_operands[4], "the IO file to read remap weights");
        check_is_parameter_string_type(function, 5, statement_operands[5], "the format of remap weights in IO file (SCRIP or C-Coupler)");
        EXECUTION_REPORT(REPORT_ERROR, words_are_the_same(statement_operands[5]->extension_names[0], "SCRIP") || words_are_the_same(statement_operands[5]->extension_names[0], "C-Coupler") ,
                     "the fifth input parameter of function \"%s\" must be a string of \"SCRIP\" or \"C-Coupler\"\n", function);
        remap_weights = new Remap_weight_of_strategy_class(statement_operands[0]->object->object_name,
                                                           statement_operands[1]->object->object_name,
                                                           statement_operands[2]->object->object_name,
                                                           statement_operands[3]->object->object_name,
                                                           statement_operands[4]->object->object_name,
                                                           statement_operands[5]->extension_names[0],
                                                           true);
        remap_weights_of_strategies.push_back(remap_weights);
    }
}


Remap_weight_of_strategy_class *Remap_weight_of_strategy_mgt::search_remap_weight_of_strategy(const char *object_name)
{
    for (int i = 0; i < remap_weights_of_strategies.size(); i ++) 
        if (remap_weights_of_strategies[i]->match_object_name(object_name))
            return remap_weights_of_strategies[i];

    return NULL;
}


Remap_weight_of_strategy_mgt::~Remap_weight_of_strategy_mgt()
{
    for (int i = 0; i < remap_weights_of_strategies.size(); i ++) 
        delete remap_weights_of_strategies[i];
}

