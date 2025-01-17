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
   Desc:   Manage the manipulation of MoveIt
*/

#ifndef PICKNIK_MAIN__MANIPULATION
#define PICKNIK_MAIN__MANIPULATION

// Picknik
#include <picknik_main/namespaces.h>
#include <picknik_main/visuals.h>
#include <picknik_main/manipulation_data.h>
#include <picknik_main/fix_state_bounds.h>
#include <picknik_main/remote_control.h>
#include <picknik_main/execution_interface.h>
#include <picknik_main/tactile_feedback.h>

// ROS
#include <ros/ros.h>

// MoveIt
#include <ompl_visual_tools/ompl_visual_tools.h>
#include <moveit/kinematic_constraints/utils.h>
#include <moveit/trajectory_processing/iterative_time_parameterization.h>
#include <moveit/planning_interface/planning_interface.h>

// OMPL
#include <ompl/tools/experience/ExperienceSetup.h>

// Grasp generation
#include <moveit_grasps/grasp_data.h>
#include <moveit_grasps/grasp_filter.h>
#include <moveit_grasps/grasp_planner.h>

namespace planning_pipeline
{
MOVEIT_CLASS_FORWARD(PlanningPipeline);
}

namespace moveit_grasps
{
// MOVEIT_CLASS_FORWARD(GraspGenerator);
// TODO add more here
}

namespace picknik_main
{
MOVEIT_CLASS_FORWARD(Manipulation);

// TODO move these, last minute sloppiness
// static const double MIN_JOINT_POSITION = 0.0;
// static const double MAX_JOINT_POSITION = 0.742;

class Manipulation
{
public:
  /**
   * \brief Constructor
   * \param verbose - run in debug mode
   */
  Manipulation(bool verbose, VisualsPtr visuals,
               planning_scene_monitor::PlanningSceneMonitorPtr planning_scene_monitor,
               ManipulationDataPtr config, moveit_grasps::GraspDatas grasp_datas,
               RemoteControlPtr remote_control, bool fake_execution,
               TactileFeedbackPtr tactile_feedback);

  /**
   * \brief Choose the grasp for the object
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  // bool chooseGrasp(WorkOrder work_order, JointModelGroup* arm_jmg,
  //                  std::vector<moveit_grasps::GraspCandidatePtr> &qgrasp_candidates,
  //                  bool verbose, moveit::core::RobotStatePtr seed_state =
  //                  moveit::core::RobotStatePtr());

  /**
   * \brief Plan entire cartesian manipulation sequence
   * \param input - description
   * \return true on success
   */
  // bool planApproachLiftRetreat(moveit_grasps::GraspCandidatePtr grasp_candidate, bool
  // verbose_cartesian_paths,
  //                              WorkOrder& work_order);

  /**
   * \brief Compute a cartesian path along waypoints
   * \return true on success
   */
  bool computeCartesianWaypointPath(
      JointModelGroup* arm_jmg, moveit::core::RobotStatePtr start_state,
      const EigenSTL::vector_Affine3d& waypoints,
      std::vector<std::vector<moveit::core::RobotStatePtr>>& robot_state_trajectory);

  /**
   * \brief Find a path that accomplishes waypoints and execute all together
   * \return true on success
   */
  bool moveCartesianWaypointPath(JointModelGroup* arm_jmg, EigenSTL::vector_Affine3d waypoints);

  /**
   * \brief Move to any pose as defined in the SRDF
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \param velocity_scaling_factor - the percent of max speed all joints should be allowed to
   * utilize
   * \return true on success
   */
  bool moveToSRDFPose(JointModelGroup* arm_jmg, const std::string& pose_name,
                      double velocity_scaling_factor, bool check_validity = true);

  /** \brief No planning dude */
  bool moveToSRDFPoseNoPlan(JointModelGroup* arm_jmg, const std::string& pose_name,
                            double duration);

  /**
   * \brief Move EE to a particular pose by solving with IK
   * \param velocity_scaling_factor - the percent of max speed all joints should be allowed to
   * utilize
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool moveToEEPose(const Eigen::Affine3d& ee_pose, double velocity_scaling_factor,
                    JointModelGroup* arm_jmg);

  /**
   * \brief Send a planning request to moveit and execute
   * \param velocity_scaling_factor - the percent of max speed all joints should be allowed to
   * utilize
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool move(const moveit::core::RobotStatePtr& start, const moveit::core::RobotStatePtr& goal,
            JointModelGroup* arm_jmg, double velocity_scaling_factor, bool verbose,
            bool execute_trajectory = true, bool check_validity = true);

  /**
   * \brief Helper for planning
   * \return true on success
   */
  bool createPlanningRequest(planning_interface::MotionPlanRequest& request,
                             const moveit::core::RobotStatePtr& start,
                             const moveit::core::RobotStatePtr& goal, JointModelGroup* arm_jmg,
                             double velocity_scaling_factor);

  /**
   * \brief Helper for move() command to allow easy re-planning
   * \return true on success
   */
  bool plan(const moveit::core::RobotStatePtr& start, const moveit::core::RobotStatePtr& goal,
            JointModelGroup* arm_jmg, double velocity_scaling_factor, bool verbose,
            moveit_msgs::RobotTrajectory& trajectory_msg);

  /**
   * \brief Allow accumlated experiences to be processed by OMPL
   * \return true on success
   */
  bool planPostProcessing();

  /**
   * \brief Print experience logs
   * \return true on success
   */
  bool printExperienceLogs();

  /**
   * \brief Interpolate
   * \return true on success
   */
  bool interpolate(robot_trajectory::RobotTrajectoryPtr robot_trajectory,
                   const double& discretization);

  /**
   * \brief Get planning debug info
   * \return string describing result
   */
  std::string getActionResultString(const moveit_msgs::MoveItErrorCodes& error_code,
                                    bool planned_trajectory_empty);

  /**
   * \brief Send a single state to the controllers for execution
   * \param velocity_scaling_factor - the percent of max speed all joints should be allowed to
   * utilize
   * \return true on success
   */
  bool executeState(const moveit::core::RobotStatePtr goal_state,
                    const moveit::core::JointModelGroup* jmg, double velocity_scaling_factor);

  /** \brief Move real fast */
  bool moveDirectToState(const moveit::core::RobotStatePtr goal_state, JointModelGroup* jmg,
                         double velocity_scaling_factor);

  /**
   * \brief Using the current EE pose and the goal grasp pose, move forward into the target object
   * \param chosen - the grasp we are using
   * \return true on success
   */
  bool executeApproachPath(moveit_grasps::GraspCandidatePtr chosen);

  /**
   * \brief Using the current EE pose and the goal grasp pose, move forward into the target object
   * \param chosen - the grasp we are using
   * \return true on success
   */
  bool executeSavedCartesianPath(moveit_grasps::GraspCandidatePtr chosen, std::size_t segment_id);

  /**
   * \brief Using the current EE pose and the goal grasp pose, move forward into the target object
   * \return true on success
   */
  bool executeSavedCartesianPath(const moveit_grasps::GraspTrajectories& segmented_cartesian_traj,
                                 const moveit::core::JointModelGroup* arm_jmg,
                                 std::size_t segment_id);

  /**
   * \brief Generate the straight line path from pregrasp to grasp
   * \param chosen - all the data on the chosen grasp
   * \return true on success
   */
  bool generateApproachPath(moveit_grasps::GraspCandidatePtr chosen,
                            moveit_msgs::RobotTrajectory& approach_trajectory_msg,
                            const moveit::core::RobotStatePtr pre_grasp_state,
                            const moveit::core::RobotStatePtr the_grasp, bool verbose);

  /**
   * \brief Helper for getting the joint that is the gantry, with error checks
   * \return NULL on failure, the joint on success
   */
  const moveit::core::JointModel* getGantryJoint();

  /**
   * \brief After grasping an object, lift object up slightly
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \param desired_lift_distance
   * \param velocity_scaling_factor
   * \param up
   * \param ignore_collision
   * \return true on success
   */
  bool executeVerticlePath(const moveit::core::JointModelGroup* arm_jmg,
                           const double& desired_lift_distance,
                           const double& velocity_scaling_factor, bool up,
                           bool ignore_collision = false);

  /**
   * \brief After grasping an object, lift object up slightly
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \param desired_lift_distance
   * \param velocity_scaling_factor
   * \param up
   * \param ignore_collision
   * \param best_attempt - even if we can't achieve the desired_list_distance, execute anyway as
   * much as possible
   * \return true on success
   */
  bool executeVerticlePathGantryOnly(const moveit::core::JointModelGroup* arm_jmg,
                                     const double& desired_lift_distance,
                                     const double& velocity_scaling_factor, bool up,
                                     bool ignore_collision, bool best_attempt = false);

