/*
 * Copyright (c) 2018 hackin zhao.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define QP_IMPL
#include "qf_port.h"
#include <qf_pkg.h>
#include <qassert.h>
#ifdef Q_SPY
    #include "qs_port.h"
#else
    #include "qs_dummy.h"
#endif /* Q_SPY */

Q_DEFINE_THIS_MODULE("qf_port")

K_THREAD_STACK_MEMBER(QP_task_stack, CONFIG_QP_TASK_STACK_SIZE);
struct k_thread QP_thread;

static void task_function(void *args);

void QF_init(void)
{
}

int_t QF_run(void)
{
	QF_onStartup();         /* the startup callback (configure/enable interrupts) */
	vTaskStartScheduler();  /* start the FreeRTOS scheduler */
	Q_ERROR_ID(110);        /* the FreeRTOS scheduler should never return */
	return (int_t)0;        /* dummy return to make the compiler happy */
}

void QF_stop(void)
{
	QF_onCleanup(); /* cleanup callback */
}

void QActive_start_(QActive *const me, uint_fast8_t prio,
		    QEvt const *qSto[], uint_fast16_t qLen,
		    void *stkSto, uint_fast16_t stkSize,
		    QEvt const *ie)
{
	/* task name provided by the user in QF_setTaskName() or default name */
	char_t const *taskName = (me->thread.pxDummy1 != (void *)0)
				 ? (char_t const *)me->thread.pxDummy1
				 : (char_t const *)"AO";

	Q_REQUIRE_ID(200, (prio < configMAX_PRIORITIES) /* not exceeding max */
		     && (qSto != (QEvt const **)0)      /* queue storage must be provided */
		     && (qLen > (uint_fast16_t)0)       /* queue size must be provided */
		     && (stkSto != (void *)0)           /* stack storage must be provided */
		     && (stkSize > (uint_fast16_t)0));  /* stack size must be provided */

	/* create the event queue for the AO */
	QEQueue_init(&me->eQueue, qSto, qLen);

	me->prio = prio;                /* save the QF priority */
	QF_add_(me);                    /* make QF aware of this active object */
	QHSM_INIT(&me->super, ie);      /* take the top-most initial tran. */
	QS_FLUSH();

	k_thread_create(&QP_thread, QP_task_stack,
			CONFIG_QP_TASK_STACK_SIZE,
			(k_thread_entry_t)task_function,
			(void *)me, NULL, NULL, K_PRIO_COOP(2), 0, 0);
}

void QActive_stop(QActive *const me)
{
	me->prio = (uint8_t)0; /* stop the thread loop */
}

void QActive_setAttr(QActive *const me, uint32_t attr1, void const *attr2)
{
	/* this function must be called before QACTIVE_START(),
	 * which implies that me->thread.pxDummy1 must not be used yet;
	 */
	Q_REQUIRE_ID(300, me->thread.pxDummy1 == (void *)0);
	switch (attr1) {
	case TASK_NAME_ATTR:
		/* temporarily store the name */
		me->thread.pxDummy1 = (void *)attr2; /* cast 'const' away */
		break;
	}
}

static void task_function(void *args)
{
	QActive *act = (QActive *)args;

	while (act->prio != (uint8_t)0) {
		QEvt const *e = QActive_get_(act);
		QHSM_DISPATCH(&act->super, e);
		QF_gc(e); /* check if the event is garbage, and collect it if so */
	}

	QF_remove_(act);
	k_thread_cancel(QP_thread);
}

/*==========================================================================*/
/* The "FromISR" QP APIs for the FreeRTOS port... */
#ifdef Q_SPY
bool QActive_postFromISR_(QActive *const me, QEvt const *const e,
			  uint_fast16_t const margin,
			  BaseType_t *const pxHigherPriorityTaskWoken,
			  void const *const sender)
#else
bool QActive_postFromISR_(QActive *const me, QEvt const *const e,
			  uint_fast16_t const margin,
			  BaseType_t *const pxHigherPriorityTaskWoken)
