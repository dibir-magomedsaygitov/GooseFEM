/* =================================================================================================

(c - GPLv3) T.W.J. de Geus (Tom) | tom@geus.me | www.geus.me | github.com/tdegeus/GooseFEM

================================================================================================= */

#ifndef GOOSEFEM_ELEMENTQUADAXISYMMETRIC_HPP
#define GOOSEFEM_ELEMENTQUADAXISYMMETRIC_HPP

// -------------------------------------------------------------------------------------------------

#include "GooseFEM.h"

// =================================================================================================

namespace GooseFEM {
namespace Element {
namespace Quad4 {

// =================================================================================================

inline QuadratureAxisymmetric::QuadratureAxisymmetric(const xt::xtensor<double,3> &x) :
  QuadratureAxisymmetric(x, Gauss::xi(), Gauss::w()) {}

// -------------------------------------------------------------------------------------------------

inline QuadratureAxisymmetric::QuadratureAxisymmetric(const xt::xtensor<double,3> &x,
  const xt::xtensor<double,2> &xi, const xt::xtensor<double,1> &w) :
  m_x(x), m_w(w), m_xi(xi)
{
  // check input
  assert( m_x.shape()[1] == m_nne  );
  assert( m_x.shape()[2] == m_ndim );

  // extract number of elements and number of integration points
  m_nelem = m_x.shape()[0];
  m_nip   = m_w.size();

  // check input
  assert( m_xi.shape()[0] == m_nip  );
  assert( m_xi.shape()[1] == m_ndim );
  assert( m_w .size()     == m_nip  );

  // allocate arrays
  m_N    = xt::empty<double>({         m_nip, m_nne                        });
  m_dNxi = xt::empty<double>({         m_nip, m_nne, m_ndim                });
  m_B    = xt::empty<double>({m_nelem, m_nip, m_nne, m_tdim, m_tdim, m_tdim});
  m_vol  = xt::empty<double>({m_nelem, m_nip                               });

  // shape functions
  for ( size_t q = 0 ; q < m_nip ; ++q )
  {
    m_N(q,0) = .25 * (1.-m_xi(q,0)) * (1.-m_xi(q,1));
    m_N(q,1) = .25 * (1.+m_xi(q,0)) * (1.-m_xi(q,1));
    m_N(q,2) = .25 * (1.+m_xi(q,0)) * (1.+m_xi(q,1));
    m_N(q,3) = .25 * (1.-m_xi(q,0)) * (1.+m_xi(q,1));
  }

  // shape function gradients in local coordinates
  for ( size_t q = 0 ; q < m_nip ; ++q )
  {
    // - dN / dxi_0
    m_dNxi(q,0,0) = -.25*(1.-m_xi(q,1));
    m_dNxi(q,1,0) = +.25*(1.-m_xi(q,1));
    m_dNxi(q,2,0) = +.25*(1.+m_xi(q,1));
    m_dNxi(q,3,0) = -.25*(1.+m_xi(q,1));
    // - dN / dxi_1
    m_dNxi(q,0,1) = -.25*(1.-m_xi(q,0));
    m_dNxi(q,1,1) = -.25*(1.+m_xi(q,0));
    m_dNxi(q,2,1) = +.25*(1.+m_xi(q,0));
    m_dNxi(q,3,1) = +.25*(1.-m_xi(q,0));
  }

  // compute the shape function gradients, based on "x"
  compute_dN();
}

// -------------------------------------------------------------------------------------------------

inline size_t QuadratureAxisymmetric::nelem() const { return m_nelem; };

inline size_t QuadratureAxisymmetric::nne() const { return m_nne; };

inline size_t QuadratureAxisymmetric::ndim() const { return m_ndim; };

inline size_t QuadratureAxisymmetric::nip() const { return m_nip; };

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::dV(xt::xtensor<double,2> &qscalar) const
{
  assert( qscalar.shape()[0] == m_nelem );
  assert( qscalar.shape()[1] == m_nip   );

  #pragma omp parallel for
  for ( size_t e = 0 ; e < m_nelem ; ++e )
    for ( size_t q = 0 ; q < m_nip ; ++q )
      qscalar(e,q) = m_vol(e,q);
}

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::dV(xt::xtensor<double,4> &qtensor) const
{
  assert( qtensor.shape()[0] == m_nelem );
  assert( qtensor.shape()[1] == m_nne   );
  assert( qtensor.shape()[2] == m_tdim  );
  assert( qtensor.shape()[3] == m_tdim  );

  #pragma omp parallel for
  for ( size_t e = 0 ; e < m_nelem ; ++e )
    for ( size_t q = 0 ; q < m_nip ; ++q )
      for ( size_t i = 0 ; i < m_tdim ; ++i )
        for ( size_t j = 0 ; j < m_tdim ; ++j )
          qtensor(e,q,i,j) = m_vol(e,q);
}

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::update_x(const xt::xtensor<double,3> &x)
{
  assert( x.shape()[0] == m_nelem    );
  assert( x.shape()[1] == m_nne      );
  assert( x.shape()[2] == m_ndim     );
  assert( x.size()     == m_x.size() );

  // update positions
  xt::noalias(m_x) = x;

  // update the shape function gradients for the new "x"
  compute_dN();
}

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::compute_dN()
{
  // zero-initialize full B-matrix (most component remain zero, and are not written)
  m_B.fill(0.0);

  #pragma omp parallel
  {
    // allocate local variables
    xt::xtensor_fixed<double, xt::xshape<2,2>> J, Jinv;

    // loop over all elements (in parallel)
    #pragma omp for
    for ( size_t e = 0 ; e < m_nelem ; ++e )
    {
      // alias nodal positions
      auto x = xt::adapt(&m_x(e,0,0), xt::xshape<m_nne,m_ndim>());

      // loop over integration points
      for ( size_t q = 0 ; q < m_nip ; ++q )
      {
        // - alias
        auto dNxi = xt::adapt(&m_dNxi(  q,0,0    ), xt::xshape<m_nne,m_ndim>());
        auto B    = xt::adapt(&m_B   (e,q,0,0,0,0), xt::xshape<m_nne,m_tdim,m_tdim,m_tdim>());
        auto N    = xt::adapt(&m_N   (  q,0      ), xt::xshape<m_nne>());

        // - Jacobian (loops unrolled for efficiency)
        //   J(i,j) += dNxi(m,i) * x(m,j);
        J(0,0) = dNxi(0,0)*x(0,0) + dNxi(1,0)*x(1,0) + dNxi(2,0)*x(2,0) + dNxi(3,0)*x(3,0);
        J(0,1) = dNxi(0,0)*x(0,1) + dNxi(1,0)*x(1,1) + dNxi(2,0)*x(2,1) + dNxi(3,0)*x(3,1);
        J(1,0) = dNxi(0,1)*x(0,0) + dNxi(1,1)*x(1,0) + dNxi(2,1)*x(2,0) + dNxi(3,1)*x(3,0);
        J(1,1) = dNxi(0,1)*x(0,1) + dNxi(1,1)*x(1,1) + dNxi(2,1)*x(2,1) + dNxi(3,1)*x(3,1);

        // - determinant and inverse of the Jacobian
        double Jdet = inv(J, Jinv);

        // - get radius for computation of volume
        double rq = N(0)*x(0,1) + N(1)*x(1,1) + N(2)*x(2,1) + N(3)*x(3,1);

        // - compute the B matrix (loops partly unrolled for efficiency)
        //   N.B. "dNx(m,i) += Jinv(i,j) * dNxi(m,j);"
        for (size_t m = 0; m < m_nne; ++m)
        {
          B(m,0,0,0) = Jinv(0,0) * dNxi(m,0) + Jinv(0,1) * dNxi(m,1); // B(m, r, r, r) = dNdx(m,1)
          B(m,0,2,2) = Jinv(1,0) * dNxi(m,0) + Jinv(1,1) * dNxi(m,1); // B(m, r, z, z) = dNdx(m,1)
          B(m,1,1,0) = 1./rq * N(m);                                  // B(m, t, t, r)
          B(m,2,0,0) = Jinv(0,0) * dNxi(m,0) + Jinv(0,1) * dNxi(m,1); // B(m, z, r, r) = dNdx(m,0)
          B(m,2,2,2) = Jinv(1,0) * dNxi(m,0) + Jinv(1,1) * dNxi(m,1); // B(m, z, z, z) = dNdx(m,0)
        }

        // - integration point volume
        m_vol(e, q) = m_w(q) * Jdet * 2. * M_PI * rq;
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::gradN_vector(
  const xt::xtensor<double,3> &elemvec, xt::xtensor<double,4> &qtensor) const
{
  assert( elemvec.shape()[0] == m_nelem );
  assert( elemvec.shape()[1] == m_nne   );
  assert( elemvec.shape()[2] == m_ndim  );
  assert( qtensor.shape()[0] == m_nelem );
  assert( qtensor.shape()[1] == m_nne   );
  assert( qtensor.shape()[2] == m_tdim  );
  assert( qtensor.shape()[3] == m_tdim  );

  // zero-initialize (zero components not written below)
  qtensor.fill(0.0);

  // loop over all elements (in parallel)
  #pragma omp parallel for
  for ( size_t e = 0 ; e < m_nelem ; ++e )
  {
    // alias element vector (e.g. nodal displacements)
    auto u = xt::adapt(&elemvec(e,0,0), xt::xshape<m_nne,m_ndim>());

    // loop over all integration points in element "e"
    for ( size_t q = 0 ; q < m_nip ; ++q )
    {
      // - alias
      auto B     = xt::adapt(&m_B    (e,q,0,0,0,0), xt::xshape<m_nne,m_tdim,m_tdim,m_tdim>());
      auto gradu = xt::adapt(&qtensor(e,q,0,0    ), xt::xshape<m_tdim,m_tdim>());

      // - evaluate dyadic product (loops unrolled for efficiency)
      //   gradu(i,j) += B(m,i,j,k) * u(m,perm(k))
      //   (where perm(0) = 1, perm(2) = 0)
      gradu(0,0) = B(0,0,0,0)*u(0,1) + B(1,0,0,0)*u(1,1) + B(2,0,0,0)*u(2,1) + B(3,0,0,0)*u(3,1);
      gradu(1,1) = B(0,1,1,0)*u(0,1) + B(1,1,1,0)*u(1,1) + B(2,1,1,0)*u(2,1) + B(3,1,1,0)*u(3,1);
      gradu(2,2) = B(0,2,2,2)*u(0,0) + B(1,2,2,2)*u(1,0) + B(2,2,2,2)*u(2,0) + B(3,2,2,2)*u(3,0);
      gradu(0,2) = B(0,0,2,2)*u(0,0) + B(1,0,2,2)*u(1,0) + B(2,0,2,2)*u(2,0) + B(3,0,2,2)*u(3,0);
      gradu(2,0) = B(0,2,0,0)*u(0,1) + B(1,2,0,0)*u(1,1) + B(2,2,0,0)*u(2,1) + B(3,2,0,0)*u(3,1);
    }
  }
}

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::gradN_vector_T(
  const xt::xtensor<double,3> &elemvec, xt::xtensor<double,4> &qtensor) const
{
  assert( elemvec.shape()[0] == m_nelem );
  assert( elemvec.shape()[1] == m_nne   );
  assert( elemvec.shape()[2] == m_ndim  );
  assert( qtensor.shape()[0] == m_nelem );
  assert( qtensor.shape()[1] == m_nne   );
  assert( qtensor.shape()[2] == m_tdim  );
  assert( qtensor.shape()[3] == m_tdim  );

  // zero-initialize (zero components not written below)
  qtensor.fill(0.0);

  // loop over all elements (in parallel)
  #pragma omp parallel for
  for ( size_t e = 0 ; e < m_nelem ; ++e )
  {
    // alias element vector (e.g. nodal displacements)
    auto u = xt::adapt(&elemvec(e,0,0), xt::xshape<m_nne,m_ndim>());

    // loop over all integration points in element "e"
    for ( size_t q = 0 ; q < m_nip ; ++q )
    {
      // - alias
      auto B     = xt::adapt(&m_B    (e,q,0,0,0,0), xt::xshape<m_nne,m_tdim,m_tdim,m_tdim>());
      auto gradu = xt::adapt(&qtensor(e,q,0,0    ), xt::xshape<m_tdim,m_tdim>());

      // - evaluate transpose of dyadic product (loops unrolled for efficiency)
      //   gradu(j,i) += B(m,i,j,k) * u(m,perm(k))
      //   (where perm(0) = 1, perm(2) = 0)
      gradu(0,0) = B(0,0,0,0)*u(0,1) + B(1,0,0,0)*u(1,1) + B(2,0,0,0)*u(2,1) + B(3,0,0,0)*u(3,1);
      gradu(1,1) = B(0,1,1,0)*u(0,1) + B(1,1,1,0)*u(1,1) + B(2,1,1,0)*u(2,1) + B(3,1,1,0)*u(3,1);
      gradu(2,2) = B(0,2,2,2)*u(0,0) + B(1,2,2,2)*u(1,0) + B(2,2,2,2)*u(2,0) + B(3,2,2,2)*u(3,0);
      gradu(2,0) = B(0,0,2,2)*u(0,0) + B(1,0,2,2)*u(1,0) + B(2,0,2,2)*u(2,0) + B(3,0,2,2)*u(3,0);
      gradu(0,2) = B(0,2,0,0)*u(0,1) + B(1,2,0,0)*u(1,1) + B(2,2,0,0)*u(2,1) + B(3,2,0,0)*u(3,1);
    }
  }
}

// -------------------------------------------------------------------------------------------------


inline void QuadratureAxisymmetric::symGradN_vector(
  const xt::xtensor<double,3> &elemvec, xt::xtensor<double,4> &qtensor) const
{
  assert( elemvec.shape()[0] == m_nelem );
  assert( elemvec.shape()[1] == m_nne   );
  assert( elemvec.shape()[2] == m_ndim  );
  assert( qtensor.shape()[0] == m_nelem );
  assert( qtensor.shape()[1] == m_nne   );
  assert( qtensor.shape()[2] == m_tdim  );
  assert( qtensor.shape()[3] == m_tdim  );

  // zero-initialize (zero components not written below)
  qtensor.fill(0.0);

  // loop over all elements (in parallel)
  #pragma omp parallel for
  for ( size_t e = 0 ; e < m_nelem ; ++e )
  {
    // alias element vector (e.g. nodal displacements)
    auto u = xt::adapt(&elemvec(e,0,0), xt::xshape<m_nne,m_ndim>());

    // loop over all integration points in element "e"
    for ( size_t q = 0 ; q < m_nip ; ++q )
    {
      // - alias
      auto B   = xt::adapt(&m_B    (e,q,0,0,0,0), xt::xshape<m_nne,m_tdim,m_tdim,m_tdim>());
      auto eps = xt::adapt(&qtensor(e,q,0,0    ), xt::xshape<m_tdim,m_tdim>());

      // - evaluate symmetrized dyadic product (loops unrolled for efficiency)
      //   gradu(j,i) += B(m,i,j,k) * u(m,perm(k))
      //   eps  (j,i)  = 0.5 * ( gradu(i,j) + gradu(j,i) )
      //   (where perm(0) = 1, perm(2) = 0)
      eps(0,0) =   B(0,0,0,0)*u(0,1) + B(1,0,0,0)*u(1,1) + B(2,0,0,0)*u(2,1) + B(3,0,0,0)*u(3,1);
      eps(1,1) =   B(0,1,1,0)*u(0,1) + B(1,1,1,0)*u(1,1) + B(2,1,1,0)*u(2,1) + B(3,1,1,0)*u(3,1);
      eps(2,2) =   B(0,2,2,2)*u(0,0) + B(1,2,2,2)*u(1,0) + B(2,2,2,2)*u(2,0) + B(3,2,2,2)*u(3,0);
      eps(2,0) = ( B(0,0,2,2)*u(0,0) + B(1,0,2,2)*u(1,0) + B(2,0,2,2)*u(2,0) + B(3,0,2,2)*u(3,0) +
                   B(0,2,0,0)*u(0,1) + B(1,2,0,0)*u(1,1) + B(2,2,0,0)*u(2,1) + B(3,2,0,0)*u(3,1) ) * 0.5;
      eps(0,2) =   eps(2,0);
    }
  }
}

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::int_N_scalar_NT_dV(
  const xt::xtensor<double,2> &qscalar, xt::xtensor<double,3> &elemmat) const
{
  assert( qscalar.shape()[0] == m_nelem      );
  assert( qscalar.shape()[1] == m_nip        );
  assert( elemmat.shape()[0] == m_nelem      );
  assert( elemmat.shape()[1] == m_nne*m_ndim );
  assert( elemmat.shape()[2] == m_nne*m_ndim );

  // zero-initialize: matrix of matrices
  elemmat.fill(0.0);

  // loop over all elements (in parallel)
  #pragma omp parallel for
  for ( size_t e = 0 ; e < m_nelem ; ++e )
  {
    // alias (e.g. mass matrix)
    auto M = xt::adapt(&elemmat(e,0,0), xt::xshape<m_nne*m_ndim,m_nne*m_ndim>());

    // loop over all integration points in element "e"
    for ( size_t q = 0 ; q < m_nip ; ++q )
    {
      // - alias
      auto  N   = xt::adapt(&m_N(q,0), xt::xshape<m_nne>());
      auto& vol = m_vol  (e,q);
      auto& rho = qscalar(e,q);

      // - evaluate scalar product, for all dimensions, and assemble
      //   M(m*ndim+i,n*ndim+i) += N(m) * scalar * N(n) * dV
      for ( size_t m = 0 ; m < m_nne ; ++m ) {
        for ( size_t n = 0 ; n < m_nne ; ++n ) {
          M(m*m_ndim+0, n*m_ndim+0) += N(m) * rho * N(n) * vol;
          M(m*m_ndim+1, n*m_ndim+1) += N(m) * rho * N(n) * vol;
        }
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::int_gradN_dot_tensor2_dV(const xt::xtensor<double, 4> &qtensor,
  xt::xtensor<double, 3> &elemvec) const
{
  assert( qtensor.shape()[0] == m_nelem );
  assert( qtensor.shape()[1] == m_nip   );
  assert( qtensor.shape()[2] == m_tdim  );
  assert( qtensor.shape()[3] == m_tdim  );
  assert( elemvec.shape()[0] == m_nelem );
  assert( elemvec.shape()[1] == m_nne   );
  assert( elemvec.shape()[2] == m_ndim  );

  // zero-initialize output: matrix of vectors
  elemvec.fill(0.0);

  // loop over all elements (in parallel)
  #pragma omp parallel for
  for ( size_t e = 0 ; e < m_nelem ; ++e )
  {
    // alias (e.g. nodal force)
    auto f = xt::adapt(&elemvec(e,0,0), xt::xshape<m_nne,m_ndim>());

    // loop over all integration points in element "e"
    for ( size_t q = 0 ; q < m_nip ; ++q )
    {
      // - alias
      auto  B   = xt::adapt(&m_B(e,q,0,0,0,0), xt::xshape<m_nne,m_tdim,m_tdim,m_tdim>());
      auto  sig = xt::adapt(&qtensor(e,q,0,0), xt::xshape<m_tdim,m_tdim>());
      auto& vol = m_vol(e,q);

      // - evaluate dot product, and assemble
      //   f(m,i) += B(m,i,j,perm(k)) * sig(i,j) * dV
      //   (where perm(0) = 1, perm(2) = 0)
      for ( size_t m = 0 ; m < m_nne ; ++m )
      {
        f(m,0) += ( B(m,2,2,2)*sig(2,2) + B(m,0,2,2)*sig(0,2)                       ) * vol;
        f(m,1) += ( B(m,0,0,0)*sig(0,0) + B(m,1,1,0)*sig(1,1) + B(m,2,0,0)*sig(2,0) ) * vol;
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

inline void QuadratureAxisymmetric::int_gradN_dot_tensor4_dot_gradNT_dV(const xt::xtensor<double,6> &qtensor,
  xt::xtensor<double,3> &elemmat) const
{
  assert( qtensor.shape()[0] == m_nelem );
  assert( qtensor.shape()[1] == m_nip   );
  assert( qtensor.shape()[2] == m_tdim  );
  assert( qtensor.shape()[3] == m_tdim  );
  assert( qtensor.shape()[4] == m_tdim  );
  assert( qtensor.shape()[5] == m_tdim  );

  assert( elemmat.shape()[0] == m_nelem      );
  assert( elemmat.shape()[1] == m_nne*m_ndim );
  assert( elemmat.shape()[2] == m_nne*m_ndim );

  // zero-initialize output: matrix of vector
  elemmat.fill(0.0);

  // loop over all elements (in parallel)
  #pragma omp parallel for
  for ( size_t e = 0 ; e < m_nelem ; ++e )
  {
    // alias (e.g. nodal force)
    auto K = xt::adapt(&elemmat(e,0,0), xt::xshape<m_nne*m_ndim,m_nne*m_ndim>());

    // loop over all integration points in element "e"
    for ( size_t q = 0 ; q < m_nip ; ++q )
    {
      // - alias
      auto  B   = xt::adapt(&m_B(e,q,0,0,0,0), xt::xshape<m_nne,m_tdim,m_tdim,m_tdim>());
      auto  C   = xt::adapt(&qtensor(e,q,0,0,0,0), xt::xshape<m_tdim,m_tdim,m_tdim,m_tdim>());
      auto& vol = m_vol(e,q);

      // - compute product:
      //   K(m*m_ndim+perm(c), n*m_ndim+perm(f)) = B(m,a,b,c) * C(a,b,d,e) * B(n,e,d,f) * vol;
      //   (where perm(0) = 1, perm(2) = 0)
      for ( size_t m = 0 ; m < m_nne ; ++m )
      {
        for ( size_t n = 0 ; n < m_nne ; ++n )
        {
          K(m*m_ndim+1, n*m_ndim+1) += B(m,0,0,0) * C(0,0,0,0) * B(n,0,0,0) * vol;
          K(m*m_ndim+1, n*m_ndim+1) += B(m,0,0,0) * C(0,0,1,1) * B(n,1,1,0) * vol;
          K(m*m_ndim+1, n*m_ndim+0) += B(m,0,0,0) * C(0,0,2,2) * B(n,2,2,2) * vol;
          K(m*m_ndim+1, n*m_ndim+0) += B(m,0,0,0) * C(0,0,2,0) * B(n,0,2,2) * vol;
          K(m*m_ndim+1, n*m_ndim+1) += B(m,0,0,0) * C(0,0,0,2) * B(n,2,0,0) * vol;

          K(m*m_ndim+1, n*m_ndim+1) += B(m,1,1,0) * C(1,1,0,0) * B(n,0,0,0) * vol;
          K(m*m_ndim+1, n*m_ndim+1) += B(m,1,1,0) * C(1,1,1,1) * B(n,1,1,0) * vol;
          K(m*m_ndim+1, n*m_ndim+0) += B(m,1,1,0) * C(1,1,2,2) * B(n,2,2,2) * vol;
          K(m*m_ndim+1, n*m_ndim+0) += B(m,1,1,0) * C(1,1,2,0) * B(n,0,2,2) * vol;
          K(m*m_ndim+1, n*m_ndim+1) += B(m,1,1,0) * C(1,1,0,2) * B(n,2,0,0) * vol;

          K(m*m_ndim+0, n*m_ndim+1) += B(m,2,2,2) * C(2,2,0,0) * B(n,0,0,0) * vol;
          K(m*m_ndim+0, n*m_ndim+1) += B(m,2,2,2) * C(2,2,1,1) * B(n,1,1,0) * vol;
          K(m*m_ndim+0, n*m_ndim+0) += B(m,2,2,2) * C(2,2,2,2) * B(n,2,2,2) * vol;
          K(m*m_ndim+0, n*m_ndim+0) += B(m,2,2,2) * C(2,2,2,0) * B(n,0,2,2) * vol;
          K(m*m_ndim+0, n*m_ndim+1) += B(m,2,2,2) * C(2,2,0,2) * B(n,2,0,0) * vol;

          K(m*m_ndim+0, n*m_ndim+1) += B(m,0,2,2) * C(0,2,0,0) * B(n,0,0,0) * vol;
          K(m*m_ndim+0, n*m_ndim+1) += B(m,0,2,2) * C(0,2,1,1) * B(n,1,1,0) * vol;
          K(m*m_ndim+0, n*m_ndim+0) += B(m,0,2,2) * C(0,2,2,2) * B(n,2,2,2) * vol;
          K(m*m_ndim+0, n*m_ndim+0) += B(m,0,2,2) * C(0,2,2,0) * B(n,0,2,2) * vol;
          K(m*m_ndim+0, n*m_ndim+1) += B(m,0,2,2) * C(0,2,0,2) * B(n,2,0,0) * vol;

          K(m*m_ndim+1, n*m_ndim+1) += B(m,2,0,0) * C(2,0,0,0) * B(n,0,0,0) * vol;
          K(m*m_ndim+1, n*m_ndim+1) += B(m,2,0,0) * C(2,0,1,1) * B(n,1,1,0) * vol;
          K(m*m_ndim+1, n*m_ndim+0) += B(m,2,0,0) * C(2,0,2,2) * B(n,2,2,2) * vol;
          K(m*m_ndim+1, n*m_ndim+0) += B(m,2,0,0) * C(2,0,2,0) * B(n,0,2,2) * vol;
          K(m*m_ndim+1, n*m_ndim+1) += B(m,2,0,0) * C(2,0,0,2) * B(n,2,0,0) * vol;
        }
      }
    }
  }
}

// -------------------------------------------------------------------------------------------------

inline xt::xtensor<double,2> QuadratureAxisymmetric::dV() const
{
  xt::xtensor<double,2> out = xt::empty<double>({m_nelem, m_nip});

  this->dV(out);

  return out;
}

// -------------------------------------------------------------------------------------------------

inline xt::xtensor<double,4> QuadratureAxisymmetric::dVtensor() const
{
  xt::xtensor<double,4> out = xt::empty<double>({m_nelem, m_nip, m_tdim, m_tdim});

  this->dV(out);

  return out;
}

// -------------------------------------------------------------------------------------------------

inline xt::xtensor<double,4> QuadratureAxisymmetric::gradN_vector(const xt::xtensor<double,3> &elemvec) const
{
  xt::xtensor<double,4> qtensor = xt::empty<double>({m_nelem, m_nip, m_tdim, m_tdim});

  this->gradN_vector(elemvec, qtensor);

  return qtensor;
}

// -------------------------------------------------------------------------------------------------

inline xt::xtensor<double,4> QuadratureAxisymmetric::gradN_vector_T(const xt::xtensor<double,3> &elemvec) const
{
  xt::xtensor<double,4> qtensor = xt::empty<double>({m_nelem, m_nip, m_tdim, m_tdim});

  this->gradN_vector_T(elemvec, qtensor);

  return qtensor;
}

// -------------------------------------------------------------------------------------------------

inline xt::xtensor<double,4> QuadratureAxisymmetric::symGradN_vector(const xt::xtensor<double,3> &elemvec) const
{
  xt::xtensor<double,4> qtensor = xt::empty<double>({m_nelem, m_nip, m_tdim, m_tdim});

  this->symGradN_vector(elemvec, qtensor);

  return qtensor;
}

// -------------------------------------------------------------------------------------------------

inline xt::xtensor<double,3> QuadratureAxisymmetric::int_N_scalar_NT_dV(
  const xt::xtensor<double,2> &qscalar) const
{
  xt::xtensor<double,3> elemmat = xt::empty<double>({m_nelem, m_nne*m_ndim, m_nne*m_ndim});

  this->int_N_scalar_NT_dV(qscalar, elemmat);

  return elemmat;
}

// -------------------------------------------------------------------------------------------------

inline xt::xtensor<double,3> QuadratureAxisymmetric::int_gradN_dot_tensor2_dV(
  const xt::xtensor<double,4> &qtensor) const
{
  xt::xtensor<double,3> elemvec = xt::empty<double>({m_nelem, m_nne, m_ndim});

  this->int_gradN_dot_tensor2_dV(qtensor, elemvec);

  return elemvec;
}

// -------------------------------------------------------------------------------------------------

inline xt::xtensor<double,3> QuadratureAxisymmetric::int_gradN_dot_tensor4_dot_gradNT_dV(
  const xt::xtensor<double,6> &qtensor) const
 {
   xt::xtensor<double,3> elemmat = xt::empty<double>({m_nelem, m_ndim*m_nne, m_ndim*m_nne});

   this->int_gradN_dot_tensor4_dot_gradNT_dV(qtensor, elemmat);

   return elemmat;
 }

// -------------------------------------------------------------------------------------------------

}}} // namespace ...

// =================================================================================================

#endif