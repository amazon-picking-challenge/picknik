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
   Desc:   Object oriented shelf system - represents shelf, bins, etc
*/

#include <picknik_main/shelf.h>

// Parameter loading
#include <ros_param_utilities/ros_param_utilities.h>

namespace picknik_main
{
// -------------------------------------------------------------------------------------------------
// Bin Object
// -------------------------------------------------------------------------------------------------

BinObject::BinObject(VisualsPtr visuals, const rvt::colors &color, const std::string &name)
  : RectangleObject(visuals, color, name)
{
}

bool BinObject::visualizeHighRes(const Eigen::Affine3d &trans) const
{
  // Show bin
  // visuals_->visual_tools_display_->publishCuboid( transform(bottom_right_, trans).translation(),
  //                                 transform(top_left_, trans).translation(),
  //                                 color_);

  // Show products
  for (std::size_t product_id = 0; product_id < products_.size(); ++product_id)
  {
    products_[product_id]->visualizeHighRes(trans *
                                            bottom_right_);  // send transform from world to bin
  }

  return true;
}

bool BinObject::visualizeAxis(const Eigen::Affine3d &trans, VisualsPtr visuals) const
{
  // Show coordinate system
  // visuals_->visual_tools_->publishAxisLabled( transform(bottom_right_, trans), name_ );

  // Show label
  // Eigen::Affine3d text_location = transform( bottom_right_, trans);

  // text_location.translation() += Eigen::Vector3d(0,getWidth()/2.0, getHeight()*0.9);

  // visuals->visual_tools_->publishText( text_location, name_, rvt::BLACK, rvt::REGULAR, false);

  return true;
}

bool BinObject::createCollisionBodiesProducts(const Eigen::Affine3d &trans) const
{
  // Show products
  for (std::size_t product_id = 0; product_id < products_.size(); ++product_id)
  {
    products_[product_id]->createCollisionBodies(
        trans * bottom_right_);  // send transform from world to bin
  }
  return true;
}

std::vector<ProductObjectPtr> &BinObject::getProducts() { return products_; }
void BinObject::getProducts(std::vector<std::string> &products)
{
  products.clear();
  for (std::size_t i = 0; i < products_.size(); ++i)
  {
    products.push_back(products_[i]->getName());
  }
}

ProductObjectPtr BinObject::getProduct(const std::string &name)
{
  // Find correct product
  for (std::size_t prod_id = 0; prod_id < products_.size(); ++prod_id)
  {
    if (products_[prod_id]->getName() == name)
    {
      return products_[prod_id];
    }
  }

  return ProductObjectPtr();
}

Eigen::Affine3d BinObject::getBinToWorld(ShelfObjectPtr &shelf)
{
  return transform(getBottomRight(), shelf->getBottomRight());
}

// -------------------------------------------------------------------------------------------------
// Shelf Object
// -------------------------------------------------------------------------------------------------

ShelfObject::ShelfObject(VisualsPtr visuals, const rvt::colors &color, const std::string &name,
                         bool use_computer_vision_shelf)
  : RectangleObject(visuals, color, name)
  , use_computer_vision_shelf_(use_computer_vision_shelf)
{
}

bool ShelfObject::initialize(const std::string &package_path, ros::NodeHandle &nh)
{
  const std::string parent_name = "shelf";  // for namespacing logging messages

  // Loaded shelf parameter values
  std::vector<double> world_to_shelf_transform_doubles;
  if (!ros_param_utilities::getDoubleParameters(parent_name, nh, "world_to_shelf_transform",
                                                world_to_shelf_transform_doubles))
    return false;
  if (!ros_param_utilities::convertDoublesToEigen(parent_name, world_to_shelf_transform_doubles,
                                                  world_to_shelf_transform_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "shelf_width", shelf_width_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "shelf_height", shelf_height_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "shelf_depth", shelf_depth_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "shelf_wall_width",
                                               shelf_wall_width_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "shelf_inner_wall_width",
                                               shelf_inner_wall_width_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "shelf_surface_thickness",
                                               shelf_surface_thickness_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "first_bin_from_bottom",
                                               first_bin_from_bottom_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "first_bin_from_right",
                                               first_bin_from_right_))
    return false;

  // Loaded bin parameter values
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "bin_right_width",
                                               bin_right_width_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "bin_middle_width",
                                               bin_middle_width_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "bin_left_width", bin_left_width_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "bin_short_height",
                                               bin_short_height_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "bin_tall_height",
                                               bin_tall_height_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "bin_depth", bin_depth_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "bin_top_margin", bin_top_margin_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "bin_left_margin",
                                               bin_left_margin_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "num_bins", num_bins_))
    return false;

  // Goal bin
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "goal_bin_x", goal_bin_x_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "goal_bin_y", goal_bin_y_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "goal_bin_z", goal_bin_z_))
    return false;

  // Side limits (walls)
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "left_wall_y", left_wall_y_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "right_wall_y", right_wall_y_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "ceiling_z", ceiling_z_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh, "collision_wall_safety_margin",
                                               collision_wall_safety_margin_))
    return false;

  // Collision Shelf
  std::vector<double> collision_shelf_transform_doubles;
  double collision_shelf_transform_x_offset;
  if (!ros_param_utilities::getDoubleParameters(parent_name, nh, "collision_shelf_transform",
                                                collision_shelf_transform_doubles))
    return false;
  if (!ros_param_utilities::convertDoublesToEigen(parent_name, collision_shelf_transform_doubles,
                                                  collision_shelf_transform_))
    return false;
  if (!ros_param_utilities::getDoubleParameter(parent_name, nh,
                                               "collision_shelf_transform_x_offset",
                                               collision_shelf_transform_x_offset))
    return false;

  // Calculate shelf corners for *this ShelfObject
  bottom_right_ = world_to_shelf_transform_;
  top_left_.translation().x() = bottom_right_.translation().x() + shelf_depth_;
  top_left_.translation().y() = shelf_width_ / 2.0;
  top_left_.translation().z() = shelf_height_;

  // Transform debugging
  // visuals_->tf_->publishTransform(world_to_shelf_transform_, "world", "shelf");

  // Create shelf parts -----------------------------
  // Note: bottom right is at 0,0,0

  Eigen::Affine3d bottom_right;
  Eigen::Affine3d top_left;
  // TODO delete first_bin_from_right_

  // Base
  bool fake_table = false;
  shelf_parts_.push_back(RectangleObject(visuals_, color_, "base"));
  RectangleObject &base = shelf_parts_.back();
  top_left = base.getTopLeft();
  top_left.translation().x() += shelf_depth_;
  top_left.translation().y() += shelf_width_;
  top_left.translation().z() += first_bin_from_bottom_;
  if (fake_table)
    top_left.translation().y() += 1.0;
  base.setTopLeft(top_left);
  if (fake_table)
  {
    Eigen::Affine3d temp = base.getBottomRight();
    temp.translation().x() = -1.0;
    temp.translation().y() = -1.0;
    base.setBottomRight(temp);
  }

  // Extend base to protect robot from table
  bool immitation_table_mount = false;
  if (immitation_table_mount)
  {
    bottom_right = Eigen::Affine3d::Identity();
    bottom_right.translation().x() -= 1.0;
    base.setBottomRight(bottom_right);
  }

  // Shelf Walls
  double previous_y = shelf_wall_width_ * 0.5;
  double this_shelf_wall_width;
  double this_bin_width;
  double bin_z;
  for (std::size_t wall_id = 0; wall_id < 4; ++wall_id)
  {
    const std::string wall_name = "wall_" + boost::lexical_cast<std::string>(wall_id);
    shelf_parts_.push_back(RectangleObject(visuals_, color_, wall_name));
    RectangleObject &wall = shelf_parts_.back();

    // Choose what wall width the current bin is
    if (wall_id == 1 || wall_id == 2)
      this_shelf_wall_width = shelf_inner_wall_width_;
    else
      this_shelf_wall_width = shelf_wall_width_;

    // Shelf Wall Geometry
    bottom_right = wall.getBottomRight();
    bottom_right.translation().x() = 0;
    bottom_right.translation().y() = previous_y - this_shelf_wall_width * 0.5;
    bottom_right.translation().z() = first_bin_from_bottom_;
    wall.setBottomRight(bottom_right);

    top_left = wall.getTopLeft();
    top_left.translation().x() = shelf_depth_;
    top_left.translation().y() = previous_y + this_shelf_wall_width * 0.5;
    top_left.translation().z() = shelf_height_;
    wall.setTopLeft(top_left);

    // Choose what width the current bin is
    if (wall_id == 0)
      this_bin_width = bin_right_width_;
    else if (wall_id == 1)
      this_bin_width = bin_middle_width_;
    else if (wall_id == 2)
      this_bin_width = bin_left_width_;
    else
      this_bin_width = 0;  // doesn't matter

    // Increment the y location
    previous_y += this_bin_width + this_shelf_wall_width;

    // Create a column of shelf bins from top to bottom
    if (wall_id < 3)
    {
      bin_z = first_bin_from_bottom_;
      insertBinHelper(wall_id + 0, bin_tall_height_, this_bin_width, top_left.translation().y(),
                      bin_z);  // bottom rop
      bin_z += bin_tall_height_;
      insertBinHelper(wall_id + 3, bin_short_height_, this_bin_width, top_left.translation().y(),
                      bin_z);  // middle bottom row
      bin_z += bin_short_height_;
      insertBinHelper(wall_id + 6, bin_short_height_, this_bin_width, top_left.translation().y(),
                      bin_z);  // middle top row
      bin_z += bin_short_height_;
      insertBinHelper(wall_id + 9, bin_tall_height_, this_bin_width, top_left.translation().y(),
                      bin_z);  // top row
    }
  }

  // Shelves
  double previous_z = first_bin_from_bottom_;
  double this_bin_height;
  for (std::size_t i = 0; i < 5; ++i)
  {
    const std::string shelf_name = "surface_" + boost::lexical_cast<std::string>(i);
    shelf_parts_.push_back(RectangleObject(visuals_, color_, shelf_name));
    RectangleObject &shelf = shelf_parts_.back();

    // Geometry
    top_left = shelf.getTopLeft();
    top_left.translation().x() = shelf_width_;
    top_left.translation().y() = shelf_depth_;
    top_left.translation().z() = previous_z;
    shelf.setTopLeft(top_left);

    bottom_right = shelf.getBottomRight();
    bottom_right.translation().z() =
        shelf.getTopLeft().translation().z() - shelf_surface_thickness_;  // add thickenss
    shelf.setBottomRight(bottom_right);

    // Choose what height the current bin is
    if (i == 1 || i == 2)
      this_bin_height = bin_short_height_;
    else
      this_bin_height = bin_tall_height_;

    // Increment the z location
    previous_z += this_bin_height;  // flush with top shelf
  }

  // Goal bin
  goal_bin_.reset(new MeshObject(visuals_, rvt::RED, "goal_bin"));
  Eigen::Affine3d goal_bin_centroid = Eigen::Affine3d::Identity();
  goal_bin_centroid.translation().x() = goal_bin_x_;
  goal_bin_centroid.translation().y() = goal_bin_y_;
  goal_bin_centroid.translation().z() = goal_bin_z_;
  goal_bin_->setCentroid(goal_bin_centroid);
  goal_bin_->setMeshCentroid(goal_bin_centroid);

  goal_bin_->setHighResMeshPath("file://" + package_path + "/meshes/goal_bin/goal_bin.stl");
  goal_bin_->setCollisionMeshPath("file://" + package_path + "/meshes/goal_bin/goal_bin.stl");

  // Computer vision shelf
  loadComputerVisionShelf(collision_shelf_transform_doubles, collision_shelf_transform_x_offset,
                          package_path);

  // Front wall limit
  front_wall_.reset(new RectangleObject(visuals_, rvt::YELLOW, "front_wall"));
  front_wall_->setBottomRight(-collision_wall_safety_margin_, -shelf_width_ / 2.0, 0);
  front_wall_->setTopLeft(COLLISION_OBJECT_WIDTH, shelf_width_ * 1.5, shelf_height_);

  // Other objects in our collision environment
  addOtherCollisionObjects();

  // Load mesh file name
  high_res_mesh_path_ = "file://" + package_path + "/meshes/kiva_pod/meshes/pod_lowres.stl";
  collision_mesh_path_ = high_res_mesh_path_;

  // Calculate offset for high-res mesh
  // high_res_mesh_offset_ = Eigen::Affine3d::Identity();
  high_res_mesh_offset_ = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitX()) *
                          Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitY()) *
                          Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ());
  high_res_mesh_offset_.translation().x() = shelf_depth_ / 2.0;
  high_res_mesh_offset_.translation().y() = shelf_width_ / 2.0;
  high_res_mesh_offset_.translation().z() =
      0;  // first_bin_from_bottom_ - 0.81; // TODO remove this height - only for temp table setup

  // Calculate offset - FOR COLLISION
  Eigen::Affine3d offset;
  offset = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitX()) *
           Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitY()) *
           Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ());
  offset.translation().x() = bottom_right_.translation().x() + shelf_depth_ / 2.0;
  offset.translation().y() = 0;

  return true;
}

