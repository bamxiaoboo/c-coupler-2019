/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file is initially finished by Mr. Yufeng Zhou,
  *  and then upgraded and merged into CoR by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#include <stdexcept>
#include <iomanip>
#include <cassert>
#include <sys/time.h>
#include <list>
#include <math.h>
#include <stdlib.h>
#include "delaunay_voronoi.h"
#include "execution_report.h"
#include "remap_common_utils.h"
#include "remap_operator_c_interface.h"
#include "remap_grid_class.h"
#include "remap_common_utils.h"
#include "remap_utils_nearest_points.h"


#define e 1.0e-10


static inline double sind(double x)
{
	return sinl(x * PI / 180);
}


static inline double cosd(double x)
{
	return cosl(x * PI / 180);
}


double det(const Point &pt1, const Point &pt2, const Point &pt3)
{
	return compute_three_3D_points_cross_product(pt3.x, pt3.y, pt3.z, pt1.x, pt1.y, pt1.z, pt2.x, pt2.y, pt2.z);
}


vector<Point>::iterator get_nearest_point(const Point &pt, vector<Point> *points)
{
	double min_dist = 100000000000;
	int i, pos =-1;

	
	for (i = 0; i < points->size(); i ++)
		if (pt.calculate_distance((*points)[i]) < min_dist) {
			min_dist = pt.calculate_distance((*points)[i]);
			pos = i;
		}

	EXECUTION_REPORT(REPORT_ERROR, pos != -1, "the input global grid is too sparse\n");
		
	return points->begin()+pos;
}


Triangle::Triangle()
{
	edge[0] = NULL;
	edge[1] = NULL;
	edge[2] = NULL;
}


void Triangle::initialize_triangle_with_edges(Edge *edge1, Edge *edge2, Edge *edge3)
{
	Point pt1, pt2, pt3;


	is_leaf = true;
	reference_count = 1;
	visited = false;
	
	pt1 = edge1->head;
	pt2 = edge2->head;
	pt3 = edge3->head;
	
	EXECUTION_REPORT(REPORT_ERROR, fabs(det(pt1, pt2, pt3)) > e && fabs(det(pt2, pt3, pt1)) > e && fabs(det(pt3, pt1, pt2)) > e,
		             "remap software error1 in new Triangle");
	EXECUTION_REPORT(REPORT_ERROR, edge1->tail==edge2->head && edge2->tail==edge3->head && edge3->tail==edge1->head, "remap software error2 in new Triangle");
   	
	v[0] = pt1;
	if (pt1.position_to_edge(pt2, pt3) == 1) {
		v[1] = pt2;
		v[2] = pt3;
		this->edge[0] = edge1;
		this->edge[1] = edge2;
		this->edge[2] = edge3;
	}
	else {
		v[1] = pt3;
		v[2] = pt2;
		this->edge[0] = edge3->twin_edge;
		this->edge[1] = edge2->twin_edge;
		this->edge[2] = edge1->twin_edge;
	}
	
	this->edge[0]->next_edge_in_triangle = this->edge[1];
	this->edge[1]->next_edge_in_triangle = this->edge[2];
	this->edge[2]->next_edge_in_triangle = this->edge[0];
	this->edge[0]->prev_edge_in_triangle = this->edge[2];
	this->edge[1]->prev_edge_in_triangle = this->edge[0];
	this->edge[2]->prev_edge_in_triangle = this->edge[1];

	this->edge[0]->triangle = this;
	this->edge[1]->triangle = this;
	this->edge[2]->triangle = this;
}


Triangle::Triangle(Point point1, Point point2, Point point3)
{
	printf("generate triangle (%lf %lf)->(%lf %lf)->(%lf %lf)\n", point1.lon, point1.lat, point2.lon, point2.lat, point3.lon, point3.lat);
	initialize_triangle_with_edges(new Edge(point1,point2), new Edge(point2,point3), new Edge(point3,point1));
}


Triangle::Triangle(Edge *edge1, Edge *edge2, Edge *edge3)
{
	initialize_triangle_with_edges(edge1, edge2, edge3);
}


