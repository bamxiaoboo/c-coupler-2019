/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef REMAP_WEIGHT_OF_STRATEGY_MGT_H
#define REMAP_WEIGHT_OF_STRATEGY_MGT_H


#include "remap_weight_of_strategy_class.h"
#include "remap_statement_operand.h"
#include <vector>


class Remap_weight_of_strategy_mgt
{
    private:
        std::vector<Remap_weight_of_strategy_class*> remap_weights_of_strategies;

    public:
        Remap_weight_of_strategy_mgt() {}
        ~Remap_weight_of_strategy_mgt();
        void execute(const char*, Remap_statement_operand **, int);
        Remap_weight_of_strategy_class *search_remap_weight_of_strategy(const char*, bool);
};

#endif
