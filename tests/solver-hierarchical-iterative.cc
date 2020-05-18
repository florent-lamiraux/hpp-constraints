// Copyright (c) 2017, Joseph Mirabel
// Authors: Joseph Mirabel (joseph.mirabel@laas.fr)
//
// This file is part of hpp-constraints.
// hpp-constraints is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-constraints is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-constraints. If not, see <http://www.gnu.org/licenses/>.

#define BOOST_TEST_MODULE HIERARCHICAL_ITERATIVE_SOLVER
#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <hpp/constraints/solver/hierarchical-iterative.hh>

#include <functional>

#include <pinocchio/algorithm/joint-configuration.hpp>

#include <hpp/pinocchio/device.hh>
#include <hpp/pinocchio/joint.hh>
#include <hpp/pinocchio/configuration.hh>
#include <hpp/pinocchio/simple-device.hh>
#include <hpp/pinocchio/liegroup-space.hh>
#include <hpp/constraints/generic-transformation.hh>
#include <hpp/constraints/implicit.hh>
#include <hpp/constraints/affine-function.hh>

#include <../tests/util.hh>

using namespace hpp::constraints;
namespace saturation = hpp::constraints::solver::saturation;

const value_type test_precision = 1e-5;

#define VECTOR2(x0, x1) ((hpp::constraints::vector_t (2) << x0, x1).finished())

using Eigen::VectorXi;
using hpp::pinocchio::LiegroupSpace;

template <typename LineSearch>
struct test_base
{
  solver::HierarchicalIterative solver;
  LineSearch ls;

  test_base (const size_type& d) : solver(LiegroupSpace::Rn (d))
  {
    solver.maxIterations(20);
    solver.errorThreshold(test_precision);
    solver.saturation(boost::make_shared<saturation::Bounds>(
          vector_t::Zero(2), vector_t::Ones(2)));
  }

  vector_t success (value_type x0, value_type x1)
  {
    vector_t x (VECTOR2(x0,x1));
    BOOST_CHECK_EQUAL(solver.solve(x, ls), solver::HierarchicalIterative::SUCCESS);
    return x;
  }

  vector_t failure (value_type x0, value_type x1)
  {
    vector_t x (VECTOR2(x0,x1));
    BOOST_CHECK_PREDICATE (std::not_equal_to<solver::HierarchicalIterative::Status>(), (solver.solve(x, ls))(solver::HierarchicalIterative::SUCCESS));
    return x;
  }
};

template <typename LineSearch = solver::lineSearch::Constant>
struct test_quadratic : test_base <LineSearch>
{
  test_quadratic (const matrix_t& A)
    : test_base<LineSearch>(A.cols())
  {
    // Find (x, y)
    // s.t. a * x^2 + b * y^2 - 1 = 0
    //      0 <= x <= 1
    //      0 <= y <= 1
    BOOST_REQUIRE_EQUAL (A.rows(), A.cols());
    BOOST_TEST_MESSAGE(A);
    Quadratic::Ptr_t f (new Quadratic (A, -1));

    this->solver.add (Implicit::create
                      (f, ComparisonTypes::nTimes(f->outputDerivativeSize (),
                                             Equality)), 0);
    BOOST_CHECK(this->solver.numberStacks() == 1);
  }
};

BOOST_AUTO_TEST_CASE(quadratic)
{
  matrix_t A(2,2);

  A << 1, 0, 0, 1;
  test_quadratic<> test (A);
  BOOST_CHECK_EQUAL (test.failure(0,0), VECTOR2(0,0));
  test.success(0.1,0);
  test.success(0,0.1);
  test.success(0.5, 0.5);

  A << 2, 0, 0, 2;
  test = test_quadratic<> (A);
  test.success(0.1,0);
  test.success(0,0.1);
  test.success(0.5, 0.5);

  A << 0.5, 0, 0, 0.5;
  test = test_quadratic<> (A);
  // This is exact because of the saturation
  BOOST_CHECK_EQUAL (test.success (1, 0.001), VECTOR2(1,1)); // Slide on the border x = 1
  BOOST_CHECK_EQUAL (test.success (0.001, 1), VECTOR2(1,1)); // Slide on the border y = 1

  A << 0.75, 0, 0, 0.75;
  test_quadratic<solver::lineSearch::FixedSequence> test4 (A);
  // This is not exact because the solver does not saturate.
  EIGEN_VECTOR_IS_APPROX (test4.success (1, 0.1), VECTOR2(1.,1/sqrt(3))); // Slide on the border x = 1
  EIGEN_VECTOR_IS_APPROX (test4.success (0.1, 1), VECTOR2(1/sqrt(3),1.)); // Slide on the border y = 1
  // There is an overshoot. To overcome this, the Hessian of the function should be obtained.
  EIGEN_VECTOR_IS_NOT_APPROX (test4.success (1, 0.001), VECTOR2(1.,1/sqrt(3))); // Slide on the border x = 1
  EIGEN_VECTOR_IS_NOT_APPROX (test4.success (0.001, 1), VECTOR2(1/sqrt(3),1.)); // Slide on the border y = 1

  // Ellipsoid: computations are approximative
  A << 0.5, 0, 0, 2;
  test_quadratic<solver::lineSearch::FixedSequence> test1 (A);
  BOOST_CHECK_EQUAL (test1.success (1, 0.5), VECTOR2(1.,0.5)); // Slide on the border x = 1
  EIGEN_VECTOR_IS_APPROX (test1.success (1, 0.1), VECTOR2(1.,0.5)); // Slide on the border x = 1
  EIGEN_VECTOR_IS_APPROX (test1.success (0, 1), VECTOR2(0.,1/sqrt(2)));
}

