// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* Mesh.cpp                                        (C) 2000-2021             */
/*                                                                           */
/* Asynchronous Mesh structure based on Neo kernel                           */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include <stdexcept>

#include "Mesh.h"
#include "Neo.h"

/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/

Neo::Mesh::Mesh(const std::string& mesh_name)
 : m_mesh_graph(std::make_unique<Neo::MeshBase>(Neo::MeshBase{mesh_name})){
}

/*-----------------------------------------------------------------------------*/

Neo::Mesh::~Mesh() = default;

/*-----------------------------------------------------------------------------*/

std::string const& Neo::Mesh::name() const noexcept {
  return m_mesh_graph->m_name;
}

/*-----------------------------------------------------------------------------*/

Neo::Family& Neo::Mesh::findFamily(Neo::ItemKind family_kind,
                                         std::string const& family_name) const noexcept(ndebug) {
  return m_mesh_graph->getFamily(family_kind,family_name);
}

/*-----------------------------------------------------------------------------*/

std::string Neo::Mesh::uniqueIdPropertyName(const std::string& family_name) const noexcept
{
  return family_name+"_uids";
}

/*-----------------------------------------------------------------------------*/

Neo::Family& Neo::Mesh::addFamily(Neo::ItemKind item_kind, std::string family_name) noexcept
{
  auto& cell_family = m_mesh_graph->addFamily(item_kind, std::move(family_name));
  cell_family.addProperty<Neo::utils::Int64>(uniqueIdPropertyName(family_name));
  return cell_family;
}

/*-----------------------------------------------------------------------------*/

void Neo::Mesh::scheduleAddItems(Neo::Family& family, std::vector<Neo::utils::Int64> uids, Neo::FutureItemRange & added_item_range) noexcept
{
  ItemRange& added_items = added_item_range;
  // Add items
  m_mesh_graph->addAlgorithm(  Neo::OutProperty{family,family.lidPropName()},
                               [&family,uids,&added_items](Neo::ItemLidsProperty & lids_property){
                                 std::cout << "Algorithm: create items in family " << family.name() << std::endl;
                                 added_items = lids_property.append(uids);
                                 lids_property.debugPrint();
                                 Neo::print() << "Inserted item range : " << added_items;
                               });
  // register their uids
  m_mesh_graph->addAlgorithm(
      Neo::InProperty{family,family.lidPropName()},
      Neo::OutProperty{family, uniqueIdPropertyName(family.name())},
      [&family,uids{std::move(uids)},&added_items](Neo::ItemLidsProperty const& item_lids_property,
                                   Neo::PropertyT<Neo::utils::Int64>& item_uids_property){
        std::cout << "Algorithm: register item uids for family " << family.name() << std::endl;
        if (item_uids_property.isInitializableFrom(added_items)){
          item_uids_property.init(added_items,std::move(uids)); // init can steal the input values
        }
        else {
          item_uids_property.append(added_items, uids);
        }
        item_uids_property.debugPrint();
      });// need to add a property check for existing uid
}

/*-----------------------------------------------------------------------------*/

template <typename ItemRangeT>
void Neo::Mesh::_scheduleAddConnectivity(Neo::Family& source_family, Neo::ItemRangeWrapper<ItemRangeT> source_items_wrapper,
                                         Neo::Family& target_family, std::vector<int> nb_connected_item_per_item,
                                         std::vector<Neo::utils::Int64> connected_item_uids,
                                         std::string const& connectivity_unique_name,
                                         ConnectivityOperation add_or_modify) {
  // add connectivity property if doesn't exist
  source_family.addArrayProperty<Neo::utils::Int32>(connectivity_unique_name);
  // Create connectivity wrapper and add it to mesh
  auto& connectivity_property = source_family.getConcreteProperty<Mesh::ConnectivityPropertyType>(connectivity_unique_name);
  auto [iterator, is_inserted] = m_connectivities.insert(std::make_pair(connectivity_unique_name,
                                                                        Connectivity{source_family,target_family,
                                                                                     connectivity_unique_name,
                                                                                     connectivity_property}));
  if (!is_inserted && add_or_modify==ConnectivityOperation::Add) {
    throw std::invalid_argument("Cannot include already inserted connectivity "+connectivity_unique_name);
  }
  m_mesh_graph->addAlgorithm(
      Neo::InProperty{source_family,source_family.lidPropName()},
      Neo::InProperty{target_family,target_family.lidPropName()},
      Neo::OutProperty{source_family, connectivity_unique_name},
      [connected_item_uids{std::move(connected_item_uids)},
          nb_connected_item_per_item{std::move(nb_connected_item_per_item)},
          source_items_wrapper, &source_family, &target_family]
          (Neo::ItemLidsProperty const& source_family_lids_property,
           Neo::ItemLidsProperty const& target_family_lids_property,
           Neo::ArrayProperty<Neo::utils::Int32> & source2target){
        std::cout << "Algorithm: register connectivity between " <<
                  source_family.m_name << "  and  " << target_family.m_name << std::endl;
        ItemRange const& source_items = source_items_wrapper.get();
        auto connected_item_lids = target_family_lids_property[connected_item_uids];
        if (source2target.isInitializableFrom(source_items)) {
          source2target.resize(std::move(nb_connected_item_per_item));
          source2target.init(source_items,std::move(connected_item_lids));
        }
        else {
          source2target.append(source_items,connected_item_lids,
                               nb_connected_item_per_item);
        }
        source2target.debugPrint();
      });
}

