/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Ioan Sucan */

#pragma once

#include <atomic>

#include <moveit/planning_interface/planning_interface.h>
#include <moveit/planning_request_adapter/planning_request_adapter.h>
#include <pluginlib/class_loader.hpp>
#include <rclcpp/rclcpp.hpp>
#include <moveit_msgs/msg/display_trajectory.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <memory>

#include <moveit_planning_pipeline_export.h>

/** \brief Planning pipeline */
namespace planning_pipeline
{
/** \brief This class facilitates loading planning plugins and
    planning request adapted plugins.  and allows calling
    planning_interface::PlanningContext::solve() from a loaded
    planning plugin and the
    planning_request_adapter::PlanningRequestAdapter plugins, in the
    specified order. */
class MOVEIT_PLANNING_PIPELINE_EXPORT PlanningPipeline
{
public:
  /** \brief When motion plans are computed and they are supposed to be automatically displayed, they are sent to this
   * topic (moveit_msgs::msg::DisplauTrajectory) */
  static inline const std::string DISPLAY_PATH_TOPIC = std::string("display_planned_path");
  /** \brief When motion planning requests are received and they are supposed to be automatically published, they are
   * sent to this topic (moveit_msgs::msg::MotionPlanRequest) */
  static inline const std::string MOTION_PLAN_REQUEST_TOPIC = std::string("motion_plan_request");
  /** \brief When contacts are found in the solution path reported by a planner, they can be published as markers on
   * this topic (visualization_msgs::MarkerArray) */
  static inline const std::string MOTION_CONTACTS_TOPIC = std::string("display_contacts");

  /** \brief Given a robot model (\e model), a node handle (\e pipeline_nh), initialize the planning pipeline.
      \param model The robot model for which this pipeline is initialized.
      \param node The ROS node that should be used for reading parameters needed for configuration
      \param parameter_namespace parameter namespace where the planner configurations are stored
  */
  PlanningPipeline(const moveit::core::RobotModelConstPtr& model, const std::shared_ptr<rclcpp::Node>& node,
                   const std::string& parameter_namespace);

  /** \brief Given a robot model (\e model), a node handle (\e pipeline_nh), initialize the planning pipeline.
      \param model The robot model for which this pipeline is initialized.
      \param node The ROS node that should be used for reading parameters needed for configuration
      \param parameter_namespace parameter namespace where the planner configurations are stored
  */
  PlanningPipeline(const moveit::core::RobotModelConstPtr& model, const std::shared_ptr<rclcpp::Node>& node,
                   const std::string& parameter_namespace, const std::string& planning_plugin_name,
                   const std::vector<std::string>& adapter_plugin_names);

  /*
  BEGIN BLOCK OF DEPRECATED FUNCTIONS: TODO(sjahr): Remove after 04/2024 (6 months from merge)
  */
  [[deprecated("Use generatePlan or ROS parameter API instead.")]] void displayComputedMotionPlans(bool /*flag*/){};
  [[deprecated("Use generatePlan or ROS parameter API instead.")]] void publishReceivedRequests(bool /*flag*/){};
  [[deprecated("Use generatePlan or ROS parameter API instead.")]] void checkSolutionPaths(bool /*flag*/){};
  [[deprecated("Use generatePlan or ROS parameter API instead.")]] bool getDisplayComputedMotionPlans() const
  {
    return false;
  }
  [[deprecated("Use generatePlan or ROS parameter API instead.")]] bool getPublishReceivedRequests() const
  {
    return false;
  }
  [[deprecated("Use generatePlan or ROS parameter API instead.")]] bool getCheckSolutionPaths() const
  {
    return false;
  }
  /*
  END BLOCK OF DEPRECATED FUNCTIONS
  */

  /** \brief Call the motion planner plugin and the sequence of planning request adapters (if any).
      \param planning_scene The planning scene where motion planning is to be done
      \param req The request for motion planning
      \param res The motion planning response
      \param publish_received_requests Flag indicating whether received requests should be published just before
      beginning processing (useful for debugging)
      \param check_solution_paths Flag indicating whether the reported plans
      should be checked once again, by the planning pipeline itself
      \param display_computed_motion_plans Flag indicating
      whether motion plans should be published as a moveit_msgs::msg::DisplayTrajectory
      */
  [[nodiscard]] bool generatePlan(const planning_scene::PlanningSceneConstPtr& planning_scene,
                                  const planning_interface::MotionPlanRequest& req,
                                  planning_interface::MotionPlanResponse& res,
                                  const bool publish_received_requests = false, const bool check_solution_paths = true,
                                  const bool display_computed_motion_plans = true) const;

  /** \brief Request termination, if a generatePlan() function is currently computing plans */
  void terminate() const;

  /** \brief Get the name of the planning plugin used */
  [[nodiscard]] const std::string& getPlannerPluginName() const
  {
    return planner_plugin_name_;
  }

  /** \brief Get the names of the planning request adapter plugins used */
  [[nodiscard]] const std::vector<std::string>& getAdapterPluginNames() const
  {
    return adapter_plugin_names_;
  }

  /** \brief Get the planner manager for the loaded planning plugin */
  [[nodiscard]] const planning_interface::PlannerManagerPtr& getPlannerManager()
  {
    return planner_instance_;
  }

  /** \brief Get the robot model that this pipeline is using */
  [[nodiscard]] const moveit::core::RobotModelConstPtr& getRobotModel() const
  {
    return robot_model_;
  }

  /** \brief Get current status of the planning pipeline */
  [[nodiscard]] bool isActive() const
  {
    return active_;
  }

private:
  void configure();

  // Flag that indicates whether or not the planning pipeline is currently solving a planning problem
  mutable std::atomic<bool> active_;

  std::shared_ptr<rclcpp::Node> node_;
  std::string parameter_namespace_;
  /// Optionally publish motion plans as a moveit_msgs::msg::DisplayTrajectory
  rclcpp::Publisher<moveit_msgs::msg::DisplayTrajectory>::SharedPtr display_path_publisher_;

  /// Optionally publish the request before beginning processing (useful for debugging)
  rclcpp::Publisher<moveit_msgs::msg::MotionPlanRequest>::SharedPtr received_request_publisher_;

  std::unique_ptr<pluginlib::ClassLoader<planning_interface::PlannerManager> > planner_plugin_loader_;
  planning_interface::PlannerManagerPtr planner_instance_;
  std::string planner_plugin_name_;

  std::unique_ptr<pluginlib::ClassLoader<planning_request_adapter::PlanningRequestAdapter> > adapter_plugin_loader_;
  std::unique_ptr<planning_request_adapter::PlanningRequestAdapterChain> adapter_chain_;
  std::vector<std::string> adapter_plugin_names_;

  moveit::core::RobotModelConstPtr robot_model_;

  /// Publish contacts if the generated plans are checked again by the planning pipeline
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr contacts_publisher_;
};

MOVEIT_CLASS_FORWARD(PlanningPipeline);  // Defines PlanningPipelinePtr, ConstPtr, WeakPtr... etc
}  // namespace planning_pipeline