void Triangle::check_and_set_twin_edge_relationship(Triangle *another_triangle)
{
	for (int i = 0; i < 3; i ++)
		for (int j = 0; j < 3; j ++)
			if (this->edge[i]->head == another_triangle->edge[j]->tail && this->edge[i]->tail == another_triangle->edge[j]->head) {
				this->edge[i]->twin_edge = another_triangle->edge[j];
				another_triangle->edge[j]->twin_edge = this->edge[i];
				printf("set twin edge\n");
			}
}


int Triangle::find_best_candidate_point()
{
	double max_min_dist = -1, min_dist, dist;
	int best_candidate_id;

	
	if (remained_points_in_triangle.size() == 0)
		return -1;

	for (int i = 0; i < remained_points_in_triangle.size(); i ++) {
		min_dist = remained_points_in_triangle[i].calculate_distance(v[0]);
		dist = remained_points_in_triangle[i].calculate_distance(v[1]);
		if (min_dist > dist)
			min_dist = dist;
		dist = remained_points_in_triangle[i].calculate_distance(v[2]);
		if (min_dist > dist)
			min_dist = dist;
		if (max_min_dist < min_dist) {
			max_min_dist = min_dist;
			best_candidate_id = i;
		}
	}

	return best_candidate_id;
}


Triangle::~Triangle()
{	
}


Point Triangle::get_center_coordinates()
{
	double temp_lon1, temp_lon2, temp_lon3, min_lon;
	temp_lon1 = v[0].lon;
	temp_lon2 = v[1].lon;
	temp_lon3 = v[2].lon;
	min_lon = temp_lon1;
	if (min_lon > temp_lon2)
		min_lon = temp_lon2;
	if (min_lon > temp_lon3)
		min_lon = temp_lon3;
	if (temp_lon1-min_lon > 180)
		temp_lon1 -= 360;
	if (temp_lon2-min_lon > 180)
		temp_lon2 -= 360;
	if (temp_lon3-min_lon > 180)
		temp_lon3 -= 360;
	center.lat = (v[0].lat+v[1].lat+v[2].lat) / 3;
	center.lon = (temp_lon1+temp_lon2+temp_lon3) / 3;
	if (center.lon < 0)
		center.lon += 360;

	return center;
}


Edge::Edge(const Point &head, const Point &tail) 
{
	this->head = head;
	this->tail = tail;
	twin_edge = NULL; 
	prev_edge_in_triangle = NULL; 
	next_edge_in_triangle = NULL;
	triangle = NULL;
}


Edge *Edge::generate_twins_edge()
{
	Edge *twins_edge = new Edge(tail, head);
	twins_edge->twin_edge = this;
	this->twin_edge = twins_edge;

	return twins_edge;
}


Edge::~Edge()
{
}


Point::Point(double lat, double lon, int id): lat(lat), lon(lon), id(id)
{
	x = cosd(lat) * cosd(lon);
	y = cosd(lat) * sind(lon);
	z = sind(lat);
}


Point::Point(const Point &pt)
{
	lat = pt.lat;
	lon = pt.lon;
	id = pt.id;
	x = pt.x;
	y = pt.y;
	z = pt.z;
}

Point& Point::operator=(const Point &pt)
{
	/* no need to self assignment check */
	lat = pt.lat;
	lon = pt.lon;
	id = pt.id;
	x = pt.x;
	y = pt.y;
	z = pt.z;
	return *this;
}

Point Point::operator-(const Point &pt) const
{
	Point ret;
	ret.x = this->x - pt.x;
	ret.y = this->y - pt.y;
	ret.z = this->z - pt.z;
	return ret;
}

double Point::calculate_distance(const Point &pt) const
{
	return calculate_distance_of_two_points_2D(lon, lat, pt.lon, pt.lat, true);
}


/**
 * Check point's position relative to an edge<pt1, pt2>
 * Points should be distinct
 * @param  pt1	the head of the edge
 * @param  pt2	the head of the edge 
 * @return	1	left
 *			0	on
 *			-1	right
 */
