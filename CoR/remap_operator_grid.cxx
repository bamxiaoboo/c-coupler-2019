/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include "remap_operator_grid.h"
#include "cor_global_data.h"
#include "radix_sort.h"
#include "remap_operator_c_interface.h"


Vertex_values_group::Vertex_values_group()
{
    num_vertex = 0;
    num_coords = 0;
    coord_value_grid = NULL;
    partial_nerghbors_indexes = NULL;
    num_neighbors = 0;
}


Vertex_values_group::~Vertex_values_group()
{
    if (partial_nerghbors_indexes != NULL)
        delete [] partial_nerghbors_indexes;
}


void Vertex_values_group::generate_vertex_values_of_one_cell_according_to_vertex_values_groups(long whole_grid_cell_index,
                                                                                              long *group_grid_cell_index,
                                                                                              int num_vertex_values_groups,
                                                                                              int num_grid_dimensions,
                                                                                              int num_vertexes,
                                                                                              Vertex_values_group **vertex_values_groups,
                                                                                              double **output_vertex_values)
{
    int group_index, i, j, k;
    int vertex_id;
    double stack_values[256][256];
    int stack_top, num_processed_coords, current_order;


    for (k = 0; k < num_grid_dimensions; k ++)
        for (j = 0; j < num_vertexes; j ++)
            output_vertex_values[k][whole_grid_cell_index*num_vertexes+j] = NULL_COORD_VALUE;
    
    num_processed_coords = 0;
    for (group_index = 0, j = 0; group_index < num_vertex_values_groups; group_index ++) 
        for (i = 0; i < vertex_values_groups[group_index]->num_coords; i ++)
            stack_values[num_processed_coords++][0] = vertex_values_groups[group_index]->vertex_coord_values[i][group_grid_cell_index[group_index]*vertex_values_groups[group_index]->num_vertex];
    stack_top = 1;
    num_processed_coords = 0;
    for (group_index = 0; group_index < num_vertex_values_groups; group_index ++) {
        current_order = 1;
        for (vertex_id = 0; vertex_id < vertex_values_groups[group_index]->num_vertex; vertex_id ++) {
            if (vertex_values_groups[group_index]->vertex_coord_values[0][group_grid_cell_index[group_index]*vertex_values_groups[group_index]->num_vertex+vertex_id] == NULL_COORD_VALUE)
                continue;
            if (current_order == 1) 
                for (j = 0, i = 0; j < stack_top; j ++) {
                    for (k = 0; k < num_grid_dimensions; k ++) 
                        output_vertex_values[k][whole_grid_cell_index*num_vertexes+i] = stack_values[k][j];
                    i ++;
                }
            else for (j = stack_top-1, i = 0; j >= 0; j --) {
                    for (k = 0; k < num_grid_dimensions; k ++)
                        output_vertex_values[k][whole_grid_cell_index*num_vertexes+i] = stack_values[k][j];
                    i ++;
                }
            for (j = 0; j < stack_top; j ++) 
                for (k = 0; k < vertex_values_groups[group_index]->num_coords; k ++)
                    output_vertex_values[num_processed_coords+k][whole_grid_cell_index*num_vertexes+j] = vertex_values_groups[group_index]->vertex_coord_values[k][group_grid_cell_index[group_index]*vertex_values_groups[group_index]->num_vertex+vertex_id];
            for (j = 0; j < stack_top; j ++)
                for (k = 0; k < num_grid_dimensions; k ++)
                    stack_values[k][stack_top*vertex_id+j] = output_vertex_values[k][whole_grid_cell_index*num_vertexes+j];
            current_order = -current_order;
        }
        stack_top *= vertex_values_groups[group_index]->num_vertex;
        num_processed_coords += vertex_values_groups[group_index]->num_coords;
    }

    for (j = 0; j < stack_top; j ++) 
        for (k = 0; k < num_grid_dimensions; k ++) 
            output_vertex_values[k][whole_grid_cell_index*num_vertexes+j] = stack_values[k][j];

    EXECUTION_REPORT(REPORT_ERROR, stack_top == num_vertexes, "remap software error1 in generate_overall_vertex_coord_values\n");
    EXECUTION_REPORT(REPORT_ERROR, num_processed_coords == num_grid_dimensions, "remap software error2 in generate_overall_vertex_coord_values\n");
}


Cell_lookup_table::Cell_lookup_table(Remap_operator_grid *remap_operator_grid)
{
    this->remap_operator_grid = (Remap_operator_grid*) remap_operator_grid;
    current_level = 0;
    num_grid_dimensions = remap_operator_grid->num_grid_dimensions;
    num_child_cell_lookup_table = 0;
    child_bound_value_lookup_table = NULL;
    child_cell_lookup_tables = NULL;
    num_cell_bounding_boxes = remap_operator_grid->grid_size;
    num_max_level_of_recursive_search_in_neighbor_cells = 1;
    if (is_coord_unit_degree[current_level]) 
        half_cycle_value = 180;
    else half_cycle_value = 0;
    cell_bounding_boxes = new Cell_bounding_box *[num_cell_bounding_boxes];
    initialize_cell_bounding_boxes();
    recursively_partition_cell_lookup_table();
}


