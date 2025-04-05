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
#include "string.h"       // For memset
#include "xdc/runtime/System.h" // For System_snprintf

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
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    // precondition:
    // - queue storage must be provided
    // - queue size must be provided
    // - stack storage must be provided
    // - stack size must be provided
    Q_REQUIRE_INCRIT(200,
        (qSto != (QEvt const **)0) && (qLen > 0U)
        && (stkSto != (void *)0) && (stkSize > 0U));
    QF_CRIT_EXIT();

    Error_Block eb;
    Error_init(&eb);

    // QF-priority of the AO
    me->prio  = (uint8_t)(prioSpec & 0xFFU);
    QActive_register_(me); // register this AO

    // create SysBIOS elements for Event Queue
    // Event data queue
    Queue_Params queueParams;
    Queue_Params_init(&queueParams);
    queueParams.instance->name = me->osObject.name;
    Queue_construct(&me->eQueue.data_queue, &queueParams);
    // Event free queue
    Queue_Params_init(&queueParams);
    queueParams.instance->name = me->osObject.name;
    Queue_construct(&me->eQueue.free_queue, &queueParams);
    // Event semaphore
    Semaphore_Params semParams;
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_COUNTING;
    semParams.instance->name = me->osObject.name;
    Semaphore_construct(&me->eQueue.data_sem, 0, &semParams);

    Queue_Handle free_queue = Queue_handle(&me->eQueue.free_queue);
    event_t * pElem = (event_t *)(qSto);  // messy recasting!
    for(uint16_t i = 0; i < qLen; i++) {
        Queue_put(free_queue, &(pElem[i].queue_elem));
    }
    me->eQueue.numFreeMsgs = qLen;

    // top-most initial tran. (virtual call)
    (*me->super.vptr->init)(&me->super, par, me->prio);
    QS_FLUSH(); // flush the QS trace buffer

    // create SysBIOS task
    Task_Params taskParams;
    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = me->prio;
    taskParams.stackSize = stkSize;
    taskParams.stack = stkSto;
    taskParams.arg0 = (UArg)me;
    taskParams.instance->name = me->osObject.name;
    Task_construct(&me->osObject.static_task, &taskFxn, &taskParams, &eb);
    me->thread = Task_handle(&me->osObject.static_task);
    QF_CRIT_ENTRY();
    Q_ASSERT_INCRIT(220, (me->thread != (Task_Handle)0) &&
                         (Error_check(&eb) == FALSE));
    QF_CRIT_EXIT();
}
//............................................................................
void QActive_setAttr(QActive *const me, uint32_t attr1, void const *attr2) {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    switch(attr1) {
        case TASK_NAME_ATTR: {
            memset(me->osObject.name, 0, sizeof(me->osObject.name));
            if(attr2 != (char *)0) {
                System_snprintf(me->osObject.name, MAX_LEN_NAME, "%s", attr2);
            } else {
                System_snprintf(me->osObject.name, MAX_LEN_NAME, "AO_no_name");
            }
            break;
        }
        default: {
            break;
        }
    }
    QF_CRIT_EXIT();
}

