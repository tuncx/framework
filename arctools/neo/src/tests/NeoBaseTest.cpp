﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2023 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* NeoBaseTest.cpp                                 (C) 2000-2023             */
/*                                                                           */
/* Base tests for Neo kernel                                                 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include <vector>
#include <array>
#include <algorithm>

#include "gtest/gtest.h"
#include "neo/Neo.h"
#include "neo/MeshKernel.h"

TEST(NeoUtils, test_array_view) {
  std::vector<int> vec{ 0, 1, 2 };
  // build a view from a vector
  Neo::utils::Span<int> view{ vec.data(), vec.size()  };
  Neo::utils::ConstSpan<int> constview{vec.data(), vec.size()};
  EXPECT_TRUE(std::equal(view.begin(), view.end(), vec.begin()));
  EXPECT_TRUE(std::equal(constview.begin(), constview.end(), vec.begin()));
  // build a vector from a view
  std::vector<int> vec2 = view.copy();
  std::vector<int> vec3(view.copy());
  std::vector<int> vec4(constview.copy());
  EXPECT_TRUE(std::equal(vec2.begin(), vec2.end(), view.begin()));
  EXPECT_TRUE(std::equal(vec3.begin(), vec3.end(), view.begin()));
  EXPECT_TRUE(std::equal(vec4.begin(), vec4.end(), constview.begin()));
  std::vector<int> dim2_vec{ 0, 1, 2, 3, 4, 5 };
  // build a dim2 view from vector
  auto dim1_size = 2;
  auto dim2_size = 3;
  Neo::utils::Span2<int> dim2_view{ dim2_vec.data(), dim1_size, dim2_size };
  Neo::utils::ConstSpan2<int> dim2_const_view{ dim2_vec.data(), dim2_size, dim1_size };
  for (auto i = 0; i < dim1_size; ++i) {
    for (auto j = 0; j < dim2_size; ++j) {
      EXPECT_EQ(dim2_view[i][j], dim2_vec[i * dim2_size + j]);
      EXPECT_EQ(dim2_const_view[j][i], dim2_vec[j * dim1_size + i]);
    }
  }
  // Copy all Array2View data into a 1D vector
  std::vector<int> dim2_view_vector_copy{ dim2_view.copy() };
  std::vector<int> dim2_const_view_vector_copy{ dim2_const_view.copy() };
  EXPECT_TRUE(std::equal(dim2_vec.begin(), dim2_vec.end(), dim2_const_view_vector_copy.begin()));
  // Try out of bound error
#ifndef _MS_REL_ // if constepxr still experiencing problems with
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(view[4], ".*i < m_size.*");
  }
  if constexpr (_debug) {
    EXPECT_DEATH(dim2_view[0][4], ".*i < m_size.*");
  }
#endif
#endif
}

void _testItemLocalIds(Neo::utils::Int32 const& first_lid,
                       int const& nb_lids,
                       std::vector<Neo::utils::Int32> const& non_contiguous_lids = {}) {
  auto item_local_ids = Neo::ItemLocalIds{ non_contiguous_lids, first_lid, nb_lids };
  auto item_array = item_local_ids.itemArray();
  EXPECT_EQ(item_local_ids.size(), nb_lids + non_contiguous_lids.size());
  Neo::utils::printContainer(item_array, "ItemLocalIds");
  std::vector<Neo::utils::Int32> item_array_ref(nb_lids);
  item_array_ref.insert(item_array_ref.begin(), non_contiguous_lids.begin(),
                        non_contiguous_lids.end());
  std::iota(item_array_ref.begin() + non_contiguous_lids.size(), item_array_ref.end(), first_lid);
  Neo::utils::printContainer(item_array_ref, "ItemArrayRef");
  EXPECT_TRUE(std::equal(item_array.begin(), item_array.end(), item_array_ref.begin()));
  for (int i = 0; i < item_local_ids.size(); ++i) {
    EXPECT_EQ(item_local_ids(i), item_array_ref[i]);
  }
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestItemLocalIds, test_item_local_ids) {
  _testItemLocalIds(0, 10);
  _testItemLocalIds(5, 10);
  _testItemLocalIds(0, 0, { 1, 3, 5, 9 });
  _testItemLocalIds(0, 10, { 1, 3, 5, 9 });
  _testItemLocalIds(5, 10, { 1, 3, 5, 9 });
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestItemRange, test_item_range) {
  // Test with only contiguous local ids
  std::cout << "== Testing contiguous item range from 0 with 5 items ==" << std::endl;
  auto nb_item = 5;
  auto ir = Neo::ItemRange{ Neo::ItemLocalIds{ {}, 0, nb_item } };
  EXPECT_EQ(ir.size(), nb_item);
  EXPECT_EQ(ir.maxLocalId(), nb_item - 1);
  std::vector<Neo::utils::Int32> local_ids;
  for (auto item : ir) {
    std::cout << "item lid " << item << std::endl;
    local_ids.push_back(item);
  }
  auto local_ids_stored = ir.localIds();
  std::cout << local_ids_stored << std::endl;
  EXPECT_TRUE(std::equal(local_ids_stored.begin(), local_ids_stored.end(), local_ids.begin()));
  local_ids.clear();
  // Test with only non contiguous local ids
  std::cout << "== Testing non contiguous item range {3,5,7} ==" << std::endl;
  std::vector non_contiguous_lids = { 3, 5, 7 };
  ir = Neo::ItemRange{ Neo::ItemLocalIds{ non_contiguous_lids, 0, 0 } };
  EXPECT_EQ(ir.size(), non_contiguous_lids.size());
  EXPECT_EQ(ir.maxLocalId(), *std::max_element(non_contiguous_lids.begin(), non_contiguous_lids.end()));
  for (auto item : ir) {
    std::cout << "item lid " << item << std::endl;
    local_ids.push_back(item);
  }
  local_ids_stored = ir.localIds();
  std::cout << local_ids_stored << std::endl;
  EXPECT_TRUE(std::equal(local_ids_stored.begin(), local_ids_stored.end(), local_ids.begin()));
  local_ids.clear();
  // Test range mixing contiguous and non contiguous local ids
  std::cout << "== Testing non contiguous item range {3,5,7} + 8 to 11 ==" << std::endl;
  auto nb_contiguous_lids = 4;
  auto first_contiguous_lid = 8;
  ir = Neo::ItemRange{ Neo::ItemLocalIds{ non_contiguous_lids, first_contiguous_lid, nb_contiguous_lids } };
  EXPECT_EQ(ir.size(), non_contiguous_lids.size() + nb_contiguous_lids);
  EXPECT_EQ(ir.maxLocalId(), first_contiguous_lid + nb_contiguous_lids - 1);
  for (auto item : ir) {
    std::cout << "item lid " << item << std::endl;
    local_ids.push_back(item);
  }
  local_ids_stored = ir.localIds();
  std::cout << local_ids_stored << std::endl;
  EXPECT_TRUE(std::equal(local_ids_stored.begin(), local_ids_stored.end(), local_ids.begin()));
  local_ids.clear();
  // Internal test for out of bound
  std::cout << "Get out of bound values (index > size) " << ir.m_item_lids(100) << std::endl;
  std::cout << "Get out of bound values (index < 0) " << ir.m_item_lids(-100) << std::endl;

  // todo test out reverse range
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestFutureItemRange, test_future_item_range) {
  Neo::FutureItemRange future_item_range{};
  // Manually fill contained ItemRange
  std::vector<Neo::utils::Int32> lids{ 0, 2, 4, 6 };
  future_item_range.new_items = Neo::ItemRange{ Neo::ItemLocalIds{ lids } };
  Neo::ItemRange& internal_range = future_item_range;
  EXPECT_EQ(&future_item_range.new_items, &internal_range);
  auto end_update = Neo::EndOfMeshUpdate{};
  {
    // declare a filtered range -- filtered by indexes
    std::vector<int> filter{ 0, 1, 2 };
    auto filtered_future_range =
    Neo::make_future_range(future_item_range, filter);
    // Get item_ranges
    auto filtered_range = filtered_future_range.get(end_update);
    auto item_range = future_item_range.get(end_update);
    // Check item ranges
    auto lids_in_range = item_range.localIds();
    EXPECT_TRUE(std::equal(lids.begin(), lids.end(), lids_in_range.begin()));
    EXPECT_THROW(future_item_range.get(end_update), std::runtime_error);

    std::vector<Neo::utils::Int32> filtered_lids = filtered_range.localIds();
    std::vector<Neo::utils::Int32> filtered_lids_ref;
    for (auto i : filter) {
      filtered_lids_ref.push_back(lids[i]);
    }
    EXPECT_EQ(filtered_lids.size(), filtered_lids_ref.size());
    EXPECT_TRUE(std::equal(filtered_lids_ref.begin(), filtered_lids_ref.end(),
                           filtered_lids.begin()));
  }
  Neo::FutureItemRange future_item_range2{};
  // Manually fill contained ItemRange
  future_item_range2.new_items = Neo::ItemRange{ Neo::ItemLocalIds{ lids } };
  {
    // declare a filtered range -- filtered by values (the filter is computed)
    std::vector<Neo::utils::Int32> value_subset{ 2, 6 };
    auto filtered_future_range = Neo::make_future_range(future_item_range2, lids, value_subset);
    // Get item_range
    auto filtered_range = filtered_future_range.get(end_update);
    auto filtered_range_lids = filtered_range.localIds();
    EXPECT_TRUE(std::equal(value_subset.begin(), value_subset.end(), filtered_range_lids.begin()));
  }
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestProperty, test_scalar_property) {
  Neo::ScalarPropertyT<Neo::utils::Int32> scalar_property{ "test_scalar_property", -1 };
  EXPECT_EQ(scalar_property.name(), "test_scalar_property");
  EXPECT_EQ(scalar_property.get(), -1);
  scalar_property.set(42);
  EXPECT_EQ(42, scalar_property.get());
  auto& const_scalar_property = scalar_property;
  EXPECT_EQ(42, const_scalar_property.get());
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestProperty, test_array_property) {
  Neo::ArrayPropertyT<Neo::utils::Int32> array_property{ "test_array_property" };
  EXPECT_EQ(array_property.name(), "test_array_property");
  array_property.reserve(10);
  EXPECT_EQ(array_property.size(), 0);
  array_property.resize(10);
  EXPECT_EQ(array_property.size(), 10);
  for (auto i = 0; i < 10; ++i) {
    array_property[i] = i;
  }
  std::vector<int> ref_values = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  array_property.debugPrint();
  EXPECT_TRUE(std::equal(array_property.begin(), array_property.end(), ref_values.begin()));
  EXPECT_EQ(array_property.back(), 9);
  auto index = 0;
  for (auto& ref_value : ref_values) {
    EXPECT_EQ(ref_value, array_property[index++]);
  }
  array_property.clear();
  for (auto i = 0; i < 10; ++i) {
    array_property.push_back(i);
  }
  EXPECT_EQ(array_property.size(), 10);
  EXPECT_TRUE(std::equal(array_property.begin(), array_property.end(), ref_values.begin()));
  EXPECT_EQ(array_property.back(), 9);
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestProperty, test_array_property_views) {
  Neo::ArrayPropertyT<Neo::utils::Int32> array_property{ "test_array_property_views" };
  array_property.init({ 0, 1, 2, 3, 4 });
  auto array_property_view = array_property.view();
  EXPECT_TRUE(std::equal(array_property_view.begin(), array_property_view.end(), array_property.begin()));
  auto array_property_const_view = array_property.constView();
  EXPECT_TRUE(std::equal(array_property_const_view.begin(), array_property_const_view.end(), array_property.begin()));
  array_property_view = array_property.subView(2, 3);
  std::vector<Neo::utils::Int32> subview_ref_values{ 2, 3, 4 };
  EXPECT_TRUE(std::equal(array_property_view.begin(), array_property_view.end(), subview_ref_values.begin()));
  // check invalid cases
  array_property_view = array_property.subView(10, 2);
  EXPECT_EQ(array_property_view.size(), 0);
  array_property_view = array_property.subView(0, 10);
  EXPECT_TRUE(std::equal(array_property_view.begin(), array_property_view.end(), array_property.begin()));
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestProperty, test_mesh_scalar_property) {
  Neo::MeshScalarPropertyT<Neo::utils::Int32> property{ "test_mesh_scalar_property" };
  EXPECT_EQ(property.name(), "test_mesh_scalar_property");
  std::vector<Neo::utils::Int32> values{ 1, 2, 3 };
  Neo::ItemRange item_range{ Neo::ItemLocalIds{ {}, 0, 3 } };
  EXPECT_TRUE(property.isInitializableFrom(item_range));
  property.init(item_range, { 1, 2, 3 });
  // Cannot init twice : test assertion
#ifndef _MS_REL_ // if constepxr still experiencing problems with MSVC
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(property.init(item_range, values), ".*Property must be empty.*");
  }
#endif
#endif
  EXPECT_EQ(values.size(), property.size());
  auto prop_values = property.values();
  EXPECT_TRUE(std::equal(prop_values.begin(), prop_values.end(), values.begin()));
  std::vector<Neo::utils::Int32> new_values{ 4, 5, 6 };
  Neo::ItemRange new_item_range{ Neo::ItemLocalIds{ {}, 3, 3 } };
  property.append(new_item_range, new_values);
  property.debugPrint();
  EXPECT_EQ(values.size() + new_values.size(), property.size());
  auto all_values{ values };
  std::copy(new_values.begin(), new_values.end(), std::back_inserter(all_values));
  auto property_values = property.values();
  for (std::size_t i = 0; i < all_values.size(); ++i) {
    EXPECT_EQ(property_values[i], all_values[i]);
  }
  // test for range loop
  auto i = 0;
  for (auto val : property) {
    EXPECT_EQ(val, all_values[i++]);
  }
  // test operator[] (4 versions)
  i = 0;
  for (auto item : item_range) {
    EXPECT_EQ(property[item], values[i++]);
  }
  i = 0;
  const auto& const_property = property;
  for (const auto& item : new_item_range) {
    EXPECT_EQ(property[item], new_values[i++]);
  }
  // check operator[item_lids] on a lids array. Extract lids and values
  std::vector<int> item_indexes{ 0, 3, 4, 5 };
  auto local_ids = item_range.localIds();
  auto lids_new_range = new_item_range.localIds();
  std::copy(lids_new_range.begin(), lids_new_range.end(),
            std::back_inserter(local_ids));
  std::vector<Neo::utils::Int32> extracted_values_ref;
  std::transform(item_indexes.begin(), item_indexes.end(), std::back_inserter(extracted_values_ref),
                 [&all_values](auto index) { return all_values[index]; });
  std::vector<Neo::utils::Int32> extracted_lids;
  std::transform(item_indexes.begin(), item_indexes.end(), std::back_inserter(extracted_lids),
                 [&local_ids](auto index) { return local_ids[index]; });
  auto extracted_values = property[extracted_lids];
  std::cout << "extracted_values " << extracted_values << std::endl;
  std::cout << "extracted_values_ref " << extracted_values_ref << std::endl;
  EXPECT_TRUE(std::equal(extracted_values.begin(), extracted_values.end(), extracted_values_ref.begin()));
  // todo check throw if lids out of bound
#ifndef _MS_REL_ // if constepxr still experiencing problems with MSVC
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(property[1000], ".*Item local id must be < max local id.*");
  }
  if constexpr (_debug) {
    EXPECT_DEATH(const_property[1000], ".*Item local id must be < max local id.*");
  }
  extracted_lids = { 100, 1000, 1000000 };
  if constexpr (_debug) {
    EXPECT_DEATH(property[extracted_lids], ".*Max input item lid.*");
  }
#endif
#endif
  // Check append with holes, contiguous range
  item_range = { Neo::ItemLocalIds{ {}, 8, 2 } };
  values = { 8, 9 };
  property.append(item_range, values, Neo::utils::NULL_ITEM_LID);
  property.debugPrint();
  extracted_values = property[{ 8, 9 }];
  EXPECT_TRUE(std::equal(values.begin(), values.end(), extracted_values.begin()));
  std::array null_ids = { Neo::utils::NULL_ITEM_LID, Neo::utils::NULL_ITEM_LID };
  auto extracted_null_ids = property[{ 6, 7 }];
  EXPECT_TRUE(std::equal(null_ids.begin(), null_ids.end(), extracted_null_ids.begin()));
  // Check append in empty property contiguous range not starting at 0
  Neo::MeshScalarPropertyT<Neo::utils::Int32> property2{ "test_property2" };
  item_range = { Neo::ItemLocalIds{ {}, 2, 3 } };
  values = { 2, 3, 4 };
  property2.append(item_range, values, Neo::utils::NULL_ITEM_LID);
  property2.debugPrint();
  extracted_values = property2[{ 2, 3, 4 }];
  EXPECT_TRUE(std::equal(values.begin(), values.end(), extracted_values.begin()));
  extracted_null_ids = property2[std::vector<int>{ 0, 1 }];
  EXPECT_TRUE(std::equal(null_ids.begin(), null_ids.end(), extracted_null_ids.begin()));
  // Check append with holes, discontiguous range
  item_range = { Neo::ItemLocalIds{ {
                                    0,
                                    1,
                                    4,
                                    },
                                    0,
                                    0 } };
  values = { 0, 1, 8 };
  property2.append(item_range, values, Neo::utils::NULL_ITEM_LID);
  property2.debugPrint();
  extracted_values = property2[{ 0, 1, 4 }];
  EXPECT_TRUE(std::equal(values.begin(), values.end(), extracted_values.begin()));
  // Check append in empty property discontiguous range
  Neo::MeshScalarPropertyT<Neo::utils::Int32> property3{ "test_property3" };
  item_range = { Neo::ItemLocalIds{ { 1, 3, 5 }, 0, 0 } };
  values = { 1, 3, 5 };
  property3.append(item_range, values, Neo::utils::NULL_ITEM_LID);
  property3.debugPrint();
  extracted_values = property3[{ 1, 3, 5 }];
  EXPECT_TRUE(std::equal(values.begin(), values.end(), extracted_values.begin()));
  // Check append with holes, mixed range
  item_range = { Neo::ItemLocalIds{ { 4 }, 0, 2 } };
  values = { 10, 11, 18 };
  property2.append(item_range, values, Neo::utils::NULL_ITEM_LID);
  property2.debugPrint();
  extracted_values = property2[{ 4, 0, 1 }];
  EXPECT_TRUE(std::equal(values.begin(), values.end(), extracted_values.begin()));
  // Check append in empty property mixed range
  Neo::MeshScalarPropertyT<Neo::utils::Int32> property4{ "test_property3" };
  item_range = { Neo::ItemLocalIds{ { 1, 3, 5 }, 7, 3 } };
  values = { 1, 3, 5, 7, 8, 9 };
  property4.append(item_range, values, Neo::utils::NULL_ITEM_LID);
  property4.debugPrint();
  extracted_values = property4[{ 1, 3, 5, 7, 8, 9 }];
  EXPECT_TRUE(std::equal(values.begin(), values.end(), extracted_values.begin()));
  // Clear properties
  property.clear();
  EXPECT_EQ(property.size(), 0);
  EXPECT_EQ(property.values().size(), 0);
  // Possible to call init again after a clear
  item_range = { Neo::ItemLocalIds{ {}, 0, 3 } };
  values = { 1, 2, 3 };
  property.init(item_range, { 1, 2, 3 });
  EXPECT_EQ(values.size(), property.size());
  values = { 1, 2, 3 };
  prop_values = property.values();
  EXPECT_TRUE(std::equal(prop_values.begin(), prop_values.end(), values.begin()));
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestArrayProperty, test_mesh_array_property) {
  auto mesh_array_property = Neo::MeshArrayPropertyT<Neo::utils::Int32>{ "test_mesh_array_property" };
  EXPECT_EQ(mesh_array_property.name(), "test_mesh_array_property");
  // check assert (debug only)
#ifndef _MS_REL_ // if constepxr still experiencing problems with MSVC
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(mesh_array_property[Neo::utils::NULL_ITEM_LID], ".*Item local id.*");
  }
#endif
#endif
  // Check data allocation with resize (not to be used if init method is used)
  std::vector<int> sizes{ 1, 2, 3 };
  mesh_array_property.resize(sizes, true); // 3 elements with respectively 1, 2 and 3 values
  auto array_property_sizes = mesh_array_property.sizes();
  EXPECT_TRUE(std::equal(array_property_sizes.begin(), array_property_sizes.end(), sizes.begin()));
  EXPECT_EQ(mesh_array_property.size(), 3);
  EXPECT_EQ(mesh_array_property.cumulatedSize(), 6);
  mesh_array_property.clear();
  EXPECT_EQ(mesh_array_property.size(), 0);
  // add elements: 5 items with one value
  Neo::ItemRange item_range{ Neo::ItemLocalIds{ {}, 0, 5 } };
  std::vector<Neo::utils::Int32> values{ 0, 1, 2, 3, 4 };
  // Check cannot Try to init before resize
#ifndef _MS_REL_ // if constepxr still experiencing problems with MSVC
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(mesh_array_property.init(item_range, values), ".*call resize before init.*");
  }
#endif
#endif
  mesh_array_property.resize({ 1, 1, 1, 1, 1 });
  mesh_array_property.init(item_range, values);
  mesh_array_property.debugPrint();
  EXPECT_EQ(item_range.size(), mesh_array_property.size());
  EXPECT_EQ(values.size(), mesh_array_property.cumulatedSize());
  // check values
  auto index = 0;
  for (auto item : item_range) {
    auto item_array = mesh_array_property[item];
    for (auto item_value : item_array) {
      EXPECT_EQ(item_value, values[index++]);
    }
  }
  // check iterators
  EXPECT_TRUE(std::equal(mesh_array_property.begin(), mesh_array_property.end(), values.begin()));
  // check view
  auto property_1D_view = mesh_array_property.view();
  auto const property_1D_const_view = mesh_array_property.constView();
  EXPECT_TRUE(std::equal(property_1D_view.begin(), property_1D_view.end(), values.begin()));
  EXPECT_TRUE(std::equal(property_1D_const_view.begin(), property_1D_const_view.end(), values.begin()));
  // Add 3 items
  std::vector<int> nb_element_per_item{ 0, 3, 1 };
  Neo::ItemRange item_range2 = { Neo::ItemLocalIds{ { 5, 6, 7 } } };
  std::vector<Neo::utils::Int32> values_added{ 6, 6, 6, 7 };
  mesh_array_property.append(item_range2, values_added, nb_element_per_item);
  mesh_array_property.debugPrint(); // expected result: "0" "1" "2" "3" "4" "6" "6" "6" "7" (check with test framework)
  EXPECT_EQ(item_range.size()+item_range2.size(), mesh_array_property.size());
  EXPECT_EQ(values.size() + values_added.size(), mesh_array_property.cumulatedSize());
  std::vector<int> ref_values = { 0, 1, 2, 3, 4, 6, 6, 6, 7 };
  EXPECT_TRUE(std::equal(ref_values.begin(), ref_values.end(), mesh_array_property.begin()));
  // check values
  index = 0;
  for (auto item : item_range2) {
    auto item_array = mesh_array_property[item];
    for (auto item_value : item_array) {
      EXPECT_EQ(item_value, values_added[index++]);
    }
  }
  // Add three more items
  Neo::ItemRange item_range3 = { Neo::ItemLocalIds{ {}, 8, 3 } };
  std::for_each(values_added.begin(), values_added.end(), [](auto& elt) { return elt += 2; });
  mesh_array_property.append(item_range3, values_added, nb_element_per_item);
  mesh_array_property.debugPrint(); // expected result: "0" "1" "2" "3" "4" "6" "6" "6" "7" "8" "8" "8" "9"
  EXPECT_EQ(item_range.size()+item_range2.size()+item_range3.size(), mesh_array_property.size());
  EXPECT_EQ(values.size() + 2 * values_added.size(), mesh_array_property.cumulatedSize());
  ref_values = { 0, 1, 2, 3, 4, 6, 6, 6, 7, 8, 8, 8, 9 };
  EXPECT_TRUE(std::equal(ref_values.begin(), ref_values.end(), mesh_array_property.begin()));
  // check values
  index = 0;
  for (auto item : item_range3) {
    auto item_array = mesh_array_property[item];
    for (auto item_value : item_array) {
      EXPECT_EQ(item_value, values_added[index++]);
    }
  }
  // Add items and modify existing item
  item_range = { Neo::ItemLocalIds{ { 0, 8, 5 }, 11, 1 } };
  nb_element_per_item = { 3, 3, 2, 1 };
  values_added = { 10, 10, 10, 11, 11, 11, 12, 12, 13 };
  mesh_array_property.append(item_range, values_added, nb_element_per_item); // expected result: "10" "10" "10" "1" "2" "3" "4" "12" "12" "6" "6" "6" "7" "11" "11" "11" "8" "8" "8" "9" "13"
  mesh_array_property.debugPrint();
  EXPECT_EQ(21, mesh_array_property.cumulatedSize());
  ref_values = { 10, 10, 10, 1, 2, 3, 4, 12, 12, 6, 6, 6, 7, 11, 11, 11, 8, 8, 8, 9, 13 };
  EXPECT_TRUE(std::equal(ref_values.begin(), ref_values.end(), mesh_array_property.begin()));
  // check values
  index = 0;
  for (auto item : item_range) {
    auto item_array = mesh_array_property[item];
    for (auto item_value : item_array) {
      EXPECT_EQ(item_value, values_added[index++]);
    }
  }

  // Check add non-0-starting contiguous range in an empty array property
  auto array_property2 = Neo::MeshArrayPropertyT<Neo::utils::Int32>{ "test_array_property2" };
  item_range = { Neo::ItemLocalIds{ {}, 3, 4 } };
  values = { 3, 4, 4, 5, 6, 6 };
  array_property2.append(item_range, values, { 1, 2, 1, 2 });
  array_property2.debugPrint();
  std::vector<int> values_check;
  for (auto item : item_range) {
    for (auto value : array_property2[item])
      values_check.push_back(value);
  }
  std::cout << values_check << std::endl;
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));
  item_range = { Neo::ItemLocalIds{ {}, 0, 2 } };
  values = { 0, 1, 1 };
  array_property2.append(item_range, values, { 1, 2 });
  array_property2.debugPrint();
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property2[item])
      values_check.push_back(value);
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));
  // Check for the whole range
  item_range = { Neo::ItemLocalIds{ {}, 0, 7 } };
  values = { 0, 1, 1, 3, 4, 4, 5, 6, 6 };
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property2[item]) {
      values_check.push_back(value);
    }
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));

  // Check with existing property but insertion past the last element
  item_range = { Neo::ItemLocalIds{ {}, 8, 3 } }; // lids {8,9,10}
  values = { 8, 9, 9, 10 };
  array_property2.append(item_range, values, { 1, 2, 1 });
  array_property2.debugPrint();
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property2[item])
      values_check.push_back(value);
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));

  // Same two tests with discontiguous range
  auto array_property3 = Neo::MeshArrayPropertyT<Neo::utils::Int32>{ "test_array_property3" };
  item_range = { Neo::ItemLocalIds{ { 3, 5, 6 } } };
  values = { 3, 3, 5, 6, 6 };
  array_property3.append(item_range, values, { 2, 1, 2 });
  array_property3.debugPrint();
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property3[item])
      values_check.push_back(value);
  }
  std::cout << values_check << std::endl;
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));
  // Fill the first items
  item_range = { Neo::ItemLocalIds{ { 0, 2 } } };
  values = { 0, 2, 2 };
  array_property3.append(item_range, values, { 1, 2 });
  array_property3.debugPrint();
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property3[item])
      values_check.push_back(value);
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));
  // Check for the whole range
  item_range = { Neo::ItemLocalIds{ {}, 0, 7 } };
  values = { 0, 2, 2, 3, 3, 5, 6, 6 };
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property3[item]) {
      values_check.push_back(value);
    }
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));
  // Check with existing property but insertion past the last element
  item_range = { Neo::ItemLocalIds{ { 8, 10, 12 } } }; // lids {8,9,10}
  values = { 8, 10, 10, 12 };
  array_property3.append(item_range, values, { 1, 2, 1 });
  array_property3.debugPrint();
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property3[item])
      values_check.push_back(value);
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));
  // Check for the whole range
  item_range = { Neo::ItemLocalIds{ {}, 0, 13 } };
  values = { 0, 2, 2, 3, 3, 5, 6, 6, 8, 10, 10, 12 };
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property3[item]) {
      values_check.push_back(value);
    }
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));

  // Same two tests with mixed range
  auto array_property4 = Neo::MeshArrayPropertyT<Neo::utils::Int32>{ "test_array_property4" };
  item_range = { Neo::ItemLocalIds{ { 4, 6, 7 }, 8, 3 } };
  values = { 4, 4, 6, 7, 7, 8, 9, 10, 10 };
  array_property4.append(item_range, values, { 2, 1, 2, 1, 1, 2 });
  array_property4.debugPrint();
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property4[item])
      values_check.push_back(value);
  }
  std::cout << values_check << std::endl;
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));
  // Fill the first items
  item_range = { Neo::ItemLocalIds{ { 2, 3 }, 0, 2 } };
  values = { 2, 2, 3, 0, 0, 1 };
  array_property4.append(item_range, values, { 2, 1, 2, 1 });
  array_property4.debugPrint();
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property4[item])
      values_check.push_back(value);
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));
  // Check for the whole range
  item_range = { Neo::ItemLocalIds{ {}, 0, 11 } };
  values = { 0, 0, 1, 2, 2, 3, 4, 4, 6, 7, 7, 8, 9, 10, 10 };
  values_check.clear();
  for (auto item : item_range) {
    for (auto value : array_property4[item]) {
      values_check.push_back(value);
    }
  }
  EXPECT_TRUE(std::equal(values.begin(), values.end(), values_check.begin()));

  // Check clear method
  mesh_array_property.clear();
  EXPECT_EQ(mesh_array_property.size(), 0);
  // Since property cleared, an init can be called after a resize
  mesh_array_property.resize({ 1, 1, 1, 2, 2 });
  mesh_array_property.init(item_range, values);
  EXPECT_EQ(mesh_array_property.size(), 5);
  EXPECT_EQ(mesh_array_property.cumulatedSize(), 7);
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestArrayProperty, test_mesh_array_property_proxy) {
  Neo::MeshArrayPropertyT<Neo::utils::Int32> mesh_array_property{"mesh_array_property_test"};
  mesh_array_property.resize({1,2,3});
  auto item_range = Neo::ItemRange{ Neo::ItemLocalIds{ {}, 0, 3 } };
  mesh_array_property.init(item_range,{1,2,2,3,3,3});
  Neo::MeshArrayPropertyProxyT<Neo::utils::Int32> mesh_array_property_proxy{mesh_array_property};
  auto mesh_array_property_values = mesh_array_property.view();
  EXPECT_EQ(mesh_array_property_proxy.arrayPropertyData(),mesh_array_property_values.m_ptr);
  EXPECT_EQ(mesh_array_property.view().size(),mesh_array_property_proxy.arrayPropertyDataSize());
  EXPECT_EQ(mesh_array_property.sizes().size(),mesh_array_property_proxy.arrayPropertyOffsetsSize());
  EXPECT_EQ(mesh_array_property.size(),mesh_array_property_proxy.arrayPropertyIndexSize()); // todo must be ok !!
  auto mesh_array_property_sizes = mesh_array_property.sizes();
  EXPECT_TRUE(std::equal(mesh_array_property_sizes.begin(), mesh_array_property_sizes.end(), mesh_array_property_proxy.arrayPropertyOffsets()));
  auto const mesh_array_property_const_proxy{mesh_array_property_proxy};
  EXPECT_EQ(mesh_array_property_const_proxy.arrayPropertyOffsets(),mesh_array_property_sizes.m_ptr);
  return;
  auto property_values = mesh_array_property.view();
  auto property_data = mesh_array_property_proxy.arrayPropertyData();
  auto property_indexes = mesh_array_property_proxy.arrayPropertyIndex();
  auto property_offsets = mesh_array_property_proxy.arrayPropertyOffsets();
  auto property_index = 0;
  auto value_index = 0;
  auto item_index = 0;
  for (auto item : item_range){
    EXPECT_EQ(value_index,property_indexes[item_index]);
    value_index += mesh_array_property[item].size();
    item_index++;
  }
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestPropertyView, test_mesh_scalar_property_view) {
  Neo::MeshScalarPropertyT<Neo::utils::Int32> property{ "name" };
  std::vector<Neo::utils::Int32> values{ 1, 2, 3, 10, 100, 1000 };
  Neo::ItemRange item_range{ Neo::ItemLocalIds{ {}, 0, 6 } };
  property.init(item_range, values);
  auto property_view = property.view();
  EXPECT_EQ(property_view.size(), item_range.size());
  std::vector<Neo::utils::Int32> local_ids{ 1, 3, 5 };
  std::vector<Neo::utils::Int32> partial_values{ 2, 10, 1000 };
  auto partial_item_range = Neo::ItemRange{ Neo::ItemLocalIds{ local_ids } };
  auto partial_property_view = property.view(partial_item_range);
  EXPECT_EQ(partial_property_view.size(), partial_item_range.size());
  for (auto i = 0; i < item_range.size(); ++i) {
    std::cout << "prop values at index " << i << " " << property_view[i] << std::endl;
    EXPECT_EQ(property_view[i], values[i]);
  }
  EXPECT_TRUE(property_view.end() == property_view.end());
  auto beg = property_view.begin();
  for (auto i = 0; i < property_view.size(); ++i) {
    ++beg;
  }
  EXPECT_EQ(beg, property_view.end());
  for (auto value_iter = property_view.begin(); value_iter != property_view.end(); ++value_iter) {
    std::cout << " view value " << *value_iter << " " << std::endl;
  }
  auto index = 0;
  for (auto value : property_view) {
    EXPECT_EQ(value, property_view[index++]);
  }
  EXPECT_TRUE(std::equal(property_view.begin(), property_view.end(), values.begin()));
  for (auto i = 0; i < partial_item_range.size(); ++i) {
    std::cout << "prop values at index " << i << " " << partial_property_view[i] << std::endl;
    EXPECT_EQ(partial_property_view[i], partial_values[i]);
  }
  EXPECT_TRUE(std::equal(partial_property_view.begin(), partial_property_view.end(), partial_values.begin()));
  // Change values
  auto new_val = 50;
  property_view[2] = new_val;
  EXPECT_EQ(property[2], new_val);
  partial_property_view[2] = new_val;
  EXPECT_EQ(property[local_ids[2]], new_val);
  // Check out of bound
#ifndef _MS_REL_ // if constepxr still experiencing problems with MSVC
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(property_view[7], ".*Assertion.*");
  }
  if constexpr (_debug) {
    EXPECT_DEATH(partial_property_view[3], ".*Error, exceeds property view size.*");
  }
#endif
#endif
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestPropertyView, test_mesh_scalar_property_const_view) {
  Neo::MeshScalarPropertyT<Neo::utils::Int32> property{ "name" };
  std::vector<Neo::utils::Int32> values{ 1, 2, 3, 10, 100, 1000 };
  Neo::ItemRange item_range{ Neo::ItemLocalIds{ {}, 0, 6 } };
  property.init(item_range, values);
  auto property_const_view = property.constView();
  EXPECT_EQ(property_const_view.size(), item_range.size());
  auto partial_item_range = Neo::ItemRange{ Neo::ItemLocalIds{ { 1, 3, 5 } } };
  std::vector<Neo::utils::Int32> partial_values{ 2, 10, 1000 };
  auto partial_property_const_view = property.constView(partial_item_range);
  EXPECT_EQ(partial_property_const_view.size(), partial_item_range.size());
  for (auto i = 0; i < item_range.size(); ++i) {
    std::cout << "prop values at index " << i << " " << property_const_view[i] << std::endl;
    EXPECT_EQ(property_const_view[i], values[i]);
  }
  for (auto i = 0; i < partial_item_range.size(); ++i) {
    std::cout << "prop values at index " << i << " " << partial_property_const_view[i] << std::endl;
    EXPECT_EQ(partial_property_const_view[i], partial_values[i]);
  }
#ifndef _MS_REL_ // if constepxr still experiencing problems with MSVC
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(property_const_view[7], ".*Assertion.*");
  }
  if constexpr (_debug) {
    EXPECT_DEATH(partial_property_const_view[3], ".*Error, exceeds property view size.*");
  }
#endif
#endif
  // test const iterator
  EXPECT_TRUE(property_const_view.end() == property_const_view.end());
  auto beg = property_const_view.begin();
  for (auto i = 0; i < property_const_view.size(); ++i) {
    ++beg;
  }
  EXPECT_EQ(beg, property_const_view.end());
  for (auto value_iter = property_const_view.begin(); value_iter != property_const_view.end(); ++value_iter) {
    std::cout << " view value " << *value_iter << " " << std::endl;
  }
  auto index = 0;
  for (auto value : property_const_view) {
    EXPECT_EQ(value, property_const_view[index++]);
  }
  EXPECT_TRUE(std::equal(property_const_view.begin(), property_const_view.end(), values.begin()));
  EXPECT_TRUE(std::equal(partial_property_const_view.begin(), partial_property_const_view.end(), partial_values.begin()));
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestPropertyView, test_mesh_array_property_view) {
  auto mesh_array_property = Neo::MeshArrayPropertyT<Neo::utils::Int32>{ "test_mesh_array_property_view" };
  // add elements: 5 items with one value
  Neo::ItemRange item_range{ Neo::ItemLocalIds{ {}, 0, 5 } };
  std::vector<int> sizes{ 1, 2, 3, 4, 5 };
  mesh_array_property.resize(sizes, true);
  auto i = 0;
  for (auto item : item_range) {
    auto item_values = mesh_array_property[item];
    std::fill(item_values.begin(), item_values.end(), i);
    i++;
  }
  // create a view on the 3 first elements
  std::vector<Neo::utils::Int32> sub_range_ref_values{ 0, 1, 1 };
  Neo::ItemRange item_sub_range{ Neo::ItemLocalIds{ {}, 0, 2 } };
  auto mesh_array_property_view = mesh_array_property.view(item_sub_range);
  // check values
  auto item_index = 0;
  auto value_index = 0;
  for (auto item : item_sub_range) {
    auto item_array = mesh_array_property_view[item];
    EXPECT_EQ(item_array.size(), sizes[item_index]);
    item_index++;
    for (auto item_value : item_array) {
      EXPECT_EQ(item_value, sub_range_ref_values[value_index++]);
    }
  }

  // check assert (debug only)
#ifndef _MS_REL_ // if constepxr still experiencing problems with MSVC
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(mesh_array_property_view[Neo::utils::NULL_ITEM_LID], ".*index must be >0*");
    EXPECT_DEATH(mesh_array_property_view[mesh_array_property_view.size()], ".*Error, exceeds property view*");
  }
#endif
#endif
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestPropertyView, test_mesh_array_property_const_view) {
  auto mesh_array_property = Neo::MeshArrayPropertyT<Neo::utils::Int32>{ "test_mesh_array_property_view" };
  // add elements: 5 items with one value
  Neo::ItemRange item_range{ Neo::ItemLocalIds{ {}, 0, 5 } };
  mesh_array_property.resize({ 1, 2, 3, 4, 5 }, true);
  auto i = 0;
  for (auto item : item_range) {
    auto item_values = mesh_array_property[item];
    std::fill(item_values.begin(), item_values.end(), i);
    i++;
  }
  // create a view on the 3 first elements
  std::vector<Neo::utils::Int32> sub_range_ref_values{ 0, 1, 1 };
  Neo::ItemRange item_sub_range{ Neo::ItemLocalIds{ {}, 0, 2 } };
  auto mesh_array_property_view = mesh_array_property.constView(item_sub_range);
  // check values
  auto index = 0;
  for (auto item : item_sub_range) {
    auto item_array = mesh_array_property_view[item];
    for (auto item_value : item_array) {
      EXPECT_EQ(item_value, sub_range_ref_values[index++]);
    }
  }

  // check assert (debug only)
#ifndef _MS_REL_ // if constepxr still experiencing problems with MSVC
#ifdef USE_GTEST_DEATH_TEST
  if constexpr (_debug) {
    EXPECT_DEATH(mesh_array_property_view[Neo::utils::NULL_ITEM_LID], ".*index must be >0*");
    EXPECT_DEATH(mesh_array_property_view[mesh_array_property_view.size()], ".*Error, exceeds property view*");
  }
#endif
#endif
}
/*-----------------------------------------------------------------------------*/

