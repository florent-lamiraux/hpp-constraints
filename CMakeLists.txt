# Copyright 2014, Florent Lamiraux, CNRS
#
# This file is part of hpp-constraints.
# hpp-constraints is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# hpp-constraints is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Lesser Public License for more details. You should have
# received a copy of the GNU Lesser General Public License along with
# hpp-constraints. If not, see <http://www.gnu.org/licenses/>.

# Requires at least CMake 2.6 to configure the package.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

CMAKE_POLICY (SET CMP0012 NEW)
SET(CXX_DISABLE_WERROR TRUE)
INCLUDE(cmake/base.cmake)
INCLUDE(cmake/boost.cmake)
INCLUDE(cmake/eigen.cmake)

SET(PROJECT_NAME hpp-constraints)
SET(PROJECT_DESCRIPTION
  "Definition of basic geometric constraints for motion planning")
SET(PROJECT_URL "https://github.com/humanoid-path-planner/hpp-constraints")

# Add a cache variable to allow not compiling and running tests
set (RUN_TESTS TRUE CACHE BOOL "compile and run unit tests")

SET(${PROJECT_NAME}_HEADERS
  include/hpp/constraints/differentiable-function.hh
  include/hpp/constraints/differentiable-function-stack.hh
  include/hpp/constraints/distance-between-bodies.hh
  include/hpp/constraints/fwd.hh
  include/hpp/constraints/svd.hh
  include/hpp/constraints/tools.hh
  include/hpp/constraints/generic-transformation.hh
  include/hpp/constraints/macros.hh
  include/hpp/constraints/convex-shape.hh
  include/hpp/constraints/convex-shape-contact.hh
  include/hpp/constraints/symbolic-calculus.hh
  include/hpp/constraints/symbolic-function.hh
  include/hpp/constraints/orientation.hh
  include/hpp/constraints/position.hh
  include/hpp/constraints/transformation.hh
  include/hpp/constraints/relative-com.hh
  include/hpp/constraints/com-between-feet.hh
  include/hpp/constraints/relative-orientation.hh
  include/hpp/constraints/relative-position.hh
  include/hpp/constraints/relative-transformation.hh
  include/hpp/constraints/configuration-constraint.hh
)

SET(${PROJECT_NAME}_OLDHEADERS
  include/hpp/_constraints/differentiable-function.hh
  include/hpp/_constraints/differentiable-function-stack.hh
  include/hpp/_constraints/distance-between-bodies.hh
  include/hpp/_constraints/fwd.hh
  include/hpp/_constraints/svd.hh
  include/hpp/_constraints/tools.hh
  include/hpp/_constraints/generic-transformation.hh
  include/hpp/_constraints/macros.hh
  include/hpp/_constraints/convex-shape.hh
  include/hpp/_constraints/convex-shape-contact.hh
  include/hpp/_constraints/symbolic-calculus.hh
  include/hpp/_constraints/symbolic-function.hh
  include/hpp/_constraints/orientation.hh
  include/hpp/_constraints/position.hh
  include/hpp/_constraints/transformation.hh
  include/hpp/_constraints/relative-com.hh
  include/hpp/_constraints/com-between-feet.hh
  include/hpp/_constraints/relative-orientation.hh
  include/hpp/_constraints/relative-position.hh
  include/hpp/_constraints/relative-transformation.hh
  include/hpp/_constraints/configuration-constraint.hh
  include/hpp/_constraints/static-stability.hh
  include/hpp/_constraints/qp-static-stability.hh
)

# Activate hpp-util logging if requested
SET (HPP_DEBUG FALSE CACHE BOOL "trigger hpp-util debug output")
IF (HPP_DEBUG)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHPP_DEBUG")
ENDIF()

# Activate hpp-util logging if requested
SET (HPP_CONSTRAINTS_NUMERIC_DEBUG ${HPP_DEBUG} CACHE BOOL "trigger hpp-util debug output")
IF (HPP_CONSTRAINTS_NUMERIC_DEBUG)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHPP_CONSTRAINTS_NUMERIC_DEBUG")
ENDIF()

OPTION(CHECK_JACOBIANS "Check jacobians against finite difference." OFF)
IF(CHECK_JACOBIANS)
  ADD_DEFINITIONS(-DCHECK_JACOBIANS)
ENDIF(CHECK_JACOBIANS)

# Add a cache variabie to remove dependency to qpOASES
SET(USE_QPOASES TRUE CACHE BOOL "Use qpOASES solver for static stability")
IF (USE_QPOASES)
  SET(${PROJECT_NAME}_HEADERS ${${PROJECT_NAME}_HEADERS}
    include/hpp/constraints/static-stability.hh
    include/hpp/constraints/qp-static-stability.hh
    )
  PKG_CONFIG_APPEND_CFLAGS (-DHPP_CONSTRAINTS_USE_QPOASES)
ENDIF ()

SETUP_PROJECT()
SET(Eigen_REQUIRED "eigen3 >= 3.2.4")
SEARCH_FOR_EIGEN()
ADD_REQUIRED_DEPENDENCY("hpp-pinocchio >= 4")
if (${USE_QPOASES})
  ADD_REQUIRED_DEPENDENCY("qpOASES >= 3.2")
ENDIF ()

ADD_SUBDIRECTORY (src)

SET (COMPARE_TO_HPP_MODEL FALSE CACHE BOOL
  "Enable comparison between hpp-model and hpp-pinocchio")
IF (COMPARE_TO_HPP_MODEL)
  ADD_REQUIRED_DEPENDENCY("hpp-model >= 3")
  ADD_SUBDIRECTORY (pmdiff)
ENDIF ()

IF (RUN_TESTS)
  SET(BOOST_COMPONENT math unit_test_framework)
  SEARCH_FOR_BOOST()
  ADD_SUBDIRECTORY(tests)
ENDIF ()

PKG_CONFIG_APPEND_LIBS("hpp-constraints")

SETUP_PROJECT_FINALIZE()
