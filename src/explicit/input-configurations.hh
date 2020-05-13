// Copyright (c) 2020, Airbus SAS and CNRS
// Authors: Florent Lamiraux
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

#ifndef HPP_CONSTRAINTS_SRC_EXPLICIT_CONFIGURATION_HH
#define HPP_CONSTRAINTS_SRC_EXPLICIT_CONFIGURATION_HH

#include <hpp/constraints/matrix-view.hh>
#include <hpp/pinocchio/device.hh>
#include <hpp/pinocchio/joint.hh>

namespace hpp {
  namespace constraints {
    namespace explicit_ {
      namespace {
        using Eigen::BlockIndex;
        inline BlockIndex::segments_t vectorOfBoolToIntervals
        (std::vector<bool>& v)
        {
          BlockIndex::segments_t ret;
          for (std::size_t i = 0; i < v.size(); ++i)
            if (v[i]) ret.push_back(BlockIndex::segment_t (i, 1));
          BlockIndex::shrink (ret);
          return ret;
        }

        // This function computes the configuration variables of
        //   - joint1 and its parents up to the common ancestor with joint2,
        //   - joint2 parent and its parents up to the common ancestor with
        //     joint1.
        // Note that configuration variables of joint2 do not belong to
        // the resulting set.
        inline std::vector<bool> relPoseConfVariables
        (const DevicePtr_t& robot, JointConstPtr_t joint1,
         JointConstPtr_t joint2)
        {
          std::vector<bool> conf (robot->configSize(), false);
          while (joint1 && joint1->index() != 0) {
            for (size_type i = 0; i < joint1->configSize(); ++i)
              conf[joint1->rankInConfiguration() + i] =
                !conf[joint1->rankInConfiguration() + i];
            hppDout (info, "Adding joint1 " << joint1->name ()
                     << " as input variable.");
            joint1 = joint1->parentJoint();
          }
          joint2 = joint2->parentJoint ();
          while (joint2 && joint2->index() != 0) {
            for (size_type i = 0; i < joint2->configSize(); ++i)
              conf[joint2->rankInConfiguration() + i] =
                !conf[joint2->rankInConfiguration() + i];
            hppDout (info, "Adding joint2 " << joint2->name ()
                     << " as input variable.");
            joint2 = joint2->parentJoint();
          }
          return conf;
        }

        // This function computes the velocity variables of
        //   - joint1 and its parents up to the root joint,
        //   - joint2 parent and its parent up to root joint.
        // Note that velocity variables of joint2 do not belong to
        // the resulting set.
        inline std::vector<bool> relPoseVelVariables
        (const DevicePtr_t& robot, JointConstPtr_t joint1,
         JointConstPtr_t joint2)
        {
          std::vector<bool> vel (robot->numberDof(), false);
          while (joint1 && joint1->index() != 0) {
            for (size_type i = 0; i < joint1->numberDof(); ++i)
              vel[joint1->rankInVelocity() + i] =
                !vel[joint1->rankInVelocity() + i];
            hppDout (info, "Adding joint1 " << joint1->name ()
                     << " as input variable.");
            joint1 = joint1->parentJoint();
          }
          joint2 = joint2->parentJoint ();
          while (joint2 && joint2->index() != 0) {
            for (size_type i = 0; i < joint2->numberDof(); ++i)
              vel[joint2->rankInVelocity() + i] =
                !vel[joint2->rankInVelocity() + i];
            hppDout (info, "Adding joint2 " << joint2->name ()
                     << " as input variable.");
            joint2 = joint2->parentJoint();
          }
          return vel;
        }
      } // namespace
      namespace relativePose {
        // This function computes the configuration variables of
        //   - joint1 and its parents up to the common ancestor with joint2,
        //   - joint2 parent and its parents up to the common ancestor with
        //     joint1.
        // Note that configuration variables of joint2 do not belong to
        // the resulting set.
        inline BlockIndex::segments_t inputConfVariables
        (const DevicePtr_t& robot, JointConstPtr_t joint1,
         JointConstPtr_t joint2)
        {
          std::vector<bool> conf(relPoseConfVariables(robot, joint1, joint2));
          return vectorOfBoolToIntervals (conf);
        }