int Point::position_to_edge(const Point &pt1, const Point &pt2) const
{
	double res1 = det(pt1, pt2, *this);
	double res2 = det(pt2, *this, pt1);
	double res3 = det(*this, pt1, pt2);

	
	if (fabs(res1) <= e || fabs(res2) <=e || fabs(res3) <= e)
		return 0;
	else if (res1 > 0)
		return 1;
	else return -1;
}


/**
 * Check point's position relative to a triangle
 * This point and points of the triangle should be distinct
 * @return	 0	inside
 *			-1	outside
 *			 1	lies on the edge <pt1, pt2>
 *			 2	lies on the edge <pt2, pt3>
 *			 3	lies on the edge <pt3, pt1>
 */
int Point::position_to_triangle(const Triangle *triangle) const
{
	int pos, ret = 0;
	pos = position_to_edge(triangle->v[0], triangle->v[1]);
	if (pos == -1)
		return -1;
	else if (pos == 0)
		ret = 1;
	pos = position_to_edge(triangle->v[1], triangle->v[2]);
	if (pos == -1)
		return -1;
	else if (pos == 0)
		ret = 2;
	pos = position_to_edge(triangle->v[2], triangle->v[0]);
	if (pos == -1)
		return -1;
	else if (pos == 0)
		ret = 3;
	return ret;
}

ostream& operator<<(ostream &out, const Point &point)
{
	out<<'('<<point.lat<<','<<point.lon<<')';
	return out;
}


void Delaunay_Voronoi::check_and_set_twin_edge_relationship(vector<Triangle*> *triangles)
{
	for (int i = 0; i < triangles->size(); i ++)
		for (int j = i+1; j < triangles->size(); j ++)
			(*triangles)[i]->check_and_set_twin_edge_relationship((*triangles)[j]);
}


Point Delaunay_Voronoi::generate_boundary_point(double lon, double lat, Triangle *root, bool is_virtual_point)
{
	vector<Point>::iterator boundary_iter;
	Point boundary_point;

	
	if (is_virtual_point)
		return Point(lat, lon);

	boundary_iter = get_nearest_point(Point(lat, lon), &(root->remained_points_in_triangle));
	boundary_point = *boundary_iter;
	root->remained_points_in_triangle.erase(boundary_iter);
	return boundary_point;
}


void Delaunay_Voronoi::generate_initial_triangles(Triangle *root, vector<Point> *boundary_points1, vector<Point> *boundary_points2, bool cyclic)
{
	int i, max_i, i1, i2, j1, j2;

	
	EXECUTION_REPORT(REPORT_ERROR, boundary_points1->size()==boundary_points2->size() || boundary_points1->size()==1 || boundary_points2->size()==1,
		             "remap software error in generate_initial_triangles");
	max_i = boundary_points1->size() > boundary_points2->size()? boundary_points1->size() : boundary_points2->size();
	if (!cyclic)
		max_i --;
	for (i = 0; i < max_i; i ++) {
		i1 = i % boundary_points1->size();
		i2 = (i+1) % boundary_points1->size();
		j1 = i % boundary_points2->size();
		j2 = (i+1) % boundary_points2->size();
		if (j1 != j2)
			root->children.push_back(new Triangle((*boundary_points1)[i1], (*boundary_points2)[j1], (*boundary_points2)[j2]));
		if (i1 != i2)
			root->children.push_back(new Triangle((*boundary_points2)[j2], (*boundary_points1)[i2], (*boundary_points1)[i1]));
	}
}


