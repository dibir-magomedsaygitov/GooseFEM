
#include "support.h"

// =================================================================================================

TEST_CASE("GooseFEM::Vector", "Vector.h")
{

// =================================================================================================

SECTION( "asDofs - nodevec" )
{
  // mesh
  GooseFEM::Mesh::Quad4::Regular mesh(2,2);

  // vector-definition
  GooseFEM::Vector vector(mesh.conn(), mesh.dofsPeriodic());

  // velocity field
  // - allocate
  GooseFEM::MatD v(mesh.nnode(),2);
  // - set periodic
  v(0,0) = 1.0;  v(0,1) = 0.0;
  v(1,0) = 1.0;  v(1,1) = 0.0;
  v(2,0) = 1.0;  v(2,1) = 0.0;
  v(3,0) = 1.5;  v(3,1) = 0.0;
  v(4,0) = 1.5;  v(4,1) = 0.0;
  v(5,0) = 1.5;  v(5,1) = 0.0;
  v(6,0) = 1.0;  v(6,1) = 0.0;
  v(7,0) = 1.0;  v(7,1) = 0.0;
  v(8,0) = 1.0;  v(8,1) = 0.0;

  // convert to DOFs
  GooseFEM::ColD V = vector.asDofs(v);

  // check
  // - size
  REQUIRE( V.size() == mesh.nnodePeriodic() * mesh.ndim() );
  // - individual entries
  EQ( V(0), v(0,0) );
  EQ( V(1), v(0,1) );
  EQ( V(2), v(1,0) );
  EQ( V(3), v(1,1) );
  EQ( V(4), v(3,0) );
  EQ( V(5), v(3,1) );
  EQ( V(6), v(4,0) );
  EQ( V(7), v(4,1) );
}

// -------------------------------------------------------------------------------------------------

SECTION( "asDofs - elemvec" )
{
  // mesh
  GooseFEM::Mesh::Quad4::Regular mesh(2,2);

  // vector-definition
  GooseFEM::Vector vector(mesh.conn(), mesh.dofsPeriodic());

  // velocity field
  // - allocate
  GooseFEM::MatD v(mesh.nnode(),2);
  // - set periodic
  v(0,0) = 1.0;  v(0,1) = 0.0;
  v(1,0) = 1.0;  v(1,1) = 0.0;
  v(2,0) = 1.0;  v(2,1) = 0.0;
  v(3,0) = 1.5;  v(3,1) = 0.0;
  v(4,0) = 1.5;  v(4,1) = 0.0;
  v(5,0) = 1.5;  v(5,1) = 0.0;
  v(6,0) = 1.0;  v(6,1) = 0.0;
  v(7,0) = 1.0;  v(7,1) = 0.0;
  v(8,0) = 1.0;  v(8,1) = 0.0;


  // convert to DOFs - element - DOFs
  GooseFEM::ColD V = vector.asDofs(vector.asElement(vector.asDofs(v)));

  // check
  // - size
  REQUIRE( V.size() == mesh.nnodePeriodic() * mesh.ndim() );
  // - individual entries
  EQ( V(0), v(0,0) );
  EQ( V(1), v(0,1) );
  EQ( V(2), v(1,0) );
  EQ( V(3), v(1,1) );
  EQ( V(4), v(3,0) );
  EQ( V(5), v(3,1) );
  EQ( V(6), v(4,0) );
  EQ( V(7), v(4,1) );
}

// -------------------------------------------------------------------------------------------------

SECTION( "asDofs - assembleDofs" )
{
  // mesh
  GooseFEM::Mesh::Quad4::Regular mesh(2,2);

  // vector-definition
  GooseFEM::Vector vector(mesh.conn(), mesh.dofsPeriodic());

  // force field
  // - allocate
  GooseFEM::MatD f(mesh.nnode(),2);
  // - set periodic
  f(0,0) = -1.0;  f(0,1) = -1.0;
  f(1,0) =  0.0;  f(1,1) = -1.0;
  f(2,0) =  1.0;  f(2,1) = -1.0;
  f(3,0) = -1.0;  f(3,1) =  0.0;
  f(4,0) =  0.0;  f(4,1) =  0.0;
  f(5,0) =  1.0;  f(5,1) =  0.0;
  f(6,0) = -1.0;  f(6,1) =  1.0;
  f(7,0) =  0.0;  f(7,1) =  1.0;
  f(8,0) =  1.0;  f(8,1) =  1.0;

  // assemble as DOFs
  GooseFEM::ColD F = vector.assembleDofs(f);

  // check
  // - size
  REQUIRE( F.size() == mesh.nnodePeriodic() * mesh.ndim() );
  // - 'analytical' result
  EQ( F(0), 0 );
  EQ( F(1), 0 );
  EQ( F(2), 0 );
  EQ( F(3), 0 );
  EQ( F(4), 0 );
  EQ( F(5), 0 );
  EQ( F(6), 0 );
  EQ( F(7), 0 );
}

// -------------------------------------------------------------------------------------------------

SECTION( "asDofs - assembleNode" )
{
  // mesh
  GooseFEM::Mesh::Quad4::Regular mesh(2,2);

  // vector-definition
  GooseFEM::Vector vector(mesh.conn(), mesh.dofsPeriodic());

  // force field
  // - allocate
  GooseFEM::MatD f(mesh.nnode(),2);
  // - set periodic
  f(0,0) = -1.0;  f(0,1) = -1.0;
  f(1,0) =  0.0;  f(1,1) = -1.0;
  f(2,0) =  1.0;  f(2,1) = -1.0;
  f(3,0) = -1.0;  f(3,1) =  0.0;
  f(4,0) =  0.0;  f(4,1) =  0.0;
  f(5,0) =  1.0;  f(5,1) =  0.0;
  f(6,0) = -1.0;  f(6,1) =  1.0;
  f(7,0) =  0.0;  f(7,1) =  1.0;
  f(8,0) =  1.0;  f(8,1) =  1.0;

  // convert to element, assemble as DOFs
  GooseFEM::ColD F = vector.assembleDofs( vector.asElement(f) );

  // check
  // - size
  REQUIRE( F.size() == mesh.nnodePeriodic() * mesh.ndim() );
  // - 'analytical' result
  EQ( F(0), 0 );
  EQ( F(1), 0 );
  EQ( F(2), 0 );
  EQ( F(3), 0 );
  EQ( F(4), 0 );
  EQ( F(5), 0 );
  EQ( F(6), 0 );
  EQ( F(7), 0 );
}

// -------------------------------------------------------------------------------------------------

// =================================================================================================

}