void ShelfObject::addOtherCollisionObjects()
{
  // Side limit walls
  if (left_wall_y_ > 0.001 || left_wall_y_ < -0.001)
  {
    RectangleObjectPtr left_wall(new RectangleObject(visuals_, rvt::YELLOW, "left_wall"));
    left_wall->setBottomRight(-1, left_wall_y_, 0);
    left_wall->setTopLeft(1, left_wall_y_ + shelf_wall_width_, shelf_height_);
    environment_objects_["left_wall"] = left_wall;
  }
  if (right_wall_y_ > 0.001 || right_wall_y_ < -0.001)
  {
    RectangleObjectPtr right_wall(new RectangleObject(visuals_, rvt::YELLOW, "right_wall"));
    right_wall->setBottomRight(-2, right_wall_y_, 0);
    right_wall->setTopLeft(0, right_wall_y_ + shelf_wall_width_, shelf_height_);
    environment_objects_["right_wall"] = right_wall;
  }

  // Ceiling wall limit
  RectangleObjectPtr ceiling_wall(
      new RectangleObject(visuals_, rvt::TRANSLUCENT_LIGHT, "ceiling_wall"));
  ceiling_wall->setBottomRight(-2, -shelf_width_, ceiling_z_);
  ceiling_wall->setTopLeft(1, shelf_width_ * 2, ceiling_z_ + COLLISION_OBJECT_WIDTH);
  environment_objects_["ceiling_wall"] = ceiling_wall;

  // Floor wall limit
  RectangleObjectPtr floor_wall(
      new RectangleObject(visuals_, rvt::TRANSLUCENT_LIGHT, "floor_wall"));
  floor_wall->setBottomRight(ceiling_wall->getBottomRight().translation().x(),
                             ceiling_wall->getBottomRight().translation().y(), 0);
  floor_wall->setTopLeft(ceiling_wall->getTopLeft().translation().x(),
                         ceiling_wall->getTopLeft().translation().y(), -COLLISION_OBJECT_WIDTH);
  environment_objects_["floor_wall"] = floor_wall;

  // Left side bars of our dummy shelf
  static const double SIDE_BAR_WIDTH = 0.12;
  RectangleObjectPtr left_shelf_bars(
      new RectangleObject(visuals_, rvt::TRANSLUCENT_LIGHT, "left_shelf_bars"));
  left_shelf_bars->setBottomRight(-0.05, -SIDE_BAR_WIDTH, 0.0);
  left_shelf_bars->setTopLeft(0, 0, shelf_height_);
  environment_objects_["left_shelf_bars"] = left_shelf_bars;

  // Right side bars of our dummy shelf
  RectangleObjectPtr right_shelf_bars(
      new RectangleObject(visuals_, rvt::TRANSLUCENT_LIGHT, "right_shelf_bars"));
  right_shelf_bars->setBottomRight(-0.05, shelf_width_, 0.0);
  right_shelf_bars->setTopLeft(0, shelf_width_ + SIDE_BAR_WIDTH, shelf_height_);
  environment_objects_["right_shelf_bars"] = right_shelf_bars;
}