Delaunay_Voronoi::Delaunay_Voronoi(int num_points, double *lat_values, double *lon_values, bool is_global_grid,
                   double min_lon, double max_lon, double min_lat, double max_lat, bool *redundant_cell_mark,
                   double **output_vertex_lon_values, double **output_vertex_lat_values, int *output_num_vertexes)
{
	Triangle *root;
	struct timeval start, end;
	gettimeofday(&start, NULL);
	Point boundary_point1, boundary_point2, boundary_point3, boundary_point4, boundary_point5, boundary_point6;
	vector<Point>::iterator boundary_iter;
	int i, j;
	int max_num_vertexes, current_num_vertices;
	double *tmp_vertexes_lons, *tmp_vertexes_lats;
	double boundary_point_lons[256];
	vector<Point> boundary_points[3];
	int set_id = 0;
	bool cyclic = min_lon==0 && max_lon==360;
	

	this->is_global_grid = is_global_grid;

	root = new Triangle();

	cells = new Cell[num_points];	
	for (i = 0; i < num_points; i ++) {
		Point point(lat_values[i], lon_values[i], i);
		cells[i].center = point;
		if (!redundant_cell_mark[i]) {
			point.current_triangle = root;
			root->remained_points_in_triangle.push_back(point);
		}
	}

	EXECUTION_REPORT(REPORT_LOG, true, "there are %d valid grid points in the grid for Voronoi generation", root->remained_points_in_triangle.size());

	if (cyclic) {
		for (i = 0; i < 4; i ++)
			boundary_point_lons[i] = i*90;
	}
	else {
		boundary_point_lons[0] = min_lon;
		boundary_point_lons[3] = max_lon;
		if (min_lon < max_lon) {
			boundary_point_lons[1] = (max_lon-min_lon)/3+min_lon;
			boundary_point_lons[2] = (max_lon-min_lon)*2/3+min_lon;
		}
		else {
			boundary_point_lons[1] = (max_lon-min_lon+360)/3+min_lon;
			boundary_point_lons[2] = (max_lon-min_lon+360)*2/3+min_lon;
			if (boundary_point_lons[1] >= 360)
				boundary_point_lons[1] -= 360;
			if (boundary_point_lons[2] >= 360)
				boundary_point_lons[2] -= 360;
		}
	}

	if (max_lat == 90) {
		if (cyclic)
			boundary_points[set_id++].push_back(generate_boundary_point(0, max_lat, root, false));
		else boundary_points[set_id++].push_back(generate_boundary_point(0, max_lat, root, true));
	}
	else {
		for (i = 0; i < 4; i ++)
			boundary_points[set_id].push_back(generate_boundary_point(boundary_point_lons[i], max_lat, root, true));
		set_id ++;
	}
	if (is_global_grid) {
		for (i = 0; i < 4; i ++)
			boundary_points[set_id].push_back(generate_boundary_point(boundary_point_lons[i], 0, root, false));
		set_id ++;		
	}
	if (min_lat == -90) {
		if (cyclic)
			boundary_points[set_id++].push_back(generate_boundary_point(0, min_lat, root, false));
		else boundary_points[set_id++].push_back(generate_boundary_point(0, min_lat, root, true));
	}
	else {
		for (i = 0; i < 4; i ++)
			boundary_points[set_id].push_back(generate_boundary_point(boundary_point_lons[i], min_lat, root, true));
		set_id ++;
	}

	generate_initial_triangles(root, &boundary_points[0], &boundary_points[1], cyclic);
	if (set_id > 2)
		generate_initial_triangles(root, &boundary_points[1], &boundary_points[2], cyclic);
	check_and_set_twin_edge_relationship(&(root->children));
	for (i = 0; i < root->children.size(); i ++)
		root->children[i]->reference_count ++;

	distribute_points_into_triangles(&(root->remained_points_in_triangle), &(root->children));
	for (int i = 0; i < root->children.size(); i ++)
		triangularization_process(root->children[i], is_global_grid);

	generate_Voronoi_diagram();
	extract_vertex_coordinate_values(num_points, is_global_grid, output_vertex_lon_values, output_vertex_lat_values, output_num_vertexes);

	/* Below is for testing */
	gettimeofday(&end, NULL);
	printf("Time: %ld\n", end.tv_sec - start.tv_sec);	
	printf("okok\n");
	printf("finish new Delaunay_Voronoi\n");
}


