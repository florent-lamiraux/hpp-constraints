// Copyright (c) 2015, Joseph Mirabel
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

#include <hpp/constraints/configuration-constraint.hh>

#include <hpp/util/debug.hh>
#include <hpp/pinocchio/device.hh>
#include <hpp/pinocchio/joint.hh>
#include <hpp/pinocchio/configuration.hh>

namespace hpp {
  namespace constraints {
    ConfigurationConstraintPtr_t ConfigurationConstraint::create (
        const std::string& name, const DevicePtr_t& robot,
        ConfigurationIn_t goal, std::vector <bool> mask)
    {
      ConfigurationConstraint* ptr = new ConfigurationConstraint
        (name, robot, goal, mask);
      return ConfigurationConstraintPtr_t (ptr);
    }

    ConfigurationConstraint::ConfigurationConstraint (
        const std::string& name, const DevicePtr_t& robot,
        ConfigurationIn_t goal, std::vector <bool> mask) :
      DifferentiableFunction (robot->configSize (), robot->numberDof (),
          1, name),
      robot_ (robot), goal_ (goal), diff_ (robot->numberDof())
    {
      mask_ = EigenBoolVector_t (robot->numberDof ());
      for (std::size_t i = 0; i < mask.size (); ++i) {
        mask_[i] = mask[i];
      }
      mask_.tail (robot->numberDof () - mask.size ()).setConstant (true);
    }

    void ConfigurationConstraint::impl_compute (vectorOut_t result,
        ConfigurationIn_t argument)
      const throw ()
    {
      // TODO: Add ability to put weights on DOF
      hpp::pinocchio::difference (robot_, argument, goal_, diff_);
      result [0] = 0.5 * mask_.select (diff_, 0).squaredNorm ();
    }

    void ConfigurationConstraint::impl_jacobian (matrixOut_t jacobian,
        ConfigurationIn_t argument) const throw ()
    {
      hpp::pinocchio::difference (robot_, argument, goal_, diff_);
      jacobian.leftCols (robot_->numberDof ()) =
        mask_.select (diff_, 0).transpose ();
    }
  } // namespace constraints
} // namespace hpp
