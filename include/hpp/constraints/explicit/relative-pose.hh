// Copyright (c) 2018, LAAS-CNRS
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

#ifndef HPP_CONSTRAINTS_EXPLICIT_RELATIVE_POSE_HH
# define HPP_CONSTRAINTS_EXPLICIT_RELATIVE_POSE_HH

# include <hpp/constraints/explicit.hh>
# include <pinocchio/spatial/se3.hpp>

namespace hpp {
  namespace constraints {
    namespace explicit_ {
      /// Constraint of relative pose between two frames on a kinematic chain
      class HPP_CONSTRAINTS_DLLAPI RelativePose : public Explicit
      {
      public:
        /// Copy object and return shared pointer to copy
        virtual ImplicitPtr_t copy () const;
        /// Create instance and return shared pointer
        ///
        /// \param name the name of the constraints,
        /// \param robot the robot the constraints is applied to,
        /// \param joint1 the first joint the transformation of which is
        ///               constrained,
        /// \param joint2 the second joint the transformation of which is
        ///               constrained,
        /// \param frame1 position of a fixed frame in joint 1,
        /// \param frame2 position of a fixed frame in joint 2,
        /// \param comp vector of comparison types
	/// \param mask mask defining which components of the error are
	///        taken into account to determine whether the constraint
	///        is satisfied.
        /// \note if joint1 is 0x0, joint 1 frame is considered to be the global
        ///       frame.
        static RelativePosePtr_t create
          (const std::string& name, const DevicePtr_t& robot,
           const JointConstPtr_t& joint1, const JointConstPtr_t& joint2,
           const Transform3f& frame1, const Transform3f& frame2,
           ComparisonTypes_t comp, std::vector<bool> mask=std::vector<bool>());

        static RelativePosePtr_t createCopy (const RelativePosePtr_t& other);

        /// Compute the value of the output configuration variables
        /// \param qin input configuration variables,
        /// \param rhs right hand side of constraint
        ///
        /// \f{equation}
        /// f \left(\mathbf{q}_{in}\right) + rhs_{expl}
        /// \f}
        /// where \f$rhs_{expl}\f$ is the explicit right hand side converted
        /// using the following expression:
        /// \f{equation}
        /// rhs_{expl} = \log_{SE(3)}\left( F_{2/J_2} rhs_{impl} F_{2/J_2}^{-1}
	///              \right)
        /// \f}
        virtual void outputValue(LiegroupElementRef result, vectorIn_t qin,
                                 LiegroupElementConstRef rhs) const;

        /// Compute Jacobian of output value
        ///
        /// \f{eqnarray*}
        /// J &=& \frac{\partial}{\partial\mathbf{q}_{in}}\left(
        ///       f(\mathbf{q}_{in}) + rhs\right).
        /// \f}
        /// \param qin vector of input variables,
        /// \param f_value \f$f(\mathbf{q}_{in})\f$ to avoid recomputation,
        /// \param rhs right hand side (of implicit formulation).
        virtual void jacobianOutputValue(vectorIn_t qin, LiegroupElementConstRef
                                         f_value, LiegroupElementConstRef rhs,
                                         matrixOut_t jacobian) const;
      protected:
        /// Constructor
        ///
        /// \param name the name of the constraints,
        /// \param robot the robot the constraints is applied to,
        /// \param joint1 the first joint the transformation of which is
        ///               constrained,
        /// \param joint2 the second joint the transformation of which is
        ///               constrained,
        /// \param frame1 position of a fixed frame in joint 1,
        /// \param frame2 position of a fixed frame in joint 2,
        /// \param comp vector of comparison types
	/// \param mask mask defining which components of the error are
	///        taken into account to determine whether the constraint
	///        is satisfied.
        /// \note if joint1 is 0x0, joint 1 frame is considered to be the global
        ///       frame.
        RelativePose (const std::string& name, const DevicePtr_t& robot,
                      const JointConstPtr_t& joint1,
                      const JointConstPtr_t& joint2,
                      const Transform3f& frame1, const Transform3f& frame2,
                      ComparisonTypes_t comp = ComparisonTypes_t(),
		      std::vector<bool> mask = std::vector<bool>(6, true));

        /// Copy constructor
        RelativePose (const RelativePose& other);

        /// Store weak pointer to itself
        void init (RelativePoseWkPtr_t weak);
      private:
        /** Convert right hand side

            \param implicitRhs right hand side of implicit formulation, this
	           is an element of $SE(3)$
            \retval explicitRhs right hand side of explicit formulation, this
                    is an element of $\mathbf{R}^6$.

            For this constraint, the implicit formulation does not derive
            from  the explicit formulation. The explicit form writes

            \f{eqnarray}
            rhs_{expl} &=& \log_{SE(3)} \left(F_{2/J_2} F_{1/J_1}^{-1} J_1^{-1}
            J_2\right)\\
            rhs_{impl} &=& F_{1/J_1}^{-1} J_1^{-1}J_2 F_{2/J_2}
            \f}
            Thus
            \f{equation}
            rhs_{expl} = \log_{SE(3)}\left( F_{2/J_2}rhs_{impl}
            F_{2/J_2}^{-1}\right)
            \f}
        */
        void implicitToExplicitRhs (LiegroupElementConstRef implicitRhs,
                                    vectorOut_t explicitRhs) const;
        /** Convert right hand side

            \param explicitRhs right hand side of explicit formulation,
            \retval implicitRhs right hand side of implicit formulation.

            For this constraint, the implicit formulation does not derive
            from  the explicit formulation. The explicit form writes

            \f{eqnarray}
            rhs_{expl} &=& \log_{SE(3)} \left(F_{2/J_2} F_{1/J_1}^{-1} J_1^{-1}
            J_2\right)\\
            rhs_{impl} &=& F_{1/J_1}^{-1} J_1^{-1}J_2 F_{2/J_2}
            \f}
            Thus
            \f{equation}
            rhs_{impl} = F_{2/J_2}^{-1} \exp_{SE(3)}(rhs_{expl}) F_{2/J_2}
            \f}
        */
        void explicitToImplicitRhs (vectorIn_t explicitRhs,
                                    LiegroupElementRef implicitRhs) const;
        // Create LiegroupSpace instances to avoid useless allocation.
        static LiegroupSpacePtr_t SE3;
        static LiegroupSpacePtr_t R3xSO3;
        static LiegroupSpacePtr_t R6;
        JointConstPtr_t joint1_, joint2_;
        Transform3f frame1_;
        Transform3f frame2_;
        RelativePoseWkPtr_t weak_;

        RelativePose() {}
        HPP_SERIALIZABLE();
      }; // class RelativePose
    } // namespace explicit_
  } // namespace constraints
} // namespace hpp

BOOST_CLASS_EXPORT_KEY(hpp::constraints::explicit_::RelativePose)

#endif //HPP_CONSTRAINTS_EXPLICIT_RELATIVE_POSE_HH