void Delaunay_Voronoi::extract_vertex_coordinate_values(int num_points, bool is_global_grid, 
                                 double **output_vertex_lon_values, double **output_vertex_lat_values, int *output_num_vertexes)
{
	int i, j; 
	int max_num_vertexes, current_num_vertices;
	double *tmp_vertexes_lons, *tmp_vertexes_lats;


	max_num_vertexes = 0;
	for (i = 0; i < num_points; i ++) {
		if (cells[i].vertexes_lons.size() > max_num_vertexes)
			max_num_vertexes = cells[i].vertexes_lons.size();
	}
	max_num_vertexes ++;
	tmp_vertexes_lons = new double [max_num_vertexes];
	tmp_vertexes_lats = new double [max_num_vertexes];

	max_num_vertexes = 0;
	for (i = 0; i < num_points; i++) {
		current_num_vertices = cells[i].vertexes_lons.size();
		for (j = 0; j < current_num_vertices; j ++) {
			tmp_vertexes_lons[j] = cells[i].vertexes_lons[j];
			tmp_vertexes_lats[j] = cells[i].vertexes_lats[j];
		}
		sort_vertexes_of_sphere_cell(current_num_vertices, tmp_vertexes_lons, tmp_vertexes_lats);
		if (!is_point_in_2D_cell(cells[i].center.lon, cells[i].center.lat, tmp_vertexes_lons, tmp_vertexes_lats, 
			                     current_num_vertices, true, true, true)) {
//			EXECUTION_REPORT(REPORT_ERROR, !is_global_grid, "remap software erorr in extract_vertex_coordinate_values\n");
			tmp_vertexes_lons[current_num_vertices] = cells[i].center.lon;
			tmp_vertexes_lats[current_num_vertices] = cells[i].center.lat;
			current_num_vertices ++;
			sort_vertexes_of_sphere_cell(current_num_vertices, tmp_vertexes_lons, tmp_vertexes_lats);
		}
		if (current_num_vertices > max_num_vertexes)
			max_num_vertexes = current_num_vertices;
		cells[i].vertexes_lons.clear();
		cells[i].vertexes_lats.clear();
		for (j = 0; j < current_num_vertices; j ++) {
			cells[i].vertexes_lons.push_back(tmp_vertexes_lons[j]);
			cells[i].vertexes_lats.push_back(tmp_vertexes_lats[j]);
		}
	}	

	*output_num_vertexes = max_num_vertexes;
	*output_vertex_lon_values = new double [max_num_vertexes*num_points];
	*output_vertex_lat_values = new double [max_num_vertexes*num_points];
	for (i = 0; i < max_num_vertexes*num_points; i ++) {
		(*output_vertex_lon_values)[i] = NULL_COORD_VALUE;
		(*output_vertex_lat_values)[i] = NULL_COORD_VALUE;
	}
	
	for (i = 0; i < num_points; i++) {
		printf("cell %d (%lf %lf): ", i, cells[i].center.lon, cells[i].center.lat);
		for (j = 0; j < cells[i].vertexes_lons.size(); j ++) {
			(*output_vertex_lon_values)[i*max_num_vertexes+j] = cells[i].vertexes_lons[j];
			(*output_vertex_lat_values)[i*max_num_vertexes+j] = cells[i].vertexes_lats[j];
			printf("<%lf %lf>  ", cells[i].vertexes_lons[j], cells[i].vertexes_lats[j]);
		}
		printf("\n");
	}

	delete [] tmp_vertexes_lons;
	delete [] tmp_vertexes_lats;
}


Triangle *Delaunay_Voronoi::search_triangle_with_point(Triangle *cur_triangle, const Point &pt)
{
	if (pt.position_to_triangle(cur_triangle) < 0)
		return NULL;
	
	if (cur_triangle->is_leaf)
		return cur_triangle;

	for (int i = 0; i < cur_triangle->children.size(); i ++) {
		Triangle *found_triangle = search_triangle_with_point(cur_triangle->children[i], pt);
		if (found_triangle != NULL)
			return found_triangle;
	}

	return NULL;
}


