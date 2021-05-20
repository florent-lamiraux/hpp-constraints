// Copyright (c) 2016, Joseph Mirabel
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

#define EIGEN_RUNTIME_NO_MALLOC

#include <hpp/constraints/generic-transformation.hh>
#include <hpp/constraints/explicit/relative-pose.hh>
#include <hpp/constraints/solver/by-substitution.hh>

#include <sstream>
#include <pinocchio/algorithm/joint-configuration.hpp>

#include <hpp/pinocchio/device.hh>
#include <hpp/pinocchio/joint.hh>
#include <hpp/pinocchio/joint-collection.hh>
#include <hpp/pinocchio/configuration.hh>
#include <hpp/pinocchio/simple-device.hh>
#include <hpp/pinocchio/serialization.hh>
#include <hpp/pinocchio/urdf/util.hh>

#include "hpp/constraints/tools.hh"

#define BOOST_TEST_MODULE hpp_constraints
#include <boost/test/included/unit_test.hpp>

#include <stdlib.h>

using hpp::pinocchio::Configuration_t;
using hpp::pinocchio::ConfigurationPtr_t;
using hpp::pinocchio::Device;
using hpp::pinocchio::DevicePtr_t;
using hpp::pinocchio::JointPtr_t;
using hpp::pinocchio::LiegroupSpace;
using hpp::pinocchio::Transform3f;

using hpp::pinocchio::urdf::loadModelFromString;

using namespace hpp::constraints;

class BasicConfigurationShooter
{
public:
  BasicConfigurationShooter (const DevicePtr_t& robot) : robot_ (robot)
  {
  }
  virtual ConfigurationPtr_t shoot () const
  {
    size_type extraDim = robot_->extraConfigSpace ().dimension ();
    size_type offset = robot_->configSize () - extraDim;

    Configuration_t config(robot_->configSize ());
    config.head(offset) = ::pinocchio::randomConfiguration(robot_->model());

    // Shoot extra configuration variables
    for (size_type i=0; i<extraDim; ++i) {
      value_type lower = robot_->extraConfigSpace ().lower (i);
      value_type upper = robot_->extraConfigSpace ().upper (i);
      value_type range = upper - lower;
      if ((range < 0) ||
          (range == std::numeric_limits<double>::infinity())) {
        std::ostringstream oss
          ("Cannot uniformy sample extra config variable ");
        oss << i << ". min = " <<lower<< ", max = " << upper << std::endl;
        throw std::runtime_error (oss.str ());
      }
      config [offset + i] = lower + (upper - lower) * rand ()/RAND_MAX;
    }
    return hpp::make_shared<Configuration_t>(config);
  }
private:
  DevicePtr_t robot_;
}; // class BasicConfigurationShooter

