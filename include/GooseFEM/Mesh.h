/*

(c - GPLv3) T.W.J. de Geus (Tom) | tom@geus.me | www.geus.me | github.com/tdegeus/GooseFEM

*/

#ifndef GOOSEFEM_MESH_H
#define GOOSEFEM_MESH_H

#include "config.h"

namespace GooseFEM {
namespace Mesh {

// Enumerator for element-types

enum class ElementType {
    Quad4, // Quadrilateral: 4-noded element in 2-d
    Hex8, // Hexahedron: 8-noded element in 3-d
    Tri3 }; // Triangle: 3-noded element in 2-d

// Extract the element type based on the connectivity

inline ElementType defaultElementType(
    const xt::xtensor<double, 2>& coor,
    const xt::xtensor<size_t, 2>& conn);

// Stitch two mesh objects, specifying overlapping nodes by hand

class ManualStitch {
public:
    ManualStitch() = default;

    ManualStitch(
        const xt::xtensor<double, 2>& coor_a,
        const xt::xtensor<size_t, 2>& conn_a,
        const xt::xtensor<size_t, 1>& overlapping_nodes_a,
        const xt::xtensor<double, 2>& coor_b,
        const xt::xtensor<size_t, 2>& conn_b,
        const xt::xtensor<size_t, 1>& overlapping_nodes_b,
        bool check_position = true,
        double rtol = 1e-5,
        double atol = 1e-8);

    xt::xtensor<double, 2> coor() const;
    xt::xtensor<size_t, 2> conn() const;
    xt::xtensor<size_t, 1> nodemap(size_t mesh_index) const;
    xt::xtensor<size_t, 1> elemmap(size_t mesh_index) const;

    // convert set of of node/element-numbers for an original mesh to the final mesh
    xt::xtensor<size_t, 1> nodeset(const xt::xtensor<size_t, 1>& set, size_t mesh_index) const;
    xt::xtensor<size_t, 1> elemset(const xt::xtensor<size_t, 1>& set, size_t mesh_index) const;

private:
    xt::xtensor<double, 2> m_coor;
    xt::xtensor<size_t, 2> m_conn;
    xt::xtensor<size_t, 1> m_map_b;
    size_t m_nnd_a;
    size_t m_nel_a;
    size_t m_nel_b;
};

// Stitch mesh objects, searching for overlapping nodes

class Stitch {
public:
    Stitch() = default;

    Stitch(double rtol, double atol);

    void push_back(const xt::xtensor<double, 2>& coor, const xt::xtensor<size_t, 2>& conn);

    xt::xtensor<double, 2> coor() const;
    xt::xtensor<size_t, 2> conn() const;
    xt::xtensor<size_t, 1> nodemap(size_t mesh_index) const;
    xt::xtensor<size_t, 1> elemmap(size_t mesh_index) const;

    // convert set of node/element-numbers for an original mesh to the final mesh
    xt::xtensor<size_t, 1> nodeset(const xt::xtensor<size_t, 1>& set, size_t mesh_index) const;
    xt::xtensor<size_t, 1> elemset(const xt::xtensor<size_t, 1>& set, size_t mesh_index) const;

    // combine set of node/element-numbers for an original to the final mesh (removes duplicates)
    xt::xtensor<size_t, 1> nodeset(const std::vector<xt::xtensor<size_t, 1>>& set) const;
    xt::xtensor<size_t, 1> elemset(const std::vector<xt::xtensor<size_t, 1>>& set) const;

private:
    xt::xtensor<double, 2> m_coor;
    xt::xtensor<size_t, 2> m_conn;
    std::vector<xt::xtensor<size_t, 1>> m_map;
    std::vector<size_t> m_nel;
    std::vector<size_t> m_el_offset;
    double m_rtol = 1e-5;
    double m_atol = 1e-8;
};

// Renumber to lowest possible index. For example [0,3,4,2] -> [0,2,3,1]

class Renumber {
public:
    // constructors
    Renumber() = default;
    Renumber(const xt::xarray<size_t>& dofs);

