/*
 * Copyright (c) 2011, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TELEOP_PANEL_H
#define TELEOP_PANEL_H

#ifndef Q_MOC_RUN
#include <ros/ros.h>

#include <rviz/panel.h>
#endif

#include <QPushButton>
#include <QComboBox>
#include <dashboard_msgs/DashboardControl.h>

class QLineEdit;
class QSpinBox;

namespace picknik_gui
{
// BEGIN_TUTORIAL
// Here we declare our new subclass of rviz::Panel.  Every panel which
// can be added via the Panels/Add_New_Panel menu is a subclass of
// rviz::Panel.
//
// PickNikPanel will show a text-entry field to set the output topic
// and a 2D control area.  The 2D control area is implemented by the
// DriveWidget class, and is described there.
class PickNikPanel : public rviz::Panel
{
  // This class uses Qt slots and is a subclass of QObject, so it needs
  // the Q_OBJECT macro.
  Q_OBJECT
public:
  // QWidget subclass constructors usually take a parent widget
  // parameter (which usually defaults to 0).  At the same time,
  // pluginlib::ClassLoader creates instances by calling the default
  // constructor (with no arguments).  Taking the parameter and giving
  // a default of 0 lets the default constructor work and also lets
  // someone using the class for something else to pass in a parent
  // widget as they normally would with Qt.
  PickNikPanel(QWidget *parent = 0);

  // Now we declare overrides of rviz::Panel functions for saving and
  // loading data from the config file.  Here the data is the
  // topic name.
  virtual void load(const rviz::Config &config);
  virtual void save(rviz::Config config) const;

  // Next come a couple of public Qt slots.
public Q_SLOTS:

  // Here we declare some internal slots.
protected Q_SLOTS:

  /// User has chosen to go to next step
  void moveNext();

  /// User has chosen to run demo
  void moveAuto();

  /// User has chosen to run demo
  void moveFullAuto();

  /// User has chosen to stop
  void moveStop();

  /// User wants to change joint mode (velocity/gravity compensation)
  void changeJointMode(int mode);

  /// User wants to reset robot
  void resetRobot();

  /// User wants to bringup robot
  void bringupRobot();

  /// User wants to send robot home
  void homeRobot();

  /// User wants to bringup robot
  void toggleGripper();

  // Then we finish up with protected member variables.
protected:
  QPushButton *btn_next_;
  QPushButton *btn_auto_;
  QPushButton *btn_full_auto_;
  QPushButton *btn_stop_;
  QPushButton *btn_reset_;
  QPushButton *btn_bringup_;
  QPushButton *btn_home_;
  QPushButton *btn_grip_;
  QSpinBox *spin_box_;
  QComboBox *combo_mode_;

  // The ROS publishers
  ros::Publisher remote_publisher_;

  // The ROS node handle.
  ros::NodeHandle nh_;

  // END_TUTORIAL
};

}  // end namespace picknik_gui

#endif  // TELEOP_PANEL_H