void Delaunay_Voronoi::distribute_points_into_triangles(vector<Point> *pnts, vector<Triangle*> *triangles)
{
	bool find_triangle;


	for (int i = 0; i < pnts->size(); i ++) {
		find_triangle = false;
		for (int j = 0; j < triangles->size(); j ++) {
			if (!((*triangles)[j])->is_leaf)
				continue;
			if ((*pnts)[i].position_to_triangle(((*triangles)[j])) >= 0) {
				(*pnts)[i].current_triangle = (*triangles)[j];
				(*triangles)[j]->remained_points_in_triangle.push_back((*pnts)[i]);
				find_triangle = true;
				break;
			}
		}
		if (!find_triangle) 
			if (is_global_grid)
				EXECUTION_REPORT(REPORT_ERROR, "CoR may have bugs, please contact liuli-cess@tsinghua.edu.cn");
			else EXECUTION_REPORT(REPORT_ERROR, "please enlarge the boundary of the regional grid"); 
	}
}


void Delaunay_Voronoi::triangularization_process(Triangle *triangle, bool is_global_grid)
{
	int best_candidate_point_id;
	Point best_candidate_point;
	vector<Triangle *> leaf_triangles;
	vector<Triangle *> existing_triangles;


	if (!triangle->is_leaf) {
		triangle->reference_count --;
		if (triangle->reference_count <= 0)
			delete triangle;
		return;
	}
		
	if (triangle->remained_points_in_triangle.size() == 0) {
		result_leaf_triangles.push_back(triangle);
		return;
	}

	triangle->is_leaf = false;
	
	best_candidate_point_id = triangle->find_best_candidate_point();
	best_candidate_point = triangle->remained_points_in_triangle[best_candidate_point_id];
	triangle->remained_points_in_triangle.erase(triangle->remained_points_in_triangle.begin()+best_candidate_point_id);

	if (best_candidate_point.position_to_triangle(triangle) == 0) {
		Edge *e_v1_can = new Edge(triangle->v[0], best_candidate_point);
		Edge *e_can_v1 = e_v1_can->generate_twins_edge();
		Edge *e_v2_can = new Edge(triangle->v[1], best_candidate_point);
		Edge *e_can_v2 = e_v2_can->generate_twins_edge();
		Edge *e_v3_can = new Edge(triangle->v[2], best_candidate_point);
		Edge *e_can_v3 = e_v3_can->generate_twins_edge();
		Triangle *t_v1_v2_can = new Triangle(triangle->edge[0], e_v2_can, e_can_v1);
		Triangle *t_v2_v3_can = new Triangle(triangle->edge[1], e_v3_can, e_can_v2);
		Triangle *t_v3_v1_can = new Triangle(triangle->edge[2], e_v1_can, e_can_v3);
		leaf_triangles.push_back(triangle);
		leaf_triangles.push_back(t_v1_v2_can);
		leaf_triangles.push_back(t_v2_v3_can);
		leaf_triangles.push_back(t_v3_v1_can);
		triangle->reference_count ++;
		t_v1_v2_can->reference_count ++;
		t_v2_v3_can->reference_count ++;
		t_v3_v1_can->reference_count ++;
		legalize_triangles(best_candidate_point, triangle->edge[0], &leaf_triangles);
		legalize_triangles(best_candidate_point, triangle->edge[1], &leaf_triangles);
		legalize_triangles(best_candidate_point, triangle->edge[2], &leaf_triangles);
	}
	else {
		Point vi, vj, vk, vl;
		Edge *eij, *ejk, *eki;
		Edge *eil, *elj, *eji;
		switch (best_candidate_point.position_to_triangle(triangle)) {
			case 1:
				vi = triangle->v[0];
				vj = triangle->v[1];
				vk = triangle->v[2];
				eij = triangle->edge[0];
				break;
			case 2:
				vi = triangle->v[1];
				vj = triangle->v[2];
				vk = triangle->v[0];
				eij = triangle->edge[1];
				break;
			case 3:
				vi = triangle->v[2];
				vj = triangle->v[0];
				vk = triangle->v[1];
				eij = triangle->edge[2];
				break;
			default:
				EXECUTION_REPORT(REPORT_ERROR, false, "remap software error2 in triangularization_process");
				break;
		}
		EXECUTION_REPORT(REPORT_ERROR, best_candidate_point.position_to_edge(vi, vj) == 0, "remap software error3 in triangularization_process");
		EXECUTION_REPORT(REPORT_ERROR, eij->twin_edge->triangle->is_leaf, "remap software error3 in triangularization_process");
		eij->twin_edge->triangle->is_leaf = false;
        ejk = eij->next_edge_in_triangle;
        eki = ejk->next_edge_in_triangle;
        eji = eij->twin_edge;
        eil = eji->next_edge_in_triangle;
        elj = eil->next_edge_in_triangle;
        vl = elj->head;
		Edge *eri = new Edge(best_candidate_point, vi);
		Edge *eir = eri->generate_twins_edge();
		Edge *erk = new Edge(best_candidate_point, vk);
		Edge *ekr = erk->generate_twins_edge();
		Edge *erj = new Edge(best_candidate_point, vj);
		Edge *ejr = erj->generate_twins_edge();
		Edge *erl = new Edge(best_candidate_point, vl);
		Edge *elr = erl->generate_twins_edge();
		Triangle* tirk = new Triangle(eir, erk, eki);
		Triangle* tilr = new Triangle(eil, elr, eri);
		Triangle* tjkr = new Triangle(ejk, ekr, erj);
		Triangle* tjrl = new Triangle(ejr, erl, elj);
		leaf_triangles.push_back(triangle);
		leaf_triangles.push_back(eij->twin_edge->triangle);
		leaf_triangles.push_back(tirk);
		leaf_triangles.push_back(tilr);
		leaf_triangles.push_back(tjkr);
		leaf_triangles.push_back(tjrl);
		triangle->reference_count ++;
		tirk->reference_count ++;
		tilr->reference_count ++;
		tjkr->reference_count ++;
		tjrl->reference_count ++;
		delete eij;
        delete eji;
		legalize_triangles(best_candidate_point, eil, &leaf_triangles);
        legalize_triangles(best_candidate_point, elj, &leaf_triangles);
        legalize_triangles(best_candidate_point, ejk, &leaf_triangles);
        legalize_triangles(best_candidate_point, eki, &leaf_triangles);	
	}

	for (int i = 0; i < leaf_triangles.size(); i ++) {
		if (leaf_triangles[i]->is_leaf)
			EXECUTION_REPORT(REPORT_ERROR, leaf_triangles[i]->remained_points_in_triangle.size() == 0, "remap software error1 in triangularization_process");
	}
	for (int i = 0; i < leaf_triangles.size(); i ++) {
		if (leaf_triangles[i]->is_leaf)
			continue;
		distribute_points_into_triangles(&(leaf_triangles[i]->remained_points_in_triangle), &leaf_triangles);
	}		
	for (int i = 0; i < leaf_triangles.size(); i ++)
		triangularization_process(leaf_triangles[i], is_global_grid);

	triangle->reference_count --;
	if (triangle->reference_count <= 0)
		delete triangle;
}


