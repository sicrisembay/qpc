//$file${include::qmpool.h} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpc.qm
// File:  ${include::qmpool.h}
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
//$endhead${include::qmpool.h} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#ifndef QMPOOL_H_
#define QMPOOL_H_

#ifndef QF_MPOOL_SIZ_SIZE
    #define QF_MPOOL_SIZ_SIZE 2U
#endif
#ifndef QF_MPOOL_CTR_SIZE
    #define QF_MPOOL_CTR_SIZE 2U
#endif

#if (QF_MPOOL_SIZ_SIZE == 1U)
    typedef uint8_t QMPoolSize;
#elif (QF_MPOOL_SIZ_SIZE == 2U)
    typedef uint16_t QMPoolSize;
#elif (QF_MPOOL_SIZ_SIZE == 4U)
    typedef uint32_t QMPoolSize;
#else
    #error "QF_MPOOL_SIZ_SIZE defined incorrectly, expected 1U, 2U, or 4U"
#endif

#if (QF_MPOOL_CTR_SIZE == 1U)
    typedef uint8_t QMPoolCtr;
#elif (QF_MPOOL_CTR_SIZE == 2U)
    typedef uint16_t QMPoolCtr;
#elif (QF_MPOOL_CTR_SIZE == 4U)
    typedef uint32_t QMPoolCtr;
#else
    #error "QF_MPOOL_CTR_SIZE defined incorrectly, expected 1U, 2U, or 4U"
#endif

#define QF_MPOOL_EL(evType_) struct { \
    QFreeBlock sto_[((sizeof(evType_) - 1U) \
                      / sizeof(QFreeBlock)) + 1U]; }

//$declare${QF::QFreeBlock} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${QF::QFreeBlock} ..........................................................
//! @struct QFreeBlock
typedef struct QFreeBlock {
// private:

    //! @private @memberof QFreeBlock
    struct QFreeBlock * next;

#ifndef Q_UNSAFE
    //! @private @memberof QFreeBlock
    uintptr_t next_dis;
#endif // ndef Q_UNSAFE
} QFreeBlock;
//$enddecl${QF::QFreeBlock} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$declare${QF::QMPool} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

//${QF::QMPool} ..............................................................
//! @class QMPool
typedef struct {
// private:

    //! @private @memberof QMPool
    QFreeBlock * start;

    //! @private @memberof QMPool
    QFreeBlock * end;

    //! @private @memberof QMPool
    QFreeBlock * volatile free_head;

    //! @private @memberof QMPool
    QMPoolSize blockSize;

    //! @private @memberof QMPool
    QMPoolCtr nTot;

    //! @private @memberof QMPool
    QMPoolCtr volatile nFree;

    //! @private @memberof QMPool
    QMPoolCtr nMin;
} QMPool;

// public:

//! @public @memberof QMPool
void QMPool_init(QMPool * const me,
    void * const poolSto,
    uint_fast32_t const poolSize,
    uint_fast16_t const blockSize);

//! @public @memberof QMPool
void * QMPool_get(QMPool * const me,
    uint_fast16_t const margin,
    uint_fast8_t const qsId);

//! @public @memberof QMPool
void QMPool_put(QMPool * const me,
    void * const block,
    uint_fast8_t const qsId);
//$enddecl${QF::QMPool} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#endif  // QMPOOL_H_
