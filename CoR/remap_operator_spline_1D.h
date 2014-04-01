/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file is initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef REMAP_OPERATOR_SPLINE_1D
#define REMAP_OPERATOR_SPLINE_1D


#include "remap_operator_basis.h"


class Remap_operator_spline_1D: public Remap_operator_basis
{
    private:
        bool periodic;
		bool set_periodic;
		bool set_period;
		bool set_enable_extrapolation;
		bool enable_extrapolation;
		bool set_keep_monotonicity;
		bool keep_monotonicity;
		double period;
		double *coord_values_src;
		double *coord_values_dst;
		double *packed_data_values_src;
		double *data_in_monotonicity_range;
		int num_useful_src_cells;
		int array_size_src;
		int *useful_src_cells_global_index;
		int *src_cell_index_left;
		int *src_cell_index_right;
		int *dst_cell_indexes_in_monotonicity_ranges;
		double *array_alpha;
		double *array_mu;
		double *array_lambda;
		double *array_h;
		double *array_d;
		double *temp_array_row;
		double *temp_array_column;
		double *final_factor1;
		double *final_factor2;
		double *final_factor3;
		double *final_factor4;
		double *final_factor5;

		void solve_aperiodic_tridiagonal_system(double*, double*, double*, double*, int);
		void solve_periodic_tridiagonal_system(double*, double*, double*, double*, int);
		void search_src_cells_around_dst_cell(double, int, int, int&, int&);
		void search_src_cells_around_dst_cell_recursively(double, int, int,  int&, int&);
		void compute_remap_weights_of_one_dst_cell(long);
		void allocate_local_arrays();
		Remap_operator_spline_1D() {}

    public:
        Remap_operator_spline_1D(const char*, int, Remap_grid_class **);
        ~Remap_operator_spline_1D();
        void set_parameter(const char *, const char *);
		void calculate_remap_weights();
		void do_remap_values_caculation(double*, double*);
        void do_src_decomp_caculation(bool*, const bool*);
        Remap_operator_basis *duplicate_remap_operator(bool);
        Remap_operator_basis *generate_parallel_remap_operator(Remap_grid_class**, int**);
};


#endif

