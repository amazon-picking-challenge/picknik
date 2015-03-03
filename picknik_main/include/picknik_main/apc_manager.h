/*********************************************************************
 * Software License Agreement
 *
 *  Copyright (c) 2015, Dave Coleman <dave@dav.ee>
 *  All rights reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *********************************************************************/

/* Author: Dave Coleman <dave@dav.ee>
   Desc:   Main logic of APC challenge
*/

#ifndef PICKNIK_MAIN__SHELF_MANAGER
#define PICKNIK_MAIN__SHELF_MANAGER

// Amazon Pick Place Challenge
#include <picknik_main/namespaces.h>
#include <picknik_main/amazon_json_parser.h>
#include <picknik_main/shelf.h>
#include <picknik_main/manipulation_pipeline.h>
#include <picknik_main/learning_pipeline.h>
#include <picknik_main/visuals.h>
#include <moveit/plan_execution/plan_execution.h>

// ROS
#include <ros/ros.h>
#include <tf/transform_listener.h>

namespace picknik_main
{

static const std::string ROBOT_DESCRIPTION = "robot_description";
static const std::string JOINT_STATE_TOPIC = "/robot/joint_states";
static const std::string PACKAGE_NAME = "picknik_main";

class APCManager
{
public:

  /**
   * \brief Constructor
   * \param verbose - run in debug mode
   */
  APCManager(bool verbose, std::string order_fp);

  /**
   * \brief Destructor
   */
  ~APCManager()
  {}

  /**
   * \brief Main program runner
   * \brief Use an experience database in planning
   * \brief Show the experience database after each plan
   * \brief Which product in the order to skip ahead to
   * \return true on success
   */
  bool runOrder(bool use_experience, bool show_database, std::size_t order_start = 0, std::size_t jump_to = 0);

  /**
   * \brief Generate a discretized array of possible pre-grasps and save into experience database
   * \return true on success
   */
  bool trainExperienceDatabase();

  /**
   * \brief Test the end effectors
   * \param input - description
   * \return true on success
   */
  bool testEndEffectors();

  /**
   * \brief Simple script to move hand up and down on z axis from whereever it currently is
   * \return true on success
   */
  bool testUpAndDown();

  /**
   * \brief Script for moving arms to locations of corner of shelf
   * \return true on success
   */
  bool testShelfLocation();

  /**
   * \brief Load shelf contents
   * \return true on success
   */
  bool loadShelfContents(std::string order_fp);

  /**
   * \brief Show detailed shelf
   */
  bool visualizeShelf();

  /**
   * \brief Connect to the MoveIt! planning scene messages
   */
  bool loadPlanningSceneMonitor();

private:

  // A shared node handle
  ros::NodeHandle nh_;
  boost::shared_ptr<tf::TransformListener> tf_;

  // Show more visual and console output, with general slower run time.
  bool verbose_;

  // For visualizing things in rviz
  VisualsPtr visuals_;

  // Core MoveIt components
  robot_model_loader::RobotModelLoaderPtr robot_model_loader_;
  robot_model::RobotModelPtr robot_model_;
  planning_scene::PlanningScenePtr planning_scene_;
  planning_scene_monitor::PlanningSceneMonitorPtr planning_scene_monitor_;

  // Trajectory execution
  trajectory_execution_manager::TrajectoryExecutionManagerPtr trajectory_execution_manager_;
  boost::shared_ptr<plan_execution::PlanExecution> plan_execution_;

  // Properties
  ShelfObjectPtr shelf_;
  WorkOrders orders_;
  std::string package_path_;

  // Main worker
  ManipulationPipelinePtr pipeline_;

  // Helper classes
  LearningPipelinePtr learning_;

}; // end class

} // end namespace


#endif