Cell_lookup_table::Cell_lookup_table(Cell_lookup_table *parent)
{
    remap_operator_grid = parent->remap_operator_grid;
    current_level = parent->current_level + 1;
    num_grid_dimensions = parent->num_grid_dimensions;
    num_child_cell_lookup_table = 0;
    child_bound_value_lookup_table = NULL;
    child_cell_lookup_tables = NULL;
    num_cell_bounding_boxes = 0;
    cell_bounding_boxes = NULL;
    half_cycle_value = 0;
    if (current_level < num_grid_dimensions && is_coord_unit_degree[current_level]) 
        half_cycle_value = 180;
}


Cell_lookup_table::~Cell_lookup_table()
{
    if (child_cell_lookup_tables != NULL) {
        for (long i = 0; i < num_child_cell_lookup_table; i ++)
            delete child_cell_lookup_tables[i];
        delete [] child_cell_lookup_tables;
        delete [] child_bound_value_lookup_table;
    }

    if (current_level == 0)
        for (long i = 0; i < num_cell_bounding_boxes; i ++) {
            delete [] cell_bounding_boxes[i]->cell_bounds;
            delete cell_bounding_boxes[i];
        }

    if (cell_bounding_boxes != NULL)
        delete [] cell_bounding_boxes;
}


void Cell_lookup_table::initialize_cell_bounding_boxes()
{
    long i, j, k;
    double cell_vertex_values[2*256];
    
    
    for (j = 0; j < remap_operator_grid->grid_size; j ++) {
        cell_bounding_boxes[j] = new Cell_bounding_box;
        cell_bounding_boxes[j]->cell_bounds = new double [num_grid_dimensions*2];
        cell_bounding_boxes[j]->cell_id = j;
        for (i = 0; i < num_grid_dimensions; i ++)
            for (k = 0; k < remap_operator_grid->num_vertexes; k ++)
                cell_vertex_values[num_grid_dimensions*k+i] = remap_operator_grid->vertex_coord_values[i][j*remap_operator_grid->num_vertexes+k];
        compute_cell_bounding_box(remap_operator_grid->num_vertexes, num_grid_dimensions, cell_vertex_values, cell_bounding_boxes[j]->cell_bounds);
    }
}


