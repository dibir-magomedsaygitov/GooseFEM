/* =================================================================================================

(c - GPLv3) T.W.J. de Geus (Tom) | tom@geus.me | www.geus.me | github.com/tdegeus/GooseFEM

================================================================================================= */

#include <GooseFEM/GooseFEM.h>
#include <pybind11/pybind11.h>
#include <pyxtensor/pyxtensor.hpp>

namespace py = pybind11;

void init_VectorPartitionedTyings(py::module& m)
{

    py::class_<GooseFEM::VectorPartitionedTyings>(m, "VectorPartitionedTyings")

        .def(
            py::init<
                const xt::xtensor<size_t, 2>&,
                const xt::xtensor<size_t, 2>&,
                const Eigen::SparseMatrix<double>&,
                const Eigen::SparseMatrix<double>&,
                const Eigen::SparseMatrix<double>&>(),
            "Switch between dofval/nodevec/elemvec",
            py::arg("conn"),
            py::arg("dofs"),
            py::arg("Cdu"),
            py::arg("Cdp"),
            py::arg("Cdi"))

        .def("nelem", &GooseFEM::VectorPartitionedTyings::nelem, "Number of element")

        .def("nne", &GooseFEM::VectorPartitionedTyings::nne, "Number of nodes per element")

        .def("nnode", &GooseFEM::VectorPartitionedTyings::nnode, "Number of nodes")

        .def("ndim", &GooseFEM::VectorPartitionedTyings::ndim, "Number of dimensions")

        .def("ndof", &GooseFEM::VectorPartitionedTyings::ndof, "Number of DOFs")

        .def("nnu", &GooseFEM::VectorPartitionedTyings::nnu, "Number of unknown DOFs")

        .def("nnp", &GooseFEM::VectorPartitionedTyings::nnp, "Number of prescribed DOFs")

        .def("nni", &GooseFEM::VectorPartitionedTyings::nni, "Number of independent DOFs")

        .def("nnd", &GooseFEM::VectorPartitionedTyings::nnd, "Number of dependent DOFs")

        .def("dofs", &GooseFEM::VectorPartitionedTyings::dofs, "DOFs")

        .def("iiu", &GooseFEM::VectorPartitionedTyings::iiu, "Unknown DOFs")

        .def("iip", &GooseFEM::VectorPartitionedTyings::iip, "Prescribed DOFs")

        .def("iii", &GooseFEM::VectorPartitionedTyings::iii, "Independent DOFs")

        .def("iid", &GooseFEM::VectorPartitionedTyings::iid, "Dependent DOFs")

        .def(
            "AsDofs_i",
            &GooseFEM::VectorPartitionedTyings::AsDofs_i,
            "Set 'dofval",
            py::arg("nodevec"))

        .def(
            "AsNode", &GooseFEM::VectorPartitionedTyings::AsNode, "Set 'nodevec", py::arg("dofval"))

        .def(
            "AsElement",
            &GooseFEM::VectorPartitionedTyings::AsElement,
            "Set 'elemvec",
            py::arg("nodevec"))

        .def(
            "AssembleDofs",
            &GooseFEM::VectorPartitionedTyings::AssembleDofs,
            "Assemble 'dofval'",
            py::arg("elemvec"))

        .def(
            "AssembleNode",
            &GooseFEM::VectorPartitionedTyings::AssembleNode,
            "Assemble 'nodevec'",
            py::arg("elemvec"))

        .def(
            "AllocateDofval",
            py::overload_cast<>(&GooseFEM::VectorPartitionedTyings::AllocateDofval, py::const_))

        .def(
            "AllocateDofval",
            py::overload_cast<double>(&GooseFEM::VectorPartitionedTyings::AllocateDofval, py::const_))

        .def(
            "AllocateNodevec",
            py::overload_cast<>(&GooseFEM::VectorPartitionedTyings::AllocateNodevec, py::const_))

        .def(
            "AllocateNodevec",
            py::overload_cast<double>(&GooseFEM::VectorPartitionedTyings::AllocateNodevec, py::const_))

        .def(
            "AllocateElemvec",
            py::overload_cast<>(&GooseFEM::VectorPartitionedTyings::AllocateElemvec, py::const_))

        .def(
            "AllocateElemvec",
            py::overload_cast<double>(&GooseFEM::VectorPartitionedTyings::AllocateElemvec, py::const_))

        .def(
            "AllocateElemmat",
            py::overload_cast<>(&GooseFEM::VectorPartitionedTyings::AllocateElemmat, py::const_))

        .def(
            "AllocateElemmat",
            py::overload_cast<double>(&GooseFEM::VectorPartitionedTyings::AllocateElemmat, py::const_))

        .def("__repr__", [](const GooseFEM::VectorPartitionedTyings&) {
            return "<GooseFEM.VectorPartitionedTyings>";
        });
}
