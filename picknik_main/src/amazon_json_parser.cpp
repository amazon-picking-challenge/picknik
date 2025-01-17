/*********************************************************************
 * Software License Agreement
 *
 *  Copyright (c) 2015, Dave Coleman <dave@dav.ee>
 *  All rights reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *********************************************************************/
/*
  Author: Dave Coleman
  Desc:   Parses a JSON file into inventory and orders for Amazon
*/

// ROS
#include <picknik_main/amazon_json_parser.h>

#include <fstream>

namespace picknik_main
{
AmazonJSONParser::AmazonJSONParser(bool verbose, VisualsPtr visuals)
  : verbose_(verbose)
  , visuals_(visuals)
{
}

AmazonJSONParser::~AmazonJSONParser() {}
bool AmazonJSONParser::parse(const std::string& file_path, const std::string& package_path,
                             ShelfObjectPtr shelf, WorkOrders& orders)
{
  std::ifstream input_stream(file_path.c_str());

  if (!input_stream.good())
  {
    ROS_ERROR_STREAM_NAMED("parser", "Unable to load file " << file_path);
    return false;
  }

  Json::Value root;  // will contains the root value after parsing
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(input_stream, root);
  if (!parsingSuccessful)
  {
    // report to the user the failure and their locations in the document.
    std::cout << "Failed to parse configuration\n" << reader.getFormattedErrorMessages();
    return false;
  }

  // Parse bin contents
  const Json::Value bin_contents = root["bin_contents"];
  if (!bin_contents)
  {
    ROS_ERROR_STREAM_NAMED("parser", "Unable to find json element 'bin_contents'");
    return false;
  }
  if (!parseBins(package_path, bin_contents, shelf))
  {
    return false;
  }

  // Parse work order
  const Json::Value work_orders = root["work_order"];
  if (!work_orders)
  {
    ROS_ERROR_STREAM_NAMED("parser", "Unable to find json element 'work_order'");
    return false;
  }
  if (!work_orders.isArray())
  {
    ROS_ERROR_STREAM_NAMED("parser", "work_order not an array");
    return false;
  }
  if (!parseWorkOrders(work_orders, shelf, orders))
  {
    return false;
  }

  return true;
}

bool AmazonJSONParser::parseBins(const std::string& package_path, const Json::Value bin_contents,
                                 ShelfObjectPtr shelf)
{
  const Json::Value::Members bin_names = bin_contents.getMemberNames();
  Eigen::Affine3d bottom_right;
  Eigen::Affine3d top_left;
  for (Json::Value::Members::const_iterator bin_it = bin_names.begin(); bin_it != bin_names.end();
       bin_it++)
  {
    const std::string& bin_name = *bin_it;
    if (verbose_)
      ROS_DEBUG_STREAM_NAMED("parser", "Loaded: " << bin_name);

    const Json::Value& bin = bin_contents[bin_name];

    // double bin_y_space = 0.08; // where to place objects

    // Get each product
    for (std::size_t index = 0; index < bin.size(); ++index)
    {
      const std::string& product_name = bin[int(index)].asString();
      if (verbose_)
        ROS_DEBUG_STREAM_NAMED("parser", "   product: " << product_name);

      // Add object to a bin
      ProductObjectPtr product(new ProductObject(visuals_, rvt::RAND, product_name, package_path));

      // Add product to correct location
      shelf->getBins()[bin_name]->getProducts().push_back(product);
    }
  }

  return true;
}

bool AmazonJSONParser::parseWorkOrders(const Json::Value work_orders, ShelfObjectPtr shelf,
                                       WorkOrders& orders)
{
  // Get each work order
  for (std::size_t work_id = 0; work_id < work_orders.size(); ++work_id)
  {
    const Json::Value& work_order = work_orders[int(work_id)];  // is an object

    if (!work_order.isObject())
    {
      ROS_ERROR_STREAM_NAMED("parser", "work_order not an object");
      return false;
    }

    const std::string bin_name = work_order["bin"].asString();
    const std::string product_name = work_order["item"].asString();

    if (verbose_)
      ROS_DEBUG_STREAM_NAMED("parser", "Creating work order with product "
                                           << product_name << " in bin " << bin_name);

    // Find ptr to the bin
    BinObjectPtr bin = shelf->getBins()[bin_name];

    if (!bin)
    {
      ROS_ERROR_STREAM_NAMED("parser", "Unable to find bin named " << bin_name);
      return false;
    }

    // Find ptr to the product
    ProductObjectPtr product = bin->getProduct(product_name);

    if (!product)
    {
      ROS_ERROR_STREAM_NAMED("parser", "Unable to find product named " << product_name);
      return false;
    }

    // Set the order
    WorkOrder order(bin, product);

    // Add to the whole order
    orders.push_back(order);
  }

  return true;
}

}  // namespace