Delaunay_Voronoi::~Delaunay_Voronoi()
{
	delete [] cells;
}


bool Delaunay_Voronoi::is_triangle_legal(const Point &pt, const Edge *edge)
{
	if (!edge->twin_edge)
		return true;

	const Point vr = pt;
	const Point vi = edge->head;
	const Point vj = edge->next_edge_in_triangle->head;
	const Point vk = edge->twin_edge->prev_edge_in_triangle->head;

	if (det(vi-vr, vj-vr, vk-vr) >= e)
		return false;
	else return true;
}


void Delaunay_Voronoi::legalize_triangles(const Point &vr, Edge *edge, vector<Triangle*> *leaf_triangles)
{
	if (is_triangle_legal(vr, edge))
		return;

	EXECUTION_REPORT(REPORT_ERROR, edge->triangle->is_leaf, "remap software error1 in legalize_triangles\n");
	EXECUTION_REPORT(REPORT_ERROR, edge->twin_edge->triangle->is_leaf, "remap software error2 in legalize_triangles %lx\n", (long)(edge->twin_edge->triangle));
	leaf_triangles->push_back(edge->twin_edge->triangle);
	edge->twin_edge->triangle->reference_count ++;
	edge->triangle->is_leaf = false;
	edge->twin_edge->triangle->is_leaf = false;

	const Point vi = edge->head;
	const Point vj = edge->tail;
	const Point vk = edge->twin_edge->prev_edge_in_triangle->head;
	Edge *eij = edge;
	Edge *ejr = eij->next_edge_in_triangle;
	Edge *eri = ejr->next_edge_in_triangle;
	Edge *eji = eij->twin_edge;
	Edge *eik = eji->next_edge_in_triangle;
	Edge *ekj = eik->next_edge_in_triangle;
	Edge *erk = new Edge(vr, vk);
	Edge *ekr = erk->generate_twins_edge();
	Triangle* tikr = new Triangle(eik,ekr,eri);
	Triangle* tjrk = new Triangle(ejr,erk,ekj);
	leaf_triangles->push_back(tikr);
	leaf_triangles->push_back(tjrk);
	tikr->reference_count ++;
	tjrk->reference_count ++;
	delete eij;
	delete eji;
	legalize_triangles(vr, eik, leaf_triangles);
	legalize_triangles(vr, ekj, leaf_triangles);
}