void Cell_lookup_table::recursively_partition_cell_lookup_table()
{
    long i, j;
    double sum_bounds_diff, lookup_table_bounds_diff, min_bounds_diff, max_bounds_diff, current_bounds_diff;
    double max_bound_value, min_bound_value;
    long *lower_table_element_ids, *higher_table_element_ids;


    if (current_level >= num_grid_dimensions)
        return;

    if (num_cell_bounding_boxes == 0)
        return;
        
    min_bounds_diff = DEFAULT_FILL_VALUE;
    max_bounds_diff = -min_bounds_diff;
    min_bound_value = min_bounds_diff;
    max_bound_value = max_bounds_diff;
    sum_bounds_diff = 0;
    for (i = 0; i < num_cell_bounding_boxes; i ++) {
        if (cell_bounding_boxes[i]->cell_bounds[current_level*2] == DEFAULT_FILL_VALUE)
            continue;
        if (cell_bounding_boxes[i]->cell_bounds[current_level*2] > cell_bounding_boxes[i]->cell_bounds[current_level*2+1]) {
            current_bounds_diff = 2*half_cycle_value - cell_bounding_boxes[i]->cell_bounds[current_level*2] + cell_bounding_boxes[i]->cell_bounds[current_level*2+1];
            min_bound_value = 0;
            max_bound_value = 2*half_cycle_value;
        }
        else {
            current_bounds_diff = cell_bounding_boxes[i]->cell_bounds[current_level*2+1] - cell_bounding_boxes[i]->cell_bounds[current_level*2];
            if (min_bound_value > cell_bounding_boxes[i]->cell_bounds[current_level*2])
                min_bound_value = cell_bounding_boxes[i]->cell_bounds[current_level*2];
            if (max_bound_value < cell_bounding_boxes[i]->cell_bounds[current_level*2+1])
                max_bound_value = cell_bounding_boxes[i]->cell_bounds[current_level*2+1];
        }
        if (min_bounds_diff > current_bounds_diff)
            min_bounds_diff = current_bounds_diff;
        if (max_bounds_diff < current_bounds_diff)
            max_bounds_diff = current_bounds_diff;
        sum_bounds_diff += fabs(current_bounds_diff);
    }
    
    EXECUTION_REPORT(REPORT_ERROR, min_bound_value <= max_bound_value, "remap software error1 in recursively_partition_cell_lookup_table\n");

    lookup_table_bounds_diff = (sum_bounds_diff/num_cell_bounding_boxes) / 2;
    num_child_cell_lookup_table = (max_bound_value - min_bound_value)/lookup_table_bounds_diff;
    if ((max_bound_value - min_bound_value)/lookup_table_bounds_diff > num_child_cell_lookup_table)
        num_child_cell_lookup_table ++;

    child_cell_lookup_tables = new Cell_lookup_table *[num_child_cell_lookup_table];
    child_bound_value_lookup_table = new double [num_child_cell_lookup_table+1];
    for (i = 0; i < num_child_cell_lookup_table; i ++) {
        child_bound_value_lookup_table[i] = lookup_table_bounds_diff*i + min_bound_value;
        child_cell_lookup_tables[i] = new Cell_lookup_table(this);
    }
    child_bound_value_lookup_table[i] = max_bound_value;
    EXECUTION_REPORT(REPORT_ERROR, child_bound_value_lookup_table[0] <= child_bound_value_lookup_table[i], "remap software error2 in recursively_partition_cell_lookup_table\n");    

    lower_table_element_ids = new long [num_cell_bounding_boxes];
    higher_table_element_ids = new long [num_cell_bounding_boxes];
    for (i = 0; i < num_cell_bounding_boxes; i ++) {
        if (cell_bounding_boxes[i]->cell_bounds[current_level*2] == DEFAULT_FILL_VALUE)
            continue;
        lower_table_element_ids[i] = lookup_in_child_bound_value_lookup_table(cell_bounding_boxes[i]->cell_bounds[current_level*2]);
        higher_table_element_ids[i] = lookup_in_child_bound_value_lookup_table(cell_bounding_boxes[i]->cell_bounds[current_level*2+1]);
        EXECUTION_REPORT(REPORT_ERROR, lower_table_element_ids[i] < num_child_cell_lookup_table && lower_table_element_ids[i] >= 0 &&
                     higher_table_element_ids[i] < num_child_cell_lookup_table && higher_table_element_ids[i] >= 0, 
                     "remap software error3 in recursively_partition_cell_lookup_table\n");
        if (lower_table_element_ids[i] <= higher_table_element_ids[i])
            for (j = lower_table_element_ids[i]; j <= higher_table_element_ids[i]; j ++)
                child_cell_lookup_tables[j]->num_cell_bounding_boxes ++;
        else {
            for (j = lower_table_element_ids[i]; j < num_child_cell_lookup_table; j ++)
                child_cell_lookup_tables[j]->num_cell_bounding_boxes ++;
            for (j = 0; j <= higher_table_element_ids[i]; j ++)
                child_cell_lookup_tables[j]->num_cell_bounding_boxes ++;
        }
    }
    
    for (i = 0; i < num_child_cell_lookup_table; i ++) {
        child_cell_lookup_tables[i]->cell_bounding_boxes = new Cell_bounding_box *[child_cell_lookup_tables[i]->num_cell_bounding_boxes];
        child_cell_lookup_tables[i]->num_cell_bounding_boxes = 0;
    }

    for (i = 0; i < num_cell_bounding_boxes; i ++) {
        if (cell_bounding_boxes[i]->cell_bounds[current_level*2] == DEFAULT_FILL_VALUE)
            continue;
        if (lower_table_element_ids[i] <= higher_table_element_ids[i])
            for (j = lower_table_element_ids[i]; j <= higher_table_element_ids[i]; j ++)
                child_cell_lookup_tables[j]->cell_bounding_boxes[child_cell_lookup_tables[j]->num_cell_bounding_boxes ++] = cell_bounding_boxes[i];
        else {
            for (j = lower_table_element_ids[i]; j < num_child_cell_lookup_table; j ++)
                child_cell_lookup_tables[j]->cell_bounding_boxes[child_cell_lookup_tables[j]->num_cell_bounding_boxes ++] = cell_bounding_boxes[i];
            for (j = 0; j <= higher_table_element_ids[i]; j ++)
                child_cell_lookup_tables[j]->cell_bounding_boxes[child_cell_lookup_tables[j]->num_cell_bounding_boxes ++] = cell_bounding_boxes[i];
        }        
    }

    delete [] lower_table_element_ids;
    delete [] higher_table_element_ids;

    for (i = 0; i < num_child_cell_lookup_table; i ++)
        child_cell_lookup_tables[i]->recursively_partition_cell_lookup_table();
}


long Cell_lookup_table::lookup_in_child_bound_value_lookup_table(double bound_value)
{
    if (num_cell_bounding_boxes == 0)
        return -1;
    if (bound_value < child_bound_value_lookup_table[0] || bound_value > child_bound_value_lookup_table[num_child_cell_lookup_table]) 
        return -1;

    return recursively_lookup_in_child_bound_value_lookup_table(bound_value, 0, num_child_cell_lookup_table);
}