/*-----------------------------------------------------------------------------*/

void Neo::Mesh::scheduleAddConnectivity(Neo::Family& source_family, Neo::ItemRange const& source_items,
                                        Neo::Family& target_family, std::vector<int> nb_connected_item_per_item,
                                        std::vector<Neo::utils::Int64> connected_item_uids,
                                        std::string const& connectivity_unique_name,
                                        ConnectivityOperation add_or_modify) {
  _scheduleAddConnectivity(source_family, Neo::ItemRangeWrapper<const ItemRange>{source_items}, target_family,
                           nb_connected_item_per_item, connected_item_uids, connectivity_unique_name, add_or_modify);
}

/*-----------------------------------------------------------------------------*/

void Neo::Mesh::scheduleAddConnectivity(Neo::Family& source_family, Neo::FutureItemRange& source_items,
                                        Neo::Family& target_family, std::vector<int> nb_connected_item_per_item,
                                        std::vector<Neo::utils::Int64> connected_item_uids,
                                        std::string const& connectivity_unique_name,
                                        ConnectivityOperation add_or_modify)
{
  _scheduleAddConnectivity(source_family,Neo::ItemRangeWrapper<FutureItemRange>{source_items},
                           target_family,std::move(nb_connected_item_per_item),
                           std::move(connected_item_uids),
                          connectivity_unique_name,add_or_modify);
}

/*-----------------------------------------------------------------------------*/

void Neo::Mesh::scheduleAddConnectivity(Neo::Family& source_family, Neo::ItemRange const& source_items,
                                        Neo::Family& target_family, int nb_connected_item_per_item,
                                        std::vector<Neo::utils::Int64>  connected_item_uids,
                                        std::string const& connectivity_unique_name,
                                        ConnectivityOperation add_or_modify)
{
  assert (("source items and connected item uids sizes are not coherent with nb_connected_item_per_item",
      source_items.size()*nb_connected_item_per_item == connected_item_uids.size()));
  std::vector<int> nb_connected_item_per_item_array(source_items.size(),nb_connected_item_per_item) ;
  scheduleAddConnectivity(source_family,source_items,target_family,
                          std::move(nb_connected_item_per_item_array),
                          std::move(connected_item_uids),
                          connectivity_unique_name,add_or_modify);
}

/*-----------------------------------------------------------------------------*/

void Neo::Mesh::scheduleAddConnectivity(Neo::Family& source_family, Neo::FutureItemRange& source_items,
                                        Neo::Family& target_family, int nb_connected_item_per_item,
                                        std::vector<Neo::utils::Int64> connected_item_uids,
                                        std::string const& connectivity_unique_name,
                                        ConnectivityOperation add_or_modify)
{
  assert (("Connected item uids size is not compatible with nb_connected_item_per_item",
      connected_item_uids.size()%nb_connected_item_per_item==0));
  auto source_item_size = connected_item_uids.size()/nb_connected_item_per_item;
  std::vector<int> nb_connected_item_per_item_array(source_item_size,nb_connected_item_per_item) ;
  _scheduleAddConnectivity(source_family,Neo::ItemRangeWrapper<FutureItemRange>{source_items},target_family,
                          std::move(nb_connected_item_per_item_array),
                          std::move(connected_item_uids),
                          connectivity_unique_name,add_or_modify);
}

/*-----------------------------------------------------------------------------*/

void Neo::Mesh::scheduleSetItemCoords(Neo::Family& item_family, Neo::FutureItemRange& future_added_item_range,std::vector<Neo::utils::Real3> item_coords) noexcept
{
  auto coord_prop_name = _itemCoordPropertyName(item_family);
  item_family.addProperty<Neo::utils::Real3>(coord_prop_name);
  ItemRange& added_items = future_added_item_range;
  m_mesh_graph->addAlgorithm(
          Neo::InProperty{item_family, item_family.lidPropName()},
          Neo::OutProperty{item_family,coord_prop_name},
          [item_coords{std::move(item_coords)},&added_items]
          (Neo::ItemLidsProperty const& item_lids_property,
                                     Neo::PropertyT<Neo::utils::Real3>& item_coords_property){
            std::cout << "Algorithm: register item coords" << std::endl;
            if (item_coords_property.isInitializableFrom(added_items)) {
              item_coords_property.init(
                  added_items,
                  std::move(item_coords)); // init can steal the input values
            }
            else {
              item_coords_property.append(added_items, item_coords);
            }
            item_coords_property.debugPrint();
          });
}