bool ShelfObject::insertBinHelper(int bin_id, double height, double width, double wall_y,
                                  double bin_z)
{
  const std::string bin_name =
      "bin_" + boost::lexical_cast<std::string>(
                   (char)(65 + num_bins_ - bin_id - 1));  // reverse the lettering
  // std::string bin_name = "bin_" + boost::lexical_cast<std::string>(bin_id);
  ROS_DEBUG_STREAM_NAMED("shelf", "Creating bin '" << bin_name << "' with id " << bin_id);

  BinObjectPtr new_bin(new BinObject(visuals_, rvt::TRANSLUCENT_DARK, bin_name));
  bins_.insert(std::pair<std::string, BinObjectPtr>(bin_name, new_bin));

  // Calculate bottom right
  Eigen::Affine3d bottom_right = Eigen::Affine3d::Identity();
  bottom_right.translation().x() = 0;
  bottom_right.translation().y() = wall_y;
  bottom_right.translation().z() = bin_z;
  new_bin->setBottomRight(bottom_right);

  // Transform debugging
  visuals_->tf_->publishTransform(bottom_right, "shelf", bin_name);

  // Calculate top left
  Eigen::Affine3d top_left = Eigen::Affine3d::Identity();
  top_left.translation().x() += bin_depth_;
  top_left.translation().y() += wall_y + width;
  top_left.translation().z() += bin_z + height - shelf_surface_thickness_;
  new_bin->setTopLeft(top_left);

  return true;
}

