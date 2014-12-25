/***************************************************************
  *  Copyright (c) 2013, Tsinghua University.
  *  This is a source file of C-Coupler.
  *  This file was initially finished by Mr. Yufeng Zhou,
  *  and then upgraded and merged into CoR by Dr. Li Liu. 
  *  If you have any problem, 
  *  please contact Dr. Li Liu via liuli-cess@tsinghua.edu.cn
  ***************************************************************/


#ifndef __DELAUNAY_VORONOI_H__
#define __DELAUNAY_VORONOI_H__


#include <vector>
#include <list>
#include <cmath>
#include <iostream>
#include "remap_grid_class.h"

using namespace std;


class Edge;
class Point;
class Triangle;


class Point
{
	public:
		double lat; 	/* latitude coordinate	*/
		double lon; 	/* lontitude coordinate */
		double x;	
		double y;
		double z;
		int id;
		Triangle *current_triangle;

		Point(double lat = double(), double lon = double(), int id = -1);
		Point(const Point &pt);
		double calculate_distance(const Point &pt) const;
		int position_to_edge(const Point &pt1, const Point&pt2) const;
		int position_to_triangle(const Triangle *) const;
		Point& operator=(const Point &pt);
		Point operator-(const Point &pt) const;

		/* only compare lat and lon */
		bool operator==(const Point &pt) const
		{
			return lat == pt.lat && lon == pt.lon;
		}

		/* only compare lat and lon */
		bool operator!=(const Point &pt) const
		{
			return lat != pt.lat || lon != pt.lon;
		}
};


class Triangle
{
	public:
		Point v[3];	/* vertexes of triangle */
		Point center;	/* circumcenter */
		Edge *edge[3];
		bool is_leaf;
		bool visited;
		unsigned reference_count;	/* reference count, used to destruct */
		vector<Point> remained_points_in_triangle;
		vector<Triangle*> children;

		Triangle();
		Triangle(Point, Point, Point);
		Triangle(Edge*, Edge*, Edge*);
		~Triangle();
		Point get_center_coordinates();
		int find_best_candidate_point();
		void check_and_set_twin_edge_relationship(Triangle*);

	private:
		Triangle(const Triangle &triangle);
		void initialize_triangle_with_edges(Edge*, Edge*, Edge*);
		Triangle& operator=(const Triangle &triangle);
};


struct Cell
{
	Point center;
	vector<double> vertexes_lons;
	vector<double> vertexes_lats;
};


class Edge
{
	public:
		Point head;
		Point tail;	/* the tail of this edge, constant */
		Edge *twin_edge;			/* the twin_edge edge, whose tail is the head of this edge and head is the tail of this edge */
		Edge *next_edge_in_triangle;			/* the next_edge_in_triangle edge, whose tail is the head of this edge but head isn't the tail of this edge */
		Edge *prev_edge_in_triangle;			/* the prev_edge_in_triangle edge, whose head is the tail of this edge but tail isn't the head of this edge */
		Triangle *triangle; /* the triangle which is composed by this edge and its next_edge_in_triangle and prev_edge_in_triangle */

		Edge(const Point &head, const Point &tail);
		~Edge();
		Edge *generate_twins_edge();

	private:
		Edge(const Edge &edge);
		Edge& operator=(const Edge &edge);
};


class Delaunay_Voronoi
{
	public:
		Cell *cells;
		vector<Triangle*> result_leaf_triangles;
		bool is_global_grid;

		Delaunay_Voronoi(int, double*, double*, bool, double, double, double, double, bool*, double**, double**, int*);
		~Delaunay_Voronoi();
		static bool is_triangle_legal(const Point &pt, const Edge *edge);
		void legalize_triangles(const Point &pt, Edge *edge, vector<Triangle*>*);

	private:
		void check_and_set_twin_edge_relationship(vector<Triangle*>*);
		Point generate_boundary_point(double, double, Triangle*, bool);
		void generate_initial_triangles(Triangle*, vector<Point>*, vector<Point>*, bool);
		void triangularization_process(Triangle*, bool);
		void distribute_points_into_triangles(vector<Point>*, vector<Triangle*>*);
		Triangle *search_triangle_with_point(Triangle*, const Point &pt);
		void generate_Voronoi_diagram();
		void extract_vertex_coordinate_values(int, bool, double**, double**, int*);
};

#endif