//============================================================================
bool QActive_post_(QActive * const me, QEvt const * const e,
                   uint_fast16_t const margin, void const * const sender)
{
#ifndef Q_SPY
    Q_UNUSED_PAR(sender);
#endif

    QF_CRIT_STAT
    QF_CRIT_ENTRY();

    const int16_t nFree = me->eQueue.numFreeMsgs;

    bool status = false;
    if (margin == QF_NO_MARGIN) {
        if (nFree > 0U) {
            status = true; // can post
        } else {
            status = false; // cannot post
            Q_ERROR_INCRIT(510); // must be able to post the event
        }
    } else if (nFree > margin) {
        status = true; // can post
    } else {
        status = false; // cannot post
    }

    if (status) { // can post the event?
        QS_BEGIN_PRE_(QS_QF_ACTIVE_POST, me->prio)
            QS_TIME_PRE_();      // timestamp
            QS_OBJ_PRE_(sender); // the sender object
            QS_SIG_PRE_(e->sig); // the signal of the event
            QS_OBJ_PRE_(me);     // this active object (recipient)
            QS_2U8_PRE_(QEvt_getPoolNum_(e), e->refCtr_); // poolNum & refCtr
            QS_EQC_PRE_((QEQueueCtr)nFree); // # free entries
            QS_EQC_PRE_(0U);     // min # free entries (unknown)
        QS_END_PRE_()

        if (QEvt_getPoolNum_(e) != 0U) { // is it a pool event?
            QEvt_refCtr_inc_(e); // increment the reference counter
        }

        event_t * ptr_elem;
        Queue_Handle data_queue = Queue_handle(&me->eQueue.data_queue);
        Queue_Handle free_queue = Queue_handle(&me->eQueue.free_queue);
        Semaphore_Handle data_sem = Semaphore_handle(&me->eQueue.data_sem);

        /* Get element from free queue */
        ptr_elem = Queue_dequeue(free_queue);
        Q_ASSERT_INCRIT(__LINE__, ptr_elem != (event_t *)free_queue);
        me->eQueue.numFreeMsgs--;
        ptr_elem->ptr_event = (QEvt *)e;
        /* Put element to tail of data queue */
        Queue_enqueue(data_queue, &(ptr_elem->queue_elem));
        /* Post to data semaphore */
        Semaphore_post(data_sem);

    } else {
        QS_BEGIN_PRE_(QS_QF_ACTIVE_POST_ATTEMPT, me->prio)
            QS_TIME_PRE_();      // timestamp
            QS_OBJ_PRE_(sender); // the sender object
            QS_SIG_PRE_(e->sig); // the signal of the event
            QS_OBJ_PRE_(me);     // this active object (recipient)
            QS_2U8_PRE_(QEvt_getPoolNum_(e), e->refCtr_); // poolNum & refCtr
            QS_EQC_PRE_((QEQueueCtr)nFree); // # free entries
            QS_EQC_PRE_(margin); // margin requested
        QS_END_PRE_()
    }
    QF_CRIT_EXIT();

    return status;
}
//............................................................................
void QActive_postLIFO_(QActive * const me, QEvt const * const e) {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();

    QS_BEGIN_PRE_(QS_QF_ACTIVE_POST_LIFO, me->prio)
        QS_TIME_PRE_();          // timestamp
        QS_SIG_PRE_(e->sig);     // the signal of this event
        QS_OBJ_PRE_(me);         // this active object
        QS_2U8_PRE_(QEvt_getPoolNum_(e), e->refCtr_); // poolNum & refCtr
        QS_EQC_PRE_((QEQueueCtr)me->eQueue.numFreeMsgs); // # free
        QS_EQC_PRE_(0U);         // min # free entries (unknown)
    QS_END_PRE_()

    if (QEvt_getPoolNum_(e) != 0U) { // is it a pool event?
        QEvt_refCtr_inc_(e); // increment the reference counter
    }

    event_t * ptr_elem;
    Queue_Handle data_queue = Queue_handle(&me->eQueue.data_queue);
    Queue_Handle free_queue = Queue_handle(&me->eQueue.free_queue);
    Semaphore_Handle data_sem = Semaphore_handle(&me->eQueue.data_sem);

    /* Get element from free queue */
    ptr_elem = Queue_dequeue(free_queue);
    Q_ASSERT_INCRIT(__LINE__, ptr_elem != (event_t *)free_queue);
    me->eQueue.numFreeMsgs--;
    ptr_elem->ptr_event = (QEvt *)e;
    /* Put element to front of data queue */
    Queue_putHead(data_queue, &(ptr_elem->queue_elem));
    /* Post to data semaphore */
    Semaphore_post(data_sem);


    QF_CRIT_EXIT();
}
//............................................................................
QEvt const * QActive_get_(QActive * const me) {
    event_t * ptr_elem;
    QEvt * e;
    Semaphore_Handle data_sem = Semaphore_handle(&me->eQueue.data_sem);
    Queue_Handle data_queue = Queue_handle(&me->eQueue.data_queue);
    Queue_Handle free_queue = Queue_handle(&me->eQueue.free_queue);

    bool ret = Semaphore_pend(data_sem, BIOS_WAIT_FOREVER);
    Q_ASSERT(true == ret);

    QS_CRIT_STAT
    QS_CRIT_ENTRY();

    /* Get element from front of data queue */
    ptr_elem = Queue_get(data_queue);
    Q_ASSERT_INCRIT(__LINE__,(ptr_elem != (event_t *)data_queue));
    e = ptr_elem->ptr_event;
    Queue_enqueue(free_queue, &(ptr_elem->queue_elem));
    me->eQueue.numFreeMsgs++;

    QS_BEGIN_PRE_(QS_QF_ACTIVE_GET, me->prio)
        QS_TIME_PRE_();          // timestamp
        QS_SIG_PRE_(e->sig);     // the signal of this event
        QS_OBJ_PRE_(me);         // this active object
        QS_2U8_PRE_(QEvt_getPoolNum_(e), e->refCtr_); // poolNum & refCtr
        QS_EQC_PRE_((QEQueueCtr)(me->eQueue.numFreeMsgs)); // # free
    QS_END_PRE_()

    QS_CRIT_EXIT();
    return e;
}
