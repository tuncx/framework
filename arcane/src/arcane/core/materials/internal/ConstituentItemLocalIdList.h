﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2024 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* ConstituentItemLocalIdList.h                                (C) 2000-2024 */
/*                                                                           */
/* Gestion des listes d'identifiants locaux de 'ComponentItemInternal'.      */
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_CORE_MATERIALS_INTERNAL_CONSTITUENTITEMLOCALIDLIST_H
#define ARCANE_CORE_MATERIALS_INTERNAL_CONSTITUENTITEMLOCALIDLIST_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/TraceAccessor.h"
#include "arcane/utils/Array.h"

#include "arcane/materials/MaterialsGlobal.h"
#include "arcane/core/materials/ComponentItemInternal.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane::Materials
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \internal
 * \brief Liste d'indices locaux pour les 'ComponentItemInternal'.
 */
class ARCANE_CORE_EXPORT ConstituentItemLocalIdList
{
 public:

  explicit ConstituentItemLocalIdList(ComponentItemSharedInfo* shared_info);

 public:

  void resize(Int32 new_size);

 public:

  ConstArrayView<ComponentItemInternal*> itemsInternalView() const
  {
    return m_items_internal;
  }

  void setConstituentItem(Int32 index, ComponentItemInternalLocalId id)
  {
    m_item_internal_local_id_list[index] = id;
    m_items_internal[index] = m_shared_info->_itemInternal(id);
  }

  void copy(ConstArrayView<ComponentItemInternalLocalId> ids)
  {
    const Int32 size = ids.size();
    resize(size);
    for (Int32 i = 0; i < size; ++i)
      setConstituentItem(i, ids[i]);
  }

  /*!
   * \brief Copie les constituents partitionnés en partie pure et partielle.
   */
  void copyPureAndPartial(ConstArrayView<ComponentItemInternalLocalId> pure_ids,
                          ConstArrayView<ComponentItemInternalLocalId> partial_ids)
  {
    Int32 nb_pure = pure_ids.size();
    Int32 nb_partial = partial_ids.size();

    resize(nb_pure + nb_partial);
    for (Int32 i = 0; i < nb_pure; ++i)
      setConstituentItem(i, pure_ids[i]);
    for (Int32 i = 0; i < nb_partial; ++i)
      setConstituentItem(nb_pure + i, partial_ids[i]);
  }

  ConstArrayView<ComponentItemInternalLocalId> localIds() const
  {
    return m_item_internal_local_id_list;
  }

  ComponentItemInternal* itemInternal(Int32 index) const
  {
    return m_shared_info->_itemInternal(localId(index));
  }

  ComponentItemInternalLocalId localId(Int32 index) const
  {
    return m_item_internal_local_id_list[index];
  }

  MatVarIndex variableIndex(Int32 index) const
  {
    return _itemInternal(index)->variableIndex();
  }

  ConstituentItemLocalIdListView view() const
  {
    return { m_shared_info, m_item_internal_local_id_list, m_items_internal };
  }

  void deprecatedCopy(ConstArrayView<ComponentItemInternal*> items)
  {
    const Int32 size = items.size();
    resize(size);
    for (Int32 i = 0; i < size; ++i)
      setConstituentItem(i, items[i]->_internalLocalId());
  }

 private:

  //! Liste des ComponentItemInternal* pour ce constituant.
  UniqueArray<ComponentItemInternal*> m_items_internal;

  //! Liste des ComponentItemInternalLocalId pour ce constituant.
  UniqueArray<ComponentItemInternalLocalId> m_item_internal_local_id_list;

  ComponentItemSharedInfo* m_shared_info = nullptr;

 private:

  ComponentItemInternal* _itemInternal(Int32 index) const
  {
    return m_shared_info->_itemInternal(localId(index));
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // namespace Arcane::Materials

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif
