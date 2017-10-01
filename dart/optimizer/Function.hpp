/*
 * Copyright (c) 2011-2016, Humanoid Lab, Georgia Tech Research Corporation
 * Copyright (c) 2011-2017, Graphics Lab, Georgia Tech Research Corporation
 * Copyright (c) 2016-2017, Personal Robotics Lab, Carnegie Mellon University
 * All rights reserved.
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DART_OPTIMIZER_FUNCTION_HPP_
#define DART_OPTIMIZER_FUNCTION_HPP_

#include <vector>
#include <memory>
#include <functional>

#include <Eigen/Dense>

namespace dart {
namespace optimizer {

class Function
{
public:
  /// Constructor
  explicit Function(const std::string& _name = "function");

  /// Destructor
  virtual ~Function();

  /// Set the name of this Function
  virtual void setName(const std::string& _newName);

  /// Get the name of this Function
  const std::string& getName() const;

  /// Evaluate and return the objective function at the point x
  virtual double eval(const Eigen::VectorXd& _x) = 0;

  /// Evaluate and return the objective function at the point x
  virtual void evalGradient(const Eigen::VectorXd& _x,
                            Eigen::Map<Eigen::VectorXd> _grad);

  /// Evaluate and return the objective function at the point x.
  ///
  /// If you have a raw array that the gradient will be passed in, then use
  /// evalGradient(const Eigen::VectorXd&, Eigen::Map<Eigen::VectorXd>)
  /// for better performance.
  void evalGradient(const Eigen::VectorXd& _x, Eigen::VectorXd& _grad);

  /// Evaluate and return the objective function at the point x
  virtual void evalHessian(
      const Eigen::VectorXd& _x,
      Eigen::Map<Eigen::VectorXd, Eigen::RowMajor> _Hess);

protected:
  /// Name of this function
  std::string mName;
};

using FunctionPtr = std::shared_ptr<Function>;

} // namespace optimizer
} // namespace dart

#endif // DART_OPTIMIZER_FUNCTION_HPP_