long Cell_lookup_table::recursively_lookup_in_child_bound_value_lookup_table(double bound_value, long begin, long end)
{
    long middle;


    middle = (begin+end) / 2;
    if (bound_value >= child_bound_value_lookup_table[middle] && bound_value <= child_bound_value_lookup_table[middle+1]) 
        return middle;

    if (bound_value < child_bound_value_lookup_table[middle])
        return recursively_lookup_in_child_bound_value_lookup_table(bound_value, begin, middle);
    else return recursively_lookup_in_child_bound_value_lookup_table(bound_value, middle, end);
}


long Cell_lookup_table::recursively_search_in_neighbor_cells_for_sphere_grid(double *point_coord_values, long current_cell_index, int num_level)
{
    int num_vertexes, i;
    long temp_cell_index;
    double cell_vertex_coord1_values[256], cell_vertex_coord2_values[256];


    if (num_level > num_max_level_of_recursive_search_in_neighbor_cells)
        return -1;

    for (i = 0, num_vertexes = 0; i < remap_operator_grid->get_num_vertexes(); i ++) {
        if (remap_operator_grid->get_vertex_coord_values()[0][remap_operator_grid->get_num_vertexes()*current_cell_index+i] == NULL_COORD_VALUE)
            continue;
        cell_vertex_coord1_values[num_vertexes] = remap_operator_grid->get_vertex_coord_values()[0][remap_operator_grid->get_num_vertexes()*current_cell_index+i];
        cell_vertex_coord2_values[num_vertexes] = remap_operator_grid->get_vertex_coord_values()[1][remap_operator_grid->get_num_vertexes()*current_cell_index+i];
        num_vertexes ++;
    }

    if (is_point_in_2D_cell(point_coord_values[0], 
                            point_coord_values[1], 
                            cell_vertex_coord1_values, 
                            cell_vertex_coord2_values,
                            num_vertexes,
                            true, true, true))
        return current_cell_index;
    
    for (i = 0; i < remap_operator_grid->get_num_neighbors(); i ++) {
        temp_cell_index = remap_operator_grid->get_cell_nerghbors_indexes()[remap_operator_grid->get_num_neighbors()*current_cell_index+i];
        if (temp_cell_index != -1 && (temp_cell_index=recursively_search_in_neighbor_cells_for_sphere_grid(point_coord_values, temp_cell_index, num_level+1)) != -1)
            return temp_cell_index;
    }

    return -1;
}


long Cell_lookup_table::search_cell_of_locating_point(double *point_coord_values, bool accurately_match)
{
    long i, j, child_id;
    Cell_lookup_table *current_cell_lookup_table;
    long cell_id;


    current_cell_lookup_table = this;
    for (i = 0; i < num_grid_dimensions; i ++) {
        child_id = current_cell_lookup_table->lookup_in_child_bound_value_lookup_table(point_coord_values[i]);
        if (child_id == -1)
            return -1;
        current_cell_lookup_table = current_cell_lookup_table->child_cell_lookup_tables[child_id];
    }

    for (i = 0; i < current_cell_lookup_table->num_cell_bounding_boxes; i ++) {
        for (j = 0; j < num_grid_dimensions; j ++) {
            if (current_cell_lookup_table->cell_bounding_boxes[i]->cell_bounds[2*j] < current_cell_lookup_table->cell_bounding_boxes[i]->cell_bounds[2*j+1]) {
                if (point_coord_values[j] < current_cell_lookup_table->cell_bounding_boxes[i]->cell_bounds[2*j] || point_coord_values[j] > current_cell_lookup_table->cell_bounding_boxes[i]->cell_bounds[2*j+1]) 
                    break;
            }
            else {
                if (point_coord_values[j] < current_cell_lookup_table->cell_bounding_boxes[i]->cell_bounds[2*j] && point_coord_values[j] > current_cell_lookup_table->cell_bounding_boxes[i]->cell_bounds[2*j+1])
                    break;
            }
        }
        if (j == num_grid_dimensions) {
            cell_id = current_cell_lookup_table->cell_bounding_boxes[i]->cell_id;
            if (!accurately_match)
                return cell_id;
            if (num_grid_dimensions == 1)
                return cell_id;
            else if (num_grid_dimensions == 2) {
                if (is_point_in_2D_cell(point_coord_values[0],
                                        point_coord_values[1], 
                                        remap_operator_grid->vertex_coord_values[0]+cell_id*remap_operator_grid->num_vertexes,
                                        remap_operator_grid->vertex_coord_values[1]+cell_id*remap_operator_grid->num_vertexes,
                                        remap_operator_grid->num_vertexes,
                                        true, true,
                                        remap_operator_grid->is_grid_sphere))
                    return cell_id;
            }
            else EXECUTION_REPORT(REPORT_ERROR, false, "remap software error1 in search_cell_of_locating_point\n");
        }
    }

    if (accurately_match && remap_operator_grid->is_grid_sphere)
        for (i = 0; i < current_cell_lookup_table->num_cell_bounding_boxes; i ++)
            if ((cell_id = recursively_search_in_neighbor_cells_for_sphere_grid(point_coord_values, current_cell_lookup_table->cell_bounding_boxes[i]->cell_id, 0)) != -1)
                return cell_id;

    return -1;
}