#endif
{
	QEQueueCtr nFree; /* temporary to avoid UB for volatile access */
	bool status;
	UBaseType_t uxSavedInterruptState;

	/** @pre event pointer must be valid */
	Q_REQUIRE_ID(400, e != (QEvt const *)0);

	uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();
	nFree = me->eQueue.nFree; /* get volatile into the temporary */

	if (margin == QF_NO_MARGIN) {
		if (nFree > (QEQueueCtr)0) {
			status = true;          /* can post */
		} else {
			status = false;         /* cannot post */
			Q_ERROR_ID(410);        /* must be able to post the event */
		}
	} else if (nFree > (QEQueueCtr)margin) {
		status = true;  /* can post */
	} else {
		status = false; /* cannot post */
	}

	if (status) { /* can post the event? */

		QS_BEGIN_NOCRIT_(QS_QF_ACTIVE_POST_FIFO,
				 QS_priv_.locFilter[AO_OBJ], me)
		QS_TIME_();                             /* timestamp */
		QS_OBJ_(sender);                        /* the sender object */
		QS_SIG_(e->sig);                        /* the signal of the event */
		QS_OBJ_(me);                            /* this active object (recipient) */
		QS_2U8_(e->poolId_, e->refCtr_);        /* pool Id & ref Count */
		QS_EQC_(nFree);                         /* number of free entries */
		QS_EQC_(me->eQueue.nMin);               /* min number of free entries */
		QS_END_NOCRIT_()

		/* is it a pool event? */
		if (e->poolId_ != (uint8_t)0) {
			QF_EVT_REF_CTR_INC_(e); /* increment the reference counter */
		}

		--nFree;                                /* one free entry just used up */
		me->eQueue.nFree = nFree;               /* update the volatile */
		if (me->eQueue.nMin > nFree) {
			me->eQueue.nMin = nFree;        /* update minimum so far */
		}

		/* empty queue? */
		if (me->eQueue.frontEvt == (QEvt const *)0) {
			me->eQueue.frontEvt = e; /* deliver event directly */
			taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);

			/* signal the event queue */
			vTaskNotifyGiveFromISR((TaskHandle_t)&me->thread,
					       pxHigherPriorityTaskWoken);
		}
		/* queue is not empty, insert event into the ring-buffer */
		else {
			/* insert event into the ring buffer (FIFO) */
			QF_PTR_AT_(me->eQueue.ring, me->eQueue.head) = e;
			if (me->eQueue.head == (QEQueueCtr)0) {         /* need to wrap head? */
				me->eQueue.head = me->eQueue.end;       /* wrap around */
			}
			--me->eQueue.head;                              /* advance the head (counter clockwise) */
			taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);
		}
	} else {

		QS_BEGIN_NOCRIT_(QS_QF_ACTIVE_POST_ATTEMPT,
				 QS_priv_.locFilter[AO_OBJ], me)
		QS_TIME_();                             /* timestamp */
		QS_OBJ_(sender);                        /* the sender object */
		QS_SIG_(e->sig);                        /* the signal of the event */
		QS_OBJ_(me);                            /* this active object (recipient) */
		QS_2U8_(e->poolId_, e->refCtr_);        /* pool Id & ref Count */
		QS_EQC_(nFree);                         /* number of free entries */
		QS_EQC_(margin);                        /* margin requested */
		QS_END_NOCRIT_()

		taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);

		QF_gcFromISR(e); /* recycle the event to avoid a leak */
	}

	return status;
}
/*..........................................................................*/
#ifdef Q_SPY
void QF_publishFromISR_(QEvt const *const e,
			BaseType_t *const pxHigherPriorityTaskWoken,
			void const *const sender)
#else
void QF_publishFromISR_(QEvt const *const e,
			BaseType_t *const pxHigherPriorityTaskWoken)