TEST(NeoTestPropertyView, test_property_iterator) {
  std::vector data{ 1, 2, 3, 4, 5, 6, 7 };
  std::vector indexes{ 0, 3, 6 };
  Neo::PropertyViewIterator<int> property_view_iterator{ indexes, indexes.begin(), data.data() };
  // right operator
  for (auto index : indexes) {
    EXPECT_EQ(*property_view_iterator, data[index]);
    property_view_iterator++;
  }
  --property_view_iterator;
  for (auto rindex_iterator = indexes.rbegin(); rindex_iterator != indexes.rend(); ++rindex_iterator) {
    EXPECT_EQ(*property_view_iterator, data[*rindex_iterator]);
    std::cout << *property_view_iterator << " ";
    property_view_iterator--;
  }
  // left operator
  for (auto index : indexes) {
    EXPECT_EQ(*property_view_iterator, data[index]);
    ++property_view_iterator;
  }
  property_view_iterator--;
  for (auto rindex_iterator = indexes.rbegin(); rindex_iterator != indexes.rend(); ++rindex_iterator) {
    EXPECT_EQ(*property_view_iterator, data[*rindex_iterator]);
    --property_view_iterator;
  }
  EXPECT_EQ(*(property_view_iterator + 2), data[indexes[2]]);
  EXPECT_EQ(*(property_view_iterator - 2), data[indexes[0]]);
  EXPECT_EQ(*(property_view_iterator += 2), data[indexes[2]]);
  EXPECT_EQ(*(property_view_iterator -= 2), data[indexes[0]]);

  auto property_view_iterator2{ property_view_iterator };
  EXPECT_TRUE(property_view_iterator == property_view_iterator2);
  EXPECT_FALSE(property_view_iterator != property_view_iterator2);

  // check operator->
  std::vector<std::string> data_string{ "hello", "world", "!" };
  indexes = { 0, 1 };
  Neo::PropertyViewIterator<std::string> property_view_iterator3{ indexes, indexes.begin(), data_string.data() };
  EXPECT_EQ(property_view_iterator3->size(), data_string[0].size());
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestPropertyGraph, test_property_graph_info) {
  std::cout << "Test Property Graph" << std::endl;
  Neo::MeshKernel::AlgorithmPropertyGraph mesh{ "test" };

  // Add a family : property always belong to a family
  Neo::Family cell_family{ Neo::ItemKind::IK_Cell, "cells" };

  // Add a consuming/producing algo
  mesh.addAlgorithm(Neo::MeshKernel::InProperty{ cell_family, "in_property" }, Neo::MeshKernel::OutProperty{ cell_family, "out_property" }, []() {});
  mesh.addAlgorithm(Neo::MeshKernel::InProperty{ cell_family, "in_property" }, Neo::MeshKernel::InProperty{ cell_family, "in_property2" },
                    Neo::MeshKernel::OutProperty{ cell_family, "out_property" }, []() {});
  mesh.addAlgorithm(Neo::MeshKernel::InProperty{ cell_family, "in_property2" }, Neo::MeshKernel::InProperty{ cell_family, "in_property2" },
                    Neo::MeshKernel::OutProperty{ cell_family, "out_property2" }, []() {});

  // add producing algos
  mesh.addAlgorithm(Neo::MeshKernel::OutProperty{ cell_family, "out_property2" }, []() {});
  mesh.addAlgorithm(Neo::MeshKernel::OutProperty{ cell_family, "out_property2" },
                    Neo::MeshKernel::OutProperty{ cell_family, "out_property3" }, []() {});

  mesh.addAlgorithm(Neo::MeshKernel::InProperty{ cell_family, "in_property" },
                    Neo::MeshKernel::InProperty{ cell_family, "in_property2" },
                    Neo::MeshKernel::OutProperty{ cell_family, "out_property2" },
                    []() {});

  // Check number of consuming algo
  auto nb_algo_consuming_in_property = 3;
  EXPECT_EQ(nb_algo_consuming_in_property, mesh.m_property_algorithms.find(Neo::MeshKernel::InProperty{ cell_family, "in_property" })->second.second.size());

  auto nb_algo_consuming_in_property2 = 4;
  EXPECT_EQ(nb_algo_consuming_in_property2, mesh.m_property_algorithms.find(Neo::MeshKernel::InProperty{ cell_family, "in_property2" })->second.second.size());

  // check number of producing algos
  auto nb_algo_producing_out_property = 2;
  EXPECT_EQ(nb_algo_producing_out_property, mesh.m_property_algorithms.find(Neo::MeshKernel::OutProperty{ cell_family, "out_property" })->second.first.size());
  auto nb_algo_producing_out_property2 = 4;
  EXPECT_EQ(nb_algo_producing_out_property2, mesh.m_property_algorithms.find(Neo::MeshKernel::OutProperty{ cell_family, "out_property2" })->second.first.size());
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestLidsProperty, test_lids_property) {
  std::cout << "Test lids_range Property" << std::endl;
  auto lid_prop = Neo::ItemLidsProperty{ "test_property" };
  EXPECT_EQ(lid_prop.name(), "test_property");
  // Check empty property
  auto empty_range = lid_prop.values();
  EXPECT_EQ(empty_range.size(), 0);
  std::vector<Neo::utils::Int64> uids{ 1, 2, 3, 4, 5 };
  auto nb_item = uids.size();
  // Checking append
  auto added_item_range = lid_prop.append(uids);
  lid_prop.debugPrint();
  EXPECT_EQ(uids.size(), lid_prop.size());
  auto i = 0;
  auto added_local_ids = lid_prop[uids];
  auto added_local_ids_ref = added_item_range.localIds();
  EXPECT_TRUE(std::equal(added_local_ids.begin(), added_local_ids.end(), added_local_ids_ref.begin()));
  for (auto item : added_item_range) {
    std::cout << " uid " << uids[i++] << " lid " << item << std::endl;
  }
  // Check values function
  auto lids_range = lid_prop.values();
  EXPECT_EQ(lids_range.size(), uids.size());
  EXPECT_TRUE(std::equal(lids_range.begin(), lids_range.end(), added_local_ids_ref.begin()));
  // Checking append with duplicates
  uids = { 6, 7, 7, 8, 1, 5, 9 };
  auto one_lid = lid_prop[{ 1 }]; // store lid of uid =1 must not change
  auto five_lid = lid_prop[{ 5 }]; // store lid of uid =5 must not change
  auto nb_duplicates = 3;
  nb_item += uids.size() - nb_duplicates;
  added_item_range = lid_prop.append(uids);
  lid_prop.debugPrint();
  i = 0;
  for (auto item : added_item_range) {
    std::cout << " uid " << uids[i++] << " lid " << item << std::endl;
  }
  added_local_ids = lid_prop[uids];
  added_local_ids_ref = added_item_range.localIds();
  EXPECT_TRUE(std::equal(added_local_ids.begin(), added_local_ids.end(), added_local_ids_ref.begin()));
  EXPECT_EQ(one_lid, lid_prop[{ 1 }]);
  EXPECT_EQ(five_lid, lid_prop[{ 5 }]);
  // Checking remove
  auto removed_uids = std::vector<Neo::utils::Int64>{ 1, 3, 5, 9 };
  auto removed_lids_ref = lid_prop[removed_uids];
  auto removed_lids = lid_prop.remove(removed_uids);
  nb_item -= removed_uids.size();
  EXPECT_TRUE(std::equal(removed_lids_ref.begin(), removed_lids_ref.end(), removed_lids.begin()));
  for (auto lid : removed_lids) {
    std::cout << "removed lids_range: " << lid << std::endl;
  }
  // Checking value function
  for (auto item : lid_prop.values()) {
    std::cout << "Item range, lid " << item << std::endl;
  }
  EXPECT_EQ(lid_prop.values().size(), lid_prop.size());
  EXPECT_EQ(lid_prop.values().size(), nb_item);
  std::vector<Neo::utils::Int64> remaining_uids{ 2, 4, 6, 7, 8 };
  auto lids_ref = lid_prop[remaining_uids];
  auto lids = lid_prop.values().localIds();
  EXPECT_TRUE(std::equal(lids_ref.begin(), lids_ref.end(), lids.begin()));
  lid_prop.debugPrint();
  // Check re-add removed items
  std::vector<Neo::utils::Int64> added_uids(removed_uids);
  auto added_items = lid_prop.append(removed_uids);
  nb_item += removed_lids.size();
  lid_prop.debugPrint();
  EXPECT_EQ(added_items.size(), removed_uids.size());
  EXPECT_EQ(std::count(added_items.begin(), added_items.end(), Neo::utils::NULL_ITEM_LID), 0);
  auto added_lids = added_items.localIds();
  auto added_lids_ref = lid_prop[added_uids];
  EXPECT_TRUE(std::equal(added_lids.begin(), added_lids.end(), added_lids_ref.begin()));
  //Check add new items
  added_uids = { 10, 11, 12 };
  added_items = lid_prop.append(added_uids);
  nb_item += added_items.size();
  lid_prop.debugPrint();
  EXPECT_EQ(added_items.size(), 3);
  EXPECT_EQ(std::count(added_items.begin(), added_items.end(), Neo::utils::NULL_ITEM_LID), 0);
  added_lids = added_items.localIds();
  added_lids_ref = lid_prop[added_uids];
  EXPECT_TRUE(std::equal(added_lids.begin(), added_lids.end(), added_lids_ref.begin()));
  // Checking value function
  for (auto item : lid_prop.values()) {
    std::cout << "Item range, lid " << item << std::endl;
  }
  EXPECT_EQ(lid_prop.values().size(), lid_prop.size());
  EXPECT_EQ(lid_prop.values().size(), nb_item);
  remaining_uids = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
  lids_ref = lid_prop[remaining_uids];
  lids = lid_prop.values().localIds();
  // reorder lids in a set since the order is not warrantied (use of removed lids)
  std::set<Neo::utils::Int32> reordered_lids_ref{ lids_ref.begin(), lids_ref.end() };
  std::set<Neo::utils::Int32> reordered_lids{ lids.begin(), lids.end() };
  EXPECT_EQ(reordered_lids_ref.size(), reordered_lids.size());
  EXPECT_TRUE(std::equal(reordered_lids_ref.begin(), reordered_lids_ref.end(), reordered_lids.begin()));
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestFamily, test_family) {
  Neo::Family family(Neo::ItemKind::IK_Dof, "MyFamily");
  EXPECT_EQ(family.lidPropName(), family._lidProp().m_name);
  EXPECT_TRUE(family.hasProperty(family.lidPropName()));
  EXPECT_FALSE(family.hasProperty());
  EXPECT_FALSE(family.hasProperty("toto"));
  std::vector<Neo::utils::Int64> uids{ 0, 1, 2 };
  family._lidProp().append(uids); // internal
  EXPECT_EQ(3, family.nbElements());
  std::string scalar_prop_name("MyScalarProperty");
  std::string array_prop_name("MyArrayProperty");
  family.addMeshScalarProperty<Neo::utils::Int32>(scalar_prop_name);
  family.addMeshArrayProperty<Neo::utils::Int32>(array_prop_name);
  EXPECT_NO_THROW(family.getProperty(scalar_prop_name));
  EXPECT_NO_THROW(family.getProperty(array_prop_name));
  EXPECT_THROW(family.getProperty("UnexistingProperty"), std::invalid_argument);
  EXPECT_EQ(scalar_prop_name, family.getConcreteProperty<Neo::MeshScalarPropertyT<Neo::utils::Int32>>(scalar_prop_name).m_name);
  EXPECT_EQ(array_prop_name, family.getConcreteProperty<Neo::MeshArrayPropertyT<Neo::utils::Int32>>(array_prop_name).m_name);
  EXPECT_EQ(3, family.all().size());
  auto i = 0;
  auto local_ids = family.itemUniqueIdsToLocalids(uids);
  for (auto item : family.all()) {
    EXPECT_EQ(local_ids[i++], item);
  }
  family.itemUniqueIdsToLocalids(local_ids, uids);
  i = 0;
  for (auto item : family.all()) {
    EXPECT_EQ(local_ids[i++], item);
  }
  family.removeProperty(scalar_prop_name);
  EXPECT_FALSE(family.hasProperty(scalar_prop_name));
  family.removeProperty(array_prop_name);
  EXPECT_FALSE(family.hasProperty(array_prop_name));
  EXPECT_FALSE(family.hasProperty());
  // try removing an unexisting property
  family.removeProperty(scalar_prop_name);
  family.addMeshScalarProperty<Neo::utils::Int32>(scalar_prop_name);
  family.addMeshArrayProperty<Neo::utils::Int32>(array_prop_name);
  family.removeProperties();
  EXPECT_FALSE(family.hasProperty());
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestBaseMesh, base_mesh_unit_test) {
  Neo::MeshKernel::AlgorithmPropertyGraph mesh{ "test" };
  Neo::Family family1{ Neo::ItemKind::IK_Cell, "family1" };
  Neo::Family family2{ Neo::ItemKind::IK_Cell, "family2" };
  Neo::Family family3{ Neo::ItemKind::IK_Node, "family3" };
  Neo::Family family4{ Neo::ItemKind::IK_Edge, "family4" };
  Neo::Family family5{ Neo::ItemKind::IK_Dof, "family5" };
  bool is_called = false;
  family1.addMeshScalarProperty<int>("prop1");
  family2.addMeshScalarProperty<int>("prop2");
  family3.addMeshScalarProperty<int>("prop3");
  family4.addMeshScalarProperty<int>("prop4");
  family5.addMeshScalarProperty<int>("prop5");
  mesh.addAlgorithm(Neo::MeshKernel::InProperty{ family1, "prop1", Neo::PropertyStatus::ExistingProperty }, Neo::MeshKernel::OutProperty{ family2, "prop2" },
                    [&is_called]([[maybe_unused]] Neo::MeshScalarPropertyT<int> const& prop1,
                                 [[maybe_unused]] Neo::MeshScalarPropertyT<int>& prop2) {
                      is_called = true;
                    });
  // copy mesh
  auto mesh2 = mesh;
  // call algorithm on mesh 2
  mesh2.applyAlgorithms();
  EXPECT_TRUE(is_called);
  // now mesh 2 does not contain any algorithm
  is_called = false;
  mesh2.applyAlgorithms();
  EXPECT_FALSE(is_called);
  // call and keep algorithm on mesh1: algo will not be removed from mesh
  mesh.applyAndKeepAlgorithms();
  EXPECT_TRUE(is_called);
  // algo is still there and can be applied again
  is_called = false;
  mesh.applyAlgorithms();
  EXPECT_TRUE(is_called);
}

