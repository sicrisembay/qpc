//============================================================================
// QP/C Real-Time Embedded Framework (RTEF)
// Version 8.0.2
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open-source GNU
// General Public License (GPL) or under the terms of one of the closed-
// source Quantum Leaps commercial licenses.
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// NOTE:
// The GPL does NOT permit the incorporation of this code into proprietary
// programs. Please contact Quantum Leaps for commercial licensing options,
// which expressly supersede the GPL and are designed explicitly for
// closed-source distribution.
//
// Quantum Leaps contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
#define QP_IMPL           // this is QP implementation
#include "qp_port.h"      // QP port
#include "qp_pkg.h"       // QP package-scope interface
#include "qsafe.h"        // QP Functional Safety (FuSa) Subsystem
#ifdef Q_SPY              // QS software tracing enabled?
    #include "qs_port.h"  // QS port
    #include "qs_pkg.h"   // QS facilities for pre-defined trace records
#else
    #include "qs_dummy.h" // disable the QS software tracing
#endif // Q_SPY

Q_DEFINE_THIS_MODULE("qf_qact")

//............................................................................
//! @protected @memberof QActive
void QActive_ctor(QActive * const me,
    QStateHandler const initial)
{
    // clear the whole QActive object, so that the framework can start
    // correctly even if the startup code fails to clear the uninitialized
    // data (as is required by the C Standard).
    QF_bzero_(me, sizeof(*me));

    // NOTE: QActive inherits the abstract QAsm class, but it calls the
    // constructor of the QHsm subclass. This is because QActive inherits
    // the behavior from the QHsm subclass.
    QHsm_ctor((QHsm *)(me), initial);

    // NOTE: this vtable is identical as QHsm, but is provided
    // for the QActive subclass to provide a UNIQUE vptr to distinguish
    // subclasses of QActive (e.g., in the debugger).
    static struct QAsmVtable const vtable = { // QActive virtual table
        &QHsm_init_,
        &QHsm_dispatch_,
        &QHsm_isIn_
#ifdef Q_SPY
        ,&QHsm_getStateHandler_
#endif
    };
    me->super.vptr = &vtable; // hook vptr to QActive vtable
}

//............................................................................
//! @private @memberof QActive
void QActive_register_(QActive * const me) {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();

    if (me->pthre == 0U) { // preemption-threshold not defined?
        me->pthre = me->prio; // apply the default
    }

#ifndef Q_UNSAFE
    Q_REQUIRE_INCRIT(100,
        (0U < me->prio) && (me->prio <= QF_MAX_ACTIVE)
        && (QActive_registry_[me->prio] == (QActive *)0)
        && (me->prio <= me->pthre));

    uint8_t prev_thre = me->pthre;
    uint8_t next_thre = me->pthre;

    for (uint_fast8_t p = (uint_fast8_t)me->prio - 1U;
         p > 0U;
         --p)
    {
        if (QActive_registry_[p] != (QActive *)0) {
            prev_thre = QActive_registry_[p]->pthre;
            break;
        }
    }
    for (uint_fast8_t p = (uint_fast8_t)me->prio + 1U;
         p <= QF_MAX_ACTIVE;
         ++p)
    {
        if (QActive_registry_[p] != (QActive *)0) {
            next_thre = QActive_registry_[p]->pthre;
            break;
        }
    }

    Q_ASSERT_INCRIT(190,
        (prev_thre <= me->pthre)
        && (me->pthre <= next_thre));
#endif // Q_UNSAFE

    // register the AO at the QF-prio.
    QActive_registry_[me->prio] = me;

    QF_CRIT_EXIT();
}

//............................................................................
//! @private @memberof QActive
void QActive_unregister_(QActive * const me) {
    uint_fast8_t const p = (uint_fast8_t)me->prio;

    QF_CRIT_STAT
    QF_CRIT_ENTRY();

    Q_REQUIRE_INCRIT(200, (0U < p) && (p <= QF_MAX_ACTIVE)
                      && (QActive_registry_[p] == me));
    QActive_registry_[p] = (QActive *)0; // free-up the prio. level
    me->super.state.fun = Q_STATE_CAST(0); // invalidate the state

    QF_CRIT_EXIT();
}