void Delaunay_Voronoi::generate_Voronoi_diagram()
{
	int num_none_virtual_vertexes, none_virtual_vertexes[3];
	int empty_id;
	int i, j;

	
	for (i = 0; i < result_leaf_triangles.size(); i ++)
		if (!result_leaf_triangles[i]->is_leaf) 
			printf("detect false leaf triangle\n");
		else {
			result_leaf_triangles[i]->center = result_leaf_triangles[i]->get_center_coordinates();
			printf("leaf triangle <%lf %lf>  <%lf %lf>  <%lf %lf>\n", result_leaf_triangles[i]->v[0].lon, result_leaf_triangles[i]->v[0].lat,
				result_leaf_triangles[i]->v[1].lon, result_leaf_triangles[i]->v[1].lat, result_leaf_triangles[i]->v[2].lon, result_leaf_triangles[i]->v[2].lat);
			EXECUTION_REPORT(REPORT_ERROR, is_triangle_legal(result_leaf_triangles[i]->v[0],result_leaf_triangles[i]->edge[1])&&
				             is_triangle_legal(result_leaf_triangles[i]->v[1],result_leaf_triangles[i]->edge[2])&&
				             is_triangle_legal(result_leaf_triangles[i]->v[2],result_leaf_triangles[i]->edge[0]),
				             "remap_software error in generate_Voronoi_diagram");
			for (j = 0, num_none_virtual_vertexes = 0; j < 3; j ++)
				if (result_leaf_triangles[i]->v[j].id != -1)
					none_virtual_vertexes[num_none_virtual_vertexes ++] = result_leaf_triangles[i]->v[j].id;
			if (num_none_virtual_vertexes == 0)
				continue;
			if (num_none_virtual_vertexes == 1) {
				result_leaf_triangles[i]->center.lat = cells[none_virtual_vertexes[0]].center.lat;
				result_leaf_triangles[i]->center.lon = cells[none_virtual_vertexes[0]].center.lon;
			}
			if (num_none_virtual_vertexes == 2) {
				result_leaf_triangles[i]->center.lat = (cells[none_virtual_vertexes[0]].center.lat+cells[none_virtual_vertexes[1]].center.lat) / 2;
				result_leaf_triangles[i]->center.lon = (cells[none_virtual_vertexes[0]].center.lon+cells[none_virtual_vertexes[1]].center.lon) / 2;;
			}
			for (j = 0, num_none_virtual_vertexes = 0; j < 3; j ++)
				if (result_leaf_triangles[i]->v[j].id != -1) {
					cells[result_leaf_triangles[i]->v[j].id].vertexes_lats.push_back(result_leaf_triangles[i]->center.lat);
					cells[result_leaf_triangles[i]->v[j].id].vertexes_lons.push_back(result_leaf_triangles[i]->center.lon);
				}		
		}
}

