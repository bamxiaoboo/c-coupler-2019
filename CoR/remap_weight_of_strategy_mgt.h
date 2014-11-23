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


class Remap_weight_of_operator_mgt
{
	private: 
		std::vector<Remap_weight_of_operator_class *> remap_weights_of_operators;

	public:
		Remap_weight_of_operator_mgt() {}
		~Remap_weight_of_operator_mgt();
		void add_remap_weights_of_operator(Remap_weight_of_operator_class*);
		Remap_weight_of_operator_class *search_remap_weights_of_operator(Remap_grid_class*, Remap_grid_class*, Remap_operator_basis*);
};


class Remap_weight_of_strategy_mgt
{
    private:
        std::vector<Remap_weight_of_strategy_class*> remap_weights_of_strategies;

    public:
        Remap_weight_of_strategy_mgt() {}
        ~Remap_weight_of_strategy_mgt();
        void execute(const char*, Remap_statement_operand **, int);
        Remap_weight_of_strategy_class *search_remap_weight_of_strategy(const char*);
};

#endif
