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
/* MpiTypeDispatcher.cc                                        (C) 2000-2018 */
/*                                                                           */
/* Gestionnaire de parallélisme utilisant MPI.                               */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/message_passing_mpi/MpiTypeDispatcherImpl.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore
{
namespace MessagePassing
{
namespace Mpi
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template class MpiTypeDispatcher<char>;
template class MpiTypeDispatcher<signed char>;
template class MpiTypeDispatcher<unsigned char>;
template class MpiTypeDispatcher<short>;
template class MpiTypeDispatcher<unsigned short>;
template class MpiTypeDispatcher<int>;
template class MpiTypeDispatcher<unsigned int>;
template class MpiTypeDispatcher<long>;
template class MpiTypeDispatcher<unsigned long>;
template class MpiTypeDispatcher<long long>;
template class MpiTypeDispatcher<unsigned long long>;
template class MpiTypeDispatcher<float>;
template class MpiTypeDispatcher<double>;
template class MpiTypeDispatcher<long double>;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Mpi
} // End namespace MessagePassing
} // End namespace Arccore

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