BOOST_AUTO_TEST_CASE (print) {
  DevicePtr_t device = hpp::pinocchio::unittest::makeDevice(
      hpp::pinocchio::unittest::HumanoidSimple);
  JointPtr_t ee1 = device->getJointByName ("lleg5_joint"),
             ee2 = device->getJointByName ("rleg5_joint");
  BOOST_REQUIRE (device);
  BasicConfigurationShooter cs (device);

  device->currentConfiguration (*cs.shoot ());
  device->computeForwardKinematics ();
  Transform3f tf1 (ee1->currentTransformation ());
  Transform3f tf2 (ee2->currentTransformation ());

  std::vector<DifferentiableFunctionPtr_t> functions;
  functions.push_back(Orientation::create            ("Orientation"           , device, ee2, tf2)          );
  functions.push_back(Position::create               ("Position"              , device, ee2, tf2, tf1)     );
  functions.push_back(Transformation::create         ("Transformation"        , device, ee1, tf1)          );
  functions.push_back(RelativeOrientation::create    ("RelativeOrientation"   , device, ee1, ee2, tf1)     );
  functions.push_back(RelativePosition::create       ("RelativePosition"      , device, ee1, ee2, tf1, tf2));
  functions.push_back(RelativeTransformation::create ("RelativeTransformation", device, ee1, ee2, tf1, tf2));

  Configuration_t q1 = *cs.shoot(),
                  q2 = *cs.shoot();
  for (std::size_t i = 0; i < functions.size(); ++i) {
    DifferentiableFunctionPtr_t f = functions[i];

    std::cout << *f << std::endl;

    LiegroupElement v (f->outputSpace());
    matrix_t J (f->outputDerivativeSize(), f->inputDerivativeSize());

    f->value    (v, q1);
    f->jacobian (J, q1);
    // TODO this is broken at the moment because of the introduction
    // of a multithreaded device.
    // Eigen::internal::set_is_malloc_allowed(false);
    f->value    (v, q2);
    f->jacobian (J, q2);
    // Eigen::internal::set_is_malloc_allowed(true);
  }

  // Check active parameters
  ArrayXb ap1 = Orientation::create ("Orientation"           , device, ee1, tf1)->activeParameters();
  ArrayXb ap2 = Orientation::create ("Orientation"           , device, ee2, tf2)->activeParameters();
  ArrayXb ap12 = RelativeOrientation::create    ("RelativeOrientation"   , device, ee1, ee2, tf1)->activeParameters();

  ArrayXb not_ap1 = (ap1 == false);
  ArrayXb not_ap2 = (ap2 == false);
  BOOST_CHECK ((ap12 == ((not_ap1 && ap2) || (ap1 && not_ap2))).all());

  // Check active derivative parameters
  ap1 = Orientation::create ("Orientation"           , device, ee1, tf1)->activeDerivativeParameters();
  ap2 = Orientation::create ("Orientation"           , device, ee2, tf2)->activeDerivativeParameters();
  ap12 = RelativeOrientation::create    ("RelativeOrientation"   , device, ee1, ee2, tf1)->activeDerivativeParameters();

  not_ap1 = (ap1 == false);
  not_ap2 = (ap2 == false);
  BOOST_CHECK ((ap12 == ((not_ap1 && ap2) || (ap1 && not_ap2))).all());
}

BOOST_AUTO_TEST_CASE (multithread) {
  DevicePtr_t device = hpp::pinocchio::unittest::makeDevice(
      hpp::pinocchio::unittest::HumanoidSimple);
  device->numberDeviceData (4);
  JointPtr_t ee1 = device->getJointByName ("lleg5_joint"),
             ee2 = device->getJointByName ("rleg5_joint");
  BOOST_REQUIRE (device);
  BasicConfigurationShooter cs (device);

  device->currentConfiguration (*cs.shoot ());
  device->computeForwardKinematics ();
  Transform3f tf1 (ee1->currentTransformation ());
  Transform3f tf2 (ee2->currentTransformation ());

  std::vector<DifferentiableFunctionPtr_t> functions;
  functions.push_back(Orientation::create            ("Orientation"           , device, ee2, tf2)          );
  functions.push_back(Position::create               ("Position"              , device, ee2, tf2, tf1)     );
  functions.push_back(Transformation::create         ("Transformation"        , device, ee1, tf1)          );
  functions.push_back(RelativeOrientation::create    ("RelativeOrientation"   , device, ee1, ee2, tf1)     );
  functions.push_back(RelativePosition::create       ("RelativePosition"      , device, ee1, ee2, tf1, tf2));
  functions.push_back(RelativeTransformation::create ("RelativeTransformation", device, ee1, ee2, tf1, tf2));
  functions.push_back(RelativeOrientation::create    ("RelativeOrientation"   , device, ee1, JointPtr_t(), tf1)     );
  functions.push_back(RelativePosition::create       ("RelativePosition"      , device, ee1, JointPtr_t(), tf1, tf2));
  functions.push_back(RelativeTransformation::create ("RelativeTransformation", device, ee1, JointPtr_t(), tf1, tf2));

  const int N = 10;
  Configuration_t q = *cs.shoot();
  for (std::size_t i = 0; i < functions.size(); ++i) {
    DifferentiableFunctionPtr_t f = functions[i];

    std::vector <LiegroupElement> vs (N, LiegroupElement (f->outputSpace()));
    std::vector <matrix_t> Js (N, matrix_t(f->outputDerivativeSize(), f->inputDerivativeSize()));
#pragma omp parallel for
    for (int j = 0; j < 10; ++j) {
      f->value    (vs[j], q);
      f->jacobian (Js[j], q);
    }

    for (int j = 1; j < N; ++j) {
      BOOST_CHECK_EQUAL (vs[0].vector(), vs[j].vector());
      BOOST_CHECK_EQUAL (Js[0]         , Js[j]);
    }
  }
}

