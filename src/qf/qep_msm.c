//$file${src::qf::qep_msm.c} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpc.qm
// File:  ${src::qf::qep_msm.c}
//
// This code has been generated by QM 6.2.0 <www.state-machine.com/qm>.
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// This code is covered by the following QP license:
// License #    : LicenseRef-QL-dual
// Issued to    : Any user of the QP/C real-time embedded framework
// Framework(s) : qpc
// Support ends : 2025-12-31
// License scope:
//
// Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
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
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//
//$endhead${src::qf::qep_msm.c} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
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

//============================================================================
//! @cond INTERNAL

Q_DEFINE_THIS_MODULE("qep_msm")

// top-state object for QMsm-style state machines
static struct QMState const l_msm_top_s = {
    (struct QMState *)0,
    Q_STATE_CAST(0),
    Q_ACTION_CAST(0),
    Q_ACTION_CAST(0),
    Q_ACTION_CAST(0)
};
//! @endcond

enum {
    // maximum depth of state nesting in a QMsm (including the top level)
    QMSM_MAX_NEST_DEPTH_  = 8,

    // maximum length of transition-action array
    QMSM_MAX_TRAN_LENGTH_ = 2*QMSM_MAX_NEST_DEPTH_,

    // maximum depth of entry levels in a MSM for tran. to history
    QMSM_MAX_ENTRY_DEPTH_ = 4
};

//============================================================================

//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 730U) || (QP_VERSION != ((QP_RELEASE^4294967295U)%0x2710U))
#error qpc version 7.3.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//$define${QEP::QMsm} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${QEP::QMsm} ...............................................................

//${QEP::QMsm::ctor} .........................................................
//! @protected @memberof QMsm
void QMsm_ctor(QMsm * const me,
    QStateHandler const initial)
{
    static struct QAsmVtable const vtable = { // QAsm virtual table
        &QMsm_init_,
        &QMsm_dispatch_,
        &QMsm_isIn_
    #ifdef Q_SPY
        ,&QMsm_getStateHandler_
    #endif
    };
    // do not call the QAsm_ctor() here
    me->super.vptr = &vtable;
    me->super.state.obj = &l_msm_top_s; // the current state (top)
    me->super.temp.fun  = initial;      // the initial tran. handler
}

//${QEP::QMsm::init_} ........................................................
//! @private @memberof QMsm
void QMsm_init_(
    QAsm * const me,
    void const * const e,
    uint_fast8_t const qsId)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(qsId);
    #endif

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    Q_REQUIRE_INCRIT(200, (me->vptr != (struct QAsmVtable *)0)
                      && (me->temp.fun != Q_STATE_CAST(0))
                      && (me->state.obj == &l_msm_top_s));
    QF_CRIT_EXIT();

    // execute the top-most initial tran.
    QState r = (*me->temp.fun)(me, Q_EVT_CAST(QEvt));

    QF_CRIT_ENTRY();
    // the top-most initial tran. must be taken
    Q_ASSERT_INCRIT(210, r == Q_RET_TRAN_INIT);

    QS_MEM_SYS();
    QS_BEGIN_PRE_(QS_QEP_STATE_INIT, qsId)
        QS_OBJ_PRE_(me); // this state machine object
        QS_FUN_PRE_(me->state.obj->stateHandler);          // source state
        QS_FUN_PRE_(me->temp.tatbl->target->stateHandler); // target state
    QS_END_PRE_()
    QS_MEM_APP();

    QF_CRIT_EXIT();

    // set state to the last tran. target
    me->state.obj = me->temp.tatbl->target;

    // drill down into the state hierarchy with initial transitions...
    int_fast8_t lbound = QMSM_MAX_NEST_DEPTH_; // fixed upper loop bound
    do {
        // execute the tran. table
        r = QMsm_execTatbl_(me, me->temp.tatbl, qsId);
        --lbound;
    } while ((r >= Q_RET_TRAN_INIT) && (lbound > 0));

    QF_CRIT_ENTRY();
    Q_ENSURE_INCRIT(290, lbound > 0);

    QS_MEM_SYS();
    QS_BEGIN_PRE_(QS_QEP_INIT_TRAN, qsId)
        QS_TIME_PRE_();    // time stamp
        QS_OBJ_PRE_(me);   // this state machine object
        QS_FUN_PRE_(me->state.obj->stateHandler); // the new current state
    QS_END_PRE_()
    QS_MEM_APP();

    QF_CRIT_EXIT();

    #ifndef Q_UNSAFE
    me->temp.uint = ~me->state.uint;
    #endif
}