/*-----------------------------------------------------------------------------*/

void add_properties(Neo::Family& cell_family, Neo::Family& node_family) {

  // Adding node family and properties
  node_family.addMeshScalarProperty<Neo::utils::Real3>(std::string("node_coords"));
  node_family.addMeshScalarProperty<Neo::utils::Int64>("node_uids");
  node_family.addMeshArrayProperty<Neo::utils::Int32>("node2cells");
  node_family.addMeshScalarProperty<Neo::utils::Int32>("internal_end_of_remove_tag"); // not a user-defined property // todo use byte ?

  // Test adds
  EXPECT_NO_THROW(node_family.getProperty("node_uids"));

  // Adding cell family and properties
  cell_family.addMeshScalarProperty<Neo::utils::Int64>("cell_uids");
  cell_family.addMeshArrayProperty<Neo::utils::Int32>("cell2nodes");
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestBaseMesh, base_mesh_creation_test) {

  std::cout << "*------------------------------------*" << std::endl;
  std::cout << "* Test framework Neo thoughts " << std::endl;
  std::cout << "*------------------------------------*" << std::endl;

  // creating mesh as a graph of algorithm and properties
  auto mesh = Neo::MeshKernel::AlgorithmPropertyGraph{ "my_neo_mesh" };
  Neo::Family node_family{ Neo::ItemKind::IK_Node, "NodeFamily" };
  Neo::Family cell_family{ Neo::ItemKind::IK_Cell, "CellFamily" };

  add_properties(cell_family, node_family);
  // return;

  // given data to create mesh. After mesh creation data is no longer available
  std::vector<Neo::utils::Int64> node_uids{ 0, 1, 2 };
  std::vector<Neo::utils::Real3> node_coords{ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
  std::vector<Neo::utils::Int64> cell_uids{ 0, 2, 7, 9 };

  // add algos:

  // create nodes
  auto added_nodes = Neo::ItemRange{};
  mesh.addAlgorithm(
  Neo::MeshKernel::OutProperty{ node_family, node_family.lidPropName() },
  [&node_uids, &added_nodes](Neo::ItemLidsProperty& node_lids_property) {
    std::cout << "Algorithm: create nodes" << std::endl;
    added_nodes = node_lids_property.append(node_uids);
    node_lids_property.debugPrint();
    std::cout << "Inserted item range : " << added_nodes;
  });

  // register node uids
  mesh.addAlgorithm(
  Neo::MeshKernel::InProperty{ node_family, node_family.lidPropName() },
  Neo::MeshKernel::OutProperty{ node_family, "node_uids" },
  [&node_uids, &added_nodes]([[maybe_unused]] Neo::ItemLidsProperty const& node_lids_property,
                             Neo::MeshScalarPropertyT<Neo::utils::Int64>& node_uids_property) {
    std::cout << "Algorithm: register node uids" << std::endl;
    if (node_uids_property.isInitializableFrom(added_nodes))
      node_uids_property.init(added_nodes, std::move(node_uids)); // init can steal the input values
    else
      node_uids_property.append(added_nodes, node_uids);
    node_uids_property.debugPrint();
  }); // need to add a property check for existing uid

  // register node coords
  mesh.addAlgorithm(
  Neo::MeshKernel::InProperty{ node_family, node_family.lidPropName() },
  Neo::MeshKernel::OutProperty{ node_family, "node_coords" },
  [&node_coords, &added_nodes]([[maybe_unused]] Neo::ItemLidsProperty const& node_lids_property,
                               Neo::MeshScalarPropertyT<Neo::utils::Real3>& node_coords_property) {
    std::cout << "Algorithm: register node coords" << std::endl;
    if (node_coords_property.isInitializableFrom(added_nodes))
      node_coords_property.init(added_nodes, std::move(node_coords)); // init can steal the input values
    else
      node_coords_property.append(added_nodes, node_coords);
    node_coords_property.debugPrint();
  });
  //
  // Add cells and connectivity

  // create cells
  auto added_cells = Neo::ItemRange{};
  mesh.addAlgorithm(
  Neo::MeshKernel::OutProperty{ cell_family, cell_family.lidPropName() },
  [&cell_uids, &added_cells](Neo::ItemLidsProperty& cell_lids_property) {
    std::cout << "Algorithm: create cells" << std::endl;
    added_cells = cell_lids_property.append(cell_uids);
    cell_lids_property.debugPrint();
    std::cout << "Inserted item range : " << added_cells;
  });

  // register cell uids
  mesh.addAlgorithm(
  Neo::MeshKernel::InProperty{ cell_family, cell_family.lidPropName() },
  Neo::MeshKernel::OutProperty{ cell_family, "cell_uids" },
  [&cell_uids, &added_cells](
  [[maybe_unused]] Neo::ItemLidsProperty const& cell_lids_property,
  Neo::MeshScalarPropertyT<Neo::utils::Int64>& cell_uids_property) {
    std::cout << "Algorithm: register cell uids" << std::endl;
    if (cell_uids_property.isInitializableFrom(added_cells))
      cell_uids_property.init(added_cells, std::move(cell_uids)); // init can steal the input values
    else
      cell_uids_property.append(added_cells, cell_uids);
    cell_uids_property.debugPrint();
  });

  // register connectivity
  // node to cell
  std::vector<Neo::utils::Int64> connected_cell_uids{ 0, 0, 2, 2, 7, 9 };
  std::vector<int> nb_cell_per_node{ 1, 2, 3 };
  mesh.addAlgorithm(
  Neo::MeshKernel::InProperty{ node_family, node_family.lidPropName() },
  Neo::MeshKernel::InProperty{ cell_family, cell_family.lidPropName() },
  Neo::MeshKernel::OutProperty{ node_family, "node2cells" },
  [&connected_cell_uids, &nb_cell_per_node, &added_nodes]([[maybe_unused]] Neo::ItemLidsProperty const& node_lids_property,
                                                          Neo::ItemLidsProperty const& cell_lids_property,
                                                          Neo::MeshArrayPropertyT<Neo::utils::Int32>& node2cells) {
    std::cout << "Algorithm: register node-cell connectivity" << std::endl;
    auto connected_cell_lids = cell_lids_property[connected_cell_uids];
    if (node2cells.isInitializableFrom(added_nodes)) {
      node2cells.resize(std::move(nb_cell_per_node));
      node2cells.init(added_nodes, std::move(connected_cell_lids));
    }
    else {
      node2cells.append(added_nodes, connected_cell_lids, nb_cell_per_node);
    }
    node2cells.debugPrint();
  });

  // cell to node
  std::vector<Neo::utils::Int64> connected_node_uids{ 0, 1, 2, 1, 2, 0, 2, 1, 0 }; // on ne connecte volontairement pas toutes les mailles pour vérifier initialisation ok sur la famille
  auto nb_node_per_cell = { 3, 0, 3, 3 };
  mesh.addAlgorithm(Neo::MeshKernel::InProperty{ node_family, node_family.lidPropName() },
                    Neo::MeshKernel::InProperty{ cell_family, cell_family.lidPropName() },
                    Neo::MeshKernel::OutProperty{ cell_family, "cell2nodes" },
                    [&connected_node_uids, &nb_node_per_cell, &added_cells](
                    Neo::ItemLidsProperty const& node_lids_property,
                    [[maybe_unused]] Neo::ItemLidsProperty const& cell_lids_property,
                    Neo::MeshArrayPropertyT<Neo::utils::Int32>& cells2nodes) {
                      std::cout << "Algorithm: register cell-node connectivity" << std::endl;
                      auto connected_node_lids = node_lids_property[connected_node_uids];
                      if (cells2nodes.isInitializableFrom(added_cells)) {
                        cells2nodes.resize(std::move(nb_node_per_cell));
                        cells2nodes.init(added_cells, std::move(connected_node_lids));
                      }
                      else
                        cells2nodes.append(added_cells, connected_node_lids, nb_node_per_cell);
                      cells2nodes.debugPrint();
                    });

  // try to modify an existing property
  // add new cells
  std::vector<Neo::utils::Int64> new_cell_uids{ 10, 11, 12 }; // elles seront toutes rouges
  auto new_cell_added = Neo::ItemRange{};
  mesh.addAlgorithm(Neo::MeshKernel::OutProperty{ cell_family, cell_family.lidPropName() },
                    [&new_cell_uids, &new_cell_added](Neo::ItemLidsProperty& cell_lids_property) {
                      std::cout << "Algorithm: add new cells" << std::endl;
                      new_cell_added = cell_lids_property.append(new_cell_uids);
                      cell_lids_property.debugPrint();
                      std::cout << "Inserted item range : " << new_cell_added;
                    });

  // register new cell uids
  mesh.addAlgorithm(
  Neo::MeshKernel::InProperty{ cell_family, cell_family.lidPropName() },
  Neo::MeshKernel::OutProperty{ cell_family, "cell_uids" },
  [&new_cell_uids, &new_cell_added](
  [[maybe_unused]] Neo::ItemLidsProperty const& cell_lids_property,
  Neo::MeshScalarPropertyT<Neo::utils::Int64>& cell_uids_property) {
    std::cout << "Algorithm: register new cell uids" << std::endl;
    // must append and not initialize
    if (cell_uids_property.isInitializableFrom(new_cell_added))
      cell_uids_property.init(new_cell_added, std::move(new_cell_uids)); // init can steal the input values
    else
      cell_uids_property.append(new_cell_added, new_cell_uids);
    cell_uids_property.debugPrint();
  });

  // add connectivity to new cells
  std::vector<Neo::utils::Int64> new_cell_connected_node_uids{ 0, 1, 2, 1, 2 }; // on ne connecte volontairement pas toutes les mailles pour vérifier initialisation ok sur la famille
  std::vector<int> nb_node_per_new_cell{ 0, 3, 2 };
  mesh.addAlgorithm(Neo::MeshKernel::InProperty{ node_family, node_family.lidPropName() },
                    Neo::MeshKernel::InProperty{ cell_family, cell_family.lidPropName() },
                    Neo::MeshKernel::OutProperty{ cell_family, "cell2nodes" },
                    [&new_cell_connected_node_uids, &nb_node_per_new_cell, &new_cell_added](
                    Neo::ItemLidsProperty const& node_lids_property,
                    [[maybe_unused]] Neo::ItemLidsProperty const& cell_lids_property,
                    Neo::MeshArrayPropertyT<Neo::utils::Int32>& cells2nodes) {
                      std::cout << "Algorithm: register new cell-node connectivity" << std::endl;
                      auto connected_node_lids = node_lids_property[new_cell_connected_node_uids];
                      if (cells2nodes.isInitializableFrom(new_cell_added)) {
                        cells2nodes.resize(std::move(nb_node_per_new_cell));
                        cells2nodes.init(new_cell_added, std::move(connected_node_lids));
                      }
                      else
                        cells2nodes.append(new_cell_added, connected_node_lids, nb_node_per_new_cell);
                      cells2nodes.debugPrint();
                    });

  // remove nodes
  std::vector<Neo::utils::Int64> removed_node_uids{ 1, 2 };
  auto removed_nodes = Neo::ItemRange{};
  mesh.addAlgorithm(
  Neo::MeshKernel::OutProperty{ node_family, node_family.lidPropName() },
  Neo::MeshKernel::OutProperty{ node_family, "internal_end_of_remove_tag" },
  [&removed_node_uids, &removed_nodes, &node_family](
  Neo::ItemLidsProperty& node_lids_property,
  Neo::MeshScalarPropertyT<Neo::utils::Int32>& internal_end_of_remove_tag) {
    // Store removed items in internal_end_of_remove_tag
    internal_end_of_remove_tag.init(node_family.all(), 0);
    for (auto removed_item : removed_nodes) {
      internal_end_of_remove_tag[removed_item] = 1;
    }
    std::cout << "Algorithm: remove nodes" << std::endl;
    removed_nodes = node_lids_property.remove(removed_node_uids);
    node_lids_property.debugPrint();
    std::cout << "removed item range : " << removed_nodes;
  });

  // handle node removal in connectivity with node family = target family
  mesh.addAlgorithm(
  Neo::MeshKernel::InProperty{ node_family, "internal_end_of_remove_tag" },
  Neo::MeshKernel::OutProperty{ cell_family, "cell2nodes" },
  [&cell_family](
  Neo::MeshScalarPropertyT<Neo::utils::Int32> const& internal_end_of_remove_tag,
  Neo::MeshArrayPropertyT<Neo::utils::Int32>& cells2nodes) {
    //                    std::transform()
    //                    Neo::ItemRange node_range {Neo::ItemLocalIds{{},0,node_family.size()}};
    for (auto cell : cell_family.all()) {
      auto connected_nodes = cells2nodes[cell];
      for (auto& connected_node : connected_nodes) {
        if (connected_node != Neo::utils::NULL_ITEM_LID && internal_end_of_remove_tag[connected_node] == 1) {
          std::cout << "modify node : " << connected_node << std::endl;
          connected_node = Neo::utils::NULL_ITEM_LID;
        }
      }
    }
  });

  // launch algos
  mesh.applyAlgorithms();
}

/*-----------------------------------------------------------------------------*/

TEST(NeoTestPartialMeshModification, partial_mesh_modif_test) {

  // WIP: test in construction
  // modify node coords
  // input data
  std::array<int, 3> node_uids{ 0, 1, 3 };
  Neo::utils::Real3 r = { 0, 0, 0 };
  std::array<Neo::utils::Real3, 3> node_coords = { r, r, r }; // don't get why I can't write {{0,0,0},{0,0,0},{0,0,0}}; ...??

  // creating mesh as a graph of Algorithms and Properties
  auto mesh = Neo::MeshKernel::AlgorithmPropertyGraph{ "my_neo_mesh" };
  Neo::Family node_family{ Neo::ItemKind::IK_Node, "NodeFamily" };
  Neo::Family cell_family{ Neo::ItemKind::IK_Cell, "CellFamily" };

  add_properties(cell_family, node_family);

  mesh.addAlgorithm(Neo::MeshKernel::InProperty{ node_family, node_family.lidPropName() },
                    Neo::MeshKernel::OutProperty{ node_family, "node_coords" },
                    [&node_coords, &node_uids](
                    [[maybe_unused]] Neo::ItemLidsProperty const& node_lids_property,
                    [[maybe_unused]] Neo::MeshScalarPropertyT<Neo::utils::Real3>& node_coords_property) {
                      std::cout << "Algorithm: register node coords" << std::endl;
                      //auto& lids = node_lids_property[node_uids];//todo
                      //node_coords_property.appendAt(lids, node_coords);// steal node_coords memory//todo
                    });

  mesh.applyAlgorithms();
}

/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