#endif
{
	QPSet subscrList; /* local, modifiable copy of the subscriber list */
	UBaseType_t uxSavedInterruptState;

	/** @pre the published signal must be within the configured range */
	Q_REQUIRE_ID(500, e->sig < (QSignal)QF_maxPubSignal_);

	uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();

	QS_BEGIN_NOCRIT_(QS_QF_PUBLISH, (void *)0, (void *)0)
	QS_TIME_();                             /* the timestamp */
	QS_OBJ_(sender);                        /* the sender object */
	QS_SIG_(e->sig);                        /* the signal of the event */
	QS_2U8_(e->poolId_, e->refCtr_);        /* pool Id & ref Count of the event */
	QS_END_NOCRIT_()

	/* is it a dynamic event? */
	if (e->poolId_ != (uint8_t)0) {
		/* NOTE: The reference counter of a dynamic event is incremented to
		 * prevent premature recycling of the event while the multicasting
		 * is still in progress. At the end of the function, the garbage
		 * collector step (QF_gcFromISR()) decrements the reference counter and
		 * recycles the event if the counter drops to zero. This covers the
		 * case when the event was published without any subscribers.
		 */
		QF_EVT_REF_CTR_INC_(e);
	}

	/* make a local, modifiable copy of the subscriber list */
	subscrList = QF_PTR_AT_(QF_subscrList_, e->sig);
	taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);

	if (QPSet_notEmpty(&subscrList)) { /* any subscribers? */
		uint_fast8_t p;

		QPSet_findMax(&subscrList, p); /* the highest-prio subscriber */

		/* no need to lock the scheduler in the ISR context */
		do {    /* loop over all subscribers */
			/* the prio of the AO must be registered with the framework */
			Q_ASSERT_ID(510, QF_active_[p] != (QActive *)0);

			/* QACTIVE_POST_FROM_ISR() asserts if the queue overflows */
			QACTIVE_POST_FROM_ISR(QF_active_[p], e,
					      pxHigherPriorityTaskWoken, sender);

			QPSet_remove(&subscrList, p);           /* remove the handled subscriber */
			if (QPSet_notEmpty(&subscrList)) {      /* still more subscribers? */
				QPSet_findMax(&subscrList, p);  /* highest-prio subscriber */
			} else {
				p = (uint_fast8_t)0;            /* no more subscribers */
			}
		} while (p != (uint_fast8_t)0);
		/* no need to unlock the scheduler in the ISR context */
	}

	/* The following garbage collection step decrements the reference counter
	 * and recycles the event if the counter drops to zero. This covers both
	 * cases when the event was published with or without any subscribers.
	 */
	QF_gcFromISR(e);
}
/*..........................................................................*/
#ifdef Q_SPY
void QF_tickXFromISR_(uint_fast8_t const tickRate,
		      BaseType_t *const pxHigherPriorityTaskWoken,
		      void const *const sender)
#else
void QF_tickXFromISR_(uint_fast8_t const tickRate,
		      BaseType_t *const pxHigherPriorityTaskWoken)