BOOST_AUTO_TEST_CASE (serialization) {
  DevicePtr_t device = hpp::pinocchio::unittest::makeDevice(
      hpp::pinocchio::unittest::HumanoidSimple);
  device->numberDeviceData (4);
  JointPtr_t ee1 = device->getJointByName ("lleg5_joint"),
             ee2 = device->getJointByName ("rleg5_joint");
  BOOST_REQUIRE (device);

  device->currentConfiguration (device->neutralConfiguration());
  device->computeForwardKinematics ();
  Transform3f tf1 (ee1->currentTransformation ());
  Transform3f tf2 (ee2->currentTransformation ());

  std::vector<DifferentiableFunctionPtr_t> functions;
  functions.push_back(Orientation::create            ("Orientation"           , device, ee2, tf2)          );
  functions.push_back(Position::create               ("Position"              , device, ee2, tf2, tf1)     );
  functions.push_back(Transformation::create         ("Transformation"        , device, ee1, tf1)          );
  functions.push_back(RelativeOrientation::create    ("RelativeOrientation"   , device, ee1, ee2, tf1)     );
  functions.push_back(RelativePosition::create       ("RelativePosition"      , device, ee1, ee2, tf1, tf2));
  functions.push_back(RelativeTransformation::create ("RelativeTransformation", device, ee1, ee2, tf1, tf2));
  functions.push_back(RelativeOrientation::create    ("RelativeOrientation"   , device, ee1, JointPtr_t(), tf1)     );
  functions.push_back(RelativePosition::create       ("RelativePosition"      , device, ee1, JointPtr_t(), tf1, tf2));
  functions.push_back(RelativeTransformation::create ("RelativeTransformation", device, ee1, JointPtr_t(), tf1, tf2));

  for (std::size_t i = 0; i < functions.size(); ++i) {
    DifferentiableFunctionPtr_t f = functions[i];

    DifferentiableFunctionPtr_t f_restored;

    std::stringstream ss;
    {
      hpp::serialization::xml_oarchive oa(ss);
      oa.insert(device->name(), device.get());
      oa << boost::serialization::make_nvp("function", f);
    }

    {
      hpp::serialization::xml_iarchive ia(ss);
      ia.insert(device->name(), device.get());
      ia >> boost::serialization::make_nvp("function", f_restored);
    }

    std::ostringstream oss1, oss2;
    oss1 << *f;
    oss2 << *f_restored;

    BOOST_CHECK_EQUAL(oss1.str(), oss2.str());
  }
}

