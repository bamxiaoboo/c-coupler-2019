/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef REMAP_OPERATOR_GRID_H
#define REMAP_OPERATOR_GRID_H

#include "remap_grid_class.h"
#include "remap_operator_basis.h"


struct Cell_bounding_box
{
    double* cell_bounds;
    long cell_id;
};


class Cell_lookup_table
{
    private:
        Remap_operator_grid *remap_operator_grid;
        int current_level;
        int num_grid_dimensions;
        long num_child_cell_lookup_table;
        double *child_bound_value_lookup_table;
        Cell_lookup_table **child_cell_lookup_tables;
        long num_cell_bounding_boxes;
        Cell_bounding_box **cell_bounding_boxes;
        double half_cycle_value;
        int num_max_level_of_recursive_search_in_neighbor_cells;

        void initialize_cell_bounding_boxes();
        void recursively_partition_cell_lookup_table();
        long lookup_in_child_bound_value_lookup_table(double);
        long recursively_lookup_in_child_bound_value_lookup_table(double, long, long);
        long recursively_search_in_neighbor_cells_for_sphere_grid(double*, long, int);

    public:
        Cell_lookup_table(Remap_operator_grid*);
        Cell_lookup_table(Cell_lookup_table*);
        ~Cell_lookup_table();
        long search_cell_of_locating_point(double*, bool);
};


class Vertex_values_group
{
    public:
        int num_vertex;
        int num_coords;
        double *vertex_coord_values[256];
        Remap_grid_class *coord_value_grid;
        long *partial_nerghbors_indexes;
        int num_neighbors;

        Vertex_values_group();
        ~Vertex_values_group();
        void generate_vertex_values_of_one_cell_according_to_vertex_values_groups(long, long*, int, int, int, Vertex_values_group**, double**);
};


class Remap_operator_grid 
{
    private:
        friend class Cell_lookup_table;
        
        Remap_operator_basis *remap_operator;
        Remap_grid_class *remap_grid;
        long grid_size;
        int num_grid_dimensions;
        int num_vertexes;
        int num_neighbors;
        std::vector<Remap_grid_data_class*> grid_center_fields;
        std::vector<Remap_grid_data_class*> grid_vertex_fields;
        double *center_coord_values[256];
        double *vertex_coord_values[256];
        bool *mask_values;
        long *cell_nerghbors_indexes;
        int num_vertex_values_groups;
        Vertex_values_group *vertex_values_groups[256];
        Cell_lookup_table *grid_cells_lookup_table;
        bool require_vertex_fields;
        bool is_grid_sphere;
        bool *cell_visiting_mark;
        long *visited_cells_indexes;
        long num_visited_cells;
        Remap_operator_grid *rotated_remap_operator_grid;
        bool is_rotated_grid;
        bool is_src_grid;
        bool *redundant_cell_mark;

        void initialize_for_vertex_coord_values_generation();
        void generate_overall_vertex_coord_values();
        void generate_neighborhood_according_to_vertexes();
        bool two_cells_have_common_bound(long, long, double);
        void add_partial_neighborhood_to_vertex_group(long, long);
        void rotate_sphere_grid();
        void remove_redundant_cells();

    public:
        void update_operator_grid_data();
        Remap_operator_grid(Remap_grid_class*, Remap_operator_basis*, bool, bool);
        ~Remap_operator_grid();

        /* Functions of getting grid properties */
        long get_grid_size() { return grid_size; }
        bool *get_mask_values() { return mask_values; }
        double **get_center_coord_values() { return center_coord_values; }
        double **get_vertex_coord_values() { return vertex_coord_values; }
        int get_num_grid_dimensions() { return num_grid_dimensions; }
        int get_num_vertexes() { return num_vertexes; }
        int get_num_neighbors() { return num_neighbors; }
        long *get_cell_nerghbors_indexes() { return cell_nerghbors_indexes; }
        bool is_cell_visited(long cell_index) { return cell_visiting_mark[cell_index]; }
        long get_num_visited_cells() { return num_visited_cells; }
        bool get_is_rotated_grid() { return is_rotated_grid; }
        Remap_operator_grid *get_rotated_remap_operator_grid() { return rotated_remap_operator_grid; }
        
        void visit_cell(long);
        void clear_cell_visiting_info();
        long search_cell_of_locating_point(double*, bool);
};


#endif