Remap_operator_grid::Remap_operator_grid(Remap_grid_class *remap_grid, Remap_operator_basis *remap_operator, bool is_src_grid, bool is_rotated_grid)
{
    int num_leaf_grids;
    long i, j;
    Remap_grid_class *leaf_grids[256];
    Remap_grid_data_class *current_grid_center_field;


    this->remap_operator = remap_operator;
    this->remap_grid = remap_grid;
    this->num_grid_dimensions = remap_grid->num_dimensions;
    this->grid_size = remap_grid->grid_size;
    this->require_vertex_fields = false;
    this->num_vertexes = 0;
    this->num_neighbors = 0;
    this->num_vertex_values_groups = 0;
    this->cell_nerghbors_indexes = NULL;
    this->grid_cells_lookup_table = NULL;
    this->rotated_remap_operator_grid = NULL;
    this->is_src_grid = is_src_grid;
    this->is_rotated_grid = is_rotated_grid;
    this->redundant_cell_mark = remap_grid->redundant_cell_mark;
    this->cell_visiting_mark = new bool [grid_size];
    this->is_grid_sphere = remap_grid->get_is_sphere_grid();
    for (i = 0; i < grid_size; i ++)
        cell_visiting_mark[i] = false;
    visited_cells_indexes = new long [grid_size];
    num_visited_cells = 0;
    for (i = 0; i < 256; i ++) {
        center_coord_values[i] = NULL;
        vertex_coord_values[i] = NULL;
    }
    if (remap_grid->grid_mask_field != NULL)
        mask_values = (bool*) remap_grid->grid_mask_field->grid_data_field->data_buf;
    else mask_values = NULL;
    
    remap_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, remap_grid);
    for (i = 0; i < num_leaf_grids; i ++)
        if (words_are_the_same(leaf_grids[i]->get_coord_unit(), COORD_UNIT_DEGREES))
            is_coord_unit_degree[i] = true;
        else is_coord_unit_degree[i] = false;

    if (remap_operator->get_is_operator_regridding()) {
        for (i = 0; i < num_leaf_grids; i ++) {
            if (leaf_grids[i]->grid_center_fields.size() == 0)
                current_grid_center_field = leaf_grids[i]->get_grid_center_field();
            else current_grid_center_field = leaf_grids[i]->grid_center_fields[0];
            EXECUTION_REPORT(REPORT_ERROR, current_grid_center_field != NULL, "remap software error1 in Remap_operator_grid\n");
            if (num_leaf_grids == 1)
                grid_center_fields.push_back(current_grid_center_field);
            else grid_center_fields.push_back(remap_grid->expand_to_generate_full_coord_value(current_grid_center_field));
            center_coord_values[i] = (double *) grid_center_fields[i]->grid_data_field->data_buf;
        }
    }

    if (remap_operator->get_num_dimensions() == 1)
        require_vertex_fields = remap_operator->does_require_grid_vertex_values();
    else require_vertex_fields = remap_operator->does_require_grid_vertex_values() || (remap_operator->get_is_operator_regridding() && is_src_grid);
    if (require_vertex_fields) 
        initialize_for_vertex_coord_values_generation();

    if (is_grid_sphere && !is_rotated_grid && remap_operator->get_is_operator_regridding()) {
        rotated_remap_operator_grid = new Remap_operator_grid(remap_grid, remap_operator, is_src_grid, true);
    }
}


Remap_operator_grid::~Remap_operator_grid()
{
    delete [] cell_visiting_mark;
    delete [] visited_cells_indexes;

    if (grid_center_fields.size() > 1)
        for (int i = 0; i < grid_center_fields.size(); i ++) 
            delete grid_center_fields[i];

    if (num_vertexes > 0)
        for (int i = 0; i < num_grid_dimensions; i ++)
            delete [] vertex_coord_values[i];

    if (cell_nerghbors_indexes != NULL)
        delete [] cell_nerghbors_indexes;

    for (int i = 0; i < num_vertex_values_groups; i ++)
        delete vertex_values_groups[i];

    if (grid_cells_lookup_table != NULL)
        delete grid_cells_lookup_table;

    if (rotated_remap_operator_grid != NULL)
        delete rotated_remap_operator_grid;
}


void Remap_operator_grid::update_operator_grid_data()
{
    if (require_vertex_fields) 
        generate_overall_vertex_coord_values();

    if (is_src_grid)
        remove_redundant_cells();
    if (remap_operator->get_num_dimensions() > 1 && remap_operator->does_require_grid_cell_neighborhood() && is_src_grid)
        generate_neighborhood_according_to_vertexes();

    if (is_rotated_grid)
        rotate_sphere_grid();
    
    if (grid_cells_lookup_table != NULL) {
        delete grid_cells_lookup_table;
        grid_cells_lookup_table = NULL;
    }
    if (remap_operator->get_num_dimensions() > 1 && remap_operator->get_is_operator_regridding() && is_src_grid) {
        grid_cells_lookup_table = new Cell_lookup_table(this);
    }

    if (rotated_remap_operator_grid != NULL)
        rotated_remap_operator_grid->update_operator_grid_data();
}