BOOST_AUTO_TEST_CASE(one_layer)
{
  DevicePtr_t device = hpp::pinocchio::unittest::makeDevice (hpp::pinocchio::unittest::HumanoidRomeo);
  BOOST_REQUIRE (device);
  device->rootJoint()->lowerBound (0, -1);
  device->rootJoint()->lowerBound (1, -1);
  device->rootJoint()->lowerBound (2, -1);
  device->rootJoint()->upperBound (0,  1);
  device->rootJoint()->upperBound (1,  1);
  device->rootJoint()->upperBound (2,  1);
  JointPtr_t ee1 = device->getJointByName ("LAnkleRoll"),
             ee2 = device->getJointByName ("RAnkleRoll");

  Configuration_t q = device->currentConfiguration (),
                  qrand = ::pinocchio::randomConfiguration(device->model());

  solver::HierarchicalIterative solver(device->configSpace());
  solver.maxIterations(20);
  solver.errorThreshold(1e-3);
  solver.saturation(boost::make_shared<solver::saturation::Device>(device));

  device->currentConfiguration (q);
  device->computeForwardKinematics ();
  Transform3f tf1 (ee1->currentTransformation ());
  Transform3f tf2 (ee2->currentTransformation ());

  ImplicitPtr_t constraint(Implicit::create
                  (Orientation::create ("Orientation", device, ee2, tf2),
                   3 * Equality));
  BOOST_CHECK(constraint->comparisonType() ==
              ComparisonTypes::nTimes(3, Equality));
  solver.add(constraint, 0);
  constraint = Implicit::create
    (Position::create    ("Position"   , device, ee2, tf2), 3 * Equality);
  BOOST_CHECK(constraint->comparisonType() ==
              ComparisonTypes::nTimes(3, Equality));
  solver.add(constraint, 0);

  BOOST_CHECK(solver.numberStacks() == 1);

  BOOST_CHECK(solver.isSatisfied(q));

  Configuration_t tmp = qrand;
  BOOST_CHECK_EQUAL(solver.solve<solver::lineSearch::Backtracking  >(qrand), solver::HierarchicalIterative::SUCCESS);
  qrand = tmp;
  BOOST_CHECK_EQUAL(solver.solve<solver::lineSearch::ErrorNormBased>(qrand), solver::HierarchicalIterative::SUCCESS);
  qrand = tmp;
  BOOST_CHECK_EQUAL(solver.solve<solver::lineSearch::FixedSequence >(qrand), solver::HierarchicalIterative::SUCCESS);
}

template <typename LineSearch = solver::lineSearch::Constant>
struct test_affine_opt : test_base <LineSearch>
{
  test_affine_opt (const matrix_t& A, const matrix_t& B)
    : test_base<LineSearch>(A.cols())
  {
    // min  X^T * B * X
    // s.t. A * X - 1 = 0
    //      0 <= X <= 1
    BOOST_REQUIRE_EQUAL (A.cols(), B.cols());
    BOOST_REQUIRE_EQUAL (A.rows(), 1);
    BOOST_TEST_MESSAGE(A);
    BOOST_TEST_MESSAGE(B);
    AffineFunctionPtr_t f (AffineFunction::create
                           (A, vector_t::Constant(1,-1)));
    Quadratic::Ptr_t cost (new Quadratic (B));
    ImplicitPtr_t f_constraint
      (Implicit::create (f, ComparisonTypes::nTimes
                         (f->outputDerivativeSize (), Equality)));
    ImplicitPtr_t cost_constraint
      (Implicit::create (cost, ComparisonTypes::nTimes
                         (f->outputDerivativeSize (), Equality)));
    this->solver.add(f_constraint, 0);
    this->solver.add(cost_constraint, 1);
    // this->solver.add(cost, 0);
    this->solver.lastIsOptional(true);
    BOOST_CHECK(this->solver.numberStacks() == 2);
  }

  vector_t optimize (value_type x0, value_type x1)
  {
    vector_t x (VECTOR2(x0,x1));
    this->solver.lastIsOptional(false);
    this->solver.solve(x, this->ls);
    this->solver.lastIsOptional(true);
    return x;
  }
};

BOOST_AUTO_TEST_CASE(affine_opt)
{
  matrix_t A(1,2);
  A << 1, 1;
  matrix_t B(2,2);
  B << 1, 0, 0, 1;

  test_affine_opt<> test (A, B);
  test.success(0  , 0  );
  test.success(0.1, 0  );
  test.success(0  , 0.1);
  test.success(0.5, 0.5);

  EIGEN_VECTOR_IS_APPROX (test.optimize(0.1,0), VECTOR2(0.5, 0.5));
  EIGEN_VECTOR_IS_APPROX (test.optimize(0,0.1), VECTOR2(0.5, 0.5));
  EIGEN_VECTOR_IS_APPROX (test.optimize(0.5, 0.5), VECTOR2(0.5, 0.5));
}

