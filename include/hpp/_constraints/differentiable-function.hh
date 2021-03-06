//
// Copyright (c) 2014 CNRS
// Authors: Florent Lamiraux
//
// This file is part of hpp-constraints
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
// hpp-constraints  If not, see
// <http://www.gnu.org/licenses/>.

#ifndef HPP__CONSTRAINTS_DIFFERENTIABLE_FUNCTION_HH
# define HPP__CONSTRAINTS_DIFFERENTIABLE_FUNCTION_HH

# include <hpp/_constraints/fwd.hh>
# include <hpp/constraints/config.hh>

namespace hpp {
  namespace _constraints {
    /// \addtogroup _constraints
    /// \{

    /// Differentiable function
    class HPP_CONSTRAINTS_DLLAPI DifferentiableFunction
    {
    public:
      virtual ~DifferentiableFunction () {}
      /// Evaluate the function at a given parameter.
      ///
      /// \note parameters should be of the correct size.
      void operator () (vectorOut_t result,
			vectorIn_t argument) const
      {
	assert (result.size () == outputSize ());
	assert (argument.size () == inputSize ());
	impl_compute (result, argument);
      }
      /// Computes the jacobian.
      ///
      /// \retval jacobian jacobian will be stored in this argument
      /// \param argument point at which the jacobian will be computed
      void jacobian (matrixOut_t jacobian, vectorIn_t argument) const
      {
	assert (argument.size () == inputSize ());
	assert (jacobian.rows () == outputDerivativeSize ());
	assert (jacobian.cols () == inputDerivativeSize ());
	impl_jacobian (jacobian, argument);
      }

      /// Get dimension of input vector
      size_type inputSize () const
      {
	return inputSize_;
      }
      /// Get dimension of input derivative vector
      ///
      /// The dimension of configuration vectors might differ from the dimension
      /// of velocity vectors since some joints are represented by non minimal
      /// size vectors: e.g. quaternion for SO(3)
      size_type inputDerivativeSize () const
      {
	return inputDerivativeSize_;
      }
      /// Get dimension of output vector
      size_type  outputSize () const
      {
	return outputSize_;
      }
      /// Get dimension of output derivative vector
      size_type  outputDerivativeSize () const
      {
	return outputDerivativeSize_;
      }
      /// \brief Get function name.
      ///
      /// \return Function name.
      const std::string& name () const
      {
	return name_;
      }

      /// Display object in a stream
      virtual std::ostream& print (std::ostream& o) const
      {
	o << "Differentiable function:" << std::endl;
	o << name ();
	return o;
      }

      std::string context () const {
        return context_;
      }

      void context (const std::string& c) {
        context_ = c;
      }

    protected:
      /// \brief Concrete class constructor should call this constructor.
      ///
      /// \param sizeInput dimension of the function input
      /// \param sizeInputDerivative dimension of the function input derivative,
      /// \param sizeOutput dimension of the output,
      /// \param name function's name
      DifferentiableFunction (size_type sizeInput,
			      size_type sizeInputDerivative,
			      size_type sizeOutput,
			      std::string name = std::string ()) :
	inputSize_ (sizeInput), inputDerivativeSize_ (sizeInputDerivative),
	outputSize_ (sizeOutput), outputDerivativeSize_ (sizeOutput),
	name_ (name)
      {
      }

      /// \brief Concrete class constructor should call this constructor.
      ///
      /// \param sizeInput dimension of the function input
      /// \param sizeInputDerivative dimension of the function input derivative,
      /// \param sizeOutput dimension of the output,
      /// \param sizeOutputDerivative dimension of the output derivative,
      /// \param name function's name
      DifferentiableFunction (size_type sizeInput,
			      size_type sizeInputDerivative,
			      size_type sizeOutput,
			      size_type sizeOutputDerivative,
			      std::string name = std::string ()) :
	inputSize_ (sizeInput), inputDerivativeSize_ (sizeInputDerivative),
	outputSize_ (sizeOutput), outputDerivativeSize_ (sizeOutputDerivative),
	name_ (name), context_ ()
      {
      }

      /// User implementation of function evaluation
      virtual void impl_compute (vectorOut_t result,
				 vectorIn_t argument) const = 0;

      virtual void impl_jacobian (matrixOut_t jacobian,
				  vectorIn_t arg) const = 0;

      /// Approximate the jacobian using forward finite difference.
      /// \param robot use to add configuration and velocities. If set to NULL,
      ///              the configuration space is considered a vector space.
      /// \param eps refers to \f$\epsilon\f$ in
      ///            http://en.wikipedia.org/wiki/Numerical_differentiation
      /// Evaluate the function (x.size() + 1) times but less precise the
      /// finiteDifferenceCentral
      void finiteDifferenceForward (matrixOut_t jacobian, vectorIn_t arg,
          DevicePtr_t robot = DevicePtr_t (),
          value_type eps = std::sqrt(Eigen::NumTraits<value_type>::epsilon())) const;

      /// Approximate the jacobian using forward finite difference.
      /// \param eps refers to \f$\epsilon\f$ in
      ///            http://en.wikipedia.org/wiki/Numerical_differentiation
      /// Evaluate the function 2*x.size() times but more precise the
      /// finiteDifferenceForward
      void finiteDifferenceCentral (matrixOut_t jacobian, vectorIn_t arg,
          DevicePtr_t robot = DevicePtr_t (),
          value_type eps = std::sqrt(Eigen::NumTraits<value_type>::epsilon())) const;

      /// Dimension of input vector.
      size_type inputSize_;
      /// Dimension of input derivative
      size_type inputDerivativeSize_;
      /// Dimension of output vector
      size_type outputSize_;
      /// Dimension of output derivative vector
      size_type outputDerivativeSize_;

    private:
      std::string name_;
      /// Context of creation of function
      std::string context_;

      friend class DifferentiableFunctionStack;
    }; // class DifferentiableFunction
    inline std::ostream&
    operator<< (std::ostream& os, const DifferentiableFunction& f)
    {
      return f.print (os);
    }
    /// \}
  } // namespace _constraints
} // namespace hpp


#endif // HPP_CONSTRAINTS_DIFFERENTIABLE_FUNCTION_HH