BOOST_AUTO_TEST_CASE(RelativeTransformation_R3xSO3)
{
  const std::string model("<robot name=\"box\">"
                          "  <link name=\"baselink\">"
                          "  </link>"
                          "</robot>");

  DevicePtr_t robot(Device::create("two-freeflyers"));
  // Create two freeflying boxes
  loadModelFromString(robot, 0, "1/", "freeflyer", model, "");
  loadModelFromString(robot, 0, "2/", "freeflyer", model, "");
  BOOST_CHECK(robot->configSize() == 14);
  BOOST_CHECK(robot->numberDof() == 12);
  BOOST_CHECK(robot->nbJoints() == 2);
  JointPtr_t j1(robot->jointAt(0));
  JointPtr_t j2(robot->jointAt(1));

  // Set joint bounds
  for (std::size_t i=0; i<2; ++i) {
    vector_t l(7); l << -2,-2,-2,-1,-1,-1,-1;
    vector_t u(7); u <<  2, 2, 2, 1, 1, 1, 1;
    robot->jointAt(i)->lowerBounds(l);
    robot->jointAt(i)->upperBounds(u);
  }
  // Create constraint
  //
  // Excerpt from romeo-placard benchmark
  // Joint1: romeo/LWristPitch
  // Frame in joint 1
  //   R = 0.7071067739978436073, 0.70710678837525142715, 0
  //       -2.2663502965461253728e-09,  2.2663502504650490188e-09, -1
  //       -0.70710678837525142715, 0.70710677399784382935, 3.2051032938795742666e-09
  //   p = 0.099999999776482578762, -3.2051032222399330291e-11, -0.029999999776482582509
  // Joint2: placard/root_joint
  // Frame in joint 2
  //   R =   1, 0, 0
  //         0, 1, 0
  //         0, 0, 1
  //   p = 0, 0, -0.34999999403953552246
  // mask: 1, 1, 1, 1, 1, 1,
  // Rhs: 0, 0, 0, 0, 0, 0, 1
              // active rows: [ 0, 5],

  matrix3_t R1, R2;
  vector3_t p1, p2;
  R1 << 0.7071067739978436073, 0.70710678837525142715, 0,
    -2.2663502965461253728e-09,  2.2663502504650490188e-09, -1,
    -0.70710678837525142715, 0.70710677399784382935, 3.2051032938795742666e-09;
  p1 << 0.099999999776482578762, -3.2051032222399330291e-11,
    -0.029999999776482582509;
  R2.setIdentity();
  p2 << 0, 0, -0.34999999403953552246;
  Transform3f tf1(R1, p1), tf2(R2,p2);
  std::vector<bool> mask = {false, false, false, false, false, true};
  ImplicitPtr_t constraint
    (Implicit::create (RelativeTransformationSE3::create
		       ("RelativeTransformationSE3", robot, j1, j2,
			tf1, tf2),
		       6 * Equality, mask));
  BasicConfigurationShooter cs (robot);
  solver::BySubstitution solver(robot->configSpace());
  solver.errorThreshold(1e-10);
  solver.add(constraint);
  // Check that after setting right hand side with a configuration
  // the configuration satisfies the constraint since comparison type is
  // Equality.
  for(size_type i=0; i<1000; ++i)
  {
    ConfigurationPtr_t q(cs.shoot());
    vector6_t error;
    solver.rightHandSideFromConfig(*q);
    BOOST_CHECK(solver.isSatisfied(*q, error));
  }

  // Create grasp constraint with one degree of freedom in rotation along z
  mask = {true, true, true, true, true, false};
  ImplicitPtr_t c1 (Implicit::create(RelativeTransformationSE3::create
				     ("RelativeTransformationSE3", robot,
				      j1, j2, tf1, tf2), 6*EqualToZero, mask));
  solver::BySubstitution s1(robot->configSpace());
  s1.errorThreshold(1e-10);
  s1.add(c1);
  // Create grasp + complement as an explicit constraint
  ExplicitPtr_t c2(explicit_::RelativePose::create
		   ("ExplicitRelativePose", robot, j1, j2, tf1, tf2,
		    5 * EqualToZero << Equality));
  solver::BySubstitution s2(robot->configSpace());
  s2.errorThreshold(1e-4);
  s2.add(c2);

  for(size_type i=0; i<0; ++i)
  {
    ConfigurationPtr_t q_near(cs.shoot());
    ConfigurationPtr_t q_new(cs.shoot());
    if (i == 0)
    {
      // These configuration reproduce a numerical issue encountered with
      // benhmark romeo-placard.
      // If computation was exact, any configuration satisfying c2 should
      // satisfy c1.
      // Configuration q_new below satisfies c2 but not c1.
      *q_near << 0.18006349590534418, 0.3627623741970175, 0.9567759630330663,
	0.044416054309488175, 0.31532356328825556, 0.4604329042168087,
	0.8286131819306576,
	0.45813483973344404, 0.23514459283216355, 0.7573015903787429,
	0.8141495491430896, 0.1383820163733335, 0.3806970356973106,
	0.4160296818567576;
      *q_new << 0.16026892741853033, 0.33925098736439646, 0.8976880203169203,
	-0.040130835169737825, 0.37473431876437147, 0.4405275981290593,
	0.8148000624051422,
	0.43787674119234027, 0.18316291571416676, 0.7189377922181226,
	0.7699579340925136, 0.1989432638510445, 0.35960786236482944,
	0.4881275886709128;
    }
    s2.rightHandSideFromConfig(*q_near);
    vector6_t error;
    BOOST_CHECK(s1.isSatisfied(*q_near, error));
    hppDout(info, error);
    BOOST_CHECK(s2.isSatisfied(*q_near, error));
    hppDout(info, error);
    BOOST_CHECK(s1.isSatisfied(*q_new, error));
    hppDout(info, error);
    BOOST_CHECK(s2.isSatisfied(*q_new, error));
    hppDout(info, error);

    hppDout(info, s1);
    hppDout(info, s2);
  }
}