  /**
   * \brief After grasping an object, lift object up slightly using IK
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \param desired_lift_distance
   * \param velocity_scaling_factor
   * \param up
   * \param ignore_collision
   * \return true on success
   */
  bool executeVerticlePathWithIK(const moveit::core::JointModelGroup* arm_jmg,
                                 const double& desired_lift_distance, bool up,
                                 bool ignore_collision = false);

  /**
   * \brief Translate arm left and right
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool executeHorizontalPath(const moveit::core::JointModelGroup* arm_jmg,
                             const double& desired_left_distance, bool left,
                             bool ignore_collision = false);

  /**
   * \brief After grasping an object, pull object out of shelf in reverse
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool executeRetreatPath(const moveit::core::JointModelGroup* arm_jmg,
                          double desired_retreat_distance, bool retreat,
                          bool ignore_collision = false);

  /** \brief Use tactile feedback
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \param desired_distance
   * \param direction_pose
   * \param direction_in
   */
  bool executeInsertionClosedLoop(JointModelGroup* arm_jmg, double desired_distance,
                                  Eigen::Affine3d& desired_pose, bool direction_in,
                                  bool& achieved_depth);

  /** \brief Insertion by stream cartesian waypoints */
  bool executeInsertionOpenLoopNew(JointModelGroup* arm_jmg, double desired_distance,
                                   double duration, Eigen::Affine3d& desired_world_to_tool,
                                   bool direction_in, bool& achieved_depth);

  /** \brief Use tactile feedback
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \param desired_distance
   * \param in
   * \param velocity_scaling_factor
   */
  bool executeInsertionOpenLoop(JointModelGroup* arm_jmg, double desired_distance, bool in,
                                double velocity_scaling_factor);

  /**
   * \brief Pose in tool form
   * \return true on success
   */
  bool executeToolPose(JointModelGroup* arm_jmg, Eigen::Affine3d& pose_world_to_tool,
                       double duration);

  /** \brief Detect when an object has been hit, and retract */
  bool adjustPoseAndReject();

  /**
   * \brief Generic execute straight line path function
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool executeCartesianPath(const moveit::core::JointModelGroup* arm_jmg,
                            const Eigen::Vector3d& direction, double desired_distance,
                            double velocity_scaling_factor, bool reverse_path,
                            bool ignore_collision = false);

  /**
   * \brief Function for testing multiple directions
   * \param approach_direction - direction to move end effector straight
   * \param desired_approach_distance - distance the origin of a robot link needs to travel
   * \param trajectory_msg - resulting path
   * \param robot_state - used as the base state of the robot when starting to move
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \param reverse_trajectory - whether to reverse the generated trajectory before displaying
   * visualizations and returning
   * \param path_length - the length of the resulting cartesian path
   * \param ignore_collision - allows recovery from a collision state
   * \return true on success
   */
  bool computeStraightLinePath(Eigen::Vector3d approach_direction, double desired_approach_distance,
                               std::vector<robot_state::RobotStatePtr>& robot_state_trajectory,
                               robot_state::RobotStatePtr robot_state,
                               const moveit::core::JointModelGroup* arm_jmg,
                               bool reverse_trajectory, double& path_length,
                               bool ignore_collision = false);

  /**
   * \brief Choose which arm to use for a particular task
   * \return joint model group of arm to use in manip
   */
  JointModelGroup* chooseArm(const Eigen::Affine3d& ee_pose);

  /**
   * \brief Get the robot state that accomplished a desired end effector pose
   * \return true on success
   */
  bool getRobotStateFromPose(const Eigen::Affine3d& ee_pose,
                             moveit::core::RobotStatePtr& robot_state, JointModelGroup* arm_jmg,
                             bool use_consistency_limits = false);

  /**
   * \brief Move a pose in a specified direction and specified length, where all poses are in the
   * world frame
   * \return true on success
   */
  bool straightProjectPose(const Eigen::Affine3d& original_pose, Eigen::Affine3d& new_pose,
                           const Eigen::Vector3d direction, double distance);

