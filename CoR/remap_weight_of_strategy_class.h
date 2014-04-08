/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef REMAP_WEIGHT_OF_STRATEGY_CLASS_H
#define REMAP_WEIGHT_OF_STRATEGY_CLASS_H


#include "remap_grid_class.h"
#include "remap_grid_data_class.h"
#include "remap_operator_basis.h"
#include <vector>


class Remap_operator_basis;
class Remap_strategy_class;
class Remap_weight_sparse_matrix;
class Remap_weight_of_strategy_class;


class Remap_weight_of_operator_class
{
    private:
        friend class Remap_weight_of_strategy_class;
        Remap_grid_class *field_data_grid_src;
        Remap_grid_class *field_data_grid_dst;
        Remap_grid_class *operator_grid_src;
        Remap_grid_class *operator_grid_dst;
        Remap_operator_basis *original_remap_operator;
        Remap_operator_basis *duplicated_remap_operator;
        long remap_beg_iter;
        long remap_end_iter;
        
    public: 
        Remap_weight_of_operator_class() {}
        Remap_weight_of_operator_class(Remap_grid_class*, Remap_grid_class*, long, Remap_operator_basis*);
        Remap_weight_of_operator_class(Remap_grid_class*, Remap_grid_class*, long, Remap_operator_basis*, Remap_operator_basis*);
        ~Remap_weight_of_operator_class();
        long get_remap_iter() { return remap_beg_iter; }
        Remap_grid_class *get_field_data_grid_src() { return field_data_grid_src; }
        Remap_grid_class *get_field_data_grid_dst() { return field_data_grid_dst; }
        Remap_weight_of_operator_class *generate_parallel_remap_weights(Remap_grid_class**, int **);
};


class Remap_weight_of_strategy_class
{
    private:
        char object_name[256];
		char input_IO_file_name[256];
		char weight_IO_format[256];
        std::vector<Remap_weight_of_operator_class*> remap_weights_of_operators;
        Remap_grid_class *data_grid_src;
        Remap_grid_class *data_grid_dst;
        Remap_strategy_class *remap_strategy;
		bool include_weight_values;
		bool read_weights;

    public:
        Remap_weight_of_strategy_class(const char*, const char*, const char*, const char*, bool);
        Remap_weight_of_strategy_class() {}
        bool match_object_name(const char*);
        ~Remap_weight_of_strategy_class();

        Remap_grid_class *get_data_grid_src() { return data_grid_src; }
        Remap_grid_class *get_data_grid_dst() { return data_grid_dst; }
        Remap_strategy_class *get_remap_strategy() { return remap_strategy; }
        int get_num_remap_operator_of_weights() { return remap_weights_of_operators.size(); }
        Remap_weight_of_operator_class *get_remap_weight_of_operator(int i) { return remap_weights_of_operators[i]; }
        Remap_operator_basis *get_remap_operator_of_weights(int i) { return remap_weights_of_operators[i]->duplicated_remap_operator; }
        void add_remap_weight_of_operator(Remap_grid_class*, Remap_grid_class*, long, Remap_operator_basis*);
        void do_remap(Remap_grid_data_class*, Remap_grid_data_class*);
        void add_weight_of_operator_class(Remap_weight_of_operator_class *weight_of_operator_class) { remap_weights_of_operators.push_back(weight_of_operator_class); }
        void calculate_src_decomp(Remap_grid_class*, Remap_grid_class*, bool*, const bool*);
        void calculate_src_decomp_recursively(int, Remap_operator_basis*, Remap_grid_data_class*, Remap_grid_data_class*, Remap_grid_data_class*, Remap_grid_data_class*);
        Remap_grid_class **get_remap_related_grids(int&);
        Remap_weight_of_strategy_class *generate_parallel_remap_weights(Remap_grid_class**, Remap_grid_class**, Remap_grid_class**, int **);
        const char *get_object_name() { return object_name; }
		void compute_or_readin_weight_values();
		void set_input_IO_info(const char*, const char*);
		void temporarily_cleanup_memory_space();
};


#endif
