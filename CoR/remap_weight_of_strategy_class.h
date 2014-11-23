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
class Remap_weight_of_operator_class;


class Remap_weight_of_operator_instance_class
{
    private:
        friend class Remap_weight_of_strategy_class;
		friend class Remap_weight_of_operator_class;
        Remap_grid_class *field_data_grid_src;
        Remap_grid_class *field_data_grid_dst;
        Remap_grid_class *operator_grid_src;
        Remap_grid_class *operator_grid_dst;
        Remap_operator_basis *original_remap_operator;
        Remap_operator_basis *duplicated_remap_operator;
        long remap_beg_iter;
        long remap_end_iter;
        
    public: 
        Remap_weight_of_operator_instance_class() {}
        Remap_weight_of_operator_instance_class(Remap_grid_class*, Remap_grid_class*, long, Remap_operator_basis*);
        Remap_weight_of_operator_instance_class(Remap_grid_class*, Remap_grid_class*, long, Remap_operator_basis*, Remap_operator_basis*);
        ~Remap_weight_of_operator_instance_class();
        long get_remap_begin_iter() { return remap_beg_iter; }
		long get_remap_end_iter() { return remap_end_iter; }
        Remap_grid_class *get_field_data_grid_src() { return field_data_grid_src; }
        Remap_grid_class *get_field_data_grid_dst() { return field_data_grid_dst; }
        Remap_weight_of_operator_instance_class *generate_parallel_remap_weights(Remap_grid_class**, int **);
};


class Remap_weight_of_operator_class
{
    private:
        friend class Remap_weight_of_strategy_class;
        Remap_grid_class *field_data_grid_src;
        Remap_grid_class *field_data_grid_dst;
        Remap_grid_class *operator_grid_src;
        Remap_grid_class *operator_grid_dst;
        Remap_operator_basis *original_remap_operator;
		std::vector<Remap_weight_of_operator_instance_class*> remap_weights_of_operator_instances;
        
    public: 
        Remap_weight_of_operator_class(Remap_grid_class*, Remap_grid_class*, Remap_operator_basis*, Remap_grid_class*, Remap_grid_class*);
        ~Remap_weight_of_operator_class();
        Remap_grid_class *get_field_data_grid_src() { return field_data_grid_src; }
        Remap_grid_class *get_field_data_grid_dst() { return field_data_grid_dst; }
		void calculate_src_decomp(Remap_grid_data_class*, Remap_grid_data_class*);
        Remap_weight_of_operator_class *generate_parallel_remap_weights(Remap_grid_class**, Remap_grid_class**, int **, int &, Remap_weight_of_strategy_class*);
        void do_remap(Remap_grid_data_class*, Remap_grid_data_class*);
		void add_remap_weight_of_operator_instance(Remap_weight_of_operator_instance_class *);
		Remap_operator_basis *get_original_remap_operator() { return original_remap_operator; }
};


class Remap_weight_of_strategy_class
{
    private:
        char object_name[256];
		std::vector<Remap_weight_of_operator_class*> remap_weights_of_operators;
        Remap_grid_class *data_grid_src;
        Remap_grid_class *data_grid_dst;
        Remap_strategy_class *remap_strategy;
		bool dynamic_vertical_remapping_weights_src;
		bool dynamic_vertical_remapping_weights_dst;
		bool public_remap_weights_of_operators;

		void read_grid_info_from_array(Remap_grid_class*, bool, const char *, long&, long);
		void read_data_from_array(void*, int, const char*, long&, long, bool);
		void read_remap_operator_instance_from_array(Remap_grid_class*, Remap_grid_class*, Remap_grid_class*, Remap_grid_class*, Remap_operator_basis*, long, long, const char*, long&, long, bool);
		void write_grid_info_into_array(Remap_grid_class*, bool, char **, long&, long &);
		void write_data_into_array(void*, int, char**, long&, long &);

    public:
        Remap_weight_of_strategy_class(const char*, const char*, const char*, const char*, const char*, const char*, bool);
		void set_basic_fields(const char*, Remap_strategy_class*, Remap_grid_class*, Remap_grid_class*);
		void initialize_object();
        Remap_weight_of_strategy_class() { initialize_object(); }
        bool match_object_name(const char*);
        ~Remap_weight_of_strategy_class();

        Remap_grid_class *get_data_grid_src() { return data_grid_src; }
        Remap_grid_class *get_data_grid_dst() { return data_grid_dst; }
        Remap_strategy_class *get_remap_strategy() { return remap_strategy; }
        Remap_operator_basis *get_unique_remap_operator_of_weights();
        void add_remap_weight_of_operator_instance(Remap_grid_class*, Remap_grid_class*, long, Remap_operator_basis*);
		void update_vertical_remap_weights_of_dynamic_sigma_grid(Remap_grid_data_class*, Remap_grid_data_class*);
        void do_remap(Remap_grid_data_class*, Remap_grid_data_class*);
        void add_remap_weight_of_operator_instance(Remap_weight_of_operator_instance_class *);
        void calculate_src_decomp(Remap_grid_class*, Remap_grid_class*, long*, const long*);
        Remap_grid_class **get_remap_related_grids(int&);
        Remap_weight_of_strategy_class *generate_parallel_remap_weights(Remap_grid_class**, Remap_grid_class**, int **);
        const char *get_object_name() { return object_name; }
		void write_remap_weights_into_array(char**, long&, bool);
		void read_remap_weights_from_array(const char*, long, bool, Remap_grid_class**, bool);
		void check_remap_weights_format();
		void add_remap_weight_of_operators_to_manager(bool);
};


#endif