#endif
{
	QTimeEvt *prev = &QF_timeEvtHead_[tickRate];
	UBaseType_t uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();

	QS_BEGIN_NOCRIT_(QS_QF_TICK, (void *)0, (void *)0)
	QS_TEC_((QTimeEvtCtr)(++prev->ctr));    /* tick ctr */
	QS_U8_((uint8_t)tickRate);              /* tick rate */
	QS_END_NOCRIT_()

	/* scan the linked-list of time events at this rate... */
	for (;;) {
		QTimeEvt *t = prev->next; /* advance down the time evt. list */

		/* end of the list? */
		if (t == (QTimeEvt *)0) {

			/* any new time events armed since the last run of QF_tickX_()? */
			if (QF_timeEvtHead_[tickRate].act != (void *)0) {

				/* sanity check */
				Q_ASSERT_ID(610, prev != (QTimeEvt *)0);
				prev->next = (QTimeEvt *)QF_timeEvtHead_[tickRate].act;
				QF_timeEvtHead_[tickRate].act = (void *)0;
				t = prev->next; /* switch to the new list */
			} else {
				break;          /* all currently armed time evts. processed */
			}
		}

		/* time event scheduled for removal? */
		if (t->ctr == (QTimeEvtCtr)0) {
			prev->next = t->next;
			t->super.refCtr_ &= (uint8_t)0x7F; /* mark as unlinked */
			/* do NOT advance the prev pointer */
			/* exit crit. section to reduce latency */
			taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);
		} else {
			--t->ctr;

			/* is time event about to expire? */
			if (t->ctr == (QTimeEvtCtr)0) {
				QActive *act = (QActive *)t->act; /* temp. for volatile */

				/* periodic time evt? */
				if (t->interval != (QTimeEvtCtr)0) {
					t->ctr = t->interval;   /* rearm the time event */
					prev = t;               /* advance to this time event */
				}
				/* one-shot time event: automatically disarm */
				else {
					prev->next = t->next;
					t->super.refCtr_ &= (uint8_t)0x7F; /* mark as unlinked */
					/* do NOT advance the prev pointer */

					QS_BEGIN_NOCRIT_(QS_QF_TIMEEVT_AUTO_DISARM,
							 QS_priv_.locFilter[TE_OBJ], t)
					QS_OBJ_(t);                     /* this time event object */
					QS_OBJ_(act);                   /* the target AO */
					QS_U8_((uint8_t)tickRate);      /* tick rate */
					QS_END_NOCRIT_()
				}

				QS_BEGIN_NOCRIT_(QS_QF_TIMEEVT_POST,
						 QS_priv_.locFilter[TE_OBJ], t)
				QS_TIME_();                     /* timestamp */
				QS_OBJ_(t);                     /* the time event object */
				QS_SIG_(t->super.sig);          /* signal of this time event */
				QS_OBJ_(act);                   /* the target AO */
				QS_U8_((uint8_t)tickRate);      /* tick rate */
				QS_END_NOCRIT_()

				/* exit critical section before posting */
				taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);

				/* QACTIVE_POST_FROM_ISR() asserts if the queue overflows */
				QACTIVE_POST_FROM_ISR(act, &t->super,
						      pxHigherPriorityTaskWoken,
						      sender);
			} else {
				prev = t; /* advance to this time event */
				/* exit crit. section to reduce latency */
				taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);
			}
		}
		/* re-enter crit. section to continue */
		uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();
	}
	taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);
}
/*..........................................................................*/
QEvt *QF_newXFromISR_(uint_fast16_t const evtSize,
		      uint_fast16_t const margin, enum_t const sig)
{
	QEvt *e;
	uint_fast8_t idx;

#ifdef Q_SPY
	UBaseType_t uxSavedInterruptState;
#endif  /* Q_SPY */

	/* find the pool index that fits the requested event size ... */
	for (idx = (uint_fast8_t)0; idx < QF_maxPool_; ++idx) {
		if (evtSize <= QF_EPOOL_EVENT_SIZE_(QF_pool_[idx])) {
			break;
		}
	}
	/* cannot run out of registered pools */
	Q_ASSERT_ID(710, idx < QF_maxPool_);

#ifdef Q_SPY
	uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();
	QS_BEGIN_NOCRIT_(QS_QF_NEW, (void *)0, (void *)0)
	QS_TIME_();             /* timestamp */
	QS_EVS_(evtSize);       /* the size of the event */
	QS_SIG_((QSignal)sig);  /* the signal of the event */
	QS_END_NOCRIT_()
	taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);