bool ShelfObject::visualizeAxis(VisualsPtr visuals) const
{
  // Show coordinate system
  // visuals_->visual_tools_->publishAxis( bottom_right_ );

  // Show each bin
  for (BinObjectMap::const_iterator bin_it = bins_.begin(); bin_it != bins_.end(); bin_it++)
  {
    bin_it->second->visualizeAxis(bottom_right_, visuals);
  }
  return true;
}

bool ShelfObject::visualizeHighRes(bool show_products) const
{
  Eigen::Affine3d high_res_pose = bottom_right_ * high_res_mesh_offset_;

  // Publish mesh
  if (!visuals_->visual_tools_display_->publishMesh(high_res_pose, high_res_mesh_path_, rvt::BROWN,
                                                    1, "Shelf"))
    return false;

  // Show each bin's products
  if (show_products)
  {
    for (BinObjectMap::const_iterator bin_it = bins_.begin(); bin_it != bins_.end(); bin_it++)
    {
      bin_it->second->visualizeHighRes(bottom_right_);
    }
  }

  // Show goal bin
  goal_bin_->visualizeHighRes(bottom_right_);

  // Show computer vision shelf
  if (use_computer_vision_shelf_)
    computer_vision_shelf_->visualizeHighRes(Eigen::Affine3d::Identity());

  // Show all other collision objects
  visualizeEnvironmentObjects();

  // Show workspace
  static const double GAP_TO_SHELF = 0.1;
  const double x1 = world_to_shelf_transform_.translation().x() - GAP_TO_SHELF;
  const double x2 = world_to_shelf_transform_.translation().x() - GAP_TO_SHELF - 2;
  const Eigen::Vector3d point1(x1, 1, 0);
  const Eigen::Vector3d point2(x2, -1, 0.001);
  visuals_->visual_tools_display_->publishCuboid(point1, point2, rvt::DARK_GREY);
  return true;
}

