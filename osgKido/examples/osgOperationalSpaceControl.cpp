/*
 * Copyright (c) 2015, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Author(s): Michael X. Grey <mxgrey@gatech.edu>
 *            Jeongseok Lee <jslee02@gmail.com>
 *
 * Georgia Tech Graphics Lab and Humanoid Robotics Lab
 *
 * Directed by Prof. C. Karen Liu and Prof. Mike Stilman
 * <karenliu@cc.gatech.edu> <mstilman@cc.gatech.edu>
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

#include "osgKido/osgKido.h"

#include "kido/kido.h"

using namespace kido::common;
using namespace kido::dynamics;
using namespace kido::math;

class OperationalSpaceControlWorld : public osgKido::WorldNode
{
public:

  OperationalSpaceControlWorld(kido::simulation::WorldPtr _world)
    : osgKido::WorldNode(_world)
  {
    // Extract the relevant pointers
    mRobot = mWorld->getSkeleton(0);
    mEndEffector = mRobot->getBodyNode(mRobot->getNumBodyNodes()-1);

    // Setup gain matrices
    size_t dofs = mEndEffector->getNumDependentGenCoords();
    mKp.setZero();
    for(size_t i=0; i<3; ++i)
      mKp(i,i) = 50.0;

    mKd.setZero(dofs,dofs);
    for(size_t i=0; i<dofs; ++i)
      mKd(i,i) = 5.0;

    // Set joint properties
    for(size_t i=0; i<mRobot->getNumJoints(); ++i)
    {
      mRobot->getJoint(i)->setPositionLimitEnforced(false);
      mRobot->getJoint(i)->setDampingCoefficient(0, 0.5);
    }

    mOffset = Eigen::Vector3d(0.05,0,0);

    // Create target Frame
    Eigen::Isometry3d tf = mEndEffector->getWorldTransform();
    tf.pretranslate(mOffset);
    mTarget = std::make_shared<SimpleFrame>(Frame::World(), "target", tf);
    ShapePtr ball(new EllipsoidShape(Eigen::Vector3d(0.05,0.05,0.05)));
    ball->setColor(Eigen::Vector3d(0.9,0,0));
    mTarget->addVisualizationShape(ball);
    mWorld->addSimpleFrame(mTarget);

    mOffset = mEndEffector->getWorldTransform().rotation().transpose() * mOffset;
  }

  // Triggered at the beginning of each simulation step
  void customPreStep() override
  {
    Eigen::MatrixXd M = mRobot->getMassMatrix();

    LinearJacobian J = mEndEffector->getLinearJacobian(mOffset);
    Eigen::MatrixXd pinv_J = J.transpose()*(J*J.transpose()
                                +0.0025*Eigen::Matrix3d::Identity()).inverse();

    LinearJacobian dJ = mEndEffector->getLinearJacobianDeriv(mOffset);
    Eigen::MatrixXd pinv_dJ = dJ.transpose()*(dJ*dJ.transpose()
                                +0.0025*Eigen::Matrix3d::Identity()).inverse();


    Eigen::Vector3d e = mTarget->getWorldTransform().translation()
                        - mEndEffector->getWorldTransform()*mOffset;

    Eigen::Vector3d de = - mEndEffector->getLinearVelocity(mOffset);

    Eigen::VectorXd Cg = mRobot->getCoriolisAndGravityForces();

    mForces = M*(pinv_J*mKp*de + pinv_dJ*mKp*e) + Cg + mKd*pinv_J*mKp*e;

    mRobot->setForces(mForces);
  }

  osgKido::DragAndDrop* dnd;

protected:

  // Triggered when this node gets added to the Viewer
  void setupViewer() override
  {
    if(mViewer)
    {
      dnd = mViewer->enableDragAndDrop(mTarget.get());
      dnd->setObstructable(false);
      mViewer->addInstructionText("\nClick and drag the red ball to move the target of the operational space controller\n");
      mViewer->addInstructionText("Hold key 1 to constrain movements to the x-axis\n");
      mViewer->addInstructionText("Hold key 2 to constrain movements to the y-axis\n");
      mViewer->addInstructionText("Hold key 3 to constrain movements to the z-axis\n");
    }
  }

  SkeletonPtr mRobot;
  BodyNode* mEndEffector;
  SimpleFramePtr mTarget;

  Eigen::Vector3d mOffset;
  Eigen::Matrix3d mKp;
  Eigen::MatrixXd mKd;
  Eigen::VectorXd mForces;
};

class ConstraintEventHandler : public osgGA::GUIEventHandler
{
public:

  ConstraintEventHandler(osgKido::DragAndDrop* dnd = nullptr)
    : mDnD(dnd)
  {
    clearConstraints();
    if(mDnD)
      mDnD->unconstrain();
  }

  void clearConstraints()
  {
    for(size_t i=0; i<3; ++i)
      mConstrained[i] = false;
  }

  virtual bool handle(const osgGA::GUIEventAdapter& ea,
                      osgGA::GUIActionAdapter&) override
  {
    if(nullptr == mDnD)
    {
      clearConstraints();
      return false;
    }

    bool handled = false;
    switch(ea.getEventType())
    {
      case osgGA::GUIEventAdapter::KEYDOWN:
      {
        switch(ea.getKey())
        {
          case '1':
            mConstrained[0] = true;
            handled = true;
            break;
          case '2':
            mConstrained[1] = true;
            handled = true;
            break;
          case '3':
            mConstrained[2] = true;
            handled = true;
            break;
        }
        break;
      }

      case osgGA::GUIEventAdapter::KEYUP:
      {
        switch(ea.getKey())
        {
          case '1':
            mConstrained[0] = false;
            handled = true;
            break;
          case '2':
            mConstrained[1] = false;
            handled = true;
            break;
          case '3':
            mConstrained[2] = false;
            handled = true;
            break;
        }
        break;
      }

      default:
        return false;
    }

    if(!handled)
      return handled;

    size_t constraintDofs = 0;
    for(size_t i=0; i<3; ++i)
      if(mConstrained[i])
        ++constraintDofs;

    if(constraintDofs==0 || constraintDofs==3)
    {
      mDnD->unconstrain();
    }
    else if(constraintDofs == 1)
    {
      Eigen::Vector3d v(Eigen::Vector3d::Zero());
      for(size_t i=0; i<3; ++i)
        if(mConstrained[i])
          v[i] = 1.0;

      mDnD->constrainToLine(v);
    }
    else if(constraintDofs == 2)
    {
      Eigen::Vector3d v(Eigen::Vector3d::Zero());
      for(size_t i=0; i<3; ++i)
        if(!mConstrained[i])
          v[i] = 1.0;

      mDnD->constrainToPlane(v);
    }

    return handled;
  }

  bool mConstrained[3];

  kido::sub_ptr<osgKido::DragAndDrop> mDnD;
};

int main()
{
  kido::simulation::WorldPtr world(new kido::simulation::World);
  kido::utils::KidoLoader loader;

  // Load the robot
  kido::dynamics::SkeletonPtr robot =
      loader.parseSkeleton(KIDO_DATA_PATH"urdf/KR5/KR5 sixx R650.urdf");
  world->addSkeleton(robot);

  // Set the colors of the models to obey the shape's color specification
  for(size_t i=0; i<robot->getNumBodyNodes(); ++i)
  {
    BodyNode* bn = robot->getBodyNode(i);
    for(size_t j=0; j<bn->getNumVisualizationShapes(); ++j)
    {
      std::shared_ptr<MeshShape> mesh =
          std::dynamic_pointer_cast<MeshShape>(bn->getVisualizationShape(j));
      if(mesh)
        mesh->setColorMode(MeshShape::SHAPE_COLOR);
    }
  }

  // Rotate the robot so that z is upwards (default transform is not Identity)
  robot->getJoint(0)->setTransformFromParentBodyNode(Eigen::Isometry3d::Identity());

  // Load the ground
  kido::dynamics::SkeletonPtr ground =
      loader.parseSkeleton(KIDO_DATA_PATH"urdf/KR5/ground.urdf");
  world->addSkeleton(ground);

  // Rotate and move the ground so that z is upwards
  Eigen::Isometry3d ground_tf =
      ground->getJoint(0)->getTransformFromParentBodyNode();
  ground_tf.pretranslate(Eigen::Vector3d(0,0,0.5));
  ground_tf.rotate(Eigen::AngleAxisd(M_PI/2, Eigen::Vector3d(1,0,0)));
  ground->getJoint(0)->setTransformFromParentBodyNode(ground_tf);

  // Create an instance of our customized WorldNode
  osg::ref_ptr<OperationalSpaceControlWorld> node =
      new OperationalSpaceControlWorld(world);
  node->setNumStepsPerCycle(10);

  // Create the Viewer instance
  osgKido::Viewer viewer;
  viewer.addWorldNode(node);
  viewer.simulate(true);

  // Add our custom event handler to the Viewer
  viewer.addEventHandler(new ConstraintEventHandler(node->dnd));

  // Print out instructions
  std::cout << viewer.getInstructions() << std::endl;

  // Set up the window to be 640x480 pixels
  viewer.setUpViewInWindow(0, 0, 640, 480);

  viewer.getCameraManipulator()->setHomePosition(osg::Vec3( 2.57,  3.14, 1.64),
                                                 osg::Vec3( 0.00,  0.00, 0.00),
                                                 osg::Vec3(-0.24, -0.25, 0.94));
  // We need to re-dirty the CameraManipulator by passing it into the viewer
  // again, so that the viewer knows to update its HomePosition setting
  viewer.setCameraManipulator(viewer.getCameraManipulator());

  // Begin the application loop
  viewer.run();
}