//${QEP::QMsm::dispatch_} ....................................................
//! @private @memberof QMsm
void QMsm_dispatch_(
    QAsm * const me,
    QEvt const * const e,
    uint_fast8_t const qsId)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(qsId);
    #endif

    QMState const *s = me->state.obj; // store the current state
    QMState const *t = s;

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    Q_REQUIRE_INCRIT(300, QEvt_verify_(e));
    Q_INVARIANT_INCRIT(302, (s != (QMState *)0)
        && (me->state.uint == (uintptr_t)(~me->temp.uint)));

    QS_MEM_SYS();
    QS_BEGIN_PRE_(QS_QEP_DISPATCH, qsId)
        QS_TIME_PRE_();               // time stamp
        QS_SIG_PRE_(e->sig);          // the signal of the event
        QS_OBJ_PRE_(me);              // this state machine object
        QS_FUN_PRE_(s->stateHandler); // the current state handler
    QS_END_PRE_()
    QS_MEM_APP();

    QF_CRIT_EXIT();

    // scan the state hierarchy up to the top state...
    QState r;
    int_fast8_t lbound = QMSM_MAX_NEST_DEPTH_; // fixed upper loop bound
    do {
        r = (*t->stateHandler)(me, e); // call state handler function

        // event handled? (the most frequent case)
        if (r >= Q_RET_HANDLED) {
            break; // done scanning the state hierarchy
        }
        // event unhandled and passed to the superstate?
        else if (r == Q_RET_SUPER) {
            t = t->superstate; // advance to the superstate
        }
        // event unhandled and passed to a submachine superstate?
        else if (r == Q_RET_SUPER_SUB) {
            t = me->temp.obj; // current host state of the submachine
        }
        else { // event unhandled due to a guard?
            QF_CRIT_ENTRY();
            // event must be unhandled due to a guard evaluating to 'false'
            Q_ASSERT_INCRIT(310, r == Q_RET_UNHANDLED);

            QS_MEM_SYS();
            QS_BEGIN_PRE_(QS_QEP_UNHANDLED, qsId)
                QS_SIG_PRE_(e->sig);  // the signal of the event
                QS_OBJ_PRE_(me);      // this state machine object
                QS_FUN_PRE_(t->stateHandler); // the current state
            QS_END_PRE_()
            QS_MEM_APP();

            QF_CRIT_EXIT();

            t = t->superstate; // advance to the superstate
        }
        --lbound;
    } while ((t != (QMState *)0) && (lbound > 0));
    QF_CRIT_ENTRY();
    Q_ENSURE_INCRIT(320, lbound > 0);
    QF_CRIT_EXIT();

    if (r >= Q_RET_TRAN) { // any kind of tran. taken?
    #ifdef Q_SPY
        QMState const * const ts = t; // tran. source for QS tracing

        QF_CRIT_ENTRY();
        // the tran. source state must not be NULL
        Q_ASSERT_INCRIT(330, ts != (QMState *)0);
        QF_CRIT_EXIT();
    #endif // Q_SPY

        lbound = QMSM_MAX_NEST_DEPTH_; // fixed upper loop bound
        do {
            // save the tran-action table before it gets clobbered
            struct QMTranActTable const * const tatbl = me->temp.tatbl;
            union QAsmAttr tmp; // temporary to save intermediate values

            // was TRAN, TRAN_INIT, or TRAN_EP taken?
            if (r <= Q_RET_TRAN_EP) {
                me->temp.obj = (QMState *)0; // clear
                QMsm_exitToTranSource_(me, s, t, qsId);
                r = QMsm_execTatbl_(me, tatbl, qsId);
                s = me->state.obj;
            }
            // was a tran. segment to history taken?
            else if (r == Q_RET_TRAN_HIST) {
                tmp.obj = me->state.obj; // save history
                me->state.obj = s; // restore the original state
                QMsm_exitToTranSource_(me, s, t, qsId);
                (void)QMsm_execTatbl_(me, tatbl, qsId);
                r = QMsm_enterHistory_(me, tmp.obj, qsId);
                s = me->state.obj;
            }
            else {
                QF_CRIT_ENTRY();
                // must be tran. to exit point
                Q_ASSERT_INCRIT(340, r == Q_RET_TRAN_XP);
                QF_CRIT_EXIT();

                tmp.act = me->state.act; // save XP action
                me->state.obj = s; // restore the original state
                r = (*tmp.act)(me); // execute the XP action
                if (r == Q_RET_TRAN) { // XP -> TRAN ?
    #ifdef Q_SPY
                    tmp.tatbl = me->temp.tatbl; // save me->temp
    #endif // Q_SPY
                    QMsm_exitToTranSource_(me, s, t, qsId);
                    // take the tran-to-XP segment inside submachine
                    (void)QMsm_execTatbl_(me, tatbl, qsId);
                    s = me->state.obj;
    #ifdef Q_SPY
                    me->temp.tatbl = tmp.tatbl; // restore me->temp
    #endif // Q_SPY
                }
                else if (r == Q_RET_TRAN_HIST) { // XP -> HIST ?
                    tmp.obj = me->state.obj; // save the history
                    me->state.obj = s; // restore the original state
                    s = me->temp.obj; // save me->temp
                    QMsm_exitToTranSource_(me, me->state.obj, t, qsId);
                    // take the tran-to-XP segment inside submachine
                    (void)QMsm_execTatbl_(me, tatbl, qsId);
    #ifdef Q_SPY
                    me->temp.obj = s; // restore me->temp
    #endif // Q_SPY
                    s = me->state.obj;
                    me->state.obj = tmp.obj; // restore the history
                }
                else {
                    QF_CRIT_ENTRY();
                    // TRAN_XP must NOT be followed by any other tran. type
                    Q_ASSERT_INCRIT(350, r < Q_RET_TRAN);
                    QF_CRIT_EXIT();
                }
            }

            t = s; // set target to the current state
           --lbound;
        } while ((r >= Q_RET_TRAN) && (lbound > 0));

        QF_CRIT_ENTRY();
        Q_ENSURE_INCRIT(360, lbound > 0);

        QS_MEM_SYS();
        QS_BEGIN_PRE_(QS_QEP_TRAN, qsId)
            QS_TIME_PRE_();                 // time stamp
            QS_SIG_PRE_(e->sig);            // the signal of the event
            QS_OBJ_PRE_(me);                // this state machine object
            QS_FUN_PRE_(ts->stateHandler);  // the tran. source
            QS_FUN_PRE_(s->stateHandler);   // the new active state
        QS_END_PRE_()
        QS_MEM_APP();

        QF_CRIT_EXIT();
    }

    #ifdef Q_SPY
    // was the event handled?
    else if (r == Q_RET_HANDLED) {
        QF_CRIT_ENTRY();
        // internal tran. source can't be NULL
        Q_ASSERT_INCRIT(380, t != (QMState *)0);

        QS_MEM_SYS();
        QS_BEGIN_PRE_(QS_QEP_INTERN_TRAN, qsId)
            QS_TIME_PRE_();                 // time stamp
            QS_SIG_PRE_(e->sig);            // the signal of the event
            QS_OBJ_PRE_(me);                // this state machine object
            QS_FUN_PRE_(t->stateHandler);   // the source state
        QS_END_PRE_()
        QS_MEM_APP();

        QF_CRIT_EXIT();
    }
    // event bubbled to the 'top' state?
    else if (t == (QMState *)0) {
        QS_CRIT_ENTRY();
        // current state can't be NULL
        Q_ASSERT_INCRIT(390, s != (QMState *)0);

        QS_MEM_SYS();
        QS_BEGIN_PRE_(QS_QEP_IGNORED, qsId)
            QS_TIME_PRE_();                 // time stamp
            QS_SIG_PRE_(e->sig);            // the signal of the event
            QS_OBJ_PRE_(me);                // this state machine object
            QS_FUN_PRE_(s->stateHandler);   // the current state
        QS_END_PRE_()
        QS_MEM_APP();
        QS_CRIT_EXIT();
    }
    #endif // Q_SPY
    else {
        // empty
    }

    #ifndef Q_UNSAFE
    me->temp.uint = ~me->state.uint;
    #endif
}

//${QEP::QMsm::isIn_} ........................................................
//! @private @memberof QMsm
bool QMsm_isIn_(
    QAsm * const me,
    QStateHandler const state)
{
    bool inState = false; // assume that this SM is not in 'state'

    QMState const *s = me->state.obj;
    int_fast8_t lbound = QMSM_MAX_NEST_DEPTH_; // fixed upper loop bound
    for (; (s != (QMState *)0) && (lbound > 0); --lbound) {
        if (s->stateHandler == state) { // match found?
            inState = true;
            break;
        }
        else {
            s = s->superstate; // advance to the superstate
        }
    }

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    Q_ENSURE_INCRIT(490, lbound > 0);
    QF_CRIT_EXIT();

    return inState;
}

//${QEP::QMsm::isInState} ....................................................
//! @private @memberof QMsm
bool QMsm_isInState(QMsm const * const me,
    QMState const * const stateObj)
{
    bool inState = false; // assume that this SM is not in 'state'

    QMState const *s = me->super.state.obj;
    int_fast8_t lbound = QMSM_MAX_NEST_DEPTH_; // fixed upper loop bound
    for (; (s != (QMState *)0) && (lbound > 0); --lbound) {
        if (s == stateObj) { // match found?
            inState = true;
            break;
        }
        else {
            s = s->superstate; // advance to the superstate
        }
    }

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    Q_ENSURE_INCRIT(590, lbound > 0);
    QF_CRIT_EXIT();

    return inState;
}

//${QEP::QMsm::childStateObj} ................................................
//! @public @memberof QMsm
QMState const * QMsm_childStateObj(QMsm const * const me,
    QMState const * const parent)
{
    QMState const *child = me->super.state.obj;
    bool isFound = false; // start with the child not found
    QMState const *s;

    int_fast8_t lbound = QMSM_MAX_NEST_DEPTH_; // fixed upper loop bound
    for (s = me->super.state.obj;
         (s != (QMState *)0) && (lbound > 0);
         s = s->superstate)
    {
        if (s == parent) {
            isFound = true; // child is found
            break;
        }
        else {
            child = s;
        }
        --lbound;
    }
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    Q_ENSURE_INCRIT(680, lbound > 0);
    QF_CRIT_EXIT();

    if (!isFound) { // still not found?
        lbound = QMSM_MAX_NEST_DEPTH_; // fixed upper loop bound
        for (s = me->super.temp.obj;
             (s != (QMState *)0) && (lbound > 0);
             s = s->superstate)
        {
            if (s == parent) {
                isFound = true; // child is found
                break;
            }
            else {
                child = s;
            }
            --lbound;
        }
    }

    QF_CRIT_ENTRY();
    // NOTE: the following postcondition can only succeed when
    // (lbound > 0), so no extra check is necessary.
    Q_ENSURE_INCRIT(690, isFound);
    QF_CRIT_EXIT();

    return child; // return the child
}

//${QEP::QMsm::execTatbl_} ...................................................
//! @private @memberof QMsm
QState QMsm_execTatbl_(
    QAsm * const me,
    QMTranActTable const * const tatbl,
    uint_fast8_t const qsId)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(qsId);
    #endif

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    // precondition:
    // - the tran-action table pointer must not be NULL
    Q_REQUIRE_INCRIT(700, tatbl != (struct QMTranActTable *)0);
    QF_CRIT_EXIT();

    QState r = Q_RET_NULL;
    int_fast8_t lbound = QMSM_MAX_TRAN_LENGTH_; // fixed upper loop bound
    QActionHandler const *a = &tatbl->act[0];
    for (; (*a != Q_ACTION_CAST(0)) && (lbound > 0); ++a) {
        r = (*(*a))(me); // call the action through the 'a' pointer
        --lbound;
    #ifdef Q_SPY
        QS_CRIT_ENTRY();
        QS_MEM_SYS();
        if (r == Q_RET_ENTRY) {
            QS_BEGIN_PRE_(QS_QEP_STATE_ENTRY, qsId)
                QS_OBJ_PRE_(me); // this state machine object
                QS_FUN_PRE_(me->temp.obj->stateHandler); // entered state
            QS_END_PRE_()
        }
        else if (r == Q_RET_EXIT) {
            QS_BEGIN_PRE_(QS_QEP_STATE_EXIT, qsId)
                QS_OBJ_PRE_(me); // this state machine object
                QS_FUN_PRE_(me->temp.obj->stateHandler); // exited state
            QS_END_PRE_()
        }
        else if (r == Q_RET_TRAN_INIT) {
            QS_BEGIN_PRE_(QS_QEP_STATE_INIT, qsId)
                QS_OBJ_PRE_(me); // this state machine object
                QS_FUN_PRE_(tatbl->target->stateHandler);          // source
                QS_FUN_PRE_(me->temp.tatbl->target->stateHandler); // target
            QS_END_PRE_()
        }
        else if (r == Q_RET_TRAN_EP) {
            QS_BEGIN_PRE_(QS_QEP_TRAN_EP, qsId)
                QS_OBJ_PRE_(me); // this state machine object
                QS_FUN_PRE_(tatbl->target->stateHandler);          // source
                QS_FUN_PRE_(me->temp.tatbl->target->stateHandler); // target
            QS_END_PRE_()
        }
        else if (r == Q_RET_TRAN_XP) {
            QS_BEGIN_PRE_(QS_QEP_TRAN_XP, qsId)
                QS_OBJ_PRE_(me); // this state machine object
                QS_FUN_PRE_(tatbl->target->stateHandler);          // source
                QS_FUN_PRE_(me->temp.tatbl->target->stateHandler); // target
            QS_END_PRE_()
        }
        else {
            // empty
        }
        QS_MEM_APP();
        QS_CRIT_EXIT();
    #endif // Q_SPY
    }
    QF_CRIT_ENTRY();
    // NOTE: the following postcondition can only succeed when
    // (lbound > 0), so no extra check is necessary.
    Q_ENSURE_INCRIT(790, *a == Q_ACTION_CAST(0));
    QF_CRIT_EXIT();

    me->state.obj = (r >= Q_RET_TRAN)
        ? me->temp.tatbl->target
        : tatbl->target;
    return r;
}

//${QEP::QMsm::exitToTranSource_} ............................................
//! @private @memberof QMsm
void QMsm_exitToTranSource_(
    QAsm * const me,
    QMState const * const cs,
    QMState const * const ts,
    uint_fast8_t const qsId)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(qsId);
    #endif

    QF_CRIT_STAT

    // exit states from the current state to the tran. source state
    QMState const *s = cs;
    int_fast8_t lbound = QMSM_MAX_NEST_DEPTH_; // fixed upper loop bound
    for (; (s != ts) && (lbound > 0); --lbound) {
        // exit action provided in state 's'?
        if (s->exitAction != Q_ACTION_CAST(0)) {
            // execute the exit action
            (void)(*s->exitAction)(me);

            QS_CRIT_ENTRY();
            QS_MEM_SYS();
            QS_BEGIN_PRE_(QS_QEP_STATE_EXIT, qsId)
                QS_OBJ_PRE_(me);              // this state machine object
                QS_FUN_PRE_(s->stateHandler); // the exited state handler
            QS_END_PRE_()
            QS_MEM_APP();
            QS_CRIT_EXIT();
        }

        s = s->superstate; // advance to the superstate

        if (s == (QMState *)0) { // reached the top of a submachine?
            s = me->temp.obj; // the superstate from QM_SM_EXIT()
            QF_CRIT_ENTRY();
            Q_ASSERT_INCRIT(880, s != (QMState *)0); // must be valid
            QF_CRIT_EXIT();
        }
    }
    QF_CRIT_ENTRY();
    Q_ENSURE_INCRIT(890, lbound > 0);
    QF_CRIT_EXIT();
}

//${QEP::QMsm::enterHistory_} ................................................
//! @private @memberof QMsm
QState QMsm_enterHistory_(
    QAsm * const me,
    QMState const *const hist,
    uint_fast8_t const qsId)
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(qsId);
    #endif

    QMState const *s = hist;
    QMState const *ts = me->state.obj; // tran. source
    QMState const *epath[QMSM_MAX_ENTRY_DEPTH_];

    QF_CRIT_STAT

    QS_CRIT_ENTRY();
    QS_MEM_SYS();
    QS_BEGIN_PRE_(QS_QEP_TRAN_HIST, qsId)
        QS_OBJ_PRE_(me);                 // this state machine object
        QS_FUN_PRE_(ts->stateHandler);   // source state handler
        QS_FUN_PRE_(hist->stateHandler); // target state handler
    QS_END_PRE_()
    QS_MEM_APP();
    QS_CRIT_EXIT();

    int_fast8_t i = 0; // tran. entry path index
    while ((s != ts) && (i < QMSM_MAX_ENTRY_DEPTH_)) {
        if (s->entryAction != Q_ACTION_CAST(0)) {
            epath[i] = s;
            ++i;
        }
        s = s->superstate;
        if (s == (QMState *)0) {
            ts = s; // force exit from the for-loop
        }
    }
    QF_CRIT_ENTRY();
    Q_ASSERT_INCRIT(910, s == ts);
    QF_CRIT_EXIT();

    // retrace the entry path in reverse (desired) order...
    while (i > 0) {
        --i;
        (void)(*epath[i]->entryAction)(me); // run entry action in epath[i]

        QS_CRIT_ENTRY();
        QS_MEM_SYS();
        QS_BEGIN_PRE_(QS_QEP_STATE_ENTRY, qsId)
            QS_OBJ_PRE_(me);
            QS_FUN_PRE_(epath[i]->stateHandler); // entered state handler
        QS_END_PRE_()
        QS_MEM_APP();
        QS_CRIT_EXIT();
    }

    me->state.obj = hist; // set current state to the tran. target

    // initial tran. present?
    QState r;
    if (hist->initAction != Q_ACTION_CAST(0)) {
        r = (*hist->initAction)(me); // execute the tran. action
    }
    else {
        r = Q_RET_NULL;
    }

    return r;
}
//$enddef${QEP::QMsm} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