#endif  /* Q_SPY */

	/* get e -- platform-dependent */
	e = QMPool_getFromISR(&QF_pool_[idx],
			      ((margin != QF_NO_MARGIN)
			       ? margin
			       : (uint_fast16_t)0));

	/* was e allocated correctly? */
	if (e != (QEvt *)0) {
		e->sig = (QSignal)sig;                          /* set signal for this event */
		e->poolId_ = (uint8_t)(idx + (uint_fast8_t)1);  /* store the pool ID */
		e->refCtr_ = (uint8_t)0;                        /* set the reference counter to 0 */
	}
	/* event cannot be allocated */
	else {
		/* must tolerate bad alloc. */
		Q_ASSERT_ID(720, margin != QF_NO_MARGIN);
	}
	return e; /* can't be NULL if we can't tolerate bad allocation */
}
/*..........................................................................*/
void QF_gcFromISR(QEvt const *const e)
{

	/* is it a dynamic event? */
	if (e->poolId_ != (uint8_t)0) {
		UBaseType_t uxSavedInterruptState;
		uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();

		/* isn't this the last ref? */
		if (e->refCtr_ > (uint8_t)1) {
			QF_EVT_REF_CTR_DEC_(e); /* decrements the ref counter */

			QS_BEGIN_NOCRIT_(QS_QF_GC_ATTEMPT, (void *)0, (void *)0)
			QS_TIME_();                             /* timestamp */
			QS_SIG_(e->sig);                        /* the signal of the event */
			QS_2U8_(e->poolId_, e->refCtr_);        /* pool Id & ref Count */
			QS_END_NOCRIT_()

			taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);
		}
		/* this is the last reference to this event, recycle it */
		else {
			uint_fast8_t idx = (uint_fast8_t)e->poolId_ - (uint_fast8_t)1;

			QS_BEGIN_NOCRIT_(QS_QF_GC, (void *)0, (void *)0)
			QS_TIME_();                             /* timestamp */
			QS_SIG_(e->sig);                        /* the signal of the event */
			QS_2U8_(e->poolId_, e->refCtr_);        /* pool Id & ref Count */
			QS_END_NOCRIT_()

			taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);

			/* pool ID must be in range */
			Q_ASSERT_ID(810, idx < QF_maxPool_);

			/* casting const away is legitimate, because it's a pool event */
			QMPool_putFromISR(&QF_pool_[idx], (QEvt *)e);
		}
	}
}
/*..........................................................................*/
void QMPool_putFromISR(QMPool *const me, void *b)
{
	UBaseType_t uxSavedInterruptState;

	/** @pre # free blocks cannot exceed the total # blocks and
	 * the block pointer must be from this pool.
	 */
	Q_REQUIRE_ID(900, (me->nFree < me->nTot)
		     && QF_PTR_RANGE_(b, me->start, me->end));

	uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();

	((QFreeBlock *)b)->next = (QFreeBlock *)me->free_head;  /* link into list */
	me->free_head = b;                                      /* set as new head of the free list */
	++me->nFree;                                            /* one more free block in this pool */

	QS_BEGIN_NOCRIT_(QS_QF_MPOOL_PUT, QS_priv_.locFilter[MP_OBJ], me->start)
	QS_TIME_();             /* timestamp */
	QS_OBJ_(me->start);     /* the memory managed by this pool */
	QS_MPC_(me->nFree);     /* the number of free blocks in the pool */
	QS_END_NOCRIT_()

	taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);
}
/*..........................................................................*/
void *QMPool_getFromISR(QMPool *const me, uint_fast16_t const margin)
{
	QFreeBlock *fb;
	UBaseType_t uxSavedInterruptState;

	uxSavedInterruptState = taskENTER_CRITICAL_FROM_ISR();

	/* have more free blocks than the requested margin? */
	if (me->nFree > (QMPoolCtr)margin) {
		void *fb_next;
		fb = (QFreeBlock *)me->free_head; /* get a free block */

		/* the pool has some free blocks, so a free block must be available */
		Q_ASSERT_ID(910, fb != (QFreeBlock *)0);

		fb_next = fb->next; /* put volatile to a temporary to avoid UB */

		/* is the pool becoming empty? */
		--me->nFree; /* one less free block */
		if (me->nFree == (QMPoolCtr)0) {
			/* pool is becoming empty, so the next free block must be NULL */
			Q_ASSERT_ID(920, fb_next == (QFreeBlock *)0);

			me->nMin = (QMPoolCtr)0; /* remember that the pool got empty */
		} else {
			/* pool is not empty, so the next free block must be in range
			 *
			 * NOTE: the next free block pointer can fall out of range
			 * when the client code writes past the memory block, thus
			 * corrupting the next block.
			 */
			Q_ASSERT_ID(930, QF_PTR_RANGE_(fb_next, me->start, me->end));

			/* is the number of free blocks the new minimum so far? */
			if (me->nMin > me->nFree) {
				me->nMin = me->nFree; /* remember the new minimum */
			}
		}

		me->free_head = fb_next; /* set the head to the next free block */

		QS_BEGIN_NOCRIT_(QS_QF_MPOOL_GET,
				 QS_priv_.locFilter[MP_OBJ], me->start)
		QS_TIME_();             /* timestamp */
		QS_OBJ_(me->start);     /* the memory managed by this pool */
		QS_MPC_(me->nFree);     /* # of free blocks in the pool */
		QS_MPC_(me->nMin);      /* min # free blocks ever in the pool */
		QS_END_NOCRIT_()

	}
	/* don't have enough free blocks at this point */
	else {
		fb = (QFreeBlock *)0;

		QS_BEGIN_NOCRIT_(QS_QF_MPOOL_GET_ATTEMPT,
				 QS_priv_.locFilter[MP_OBJ], me->start)
		QS_TIME_();             /* timestamp */
		QS_OBJ_(me->start);     /* the memory managed by this pool */
		QS_MPC_(me->nFree);     /* the number of free blocks in the pool */
		QS_MPC_(margin);        /* the requested margin */
		QS_END_NOCRIT_()
	}
	taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptState);

	return fb; /* return the pointer to memory block or NULL to the caller */
}

