//============================================================================
// QP/C Real-Time Embedded Framework (RTEF)
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
//! @date Last updated on: 2024-06-11
//! @version Last updated for: @ref qpc_7_4_0
//!
//! @file
//! @brief QP/C port to SysBIOS 6.83.x, generic C11 compiler
//!
//! @history
//! - 2025-03-30  (Sicris Rey Embay):  Port to SysBIOS v6.83.x

#define QP_IMPL           // this is QP implementation
#include "qp_port.h"      // QP port
#include "qp_pkg.h"       // QP package-level interface
#include "qsafe.h"        // QP Functional Safety (FuSa) Subsystem
#ifdef Q_SPY              // QS software tracing enabled?
    #include "qs_port.h"  // QS port
    #include "qs_pkg.h"   // QS package-scope internal interface
#else
    #include "qs_dummy.h" // disable the QS software tracing
#endif // Q_SPY

Q_DEFINE_THIS_MODULE("qf_port")

// Local objects -------------------------------------------------------------
static Void taskFxn(UArg a0, UArg a1);  // SysBIOS task signature

//============================================================================
void QF_init(void) {
    // TODO
}
//............................................................................
int_t QF_run(void) {
    QF_onStartup(); // the startup callback (configure/enable interrupts)

    // produce the QS_QF_RUN trace record
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QS_BEGIN_PRE_(QS_QF_RUN, 0U)
    QS_END_PRE_()
    QF_CRIT_EXIT();

    BIOS_start();

    QF_CRIT_ENTRY();
    Q_ERROR_INCRIT(110); // BIOS_start() should never return
    QF_CRIT_EXIT();

    return 0;
}
//............................................................................
void QF_stop(void) {
    QF_onCleanup(); // cleanup callback
}

//............................................................................
static Void taskFxn(UArg a0, UArg a1) {
    QActive *act = (QActive *)a0;

    for (;;) // for-ever
    {
        QEvt const *e = QActive_get_(act); // wait for event
        QASM_DISPATCH(&act->super, e, act->prio); // dispatch to the SM
        QF_gc(e); // check if the event is garbage, and collect it if so
    }
}

//............................................................................
void QActive_start_(QActive * const me,
    QPrioSpec const prioSpec,
    QEvt const * * const qSto,
    uint_fast16_t const qLen,
    void * const stkSto,
    uint_fast16_t const stkSize,
    void const * const par)
{
    /// TODO
}
//............................................................................
void QActive_setAttr(QActive *const me, uint32_t attr1, void const *attr2) {
    /// TODO
}

//============================================================================
bool QActive_post_(QActive * const me, QEvt const * const e,
                   uint_fast16_t const margin, void const * const sender)
{
    bool status = false;
    /// TODO
    return status;
}
//............................................................................
void QActive_postLIFO_(QActive * const me, QEvt const * const e) {
    /// TODO
}
//............................................................................
QEvt const *QActive_get_(QActive * const me) {
    QEvt const *e;
    /// TODO
    return e;
}
