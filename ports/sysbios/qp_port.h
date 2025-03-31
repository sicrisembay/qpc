//============================================================================
// QP/C Real-Time Embedded Framework (RTEF)
//
//                   Q u a n t u m  L e a P s
//                   ------------------------
//                   Modern Embedded Software
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open source GNU
// General Public License version 3 (or any later version), or alternatively,
// under the terms of one of the closed source Quantum Leaps commercial
// licenses.
//
// The terms of the open source GNU General Public License version 3
// can be found at: <www.gnu.org/licenses/gpl-3.0>
//
// The terms of the closed source Quantum Leaps commercial licenses
// can be found at: <www.state-machine.com/licensing>
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// Contact information:
// <www.state-machine.com>
// <info@state-machine.com>
//============================================================================
//! @date Last updated on: 2023-09-07
//! @version Last updated for: @ref qpc_7_3_0
//!
//! @file
//! @brief QP/C port to SysBIOS 6.83.x, generic C11 compiler
//!
//! @history
//! - 2025-03-30  (Sicris Rey Embay):  Port to SysBIOS v6.83.x

#ifndef QP_PORT_H_
#define QP_PORT_H_

#include <stdint.h>  // Exact-width types. WG14/N843 C99 Standard
#include <stdbool.h> // Boolean type.      WG14/N843 C99 Standard

#define C2000_QPC_PORT      (1)

#ifdef QP_CONFIG
#include "qp_config.h" // external QP configuration
#endif

// no-return function specifier (C11 Standard)
#define Q_NORETURN   _Noreturn void

// QActive event queue and thread types for SysBIOS
#define QACTIVE_EQUEUE_TYPE     Queue_Struct
//#define QACTIVE_OS_OBJ_TYPE     TODO:: TBD
#define QACTIVE_THREAD_TYPE     Task_Struct

// QF interrupt disabling/enabling
#define QF_INT_DISABLE()        /// TODO: TBD
#define QF_INT_ENABLE()         /// TODO: TBD

// QF critical section for SysBIOS
#define QF_CRIT_STAT            /// TODO: TBD
#define QF_CRIT_ENTRY()         /// TODO: TBD
#define QF_CRIT_EXIT()          /// TODO: TBD

// include files -------------------------------------------------------------
#include "ti/sysbios/BIOS.h"
#include "ti/sysbios/knl/Task.h"
#include "ti/sysbios/knl/Queue.h"

#ifndef uint8_t
#define uint8_t uint16_t    // C2000 does not support uint8_t
#endif

#include "qequeue.h"   // QP event queue (for deferring events)
#include "qmpool.h"    // QP memory pool (for event pools)
#include "qp.h"        // QP platform-independent public interface

//============================================================================
// interface used only inside QF, but not in applications

#ifdef QP_IMPL
    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(prio_)   /// TODO
    #define QF_SCHED_UNLOCK_()      /// TODO

    // native QF event pool customization
    #define QF_EPOOL_TYPE_        QMPool
    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
        (QMPool_init(&(p_), (poolSto_), (poolSize_), (evtSize_)))
    #define QF_EPOOL_EVENT_SIZE_(p_)  ((uint_fast16_t)(p_).blockSize)
    #define QF_EPOOL_GET_(p_, e_, m_, qsId_) \
        ((e_) = (QEvt *)QMPool_get(&(p_), (m_), (qsId_)))
    #define QF_EPOOL_PUT_(p_, e_, qsId_) \
        (QMPool_put(&(p_), (e_), (qsId_)))

#endif // QP_IMPL

#endif // QP_PORT_H_