bool ShelfObject::visualizeEnvironmentObjects() const
{
  // Show all other environmental objects (walls, etc)
  for (std::map<std::string, RectangleObjectPtr>::const_iterator env_it =
           environment_objects_.begin();
       env_it != environment_objects_.end(); env_it++)
  {
    env_it->second->visualizeHighRes(bottom_right_);
  }
  return true;
}

bool ShelfObject::createCollisionBodiesEnvironmentObjects() const
{
  // Show all other environmental objects (walls, etc)
  for (std::map<std::string, RectangleObjectPtr>::const_iterator env_it =
           environment_objects_.begin();
       env_it != environment_objects_.end(); env_it++)
  {
    env_it->second->createCollisionBodies(bottom_right_);
  }
  return true;
}

bool ShelfObject::createCollisionBodies(const std::string &focus_bin_name,
                                        bool only_show_shelf_frame, bool show_all_products)
{
  // Publish in batch
  visuals_->visual_tools_->enableBatchPublishing(true);

  bool show_full_res = true;

  if (show_full_res)
  {
    // Show full resolution shelf -----------------------------------------------------------------
    createCollisionShelfDetailed();
  }
  else
  {
    // Show simple version of shelf
    // -----------------------------------------------------------------

    // Create side walls of shelf
    for (std::size_t i = 0; i < shelf_parts_.size(); ++i)
    {
      shelf_parts_[i].createCollisionBodies(bottom_right_);
    }
  }

  // Show each bin except the focus on
  if (!only_show_shelf_frame)
  {
    // Send all the shapes for the shelf first, then secondly the products
    BinObjectPtr focus_bin;

    for (BinObjectMap::const_iterator bin_it = bins_.begin(); bin_it != bins_.end(); bin_it++)
    {
      // Check if this is the focused bin
      if (bin_it->second->getName() == focus_bin_name)
      {
        focus_bin = bin_it->second;  // save for later
      }
      else if (!show_all_products)
      {
        // Fill in bin as simple rectangle (disabled mode)
        bin_it->second->createCollisionBodies(bottom_right_);
      }

      // Optionally add all products to shelves
      if (show_all_products)
      {
        bin_it->second->createCollisionBodiesProducts(bottom_right_);
      }
    }

    if (focus_bin && !show_all_products)  // don't redisplay products if show_all_products is true
    {
      // Add products to shelves
      focus_bin->createCollisionBodiesProducts(bottom_right_);
    }
  }

  // Show goal bin
  goal_bin_->createCollisionBodies(bottom_right_);

  // Show computer vision shelf
  if (use_computer_vision_shelf_)
    computer_vision_shelf_->createCollisionBodies(Eigen::Affine3d::Identity());

  // Show axis
  // goal_bin_->visualizeAxis(bottom_right_);

  // Show all other environmental objects (walls, etc)
  createCollisionBodiesEnvironmentObjects();

  return visuals_->visual_tools_->triggerBatchPublishAndDisable();
}