/*-----------------------------------------------------------------------------*/

Neo::EndOfMeshUpdate Neo::Mesh::applyScheduledOperations() noexcept
{
  return m_mesh_graph->applyAlgorithms();
}

/*-----------------------------------------------------------------------------*/

Neo::Mesh::CoordPropertyType& Neo::Mesh::getItemCoordProperty(Neo::Family & family)
{
  return family.getConcreteProperty<CoordPropertyType>(_itemCoordPropertyName(family));
}

/*-----------------------------------------------------------------------------*/

Neo::Mesh::Connectivity const Neo::Mesh::getConnectivity(Neo::Family const& source_family,Neo::Family const& target_family,std::string const& connectivity_name)
{
  auto connectivity_iter = m_connectivities.find(connectivity_name);
  if (connectivity_iter == m_connectivities.end()) throw std::invalid_argument("Cannot find Connectivity "+connectivity_name);
  return connectivity_iter->second;
  // todo check source and target family type...(add operator== on family)
}

/*-----------------------------------------------------------------------------*/

Neo::Mesh::CoordPropertyType const& Neo::Mesh::getItemCoordProperty(Neo::Family const& family) const
{
  return family.getConcreteProperty<CoordPropertyType>(_itemCoordPropertyName(family));
}

/*-----------------------------------------------------------------------------*/

std::vector<Neo::Mesh::Connectivity> Neo::Mesh::items(Neo::Family const& source_family, Neo::ItemKind item_kind) const noexcept
{
  std::vector<Connectivity> connectivities_vector{};
  std::transform(m_connectivities.begin(),m_connectivities.end(),std::back_inserter(connectivities_vector),
                 [](auto& name_connectivity_pair){
                   return name_connectivity_pair.second;});
  std::vector<Connectivity> item_connectivities_vector{};
  std::copy_if(connectivities_vector.begin(),connectivities_vector.end(),std::back_inserter(item_connectivities_vector),
               [&source_family,item_kind](auto& connectivity){
                 return (connectivity.source_family.name() == source_family.name() && // todo replace by a comparison operator betwenn families
                         connectivity.target_family.itemKind() == item_kind);
               });
  return item_connectivities_vector;
}

/*-----------------------------------------------------------------------------*/

std::vector<Neo::Mesh::Connectivity> Neo::Mesh::edges(Neo::Family const& source_family) const noexcept
{
  return items(source_family,Neo::ItemKind::IK_Edge);
}

/*-----------------------------------------------------------------------------*/

std::vector<Neo::Mesh::Connectivity> Neo::Mesh::nodes(Neo::Family const& source_family) const noexcept
{
  return items(source_family,Neo::ItemKind::IK_Node);
}
/*-----------------------------------------------------------------------------*/

std::vector<Neo::Mesh::Connectivity> Neo::Mesh::faces(Neo::Family const& source_family) const noexcept
{
  return items(source_family,Neo::ItemKind::IK_Face);
}

/*-----------------------------------------------------------------------------*/

std::vector<Neo::Mesh::Connectivity> Neo::Mesh::cells(Neo::Family const& source_family) const noexcept
{
  return items(source_family,Neo::ItemKind::IK_Cell);
}

/*-----------------------------------------------------------------------------*/

std::vector<Neo::Mesh::Connectivity> Neo::Mesh::dofs(Neo::Family const& source_family) const noexcept
{
  return items(source_family,Neo::ItemKind::IK_Dof);
}

std::vector<Neo::utils::Int64> Neo::Mesh::uniqueIds(Neo::Family const& item_family,
                                                    std::vector<Neo::utils::Int32> const& item_lids) const noexcept
{
  auto const& uid_property = getItemUidsProperty(item_family);
  return uid_property[item_lids];
}

/*-----------------------------------------------------------------------------*/

const Neo::Mesh::UidPropertyType & Neo::Mesh::getItemUidsProperty(const Neo::Family &item_family) const noexcept
{
  return item_family.getConcreteProperty<UidPropertyType>(uniqueIdPropertyName(item_family.name()));
}

/*-----------------------------------------------------------------------------*/

std::vector<Neo::utils::Int32> Neo::Mesh::localIds(Neo::Family const& item_family,
                                                    std::vector<Neo::utils::Int64> const& item_uids) const noexcept
{
  return item_family.itemUniqueIdsToLocalids(item_uids);
}
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/