  /**
   * \brief Convert and parameterize a trajectory with velocity information
   * \param velocity_scaling_factor - the percent of max speed all joints should be allowed to
   * utilize
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool convertRobotStatesToTrajectory(
      const std::vector<robot_state::RobotStatePtr>& robot_state_trajectory,
      moveit_msgs::RobotTrajectory& trajectory_msg, JointModelGroup* arm_jmg,
      const double& velocity_scaling_factor, bool interpolate = true);

  /**
   * \brief Open both end effectors in hardware
   * \return true on success
   */
  bool openEEs(bool open);

  /**
   * \brief Open/close grippers using velocity commands
   * \param bool if it should be open or closed
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on sucess
   */
  bool openEE(bool open, JointModelGroup* arm_jmg);

  /**
   * \brief Set the joint values of the finger joints - TODO - this is very Jaco-specific
   * \param joint_position
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on sucess
   */
  bool setEEJointPosition(double joint_position, JointModelGroup* arm_jmg);

  /**
   * \brief Set the full grasp posture of the finger joints
   * \param joint_position
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on sucess
   */
  bool setEEGraspPosture(trajectory_msgs::JointTrajectory grasp_posture, JointModelGroup* arm_jmg);

  /**
   * \brief Set a robot state to have an open or closed EE. Does not actually affect hardware
   * \return true on success
   */
  // bool setStateWithOpenEE(bool open, moveit::core::RobotStatePtr robot_state);

  /**
   * \brief Get the interface to execution calling
   */
  ExecutionInterfacePtr getExecutionInterface();

  /**
   * \brief Attempt to fix when the robot is in collision by moving arm out of way
   * \return true on success
   */
  bool fixCollidingState(planning_scene::PlanningScenePtr cloned_scene);

  /**
   * \brief Save a trajectory as CSV to file
   * \return true on success
   */
  bool saveTrajectory(const moveit_msgs::RobotTrajectory& trajectory_msg,
                      const std::string& file_name);

  /**
   * \brief Move both arms to their start location
   * \param optionally specify which arm to use
   * \return true on success
   */
  bool moveToStartPosition(JointModelGroup* arm_jmg, bool check_validity = true);

  /**
   * \brief Prevent a product from colliding with the fingers
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool allowFingerTouch(const std::string& object_name, JointModelGroup* arm_jmg);

  /**
   * \brief Load it
   * \return true on success
   */
  void loadPlanningPipeline();

  /**
   * \brief Helper function for determining if robot is already in desired state
   * \param robotstate to compare to
   * \param robotstate to compare to
   * \param arm_jmg - only compare joints in this joint model group
   * \return true if states are close enough in similarity
   */
  bool statesEqual(const moveit::core::RobotState& s1, const moveit::core::RobotState& s2,
                   JointModelGroup* arm_jmg);

  /**
   * \brief Helper to access the experience database
   * \return pointer to a loaded expience database
   */
  ompl::tools::ExperienceSetupPtr getExperienceSetup(JointModelGroup* arm_jmg);

  /**
   * \brief Show the trajectories saved in the experience database
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool displayExperienceDatabase(JointModelGroup* arm_jmg);

  /**
   * \brief Visulization function
   * \param input - description
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  // bool visualizeGrasps(std::vector<moveit_grasps::GraspCandidatePtr> grasp_candidates, const
  // moveit::core::JointModelGroup *arm_jmg,
  //                     bool show_cartesian_path = true);

  /**
   * \brief Visalize ik solutions
   * \param input - description
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true on success
   */
  bool visualizeIKSolutions(std::vector<moveit_grasps::GraspCandidatePtr> grasp_candidates,
                            const moveit::core::JointModelGroup* arm_jmg, double display_time = 2);

  /**
   * \brief Update the current_state_ RobotState with latest from planning scene
   */
  moveit::core::RobotStatePtr getCurrentState();

  /**
   * \brief Wait until robot has zero velcity
   * \param timeout
   * \return true if robots velocity reached the threshold
   */
  bool waitForRobotToStop(const double& timeout);

  /**
   * \brief Check if current state is in collision or out of bounds
   * \param arm_jmg - the kinematic chain of joint that should be controlled (a planning group)
   * \return true if not in collision and not out of bounds
   */
  bool fixCurrentCollisionAndBounds(JointModelGroup* arm_jmg);

  /**
   * \brief Check if states are in collision or out of bounds
   * \param start_state to check
   * \param goal_state OPTIONAL to check
   * \return true if not in collision and not out of bounds
   */
  bool checkCollisionAndBounds(
      const robot_state::RobotStatePtr& start_state,
      const robot_state::RobotStatePtr& goal_state = robot_state::RobotStatePtr(),
      bool verbose = true);