void Remap_operator_grid::remove_redundant_cells()
{
    long i, j, k;


    if (center_coord_values[0] == NULL || redundant_cell_mark == NULL)
        return;

    for (i = 0; i < grid_size; i ++)
        if (redundant_cell_mark[i]) {
            for (j = 0; j < num_grid_dimensions; j ++) {
                center_coord_values[j][i] = NULL_COORD_VALUE;
                if (vertex_coord_values[j] != NULL)
                    for (k = 0; k < num_vertexes; k ++)
                        vertex_coord_values[j][i*num_vertexes+k] = NULL_COORD_VALUE;
            }
        }
}


void Remap_operator_grid::rotate_sphere_grid()
{
    int lon_index, lat_index, num_leaf_grids;
    long i, j, next_j, array_size;
    Remap_grid_class *leaf_grids[256];
    double *lon_data_array, *lat_data_array, lon_diff_value;
    double eps = 1.0e-5;
    

    remap_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, remap_grid);
    for (i = 0; i < num_leaf_grids; i ++)
        if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LON))
            lon_index = i;
        else if (words_are_the_same(leaf_grids[i]->get_coord_label(), COORD_LABEL_LAT))
            lat_index = i;

    array_size = grid_size*num_vertexes;
    lon_data_array = vertex_coord_values[lon_index];
    lat_data_array = vertex_coord_values[lat_index];
    for (i = 0; i < array_size; i ++)
        if (lon_data_array[i] != NULL_COORD_VALUE)
            rotate_sphere_coordinate(lon_data_array[i], lat_data_array[i], lon_data_array[i], lat_data_array[i]);
    for (i = 0; i < grid_size; i ++) {
        for (j = 0; j < num_vertexes; j ++) {
            if (lon_data_array[i*num_vertexes+j] == NULL_COORD_VALUE)
                continue;
            next_j = (j+1)%num_vertexes;
            while(lon_data_array[i*num_vertexes+next_j] == NULL_COORD_VALUE)
                next_j = (next_j+1)%num_vertexes;
            lon_diff_value = compute_difference_of_two_coord_values(lon_data_array[i*num_vertexes+j], lon_data_array[i*num_vertexes+next_j], lon_index);
            if (fabs(lon_diff_value) < eps && fabs(lat_data_array[i*num_vertexes+j] - lat_data_array[i*num_vertexes+next_j]) < eps) {
                lon_data_array[i*num_vertexes+next_j] = NULL_COORD_VALUE;
                lat_data_array[i*num_vertexes+next_j] = NULL_COORD_VALUE;
            }
        }
    }

    array_size = grid_center_fields[lon_index]->get_grid_data_field()->required_data_size;
    lon_data_array = (double*) grid_center_fields[lon_index]->get_grid_data_field()->data_buf;
    lat_data_array = (double*) grid_center_fields[lat_index]->get_grid_data_field()->data_buf;
    for (i = 0; i < array_size; i ++) 
        if (lon_data_array[i] != NULL_COORD_VALUE)    
            rotate_sphere_coordinate(lon_data_array[i], lat_data_array[i], lon_data_array[i], lat_data_array[i]);    
}


void Remap_operator_grid::initialize_for_vertex_coord_values_generation()
{
    int num_leaf_grids;
    Remap_grid_class *leaf_grids[256], *super_grid;
    long i, j, last_index;
    Remap_grid_data_class *current_vertex_field;


    remap_grid->get_leaf_grids(&num_leaf_grids, leaf_grids, remap_grid);    
    for (i = 0; i < num_leaf_grids; i ++) {
        if (leaf_grids[i]->grid_vertex_fields.size() == 0)
            current_vertex_field = leaf_grids[i]->get_grid_vertex_field();
        else current_vertex_field = leaf_grids[i]->grid_vertex_fields[0];
		EXECUTION_REPORT(REPORT_ERROR, current_vertex_field != NULL, 
						 "The vertex coordinate values of the grid %s are missing, which are not specified by users or generated automatically",
						 remap_grid->get_grid_name());
        grid_vertex_fields.push_back(current_vertex_field);
    }

    this->num_vertexes = 1;
    for (i = 0; i < num_leaf_grids; i ++) {
        if (leaf_grids[i] == NULL)
            continue;
        super_grid = leaf_grids[i]->get_super_grid_of_setting_coord_values();
        if (super_grid->num_dimensions == 1)
            EXECUTION_REPORT(REPORT_ERROR, super_grid->num_vertexes == 2, "remap software error2 in initialize_for_vertex_coord_values_generation\n");
        this->num_vertexes *= super_grid->num_vertexes;
        vertex_values_groups[num_vertex_values_groups] = new Vertex_values_group();
        vertex_values_groups[num_vertex_values_groups]->num_vertex = super_grid->num_vertexes;
        vertex_values_groups[num_vertex_values_groups]->num_coords = 1;
        vertex_values_groups[num_vertex_values_groups]->coord_value_grid = super_grid;
        vertex_values_groups[num_vertex_values_groups]->vertex_coord_values[0] = (double*) grid_vertex_fields[i]->grid_data_field->data_buf;
        last_index = i;
        for (j = i+1; j < num_leaf_grids; j ++) {
            if (leaf_grids[j] == NULL)
                continue;
            if (leaf_grids[j]->get_super_grid_of_setting_coord_values() == super_grid) {
                EXECUTION_REPORT(REPORT_ERROR, last_index+1 == j, "remap software error3 in initialize_for_vertex_coord_values_generation\n");
                vertex_values_groups[num_vertex_values_groups]->vertex_coord_values[vertex_values_groups[num_vertex_values_groups]->num_coords++] = (double*) grid_vertex_fields[j]->grid_data_field->data_buf;
                last_index = j;
                leaf_grids[j] = NULL;
            }
        }
        num_vertex_values_groups ++;
    }

    for (i = 0; i < num_grid_dimensions; i ++)
        vertex_coord_values[i] = new double [grid_size*num_vertexes];

    num_neighbors = num_vertexes;
    cell_nerghbors_indexes = new long [num_neighbors*grid_size];
}