    // get renumbered DOFs (same as "Renumber::apply(dofs)")
    xt::xtensor<size_t, 2> get(const xt::xtensor<size_t, 2>& dofs) const;

    // apply renumbering to other set
    template <class T>
    T apply(const T& list) const;

    // get the list needed to renumber, e.g.:
    //   dofs_renumbered(i,j) = index(dofs(i,j))
    xt::xtensor<size_t, 1> index() const;

private:
    xt::xtensor<size_t, 1> m_renum;
};

// Reorder to lowest possible index, in specific order.
//
// For example for "Reorder({iiu,iip})" after reordering:
//
//   iiu = xt::range<size_t>(nnu);
//   iip = xt::range<size_t>(nnp) + nnu;

class Reorder {
public:
    // constructors
    Reorder() = default;
    Reorder(const std::initializer_list<xt::xtensor<size_t, 1>> args);

    // get reordered DOFs (same as "Reorder::apply(dofs)")
    xt::xtensor<size_t, 2> get(const xt::xtensor<size_t, 2>& dofs) const;

    // apply renumbering to other set
    template <class T>
    T apply(const T& list) const;

    // get the list needed to reorder, e.g.:
    // dofs_reordered(i,j) = index(dofs(i,j))
    xt::xtensor<size_t, 1> index() const;

private:
    xt::xtensor<size_t, 1> m_renum;
};

// list with DOF-numbers in sequential order
inline xt::xtensor<size_t, 2> dofs(size_t nnode, size_t ndim);

// renumber to lowest possible index (see "GooseFEM::Mesh::Renumber")
inline xt::xtensor<size_t, 2> renumber(const xt::xtensor<size_t, 2>& dofs);

// number of elements connected to each node
inline xt::xtensor<size_t, 1> coordination(const xt::xtensor<size_t, 2>& conn);

// elements connected to each node
inline std::vector<std::vector<size_t>> elem2node(
    const xt::xtensor<size_t, 2>& conn,
    bool sorted=true); // ensure the output to be sorted

// return size of each element edge
inline xt::xtensor<double, 2> edgesize(
    const xt::xtensor<double, 2>& coor,
    const xt::xtensor<size_t, 2>& conn,
    ElementType type);

inline xt::xtensor<double, 2> edgesize(
    const xt::xtensor<double, 2>& coor,
    const xt::xtensor<size_t, 2>& conn); // extract element-type based on shape of "conn"

// Coordinates of the center of each element
inline xt::xtensor<double, 2> centers(
    const xt::xtensor<double, 2>& coor,
    const xt::xtensor<size_t, 2>& conn,
    ElementType type);

inline xt::xtensor<double, 2> centers(
    const xt::xtensor<double, 2>& coor,
    const xt::xtensor<size_t, 2>& conn); // extract element-type based on shape of "conn"

// Convert an element-map to a node-map.
// Input: new_elvar = elvar[elem_map]
// Return: new_nodevar = nodevar[node_map]
inline xt::xtensor<size_t, 1> elemmap2nodemap(
    const xt::xtensor<size_t, 1>& elem_map,
    const xt::xtensor<double, 2>& coor,
    const xt::xtensor<size_t, 2>& conn); // extract element-type based on shape of "conn"

inline xt::xtensor<size_t, 1> elemmap2nodemap(
    const xt::xtensor<size_t, 1>& elem_map,
    const xt::xtensor<double, 2>& coor,
    const xt::xtensor<size_t, 2>& conn,
    ElementType type);

// Find overlapping nodes
// Return: [[nodes_from_mesh_a],
//          [nodes_from_mesh_b]]
inline xt::xtensor<size_t, 2> overlapping(
    const xt::xtensor<double, 2>& coor_a,
    const xt::xtensor<double, 2>& coor_b,
    double rtol = 1e-5,
    double atol = 1e-8);

} // namespace Mesh
} // namespace GooseFEM

#include "Mesh.hpp"

#endif