        // This function computes the velocity variables of
        //   - joint1 and its parents up to the root joint,
        //   - joint2 parent and its parent up to root joint.
        // Note that velocity variables of joint2 do not belong to
        // the resulting set.
        inline BlockIndex::segments_t inputVelocityVariables
        (const DevicePtr_t& robot, JointConstPtr_t joint1,
         JointConstPtr_t joint2)
        {
          std::vector<bool> vel(relPoseVelVariables(robot, joint1, joint2));
          return vectorOfBoolToIntervals (vel);
        }

        // Return configuration variables of a joint
        inline BlockIndex::segments_t jointConfInterval (JointConstPtr_t j) {
          return BlockIndex::segments_t(1, BlockIndex::segment_t
                                        (j->rankInConfiguration(),
                                         j->configSize()));
        }
        // Return velocity variables of a joint
        inline BlockIndex::segments_t jointVelInterval (JointConstPtr_t j) {
          return BlockIndex::segments_t(1, BlockIndex::segment_t
                                        (j->rankInVelocity(), j->numberDof()));
        }
      } // namespace relativePose
      namespace contact {
        // Compute input configuration variables of
        // explicit_::ConvexShapeContact.
        inline BlockIndex::segments_t inputConfVariables
        (const DevicePtr_t& robot, const JointAndShapes_t& floorSurfaces,
         const JointAndShapes_t& objectSurfaces)
        {
          std::vector<bool> res (robot->configSize(), false);
          assert(objectSurfaces.size()>0);
          JointPtr_t objectJoint = objectSurfaces.front().first;
          for (JointAndShapes_t::const_iterator it(floorSurfaces.begin());
               it != floorSurfaces.end(); ++it)
          {
            JointPtr_t joint1(it->first);
            JointPtr_t joint2(objectJoint);
            std::vector<bool> conf(relPoseConfVariables(robot, joint1, joint2));
            for (std::size_t i=0; i<res.size(); ++i)
            {
              if (conf[i]) res[i] = true;
            }
          }
          return vectorOfBoolToIntervals(res);
        }

        // Compute size of input configuration variables of
        // explicit_::ConvexShapeContact.
        inline size_type inputSize
        (const DevicePtr_t& robot, const JointAndShapes_t& floorSurfaces,
         const JointAndShapes_t& objectSurfaces)
        {
          BlockIndex::segments_t variables(inputConfVariables
            (robot,floorSurfaces, objectSurfaces));
          return (size_type)BlockIndex::cardinal(variables);
        }
        // Compute input configuration variables of
        // explicit_::ConvexShapeContact.
        inline BlockIndex::segments_t inputVelocityVariables
        (const DevicePtr_t& robot, const JointAndShapes_t& floorSurfaces,
         const JointAndShapes_t& objectSurfaces)
        {
          std::vector<bool> res (robot->numberDof(), false);
          assert(objectSurfaces.size()>0);
          JointPtr_t objectJoint = objectSurfaces.front().first;
          for (JointAndShapes_t::const_iterator it(floorSurfaces.begin());
               it != floorSurfaces.end(); ++it)
          {
            JointPtr_t joint1(it->first);
            JointPtr_t joint2(objectJoint);
            std::vector<bool> conf(relPoseVelVariables
                                   (robot,joint1,joint2));
            for (std::size_t i=0; i<res.size(); ++i)
            {
              if (conf[i]) res[i] = true;
            }
          }
          return vectorOfBoolToIntervals(res);
        }
        inline size_type inputDerivSize
        (const DevicePtr_t& robot, const JointAndShapes_t& floorSurfaces,
         const JointAndShapes_t& objectSurfaces)
        {
          BlockIndex::segments_t variables(inputVelocityVariables
            (robot,floorSurfaces, objectSurfaces));
          return (size_type)BlockIndex::cardinal(variables);
        }
      }
    } // namespace explicit_
  } // namespace constraints
} // namespace hpp

#endif // HPP_CONSTRAINTS_SRC_EXPLICIT_CONFIGURATION_HH