void Remap_operator_grid::generate_overall_vertex_coord_values()
{
    long whole_grid_cell_index, group_grid_cell_index[256];
    int i, j;


    if (num_vertex_values_groups == 1) {
        for (i = 0; i < vertex_values_groups[0]->num_coords; i ++)
            memcpy(vertex_coord_values[i], vertex_values_groups[0]->vertex_coord_values[i], vertex_values_groups[0]->coord_value_grid->grid_size*vertex_values_groups[0]->num_vertex*sizeof(double));
        return;
    }

    for (i = 0; i < num_vertex_values_groups; i ++)
        group_grid_cell_index[i] = 0;
    for (whole_grid_cell_index = 0; whole_grid_cell_index < grid_size; whole_grid_cell_index ++) {
        vertex_values_groups[0]->generate_vertex_values_of_one_cell_according_to_vertex_values_groups(whole_grid_cell_index,
                                                                                                      group_grid_cell_index, 
                                                                                                      num_vertex_values_groups,
                                                                                                      num_grid_dimensions,
                                                                                                      num_vertexes,
                                                                                                      vertex_values_groups,
                                                                                                      vertex_coord_values);
        group_grid_cell_index[0] ++;
        for (i = 0; i < num_vertex_values_groups; i ++) 
            if (group_grid_cell_index[i] == vertex_values_groups[i]->coord_value_grid->grid_size) {
                group_grid_cell_index[i] = 0;
                group_grid_cell_index[i+1] ++;
            }
            else break;
    }
}


void Remap_operator_grid::generate_neighborhood_according_to_vertexes()
{
    long i, j, k, l;
    long *vertex_cell_index;
    Radix_sort<double, long> *radix_sort;

    
    for (i = 0; i < grid_size*num_neighbors; i ++)
        cell_nerghbors_indexes[i] = -1;
    
    vertex_cell_index = new long [grid_size*num_vertexes];
    for (i = 0; i < grid_size; i ++)
        for (j = 0; j < num_vertexes; j ++)
            vertex_cell_index[i*num_vertexes+j] = i;
    radix_sort = new Radix_sort<double, long>(vertex_coord_values, 
                                              num_grid_dimensions, 
                                              vertex_cell_index,
                                              grid_size*num_vertexes,
                                              TOLERABLE_ERROR);
    radix_sort->do_radix_sort();

    for (i = 0; i < grid_size*num_vertexes; i ++) {
        if (radix_sort->radix_values[0][i] == NULL_COORD_VALUE)
            continue;
        for (j = i+1; j < grid_size*num_vertexes; j ++) {
            for (k = 0; k < num_grid_dimensions; k ++)
                if (fabs(radix_sort->radix_values[k][i] - radix_sort->radix_values[k][j]) > TOLERABLE_ERROR)
                    break;
            if (k < num_grid_dimensions)
                break;
        }
        if (i+1 == j)
            continue;
        for (l = i; l < j; l ++) {
            if (center_coord_values[0][radix_sort->content[l]] == NULL_COORD_VALUE)
                continue;
            for (k = l+1; k < j; k ++) {
                if (center_coord_values[0][radix_sort->content[k]] == NULL_COORD_VALUE)
                    continue;
                EXECUTION_REPORT(REPORT_ERROR, radix_sort->content[l] != radix_sort->content[k], "remap software error1 in generate_neighborhood_according_to_vertexes\n");
                if (two_cells_have_common_bound(radix_sort->content[l], radix_sort->content[k], TOLERABLE_ERROR)) 
                    add_partial_neighborhood_to_vertex_group(radix_sort->content[l], radix_sort->content[k]);
            }
        }
        i = j - 1;
    }

    delete [] vertex_cell_index;
    delete radix_sort;
}