  /**
   * \brief Setup products randomly
   * \return true on success
   */
  bool generateRandomProductPoses();

  /**
   * \brief Get pose from TF for any frame
   * \return true on success
   */
  bool getPose(Eigen::Affine3d& pose, const std::string& frame_name);

  /**
   * \brief Get max value of a joint that has only one variable
   * \return true on success
   */
  double getMaxJointLimit(const moveit::core::JointModel* joint);

  /**
   * \brief Get min value of a joint that has only one variable
   * \return true on success
   */
  double getMinJointLimit(const moveit::core::JointModel* joint);

  /**
   * \brief Debug visualization tool for joint limits
   * \return true on success
   */
  bool showJointLimits(JointModelGroup* jmg);

  /** \brief Quickly response to pose requests. Uses IK on dev computer, not embedded */
  bool teleoperation(const Eigen::Affine3d& ee_pose, bool move, JointModelGroup* arm_jmg);

  /** \brief Respond to touch sensors on hand */
  bool beginTouchControl();

  /** \brief Adjust the teleop tool pose to tactile feedback
      \return true if pose was adjusted, false if remained same
   */
  bool adjustPoseFromTactile();

  /** \brief Help display which way the EE is moving */
  void showDirectionArrow(double torque, bool show);

  /** \brief Callback whenever new gelsight data recieved */
  void updateTouchControl(JointModelGroup* arm_jmg);

  /** \brief Setup robot state for teleop */
  bool enableTeleoperation();

  /** \brief Convert from world frame to base_link frame */
  void transformWorldToBase(Eigen::Affine3d& pose_world, Eigen::Affine3d& pose_base);

  /**
   * \brief Get the iterative smoother
   */
  trajectory_processing::IterativeParabolicTimeParameterization getIterativeSmoother()
  {
    return iterative_smoother_;
  }

protected:
  // A shared node handle
  ros::NodeHandle nh_;

  // Show more visual and console output, with general slower run time.
  bool verbose_;

  // For executing trajectories
  ExecutionInterfacePtr execution_interface_;

  // For visualizing things in rviz
  VisualsPtr visuals_;
  ovt::OmplVisualToolsPtr ompl_visual_tools_;

  // Core MoveIt components
  planning_scene_monitor::PlanningSceneMonitorPtr planning_scene_monitor_;
  robot_model::RobotModelConstPtr robot_model_;
  planning_pipeline::PlanningPipelinePtr planning_pipeline_;
  planning_interface::PlanningContextPtr planning_context_handle_;

  // Allocated memory for robot state
  moveit::core::RobotStatePtr current_state_;
  moveit::core::RobotStatePtr first_state_in_trajectory_;  // for use with generateApproachPath()
  moveit::core::RobotStatePtr teleop_state_;

  // Robot-sepcific data for the APC
  ManipulationDataPtr config_;

  // Robot-specific data for generating grasps
  moveit_grasps::GraspDatas grasp_datas_;

  // Remote control
  RemoteControlPtr remote_control_;

  // End effector sheer force teleoperation
  TactileFeedbackPtr tactile_feedback_;
  Eigen::Vector3d teleop_direction_;
  Eigen::Vector3d teleop_rotated_direction_;
  Eigen::Affine3d teleop_world_to_tool_;
  Eigen::Affine3d teleop_world_to_ee_;
  Eigen::Affine3d teleop_base_to_ee_;

  // Experience-based planning
  bool use_experience_;
  bool use_loggaing_;
  std::ofstream logging_file_;

  // Grasp generator
  moveit_grasps::GraspGeneratorPtr grasp_generator_;
  moveit_grasps::GraspFilterPtr grasp_filter_;
  moveit_grasps::GraspPlannerPtr grasp_planner_;

  // State modification helper
  FixStateBounds fix_state_bounds_;
  trajectory_processing::IterativeParabolicTimeParameterization iterative_smoother_;

};  // end class

}  // end namespace

namespace
{
bool isStateValid(const planning_scene::PlanningScene* planning_scene, bool verbose,
                  bool only_check_self_collision, picknik_main::VisualsPtr visuals,
                  robot_state::RobotState* state, const robot_state::JointModelGroup* group,
                  const double* ik_solution);
}

#endif