bool ShelfObject::createCollisionShelfDetailed()
{
  ROS_DEBUG_STREAM_NAMED("shelf", "Creating collision body with name " << collision_object_name_);

  Eigen::Affine3d high_res_pose = bottom_right_ * high_res_mesh_offset_;

  // Publish mesh
  if (!visuals_->visual_tools_->publishCollisionMesh(high_res_pose, collision_object_name_,
                                                     high_res_mesh_path_, color_))
    return false;
  return true;
}

BinObjectMap &ShelfObject::getBins() { return bins_; }
BinObjectPtr ShelfObject::getBin(std::size_t bin_id)
{
  const std::string bin_name = "bin_" + boost::lexical_cast<std::string>((char)(64 + bin_id));
  std::cout << "bin_name: " << bin_name << std::endl;
  return bins_[bin_name];
}

ProductObjectPtr ShelfObject::getProduct(const std::string &bin_name,
                                         const std::string &product_name)
{
  // Find correct bin
  BinObjectPtr bin = bins_[bin_name];

  if (!bin)
  {
    ROS_WARN_STREAM_NAMED("shelf", "Unable to find product " << product_name << " in bin "
                                                             << bin_name << " in the database");
    return ProductObjectPtr();
  }

  ProductObjectPtr product = bin->getProduct(product_name);

  if (!product)
  {
    ROS_WARN_STREAM_NAMED("shelf", "Unable to find product " << product_name << " in bin "
                                                             << bin_name << " in the database");
    return ProductObjectPtr();
  }

  return product;
}

bool ShelfObject::getAllProducts(std::vector<ProductObjectPtr> &products)
{
  products.clear();
  for (BinObjectMap::const_iterator bin_it = bins_.begin(); bin_it != bins_.end(); bin_it++)
  {
    for (std::vector<ProductObjectPtr>::const_iterator product_it =
             bin_it->second->getProducts().begin();
         product_it != bin_it->second->getProducts().end(); product_it++)
    {
      products.push_back(*product_it);
    }
  }
  return true;
}

bool ShelfObject::deleteProduct(BinObjectPtr bin, ProductObjectPtr product)
{
  std::vector<ProductObjectPtr> &products = bin->getProducts();
  // Find correct product
  for (std::size_t prod_id = 0; prod_id < products.size(); ++prod_id)
  {
    if (products[prod_id] == product)
    {
      products.erase(products.begin() + prod_id);
      return true;
    }
  }

  ROS_WARN_STREAM_NAMED("shelf", "Unable to delete product " << product->getName() << " in bin "
                                                             << bin->getName()
                                                             << " in the database");
  return false;
}

bool ShelfObject::loadComputerVisionShelf(
    const std::vector<double> &collision_shelf_transform_doubles,
    double collision_shelf_transform_x_offset, const std::string &package_path)
{
  // Lu Ma Saves this model in the WORLD COORDINATE SYSTEM
  ROS_DEBUG_STREAM_NAMED("shelf", "Loading shelf from computer vision...");
  computer_vision_shelf_.reset(new MeshObject(visuals_, rvt::YELLOW, "computer_vision_shelf"));

  // pose from Lu Ma, mesh file is saved in computer vision orientation of "xtion_right_rgb_frame"
  Eigen::Affine3d computer_vision_shelf_pose = Eigen::Affine3d::Identity();
  // //0.0111163    0.383599    1.70677    0.00241    0.286    -0.2208
  Eigen::Vector3d translation =
      Eigen::Vector3d(collision_shelf_transform_doubles[0], collision_shelf_transform_doubles[1],
                      collision_shelf_transform_doubles[2]);
  Eigen::Vector3d rotation =
      Eigen::Vector3d(collision_shelf_transform_doubles[3], collision_shelf_transform_doubles[4],
                      collision_shelf_transform_doubles[5]);

  // construct world pose to camera pose (pose Lu Ma took as origin when constructing the model)
  computer_vision_shelf_pose *= Eigen::AngleAxisd(rotation[2], Eigen::Vector3d::UnitZ()) *
                                Eigen::AngleAxisd(rotation[1], Eigen::Vector3d::UnitY()) *
                                Eigen::AngleAxisd(rotation[0], Eigen::Vector3d::UnitX());
  computer_vision_shelf_pose.translation() = translation;

  // convert to ros frame (mesh is saved in computer vision frame)
  collision_shelf_transform_ = computer_vision_shelf_pose *
                               Eigen::AngleAxisd(-M_PI / 2.0, Eigen::Vector3d::UnitX()) *
                               Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitY());

  // Add padding for not grazing shelf all the time
  collision_shelf_transform_.translation().x() -= collision_shelf_transform_x_offset;

  // computer vision shelf
  if (use_computer_vision_shelf_)
  {
    // visuals_->visual_tools_->publishAxisLabeled(collision_shelf_transform_,"CV_FRAME");

    computer_vision_shelf_->setCentroid(collision_shelf_transform_);
    computer_vision_shelf_->setMeshCentroid(collision_shelf_transform_);

    computer_vision_shelf_->setHighResMeshPath("file://" + package_path +
                                               "/meshes/computer_vision/shelf.stl");
    computer_vision_shelf_->setCollisionMeshPath("file://" + package_path +
                                                 "/meshes/computer_vision/shelf.stl");
  }

  return true;
}

// -------------------------------------------------------------------------------------------------
// Product Object
// -------------------------------------------------------------------------------------------------
ProductObject::ProductObject(VisualsPtr visuals, const rvt::colors &color, const std::string &name,
                             const std::string &package_path)
  : MeshObject(visuals, color, name)
{
  // Prepend the collision object type to the collision object name
  collision_object_name_ = "product_" + collision_object_name_;

  // Cache the object's mesh
  high_res_mesh_path_ = "file://" + package_path + "/meshes/products/" + name_ + "/recommended.dae";
  collision_mesh_path_ = "file://" + package_path + "/meshes/products/" + name_ + "/collision.stl";

  // Debug
  ROS_DEBUG_STREAM_NAMED("shelf", "Creating collision product with name "
                                      << collision_object_name_
                                      << " from mesh: " << high_res_mesh_path_
                                      << "\n Collision mesh: " << collision_mesh_path_);
}

ProductObject::ProductObject(const ProductObject &copy)
  : MeshObject(copy)
{
}

Eigen::Affine3d ProductObject::getWorldPose(const ShelfObjectPtr &shelf, const BinObjectPtr &bin)
{
  return shelf->getBottomRight() * bin->getBottomRight() * getCentroid();
}

}  // namespace