bool Remap_operator_grid::two_cells_have_common_bound(long cell_id1, 
                                                      long cell_id2, 
                                                      double tolerable_error)
{
    int i, j, k, num_common_vertex;

    if (cell_id1 == cell_id2)
        return false;
    
    for (i = 0, num_common_vertex = 0; i < num_vertexes; i ++) {
        if (vertex_coord_values[0][cell_id1*num_vertexes+i] == NULL_COORD_VALUE)
            continue;
        for (j = 0; j < num_vertexes; j ++) {
            if (vertex_coord_values[0][cell_id2*num_vertexes+j] == NULL_COORD_VALUE)
                continue;            
            for (k = 0; k < num_grid_dimensions; k ++)
                if (fabs(vertex_coord_values[k][cell_id1*num_vertexes+i] - vertex_coord_values[k][cell_id2*num_vertexes+j]) > tolerable_error)
                    break;
            if (k == num_grid_dimensions) {
                num_common_vertex ++;
                break;
            }
        }
    }

    EXECUTION_REPORT(REPORT_ERROR, cell_id1 != cell_id2,
                 "remap software error1 in two_cells_have_common_bound\n");

    return num_common_vertex >= num_grid_dimensions;
}


void Remap_operator_grid::add_partial_neighborhood_to_vertex_group(long cell_id1, long cell_id2)
{
    int i, j;


    for (i = 0; i < num_neighbors; i ++)
        if (cell_nerghbors_indexes[cell_id1*num_neighbors+i] == (long) -1 ||
            cell_nerghbors_indexes[cell_id1*num_neighbors+i] == cell_id2)
            break;
    for (j = 0; j < num_neighbors; j ++) 
        if (cell_nerghbors_indexes[cell_id2*num_neighbors+j] == (long) -1 ||
            cell_nerghbors_indexes[cell_id2*num_neighbors+j] == cell_id1)
            break;

    EXECUTION_REPORT(REPORT_ERROR, i < num_neighbors && j < num_neighbors,
                 "remap software error1 in add_partial_neighborhood_to_vertex_group\n");
    if (cell_nerghbors_indexes[cell_id1*num_neighbors+i] == (long) -1)
        EXECUTION_REPORT(REPORT_ERROR, cell_nerghbors_indexes[cell_id2*num_neighbors+j] == (long) -1,
                     "remap software error2 in add_partial_neighborhood_to_vertex_group\n");
    else EXECUTION_REPORT(REPORT_ERROR, cell_nerghbors_indexes[cell_id2*num_neighbors+j] == cell_id1 &&
                      cell_nerghbors_indexes[cell_id1*num_neighbors+i] == cell_id2,
                      "remap software error3 in add_partial_neighborhood_to_vertex_group\n");

    if (cell_nerghbors_indexes[cell_id1*num_neighbors+i] == (long) -1) {
        cell_nerghbors_indexes[cell_id1*num_neighbors+i] = cell_id2;
        cell_nerghbors_indexes[cell_id2*num_neighbors+j] = cell_id1;
    }
}


long Remap_operator_grid::search_cell_of_locating_point(double *point_coord_values, bool accurately_match)
{
    long i;

    
    if (this->num_grid_dimensions == 1) {
        for (i = 0; i < grid_size - 1; i ++)
            if (compute_difference_of_two_coord_values(center_coord_values[0][i],point_coord_values[0],0) <= 0 && 
                compute_difference_of_two_coord_values(center_coord_values[0][i+1],point_coord_values[0],0) >= 0 ||
                compute_difference_of_two_coord_values(center_coord_values[0][i],point_coord_values[0],0) >= 0 && 
                compute_difference_of_two_coord_values(center_coord_values[0][i+1],point_coord_values[0],0) <= 0)
                return i;
        if (compute_difference_of_two_coord_values(center_coord_values[0][0],center_coord_values[0][grid_size-1],0) <= 0)
            if (compute_difference_of_two_coord_values(point_coord_values[0],center_coord_values[0][0],0) <= 0)
                return 0;
            else return grid_size - 1;
        else if (compute_difference_of_two_coord_values(point_coord_values[0],center_coord_values[0][0],0) >= 0)
            return 0;
        else return grid_size - 1;
    }
    else return grid_cells_lookup_table->search_cell_of_locating_point(point_coord_values, accurately_match);
}


void Remap_operator_grid::visit_cell(long cell_index)
{
    EXECUTION_REPORT(REPORT_ERROR, !cell_visiting_mark[cell_index],
                 "remap software error in visit_cell");

    cell_visiting_mark[cell_index] = true;
    visited_cells_indexes[num_visited_cells++] = cell_index;
}


void Remap_operator_grid::clear_cell_visiting_info()
{
    for (long i = 0; i < num_visited_cells; i ++)
        cell_visiting_mark[visited_cells_indexes[i]] = false;
    num_visited_cells = 0;
}


