﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2020 IFPEN-CEA
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* SerializeMessageList.h                                      (C) 2000-2020 */
/*                                                                           */
/* Liste de messages de sérialisation.                                       */
/*---------------------------------------------------------------------------*/
#ifndef ARCCORE_MESSAGEPASSING_SERIALIZEMESSAGEMESSAGELIST_H
#define ARCCORE_MESSAGEPASSING_SERIALIZEMESSAGEMESSAGELIST_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/message_passing/ISerializeMessageList.h"

#include "arccore/message_passing/PointToPointMessageInfo.h"
#include "arccore/base/Ref.h"
#include "arccore/trace/TimeMetric.h"
#include "arccore/collections/Array.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore::MessagePassing::internal
{
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \internal
 * \brief Liste de messages de sérialisation.
 *
 * Cette classe supporte le cas où une implémentation ne supporte par les
 * messages ayant comme destionation n'importe quel rang (c'est par exemple
 * le cas du mode hybride).
 */
class ARCCORE_MESSAGEPASSING_EXPORT SerializeMessageList
: public ISerializeMessageList
{
  using SerializeMessageContainer = UniqueArray<ISerializeMessage*>;

  struct ProbeInfo
  {
   public:
    ProbeInfo() = default;
    ProbeInfo(ISerializeMessage* sm,const PointToPointMessageInfo& message_info)
    : m_serialize_message(sm), m_message_info(message_info){}
   public:
    ISerializeMessage* m_serialize_message = nullptr;
    PointToPointMessageInfo m_message_info;
    bool m_is_probe_done = false;
  };

 public:

  SerializeMessageList(IMessagePassingMng* mpm);

 public:

  void addMessage(ISerializeMessage* msg) override;
  void processPendingMessages() override;
  Integer waitMessages(eWaitType wait_type) override;
  Ref<ISerializeMessage>
  createAndAddMessage(MessageRank destination,ePointToPointMessageType type) override;

  void setAllowAnyRankReceive(bool v) { m_allow_any_rank_receive = v; }

 private:

  IMessagePassingMng* m_message_passing_mng = nullptr;
  SerializeMessageContainer m_messages_to_process;
  Ref<IRequestList> m_request_list;
  SerializeMessageContainer m_messages_serialize;
  SerializeMessageContainer m_remaining_serialize_messages;
  UniqueArray<ProbeInfo> m_messages_to_probe;
  bool m_allow_any_rank_receive = true;
  TimeMetricAction m_message_passing_phase;

  Integer _waitMessages(eWaitType wait_type);
  void _addMessage(ISerializeMessage* sm,const PointToPointMessageInfo& message_info);
  PointToPointMessageInfo buildMessageInfo(ISerializeMessage* sm);
  void _doProbe();
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arccore::MessagePassing::internal

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  

