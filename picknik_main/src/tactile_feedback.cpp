/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2015, Dave Coleman <dave@dav.ee>
 *  All rights reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *********************************************************************/
/*
  Author: Dave Coleman <dave@dav.ee>
  Desc:   Integrate feedback and command of a tactile sensor
*/

// PickNik
#include <picknik_main/tactile_feedback.h>

// ROS
#include <std_msgs/Bool.h>
#include <std_msgs/Float64MultiArray.h>

namespace picknik_main
{
TactileFeedback::TactileFeedback(ManipulationDataPtr config)
  : sheer_theta_(0.0)
  , end_effector_data_cached_(ALWAYS_AT_END, 0.0)  // initial dummy data
{
  // Load visual tools
  visual_tools_.reset(new rviz_visual_tools::RvizVisualTools(config->robot_base_frame_,
                                                             "/picknik_main/tactile_feedback"));

  const std::size_t queue_size = 1;
  end_effector_data_sub_ =
      nh_.subscribe("/end_effector_data", queue_size, &TactileFeedback::dataCallback, this);

  tactile_calibration_pub_ = nh_.advertise<std_msgs::Bool>("/calibrate_tactile_sensor", queue_size);
}

void TactileFeedback::recalibrateTactileSensor()
{
  ROS_INFO_STREAM_NAMED("tactile_feedback", "Recalibrating");
  std_msgs::Bool msg;
  tactile_calibration_pub_.publish(msg);
}

void TactileFeedback::dataCallback(const std_msgs::Float64MultiArray::ConstPtr& msg)
{
  // Error check
  if (msg->data.size() < ALWAYS_AT_END)
  {
    ROS_ERROR_STREAM_NAMED(
        "tactile_feedback",
        "Invalid number of end effector data points recieved: " << msg->data.size());
    return;
  }

  // Save latest data
  end_effector_data_cached_ = msg->data;

  // displayLineDirection();
  displaySheerForce();

  // Do callback if provided
  if (end_effector_data_callback_)
    end_effector_data_callback_();
}

void TactileFeedback::displayLineDirection()
{
  bool verbose = false;

  // Unpack vector to variable names
  const double& pt1_x = end_effector_data_cached_[LINE_CENTER_X];
  const double& pt1_y = end_effector_data_cached_[LINE_CENTER_Y];
  const double& eigen_vec_x = end_effector_data_cached_[LINE_EIGEN_VEC_X];
  const double& eigen_vec_y = end_effector_data_cached_[LINE_EIGEN_VEC_Y];
  const double& eigen_vec_val = end_effector_data_cached_[LINE_EIGEN_VAL];
  const double& image_height = end_effector_data_cached_[IMAGE_HEIGHT];
  const double& image_width = end_effector_data_cached_[IMAGE_WIDTH];

  // Calculate point 2
  const double distance_between_points = 2000;
  double pt2_x = pt1_x + distance_between_points * (eigen_vec_x * eigen_vec_val);
  double pt2_y = pt1_y + distance_between_points * (eigen_vec_y * eigen_vec_val);

  if (verbose)
    std::cout << "Pixels: (" << pt1_x << ", " << pt1_y << ") (" << pt2_x << ", " << pt2_y << ")";

  // Convert to ROS msg
  geometry_msgs::PoseStamped pt1;
  pt1.pose.position.x = pt1_x;
  pt1.pose.position.y = pt1_y;
  pt1.pose.position.z = 0;
  pt1.header.frame_id = ATTACH_FRAME;

  geometry_msgs::PoseStamped pt2;
  pt2.pose.position.x = pt2_x;
  pt2.pose.position.y = pt2_y;
  pt2.pose.position.z = 0;
  pt2.header.frame_id = ATTACH_FRAME;

  // Convert to meters
  convertPixelToMeters(pt1.pose, image_height, image_width);
  convertPixelToMeters(pt2.pose, image_height, image_width);

  if (verbose)
    std::cout << "Meters: (" << pt1.pose.position.x << ", " << pt1.pose.position.y << ") ("
              << pt2.pose.position.x << ", " << pt2.pose.position.y << ")" << std::endl;

  // Publish visuals
  visual_tools_->publishSphere(pt1, rvt::GREEN, visual_tools_->getScale(rvt::XXSMALL), "Sphere1",
                               1);
  visual_tools_->publishSphere(pt2, rvt::BLUE, visual_tools_->getScale(rvt::XXSMALL), "Sphere2", 2);
  publishUpdatedLine(pt1.pose.position, pt2.pose.position);

  // Get tool direction
  double theta = atan2(eigen_vec_y, eigen_vec_x);

  // Visualize tool always pointing down away from gripper
  Eigen::Affine3d eigen_pose = visual_tools_->convertPose(pt1.pose);

  if (theta > M_PI / 2.0)
  {
    theta -= M_PI;
    ROS_DEBUG_STREAM_NAMED("tactile_feedback", "angle of line: " << theta << " (substracted 3.14)");
  }
  else
  {
    ROS_DEBUG_STREAM_NAMED("tactile_feedback", "angle of line: " << theta);
  }

  eigen_pose = eigen_pose * Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ());

  geometry_msgs::PoseStamped pose_msg;
  pose_msg.header.frame_id = ATTACH_FRAME;
  pose_msg.pose = visual_tools_->convertPose(eigen_pose);

  // Visualize arrow
  const double length = 0.1;
  const int id = 1;
  visual_tools_->publishArrow(pose_msg, rvt::GREY, rvt::SMALL, length, id);
}

void TactileFeedback::displaySheerForce()
{
  const double& sheer_displacement_x = end_effector_data_cached_[SHEER_DISPLACEMENT_X];
  const double& sheer_displacement_y = end_effector_data_cached_[SHEER_DISPLACEMENT_Y];
  const double& image_height = end_effector_data_cached_[IMAGE_HEIGHT];
  const double& image_width = end_effector_data_cached_[IMAGE_WIDTH];

  // Convert to ROS msg
  geometry_msgs::PoseStamped pt1;
  pt1.pose.position.x = image_height / 2.0;
  pt1.pose.position.y = image_width / 2.0;
  pt1.pose.position.z = 0;
  pt1.header.frame_id = ATTACH_FRAME;

  geometry_msgs::PoseStamped pt2;
  pt2.pose.position.x = sheer_displacement_x;
  pt2.pose.position.y = sheer_displacement_y;
  pt2.pose.position.z = 0;
  pt2.header.frame_id = ATTACH_FRAME;

  // Convert to meters
  convertPixelToMeters(pt1.pose, image_height, image_width);
  convertPixelToMeters(pt2.pose, image_height, image_width);

  visual_tools_->publishSphere(pt1, rvt::GREEN, visual_tools_->getScale(rvt::XXSMALL), "Sphere1",
                               1);
  visual_tools_->publishSphere(pt2, rvt::BLUE, visual_tools_->getScale(rvt::XXSMALL), "Sphere2", 2);

  // Visualize tool always pointing down away from gripper
  Eigen::Affine3d eigen_pose = visual_tools_->convertPose(pt1.pose);

  // Find the angle between the line and the horizontal (x) axis
  double delta_y = pt2.pose.position.y - pt1.pose.position.y;
  double delta_x = pt2.pose.position.x - pt1.pose.position.x;

  // Save for later use
  sheer_theta_ = atan2(delta_y, delta_x);  // radians

  // Create new pose
  eigen_pose = eigen_pose * Eigen::AngleAxisd(sheer_theta_, Eigen::Vector3d::UnitZ());

  geometry_msgs::PoseStamped pose_msg;
  pose_msg.header.frame_id = ATTACH_FRAME;
  pose_msg.pose = visual_tools_->convertPose(eigen_pose);

  // Visualize arrow
  static const double VISUAL_MAX_LENGTH = 0.5;  // based on personal visual preferance
  static const double SHEER_FORCE_MAX = 20;     // ignore sheer force higher than this
  static const double SHEER_RATIO = VISUAL_MAX_LENGTH / SHEER_FORCE_MAX;
  double sheer_force = std::min(SHEER_FORCE_MAX, getSheerForce());
  const double length = sheer_force * SHEER_RATIO;
  const int id = 1;
  visual_tools_->publishArrow(pose_msg, rvt::RED, rvt::REGULAR, length, id);
}

void TactileFeedback::publishUpdatedLine(geometry_msgs::Point& pt1, geometry_msgs::Point& pt2)
{
  // Make custom marker because we want it in from of reference of finger
  visualization_msgs::Marker line_marker;
  line_marker.header.frame_id = ATTACH_FRAME;
  line_marker.header.stamp = ros::Time::now();
  line_marker.ns = "LineObject";
  line_marker.type = visualization_msgs::Marker::LINE_STRIP;
  line_marker.action = visualization_msgs::Marker::ADD;
  line_marker.id = 0;
  line_marker.color = visual_tools_->getColor(rvt::RED);

  // Base pose is identity
  line_marker.pose.orientation.w = 1.0;

  // Calculate scale
  line_marker.scale.x = 0.001;  // Scale y and z not used

  // Add two points
  line_marker.points.push_back(pt1);
  line_marker.points.push_back(pt2);

  visual_tools_->publishMarker(line_marker);
}

void TactileFeedback::convertPixelToMeters(geometry_msgs::Pose& pose, int height, int width)
{
  // Input points are in the range of x: (0-480) y: (0-640)
  // Transform these to the width of the gelsight 25mm x 25mm (0.025m x 0.025m)
  //  in     out
  //  ----  -----
  //  640   0.025
  //
  // out = 0.025 * in / 640
  static const double WIDTH_OF_GELSIGHT = 0.01595;   // 15.95 mm
  static const double HEIGHT_OF_GELSIGHT = 0.01668;  // 16.68 mm

  const double& image_height = height;
  const double& image_width = width;

  pose.position.x = WIDTH_OF_GELSIGHT * pose.position.x / image_width;
  pose.position.y = HEIGHT_OF_GELSIGHT * pose.position.y / image_height;
}

}  // end namespace
