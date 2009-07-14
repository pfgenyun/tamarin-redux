/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is [Open Source Virtual Machine.].
 *
 * The Initial Developer of the Original Code is
 * Adobe System Incorporated.
 * Portions created by the Initial Developer are Copyright (C) 2004-2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Adobe AS3 Team
 *   leon.sha@sun.com
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __GC__
#define __GC__

#define MMGC_GCENTER(_gc)  MMgc::GCAutoEnter __mmgc_auto_enter(_gc);

#if defined(MMGC_IA32) && defined(_MSC_VER)

#define MMGC_GET_STACK_EXTENTS(_gc, _stack, _size)						\
	jmp_buf __mmgc_env;													\
	setjmp(__mmgc_env);													\
	__asm { mov _stack,esp };											\
	_size = (uint32_t)(_gc->GetStackTop() - (uintptr_t)_stack);

#elif AVMSYSTEM_SPARC == 1

#define MMGC_GET_STACK_EXTENTS(_gc, _stack, _size)						\
	jmp_buf __mmgc_env;													\
	setjmp(__mmgc_env);													\
	_stack = (void*)_getsp();											\
	_size = (uint32_t)(_gc->GetOSStackTop() - (uintptr_t)_stack);

#else

#define MMGC_GET_STACK_EXTENTS(_gc, _stack, _size)						\
	jmp_buf __mmgc_env;													\
	setjmp(__mmgc_env);													\
	_stack = &__mmgc_env;												\
	_size = (uint32_t)(_gc->GetStackTop() - (uintptr_t)_stack);

#endif

// This macro creates a GC root of the current threads stack, there
// are two use cases for this: 
//
// 1) the main thread is halting to let another thread run doing
// possible GC activity (debugger use case)
//
// 2) the stack is moving to another area of memory (stack switching)
// but the old stack must still get scanned
#define MMGC_GC_ROOT_THREAD(_gc)						\
	void *__stack;										\
	size_t __stackSize;									\
	MMGC_GET_STACK_EXTENTS(_gc, __stack, __stackSize);	\
	MMgc::GCRoot root(_gc, __stack, __stackSize);		\
	MMgc::GCAutoEnterPause __mmgc_enter_pause(_gc);

// Enable our own alloca() replacement that always allocates in the heap, this is good on
// systems with limited memory or limited stack
#define AVMPLUS_PARAM_ALLOCA_CUTOFF		4000	// Don't make real alloca() blow the stack; this limit is heuristic
#define AVMPLUS_PARAM_ALLOCA_DEFSIZE	1000	// Default number of bytes in a stack segment for heap-based alloca()
#ifdef AVMPLUS_HEAP_ALLOCA
#  define VMPI_alloca(core, autoptr, nbytes)	core->gc->allocaPush(nbytes, autoptr)
#  define VMPI_alloca_gc(gc, autoptr, nbytes)	gc->allocaPush(nbytes, autoptr)
#else
#  define VMPI_alloca(core, autoptr, nbytes)  (nbytes > AVMPLUS_PARAM_ALLOCA_CUTOFF ? core->gc->allocaPush(nbytes, autoptr) : alloca(nbytes))
#  define VMPI_alloca_gc(gc, autoptr, nbytes)  (nbytes > AVMPLUS_PARAM_ALLOCA_CUTOFF ? gc->allocaPush(nbytes, autoptr) : alloca(nbytes))
#endif

namespace avmplus
{
	class AvmCore;
}

namespace MMgc
{
	class GCAutoEnter;

	/**
	 * Conservative collector unit of work
	 */
	class GCWorkItem
	{
	public:
		// FIXME? The initialization is redundant for most locals and for the mark stack we
		// don't want to have to init all the elements in the array as it makes allocating a mark
		// stack segment expensive.  I believe we could safely get rid of the two initializing
		// clauses here.  --lars
		GCWorkItem() : ptr(NULL), _size(0) { }
		inline GCWorkItem(const void *p, uint32_t s, bool isGCItem);

		uint32_t GetSize() const { return _size & ~1; }
		uint32_t IsGCItem() const { return _size & 1; }

		// If a WI is a GC item, `ptr` is the UserPointer; it must not
		// be the RealPointer nor an interior pointer
		const void *ptr;

		// The low bit of _size stores whether this is a GC item.
		// Always access this through `GetSize` and `IsGCItem`
		uint32_t _size;
	};

	/**
	 * GCRoot is root in the reachability graph, it contains a pointer a size 
	 * and will be searched for things.  
	 */
	class GCRoot
	{
		friend class GC;
		GCRoot();
		void init(GC*gc, const void *object, size_t size);
	public:
		/** subclassing constructor */
		GCRoot(GC *gc);
		/** general constructor */
		GCRoot(GC *gc, const void *object, size_t size);
		virtual ~GCRoot();

		// override new and delete so we can know the objects extents (via FixedMalloc::Size())
		void *operator new(size_t size)
		{
			void *space = GCHeap::GetGCHeap()->GetFixedMalloc()->Alloc(size);
			// GCRoots are auto-scanned so clean them, don't rely on ctors to init the object properly
			VMPI_memset(space, 0, GCHeap::GetGCHeap()->GetFixedMalloc()->Size(space));
			return space;
		}
        
		void operator delete (void *object)
		{
			GCHeap::GetGCHeap()->GetFixedMalloc()->Free(object);
		}

		const void *Get() const { return object; }
		void Set(const void *object, size_t size);

		GC *GetGC() const { return gc; }
		/** if your object goes away after the GC is deleted this can be useful */
		void Destroy();
	
	private:
		GC * gc;

		GCRoot *next;
		GCRoot *prev;
		const void *object;
		size_t size;

		GCWorkItem GetWorkItem() const { return GCWorkItem(object, (uint32_t)size, false); }
	};

	/**
	 * GCCallback is an interface that allows the application to get
	 * callbacks at interesting GC points.
	 */
	class GCCallback
	{
		friend class GC;
		friend class ZCT;
	public:
		GCCallback(GC *gc);
		virtual ~GCCallback();
		
		GC *GetGC() const { return gc; }
		/** if your object goes away after the GC is deleted this can be useful */
		void Destroy();

		/**
		 * This method is invoked after all marking and before any
		 * sweeping, useful for bookkeeping based on whether things
		 * got marked
		 */
		virtual void presweep() {}

		/**
		 * This method is invoked after all sweeping
		 */
		virtual void postsweep() {}

		// called before a ZCT reap begins
		virtual void prereap() {}

		// called after a ZCT reap completes
		virtual void postreap() {}

		/**
		 * This method is called before an RC object is reaped
		 */
		virtual void prereap(void* /*rcobj*/) {}

	private:
		GC *gc;
		GCCallback *nextCB;
		GCCallback *prevCB;
	};

	#ifdef MMGC_64BIT
	#define HIDDENPTRMASK (uintptr_t(0x1L)<<63)
	#else
	#define HIDDENPTRMASK (uintptr_t(0x1L)<<31)
	#endif

	template <class T>
	class GCHiddenPointer
	{
	public:
		GCHiddenPointer(T obj=NULL) { set(obj); }
		operator T() const { return (T) (val^HIDDENPTRMASK);	 }
		T operator=(T tNew) 
		{ 
			set(tNew); 
			return (T)this; 
		}
		T operator->() const { return (T) (val^HIDDENPTRMASK); }

	private:
		// private to prevent its use and someone adding it, GCC creates
		// WriteBarrier's on the stack with it
		GCHiddenPointer(const GCHiddenPointer<T>& toCopy) { GCAssert(false); }
		
		void set(T obj) 
		{
			uintptr_t p = (uintptr_t)obj;
			val = p ^ HIDDENPTRMASK;
		}
		uintptr_t val;
	};

	
	/**
	 * A policy manager for garbage collection.
	 *
	 * The policy manager centralizes policy decisions about how and when to run garbage
	 * collection.  It is queried from the collector code, and collector code signals
	 * to the policy manager at the occurence of certain events.
	 *
	 * Typically the GCPolicyManager is simply embedded inside a GC instance, and manages
	 * policy for that GC.
	 *
	 * ----
	 *
	 * Notes, 2009-06-23 / lhansen:
	 *
	 * This is the second cut, and it implements an allocation-driven policy that is
	 * documented extensively in GC.cpp and in doc/mmgc/policy.pdf.  The policy improves
	 * on the first-cut policy by running the GC less often and having lower pause times.
	 *
	 * ZCT reaping times are not bounded, so ZCT reaping may violate any kind of
	 * incrementality guarantee.  This will be fixed by and by.
	 */
	class GCPolicyManager {
	public:
		GCPolicyManager(GC* gc, GCHeap* heap);
		
		/**
		 * Clean up and print any final statistics.  Should be called from the very
		 * start of the GC's destructor.
		 */
		void shutdown();
		
		/**
		 * Situation: the GC is about to run the incremental marker.
		 *
		 * @return the desired length of the next incremental mark quantum.
		 * @note the result can vary from call to call; the function should
		 *       be called as an incremental mark is about to start and the
		 *       result should not be cached.
		 */
		uint32_t incrementalMarkMilliseconds();
		
		/**
		 * @return the number of blocks owned by this GC, as accounted for by calls to
		 * signalBlockAllocation and signalBlockDeallocation.
		 */
		uint64_t blocksOwnedByGC();

		/**
		 * @return the number of objects reported marked by signalMarkWork, since startup.
		 */
		uint64_t objectsMarked();
		
		/**
		 * @return the number of bytes reported marked by signalMarkWork, since startup.
		 */
		uint64_t bytesMarked();
		
		/**
		 * Compute a new reap threshold for the ZCT based on its current size and occupancy.
		 *
		 * @return the new reap threshold (ZCT occupancy upper limit).
		 *
		 * (Note, an alternative strategy for the ZCT would be to call a policy function
		 * every time the ZCT has to be extended, and to shrink the ZCT following a reap.)
		 */
		uint64_t zctNewReapThreshold(uint64_t zctBlocks, uint64_t zctOccupancy);

		/**
		 * Situation: The ZCT has just been reaped and there are zctBlocks live blocks
		 * and zctOccupancy live pointers.  Should we grow the zct by a block?
		 *
		 * @return true if we should grow the ZCT.
		 */
		bool queryZctShouldGrowAfterReap(uint64_t zctBlocks, uint64_t zctOccupancy);
		
		/**
		 * Set the lower limit beyond which we try not to run the garbage collector.
		 * The value is specified in 4k blocks, thus 256 == 1MB.
		 */
		void setLowerLimitCollectionThreshold(uint32_t blocks);

		enum PolicyEvent 
		{
			NO_EVENT,
			START_StartIncrementalMark,	// also, start of garbage collection
			END_StartIncrementalMark,
			START_IncrementalMark,
			END_IncrementalMark,
			START_FinalRootAndStackScan,
			END_FinalRootAndStackScan,
			START_FinalizeAndSweep,
			END_FinalizeAndSweep,		// also, end of garbage collection
			START_ReapZCT,
			END_ReapZCT
		};
		
		/**
		 * Situation: a GC event corresponding to one of the PolicyEvent values occurs.
		 * Tell the policy manager about it.
		 */
		void signal(PolicyEvent ev);
		
		/**
		 * Situation: the memory status of the system changes, and a message is being
		 * broadcast to make this known.
		 */
		void signalMemoryStatusChange(MemoryStatus from, MemoryStatus to);
		
		/**
		 * Situation: 'numblocks' blocks have just been obtained by this GC from the GCHeap.
		 */
		void signalBlockAllocation(size_t numblocks);
		
		/** 
		 * Situation: 'numblocks' blocks have just been returned from this GC to the GCHeap.
		 */
		void signalBlockDeallocation(size_t numblocks);
		
		/**
		 * Situation: signal that one pointer-containing object, whose size is nbytes,
		 * has been scanned by the garbage collector.
		 *
		 * This may only be called between the signals START_StartIncrementalMark and 
		 * END_FinalizeAndSweep, and mark work signaled after a START event may not be
		 * reflected in the values returned by objectsMarked() and bytesMarked() until
		 * after the corresponding END event has been signaled.
		 */
		/*REALLY_INLINE*/ void signalMarkWork(size_t nbytes);
		
		/**
		 * Situation: signal that some number of bytes have just been successfully
		 * allocated and are about to be returned to the caller of the allocator.
		 */
		/*REALLY_INLINE*/ void signalAllocWork(size_t nbytes);

		/**
		 * Situation: signal that some number of bytes have just been successfully
		 * freed.
		 */
		/*REALLY_INLINE*/ void signalFreeWork(size_t nbytes);

		/**
		 * Situation: we've just entered the allocator and we need to know whether the
		 * allocation budget has already been exhausted so that collection work should
		 * be triggered.
		 *
		 * Note the decision is independent of the request size; consequently, we only
		 * react /after/ the budget is exhausted, and this may cause us to overallocate
		 * a little (typically) or more than that (rarely).
		 */
		/*REALLY_INLINE*/ bool queryCollectionWork();

		/**
		 * Situation: the incremental marker has been started, and we need to know whether
		 * to run another mark increment or push the conclusion to a finish (because the
 		 * total allocation budget for the collection cycle has been exhausted).  This
		 * predicate returns true in the latter case.
		 */
		bool queryEndOfCollectionCycle();

#ifdef MMGC_POLICY_PROFILING
		/**
		 * Situation: signal that one write has been examined by the write barrier and made
		 * it to the given stage (0..3) of the barrier.   Stages are:
		 *  0 - entered the write barrier
		 *  1 - passed cheap filtering
		 *  2 - passed GetMark filtering (lhs is marked)
		 *  3 - passed IsWhite filtering (rhs is not marked)
		 */
		/*REALLY_INLINE*/ void signalWriteBarrierWork(int stage);

		/**
		 * Situation: signal that the ZCT reaper has run and performed some work.
		 */
		void signalReapWork(uint32_t objects_reaped, uint32_t bytes_reaped, uint32_t objects_pinned);
#endif
#ifdef MMGC_POINTINESS_PROFILING
		/**
		 * Situation: signal that 'words' words have been scanned; that 'could_be_pointer'
		 * number of these words passed the initial heap range checks; and that 
		 * 'actually_is_pointer' number of these words were conservatively found to
		 * be pointers to heap objects.
		 */
		/*inline*/ void signalDemographics(size_t words, size_t could_be_pointer, size_t actually_is_pointer);
#endif

		/**
		 * The collector 'gc' (which is not the collector for this manager) has started
		 * a garbage collection, indicating perhaps some memory pressure in that heap.
		 */
		void signalStartCollection(GC* gc);
		
		/**
		 * The collctor 'gc' (which is not the collector for this manager) has completed
		 * a garbage collection.
		 */
		void signalEndCollection(GC* gc);

		/**
		 * Request a full collection happen at the next GC edge (enter/exit)
		 */
		void queueFullCollection() { fullCollectionQueued = true; }

		/**
		 * called after a full collection is done
		 */
		void fullCollectionComplete() { fullCollectionQueued = false; }

		/**
		 * called to find out if a full collection has been requested
		 */
		bool queryFullCollectionQueued() { return fullCollectionQueued; }

		// ----- Public data --------------------------------------
		
		// Elapsed time (in ticks) for various collection phases, and the maximum phase time
		// for each phase.  They are updated when an END event is signaled.  Client code must
		// not modify these variables.

		// The total time for various collection phases across the run
		uint64_t timeStartIncrementalMark;
		uint64_t timeIncrementalMark;
		uint64_t timeFinalRootAndStackScan;
		uint64_t timeFinalizeAndSweep;
		uint64_t timeReapZCT;

		// The total time doing collection work (sum of the variables above except the ZCT reap
		// time) and the elapsed time from the start of StartIncrementalMark to the end of
		// FinalizeAndSweep.  Together they provide a crude approximation to a measure of 
		// pause clustering.
		uint64_t timeInLastCollection;
		uint64_t timeEndToEndLastCollection;

		// Time for ZCT reaping during the last collection cycle (end of one FinalizeAndSweep
		// to the end of the next one).
		uint64_t timeReapZCTLastCollection;

		// The maximum latceny for various collection phases across the run
		uint64_t timeMaxStartIncrementalMark;
		uint64_t timeMaxIncrementalMark;
		uint64_t timeMaxFinalRootAndStackScan;
		uint64_t timeMaxFinalizeAndSweep;
		uint64_t timeMaxReapZCT;
		
		// The maximum latcency for various collection phases during the previous collection cycle
		uint64_t timeMaxStartIncrementalMarkLastCollection;
		uint64_t timeMaxIncrementalMarkLastCollection;
		uint64_t timeMaxFinalRootAndStackScanLastCollection;
		uint64_t timeMaxFinalizeAndSweepLastCollection;
		uint64_t timeMaxReapZCTLastCollection;

		// The total number of times each phase was run
		uint64_t countStartIncrementalMark;
		uint64_t countIncrementalMark;
		uint64_t countFinalRootAndStackScan;
		uint64_t countFinalizeAndSweep;
		uint64_t countReapZCT;
		
	private:
		// The following parameters can vary not just from machine to machine and
		// run to run on the same machine, but within a run in response to memory
		// pressures and other feedback.
		
		// The lower limit beyond which we try not to run the garbage collector.
		uint32_t lowerLimitCollectionThreshold();
		
		// Get the current time (in ticks).
		uint64_t now();

#ifdef MMGC_POLICY_PROFILING
		// Convert ticks to milliseconds, as a double (used for printing)
		double ticksToMillis(uint64_t ticks);
		
		// @return true if we should print policy data
		bool summarizeGCBehavior();

		// Print policy data.  At shutdown, afterCollection will be passed as false.
		void PrintGCBehaviorStats(bool afterCollection=true);
#endif

 		// Various private methods for the GC policy follow.  See comment in GC.cpp for details.
 
 		// Amount of GC work to perform (bytes to scan) per byte allocated while the GC is active
 		double W();
 
 		// Amount of allocation to allow between two invocations of IncrementalMark
 		double A();
 
 		// Called from the policy event handler to start computing adjustments to R: before any
 		// mark work is performed.
 		void startAdjustingR();
 
 		// Called from the policy event handler to finish computing adjustments to R: after all
 		// mark work has been performed.
 		void endAdjustingR();
 
 		// Called from adjustPolicyForNextMajorCycle to compute the effective L for the next
 		// collection cycle
 		void adjustL();
 
 		// Called from the policy event handler to compute the GC policy for the next 
 		// major collection cycle (from the end of one FinishIncrementalMark to the start
 		// of the next one)
 		void adjustPolicyForNextMajorCycle();
 
 		// Called from the policy event handler to compute the GC policy for the next
 		// minor collection cycle (from the end of one IncrementalMark to the start of the
 		// next one)
 		void adjustPolicyForNextMinorCycle();
 
		// ----- Private data --------------------------------------
		
		GC * const gc;
		GCHeap * const heap;
		
		// The time recorded the last time we received signalEndOfIncrementalMark
		uint64_t timeEndOfLastIncrementalMark;

		// The time recorded the last time we received signalStartOfCollection
		uint64_t timeStartOfLastCollection;

		// The time recorded the last time we received signalEndOfCollection
		uint64_t timeEndOfLastCollection;

		// The total number of blocks owned by GC, and the maximum such number
		uint64_t blocksOwned;
		uint64_t maxBlocksOwned;
		
		// The number of objects scanned since startup (which is equivalent to the number
		// of calls to GC::MarkItem), less the number scanned during the last
		// collection cycle.
		uint64_t objectsScannedTotal;
		
		// The number of objects scanned during the last collection cycle.
		uint32_t objectsScannedLastCollection;
		
		// The number of bytes scanned since startup less the ones scanned during the 
		// last collection cycle.
		uint64_t bytesScannedTotal;
		
		// The number of bytes scanned during the last collection cycle.
		uint32_t bytesScannedLastCollection;

		// Temporaries for holding the start time / start event until the end event arrives
		uint64_t start_time;
		PolicyEvent start_event;

		// Value returned by lowerLimitCollectionThreshold() and set by setLowerLimitCollectionThreshold():
		// the heap size, in blocks, below which we do not collect.
		uint32_t collectionThreshold;

		// true if a forced garbage collection has been requested
		bool fullCollectionQueued;

		// true if the ZCT statistics for the current GC cycle should be cleared;
		// this is required because the ZCT stats may need to be updated before
		// StartIncrementalMark but can't be cleared at the end of FinalizeAndSweep.
		bool pendingClearZCTStats;

#ifdef MMGC_POLICY_PROFILING
		// Records the heap population before we sweep.  Unit is blocks except where noted.
		size_t heapAllocatedBeforeSweep;
		size_t heapUsedBeforeSweep;
		size_t gcAllocatedBeforeSweep;
		size_t gcBytesUsedBeforeSweep;
		
		// Barrier stages hit (writes examined by InlineWriteBarrierTrap) overall, less the last
		// collection cycle, and for the last collection cycle. 
		uint64_t barrierStageTotal[3];
		uint32_t barrierStageLastCollection[3];

		// Reap work, overall
		uint64_t objectsReaped;
		uint64_t bytesReaped;
		uint64_t objectsPinned;
#endif
#ifdef MMGC_POINTINESS_PROFILING
		// Track the number of scannable words, the number that passes the initial range
		// check, and the number that turn out to be (conservative) pointers.
		//
		// These are cleared at the start of each GC so stats are only valid for one
		// GC cycle.
		uint64_t candidateWords;
		uint64_t couldBePointer;
		uint64_t actuallyIsPointer;
#endif

		// Various policy parameters.  For more documentation, see comments in GC.cpp.
		
		// max pause time in seconds
		double P;

		// approximate mark rate in bytes/sec, [1M,infty)
		double R;

		// requested inverse load factor (1,infty)
		double L_ideal;
		
		// adjusted inverse load factor (adjusted for heap pressure, growth, etc)
		double L_actual;
		
		// gc trigger as fraction of allocation budget to use before triggering GC [0,1]
		double T;

		// ratio of gc work to mutator work while the gc is running
		double G;

		// largest multiple of L_ideal to which L_actual can grow (unless 0, which means unlimited)
		double X;

		// the remaining allocation budget for the major GC cycle.  (This can go negative
		// and the variable must accomodate negative values.  It's not frequently accessed.)
		double remainingMajorAllocationBudget;

		// the allocation budget for the minor GC cycle.  This variable remains constant
		// throughout the minor cycle.
		int32_t minorAllocationBudget;
		
		// the remaining allocation budget for the minor GC cycle.  Initially this has the
		// value of minorAllocationBudget; the allocation request size is subtracted for
		// every allocation.  This variable can go negative because we can overshoot the
		// budget.
		int32_t remainingMinorAllocationBudget;
		
		// Temporaries used to compute R
		uint64_t adjustR_startTime;
		uint64_t adjustR_totalTime;
	};

#ifdef MMGC_POLICY_PROFILING
	// Must be here (and not in GC.cpp) because it's accessed from the inline write 
	// barrier in WriteBarrier.h
	REALLY_INLINE void GCPolicyManager::signalWriteBarrierWork(int stage)
	{
		GCAssert(ARRAY_SIZE(barrierStageLastCollection) > size_t(stage));
		barrierStageLastCollection[stage]++;
	}
#endif
	
	/**
	 * The Zero Count Table used by DRC.
	 */
	class ZCT
	{
	public:
		ZCT();
		void Destroy();
		
		// Associate the ZCT with 'gc' and perform final initialization of the ZCT.  To be 
		// called exactly once after the GC has been properly constructed: SetGC() uses
		// 'gc' to allocate memory.
		void SetGC(GC* gc);
		
		// Add obj to the ZCT; it must not already be in the ZCT.  This method can fail silently,
		// leaving the GC to reap the object in case its reference count stays zero.
		void Add(RCObject *obj);
		
		// Remove obj from the ZCT; it must already be in the ZCT.
		void Remove(RCObject *obj);
		
		// Reap the ZCT: destroy every object in the ZCT that is not referenced from the
		// calling thread's stack or specially marked stack-like data structures in the GC
		// (see GC::allocaPush() and associated code).
		void Reap(bool scanStack=true);

		// Throw away unused memory (discretionary); to be called at the end of a GC.
		void Prune();
		
		bool IsReaping()
		{
			return reaping;
		}

	private:
		// @return the number of elements the table can accomodate
		int Capacity();

		// @return true iff the table can grow by at least one more block.
		bool CanGrow();
		
		// @return true iff the free list is not empty (and it's OK to call GrabFree)
		bool HasFree();

		// @return the next element off the free list
		int GrabFree();
	
		// Add the element at 'index' to the free list
		void MakeFree(int index);

		// @return the value stored at 'index'
		RCObject* Get(int index);

		// Store 'value' at 'index'
		void Put(int index, RCObject* value);

		// @return the block number for 'index'
		int BlockNumber(int index);

		// @return the entry within a block for 'index'
		int EntryNumber(int index);
		
		// @return the address corresponding to ZCT offset 'index'
		RCObject** PointerTo(int index);
		
		// If CanGrow is true, add a block of memory to the zct.  This may fail to
		// allocate that block; failure is silent but reflected in Capacity returning
		// the same value before and after the call.
		void Grow();

		// Conservatively scan memory starting at 'start' for 'len' bytes, examining each
		// aligned word.  If the value stored can be interpreted as a (possibly tagged)
		// pointer to the start of an RCObject, then pin that RCObject.  The value of
		// 'start' must itself be aligned.
		void PinStackObjects(const void *start, size_t len);

		// Private data

		GC *gc;
		RCObject ***blocktable;		// Table of pointers to individual blocks
		RCObject ***blocktop;		// Next free item in blocktable
		int freelist;				// index of first free element (element holds index of next free, or -1)
		bool reaping;				// are we reaping the zct?
		int count;					// table population in number of live elements
		int zctNext;				// index of first free element at the end
		int zctReapThreshold;		// if count reaches this then we reap
		int zctIndex;				// during a reap: next element to process (stepping up)
		int nextPinnedIndex;		// during a reap: next free slot for pinned elements (stepping up)
	};

	/**
	 * This is a general-purpose garbage collector used by the Flash Player.
	 * Application code must implement the GCRoot interface to mark all
	 * reachable data.  Unreachable data is automatically destroyed.
	 * Objects may optionally be finalized by subclassing the GCObject
	 * interface.
	 *
	 * This garbage collector is intended to be modular, such that
	 * it can be used in other products or replaced with a different
	 * collector in the future.
	 *
	 * Memory allocation and garbage collection strategy:
	 * Mark and sweep garbage collector using fixed size classes
	 * implemented by the GCAlloc class.  Memory blocks are obtained
	 * from the OS via the GCHeap heap manager class.
	 *
	 * When an allocation fails because a suitable memory block is
	 * not available, the garbage collector decides either to garbage
	 * collect to free up space, or to expand the heap.  The heuristic
	 * used to make the decision to collect or expand is taken from the
	 * Boehm-Demers-Weiser (BDW) garbage collector and memory allocator.
	 * The BDW algorithm is (pseudo-code):
	 *
	 *    if (allocs since collect >= heap size / FSD)
	 *      collect
	 *    else
	 *      expand(request size + heap size / FSD)
	 *
	 * The FSD is the "Free Space Divisor."  For the first cut, I'm trying
	 * 4.  TODO: Try various FSD values against real Player workloads
	 * to find the optimum value.
	 *
	 */
	class GC : public OOMCallback
	{
		friend class GCRoot;
		friend class GCCallback;
		friend class GCAlloc;
		friend class GCLargeAlloc;
		friend class RCObject;
		friend class GCInterval;
		friend class ZCT;
	public:

		/**
		 * If you need context vars use this!
		 */
		enum
		{
			GCV_COREPLAYER,
			GCV_AVMCORE,
			GCV_COUNT
		};
		void *GetGCContextVariable(int var) const { return m_contextVars[var]; }
		void SetGCContextVariable(int var, void *val) { m_contextVars[var] = val; }
		
		avmplus::AvmCore *core() const { return (avmplus::AvmCore*)GetGCContextVariable(GCV_AVMCORE); }

		/**
		 * greedy is a debugging flag.  When set, every allocation
		 * will cause a garbage collection.  This makes code run
		 * abysmally slow, but can be useful for detecting mark
		 * bugs.
		 *
		 * The GC reads this flag only when holding the GC lock.  It is best
		 * to set it as soon as the GC is created.
		 */
		bool greedy;

		/**
		 * nogc is a debugging flag.  When set, garbage collection
		 * never happens.
		 */
		bool nogc;

		/**
	      * findUnmarkedPointers is a debugging flag, only 
		  */
		bool findUnmarkedPointers;

		/**
		* turns on code that does a trace before reaping zero count object and asserting on
		* any objects that get marked, debug builds only
		*/
		bool validateDefRef;		
		
		/**
		 * turn on code that keeps the call stacks of all IncrementRef and DecrementRef operations
		 * on each RC object, and prints this history when the reference count is already zero
		 * on entry to DecrementRef, debug builds only.
		 */
		bool keepDRCHistory;

		bool dontAddToZCTDuringCollection;
		bool incrementalValidation;

#ifdef _DEBUG
		bool incrementalValidationPedantic;
#endif

		/**
		 * Configuration flag enabling incremental collection.
		 */
		bool incremental;

		// -- Interface
		GC(GCHeap *heap);
		virtual ~GC();
		
		/**
		 * Causes an immediate stop-the-world garbage collection (or finishes any
		 * incremental collection underway).
		 */
		void Collect(bool scanStack=true);

		/**
		 * Do a full collection at the next MMGC_GCENTER macro site
		 */
		void QueueCollection() { policy.queueFullCollection(); }

		/**
		* flags to be passed as second argument to alloc
		*/
		enum AllocFlags
		{
			kZero=1,
			kContainsPointers=2,
			kFinalize=4,
			kRCObject=8,
			kCanFail=16			
		};

		enum PageType
		{
			kNonGC = 0,
			kGCAllocPage = 1,
			kGCLargeAllocPageRest = 2,
			kGCLargeAllocPageFirst = 3
		};

		/**
		 * Main interface for allocating memory.  Default flags are
		 * no finalization, not containing pointers, not zero'd, and not ref-counted
		 *
		 * Do not call this from a finalizer.
		 */
		void *Alloc(size_t size, int flags=0);

		/**
		 * Just like Alloc but can return NULL
		 */
		void *PleaseAlloc(size_t size, int flags=0)
		{
			return Alloc(size, flags | kCanFail);			
		}

	private:
		class RCRootSegment : public GCRoot
		{
		public:
			RCRootSegment(GC* gc, void* mem, size_t size);
			void*			mem;
			size_t			size;
			RCRootSegment*	prev;
			RCRootSegment*	next;
		};
		
		RCRootSegment* rcRootSegments;
		
	public:
		
		/**
		 * Allocate memory that will be scanned for pointers to GC memory
		 * and from which pointers to RC objects will pin those objects.
		 */
		void* AllocRCRoot(size_t size);
		
		/**
		 * Free memory allocated with AllocRCRoot.
		 */
		void FreeRCRoot(void* mem);
		
		/**
		 * overflow checking way to call Alloc for a # of n size'd items,
		 * all instance of Alloc(num*sizeof(thing)) should be replaced with:
		 * Calloc(num, sizeof(thing))
		 *
		 * Do not call this from a finalizer.
		 */
		void *Calloc(size_t num, size_t elsize, int flags=0);

		/**
		 * One can free a GC allocated pointer, this will throw an assertion
		 * if called during the Sweep phase (ie via a finalizer) it can only be
		 * used outside the scope of a collection
		 */
		void Free(const void *ptr);

		/**
		 * return the size of a piece of memory, may be bigger than what was asked for.
		 *
		 * NOTE! This happens to work because the usableSize field of a large block is of the
		 * same type and at the same offset as the size field of a non-large block...
		 * none of which is documented anywhere.  Deriving GCBlock and LargeBlock from some
		 * common base type in GC would have been rather cleaner.
		 */
		static size_t Size(const void *ptr)
		{
			GCAssert(GetGC(ptr)->IsGCMemory(ptr));			
			size_t size = GCLargeAlloc::GetBlockHeader(ptr)->usableSize;
			size -= DebugSize();
			return size;

		}

		/**
		 * Tracers should employ GetMark and SetMark to
		 * set the mark bits during the mark pass.
		 */
		static int GetMark(const void *item);

		static int SetMark(const void *item)
		{
			GCAssert(item != NULL);
#ifdef MMGC_MEMORY_INFO
			GC *gc = GetGC(item);	
			item = GetRealPointer(item);
			GCAssert(gc->IsPointerToGCPage(item));
#endif 			
			if (GCLargeAlloc::IsLargeBlock(item)) {
				return GCLargeAlloc::SetMark(item);
			} else {
				return GCAlloc::SetMark(item);
			}
		}
		
		void SetQueued(const void *item)
		{
#ifdef MMGC_MEMORY_INFO
			item = GetRealPointer(item);
			GCAssert(IsPointerToGCPage(item));
#endif 			
			if (GCLargeAlloc::IsLargeBlock(item)) {
				GCLargeAlloc::SetQueued(item);
			} else {
				GCAlloc::SetQueued(item);
			}
		}

		void ClearQueued(const void *item)
		{
#ifdef MMGC_MEMORY_INFO
			item = GetRealPointer(item);
			GCAssert(IsPointerToGCPage(item));
#endif 			
			if (GCLargeAlloc::IsLargeBlock(item)) {
				GCLargeAlloc::ClearQueued(item);
			} else {
				GCAlloc::ClearQueued(item);
			}
		}
		
		static void ClearFinalized(const void *item)
		{
#ifdef MMGC_MEMORY_INFO
			GC *gc = GetGC(item);	
			item = GetRealPointer(item);
			GCAssert(gc->IsPointerToGCPage(item));
#endif 			
			if (GCLargeAlloc::IsLargeBlock(item)) {
				GCLargeAlloc::ClearFinalized(item);
			} else {
				GCAlloc::ClearFinalized(item);
			}
		}

		static void SetFinalize(const void *item)
		{
#ifdef MMGC_MEMORY_INFO
			GC *gc = GetGC(item);	
			item = GetRealPointer(item);
			GCAssert(gc->IsPointerToGCPage(item));
#endif 			
			if (GCLargeAlloc::IsLargeBlock(item)) {
				GCLargeAlloc::SetFinalize(item);
			} else {
				GCAlloc::SetFinalize(item);
			}
		}

		static int IsFinalized(const void *item)
		{
#ifdef MMGC_MEMORY_INFO
			GC *gc = GetGC(item);	
			item = GetRealPointer(item);
			GCAssert(gc->IsPointerToGCPage(item));
#endif 			
			if (GCLargeAlloc::IsLargeBlock(item)) {
				return GCLargeAlloc::IsFinalized(item);
			} else {
				return GCAlloc::IsFinalized(item);
			}
		}

		static int HasWeakRef(const void *item)
		{
#ifdef MMGC_MEMORY_INFO
			GC *gc = GetGC(item);	
			item = GetRealPointer(item);
			GCAssert(gc->IsPointerToGCPage(item));
#endif 			
			if (GCLargeAlloc::IsLargeBlock(item)) {
				return GCLargeAlloc::HasWeakRef(item);
			} else {
				return GCAlloc::HasWeakRef(item);
			}
		}

		/**
		 * Utility function: Returns the GC object
		 * associated with an allocated object
		 */
		static GC* GetGC(const void *item)
		{
			GC **gc = (GC**) ((uintptr_t)item&~0xfff);
			return *gc;
		}

		/**
		 * Used by sub-allocators to obtain memory.
		 */
		void* AllocBlock(int size, int pageType, bool zero=true, bool canFail=false);

		void FreeBlock(void *ptr, uint32_t size);

		GCHeap *GetGCHeap() const { return heap; }

		void ReapZCT(bool scanStack=true) { zct.Reap(scanStack); }
		bool Reaping() { return zct.IsReaping(); }
#ifdef _DEBUG
		void RCObjectZeroCheck(RCObject *);
#endif

		/**
		 * debugging tool
	     */
		bool IsPointerToGCPage(const void *item);

#ifdef _DEBUG
		// Test whether 'item' points into a non-free object in the gc'd heap.
		bool IsPointerIntoGCObject(const void* item);
#endif

		/**
		 * Do as much marking as possible in time milliseconds
		 */
		void IncrementalMark();

		/**
		 * Perform some garbage collection work: the allocation budget has been exhausted.
		 */
		void CollectionWork();

		/**
		 * Are we currently marking
		 */
		inline bool IncrementalMarking() const { return marking; }

		//////////////////////////////////////////////////////////////////////////
		//
		// Write barrier.  Those that are REALLY_INLINE are defined in WriteBarrier.h,
		// to handle circularity problems: RCObject is not yet defined in GC.h.

	private:
		/**
		 * Perform the actual store of value into *address, adjusting reference counts.
		 */
		/*REALLY_INLINE*/ void WriteBarrierWriteRC(const void *address, const void *value);
		
		/**
		 * Perform the actual store of value into *address.  (This is just a store, but there
		 * is additional error checking in debug builds.)
		 */
		/*REALLY_INLINE*/ void WriteBarrierWrite(const void *address, const void *value);

	private:
		/**
		 * Implementation of privateWriteBarrier; too large to be inlined everywhere.
		 */
		/*REALLY_INLINE*/ void privateInlineWriteBarrier(const void *container, const void *address, const void *value);

		/**
		 * Implementation of privateWriteBarrierRC; too large to be inlined everywhere.
		 */
		/*REALLY_INLINE*/ void privateInlineWriteBarrierRC(const void *container, const void *address, const void *value);

	public:
		/**
		 * General, conditional write barrier trap.  Tests that incremental marking is in fact ongoing
		 * and that the container is black (marked, not queued); if so, it makes the container gray
		 * by scheduling it for rescanning at some appropriate time.
		 *
		 * Container must be a non-NULL untagged pointer to the beginning of an object on a page
		 * owned by this GC.
		 */
		void WriteBarrierTrap(const void *container);
		
		/**
		 * Inline implementation of WriteBarrierTrap; probably too large to be inlined everywhere
		 * so use sparingly.  AvmCore::atomWriteBarrier benefits substantially from inlining it.
		 */
		/*REALLY_INLINE*/ void InlineWriteBarrierTrap(const void *container);
		
		/**
		 * Standard write barrier write for non-RC values.  If marking is ongoing, and the 'container'
		 * is black (ie marked and not queued) and the 'value' is white (ie unmarked and not queued) 
		 * then make sure to queue value for marking.
		 *
		 * Finally stores value into *address.
		 *
		 * 'container' may be NULL, in which case this is just a store and 'address' can be arbitrary.
		 *
		 * If 'container' is not NULL then it is constrained as for WriteBarrierTrap, and 'address'
		 * must point into the object referenced by 'container'.
		 *
		 * Value can be anything except a reference to an RCObject.
		 *
		 * This is called by the WB macro in WriteBarrier.h - not an API to be used otherwise.
		 */
		void privateWriteBarrier(const void *container, const void *address, const void *value);

		/**
		 * A write barrier that finds the container's address and the container's
		 * GC and then performs a standard write barrier operation (see privateWriteBarrier).
		 * Finally stores value into *address.
		 */
		static void WriteBarrier(const void *address, const void *value);
		
		/**
		 * Standard write barrier write for RC values.  If marking is ongoing, and the 'container'
		 * is black (ie marked and not queued) and the 'value' is white (ie unmarked and not queued) 
		 * then make sure to queue value for marking.
		 *
		 * Finally stores value into *address, adjusting the reference counts of both the old value
		 * and the new value.
		 *
		 * 'container' is constrained as for WriteBarrierTrap.
		 *
		 * 'address' must point into the object referenced by 'container'.  The value there, if not
		 * NULL, must be a (possibly tagged) pointer to an RC object.
		 *
		 * 'value', if not NULL, must point to an RC object owned by this GC.
		 * 
		 * This is called by the WBRC macro in WriteBarrier.h - not an API to be used otherwise.
		 */
		void privateWriteBarrierRC(const void *container, const void *address, const void *value);
		
		/**
		 * A write barrier that finds the container's address and the container's GC
		 * and then performs a standard RC write barrier operation (see privateWriteBarrierRC).
		 */
		static void WriteBarrierRC(const void *address, const void *value);

		/**
		 * Host API: if 'address' points to a GC page (it can point into an object, not just
		 * to the start of one), and 'value' points to a GC page as well, record that 'value'
		 * has been stored into the object containing 'address'.
		 *
		 * The function does /not/ perform the store (this is what NoSubstitute means).
		 *
		 * The function works for both RCObjects and GCObjects.
		 */
		void ConservativeWriteBarrierNoSubstitute(const void *address, const void *value);

		/**
		 * Host API: 'container' must be a non-NULL untagged pointer to the beginning of an
		 * object on a page owned by this GC.  Record that 'value' has been stored into
		 * 'container'.
		 *
		 * The function does /not/ perform the store (this is what NoSubstitute means).
		 *
		 * The function works for both RCObjects and GCObjects.
		 */
		void WriteBarrierNoSubstitute(const void *container, const void *value);
		
	public:
		GCPolicyManager policy;
		
	public:

		bool ContainsPointers(const void *item);

		void *FindBeginning(const void *gcItem)
		{
			GCAssert(gcItem != NULL);
			GCAssert(GetPageMapValue((uintptr_t)gcItem) != 0);
			void *realItem = NULL;
			if((uintptr_t)gcItem < memStart || (uintptr_t)gcItem >= memEnd)
				return NULL;
			int bits = GetPageMapValue((uintptr_t)gcItem);
			switch(bits)
			{
			case kGCAllocPage:
				realItem = GCAlloc::FindBeginning(gcItem);
				break;
			case kGCLargeAllocPageFirst:
				realItem = GCLargeAlloc::FindBeginning(gcItem);
				break;
			case kGCLargeAllocPageRest:
				while(bits == kGCLargeAllocPageRest)
				{
					gcItem = (void*) ((uintptr_t)gcItem - GCHeap::kBlockSize);
					bits = GetPageMapValue((uintptr_t)gcItem);
				}
				realItem = GCLargeAlloc::FindBeginning(gcItem);
				break;
			default:
				return NULL;
			}		
#ifdef MMGC_MEMORY_INFO
			realItem = GetUserPointer(realItem);
#endif
			return realItem;
		}

		bool IsRCObject(const void *);

		/**
		 * True during Sweep phase.  Application code can use this to
		 * determine if it's being called (directly or indirectly) from a
		 * finalizer.
		 *
		 * @see IsGCRunning()
		 */
		bool Collecting() const
		{
			return collecting;
		}

		bool IsGCMemory (const void *);

		static double duration(uint64_t start) 
		{
			return (double(VMPI_getPerformanceCounter() - start) * 1000) / VMPI_getPerformanceFrequency();
		}

		/** GC initialization time, in ticks.  Used for logging. */
		const uint64_t t0;

		// a tick is the unit of VMPI_getPerformanceFrequency()
		static uint64_t ticksToMicros(uint64_t ticks) 
		{ 
			return (ticks*1000000)/VMPI_getPerformanceFrequency();
		}


		static uint64_t ticksToMillis(uint64_t ticks) 
		{ 
			return (ticks*1000)/VMPI_getPerformanceFrequency();
		}

		/**
		 * Total number of bytes of pointer-containing memory scanned by this
		 * GC.  Used to measure marking rate, which is
		 * <code>bytesMarked()/ticksToMillis(markTicks())</code>.
		 */
		uint64_t bytesMarked() { return policy.bytesMarked(); }

		/**
		 * Total time spent doing incremental marking, in ticks.  See
		 * bytesMarked.
		 */
		uint64_t markTicks() { return policy.timeStartIncrementalMark + policy.timeIncrementalMark; }

		// calls to mark item
		uint32_t lastStartMarkIncrementCount;
		uint32_t markIncrements() { return (uint32_t)policy.countIncrementalMark; }

		/**
		 * Number of calls to MarkItem().
		 */
		uint32_t marks() { return (uint32_t)policy.objectsMarked(); }

		/**
		 * Number of calls to Sweep().
		 */
        uint32_t sweeps;

		/**
		 * Number of calls to MarkItem() during the current (or most recent)
		 * IncrementalMark().
		 */
		uint32_t numObjects;

		/**
		 * Number of bytes scanned in MarkItem() during the current (or most
		 * recent) IncrementalMark().
		 */
		size_t objSize;

		/**
		 * Time of the latest FinishIncrementalMark() call, in ticks.
		 */
		uint64_t sweepStart;

		// called at some apropos moment from the mututor, ideally at a point
		// where all possible GC references will be below the current stack pointer
		// (ie in memory we can safely zero).  This will return right away if
		// called more than once between collections so it can safely be called
		// a lot without impacting performance
		void CleanStack(bool force=false);

		bool Destroying() { return destroying; }

		static GCWeakRef *GetWeakRef(const void *obj);
		
		// a WeakRef that always refers to null. useful if you need one.
		GCWeakRef* emptyWeakRef;

		void ClearWeakRef(const void *obj);

		// legacy API that gets physical start of OS thread
		uintptr_t GetOSStackTop() const;
		uintptr_t GetStackTop() const
		{
			// temporary crutch until we're moved over to the MMGC_GCENTER system
			if(stackEnter == NULL)
				return GetOSStackTop();
			return GetStackEnter();
		}

		uintptr_t GetStackEnter() const 
		{ 
			return (uintptr_t)stackEnter; 
		}

		// for deciding a tree of things should be scanned from presweep
		void PushWorkItem_MayFail(GCWorkItem &item);
		
	private:

		// item represents a container object, now marked 'Queued', that should be re-scanned 
		// at some later time because it's been stored into.
		void PushBarrierItem(GCWorkItem &item);
		
		// heapAlloc is like heap->Alloc except that it also calls policy.signalBlockAllocation
		// if the allocation succeeded.
		void *heapAlloc(size_t size, int flags=GCHeap::kExpand|GCHeap::kZero|GCHeap::kProfile);

		// heapFree is like heap->Free except that it also calls policy.signalBlockDeallocation.
		void heapFree(void *ptr, size_t siz=0, bool profile=true);

		friend class GCAutoEnter;
		friend class GCAutoEnterPause;
		void SetStackEnter(GCAutoEnter *enter);
		GCAutoEnter *GetAutoEnter() { return stackEnter; }

 		vmpi_spin_lock_t const m_gcLock;

 		bool onThread() { return VMPI_currentThread() == m_gcThread; }

		// store a handle to the thread that create the GC to ensure thread safety
		vmpi_thread_t m_gcThread;

		void gclog(const char *format, ...);
		void log_mem(const char *name, size_t s, size_t comp );

		const static int kNumSizeClasses = 40;

		// FIXME: only used for FixedAlloc, GCAlloc sized dynamically
		const static int kPageUsableSpace = 3936;

		uint32_t *GetBits(int numBytes, int sizeClass);

		void FreeBits(uint32_t *bits, int sizeClass)
		{
#ifdef _DEBUG
			for(int i=0, n=noPointersAllocs[sizeClass]->m_numBitmapBytes; i<n;i++) GCAssert(((uint8_t*)bits)[i] == 0);
#endif
			*(uint32_t**)bits = m_bitsFreelists[sizeClass];
			m_bitsFreelists[sizeClass] = bits;
		}

		uint32_t *m_bitsFreelists[kNumSizeClasses];
		uint32_t *m_bitsNext;

		GCHashtable weakRefs;

		bool destroying;

		// we track the top and bottom of the stack for cleaning purposes.
		// the top tells us how far up the stack as been dirtied.
		// the bottom is also tracked so we can ensure we're on the same
		// stack that the GC responsible for cleaning.  necessary if multiple
		// threads use the GC.  only thread that creates the GC will have its stack
		// tracked and cleaned.
		bool stackCleaned;
		const void *rememberedStackTop;
		GCAutoEnter* stackEnter;

		GCRoot* emptyWeakRefRoot;

		/**
		 * True if incremental marking is on and some objects have been marked.
		 * This means write barriers are enabled.
		 *
		 * The GC thread may read and write this flag.  Application threads in
		 * requests have read-only access.
		 *
		 * Note that this is not obviously exclusive with 'collecting'.
		 */
		bool marking;
		GCStack<GCWorkItem> m_incrementalWork;
		void StartIncrementalMark();
		void FinishIncrementalMark(bool scanStack);

		bool m_markStackOverflow;
		void HandleMarkStackOverflow();
		void SignalMarkStackOverflow(GCStack<GCWorkItem> &stack, GCWorkItem& item);
		
		GCStack<GCWorkItem> m_barrierWork;
		void CheckBarrierWork();
		void FlushBarrierWork();

		// Set at initialization to the deepest recursion level MarkItem is allowed to
		// reach.  Managed entirely within MarkItem.
		uint32_t mark_item_recursion_control;
		
		int IsWhite(const void *item);

		// Used heavily by the write barrier.
		REALLY_INLINE bool IsMarkedThenMakeQueued(const void* userptr)
		{
			const void* realptr = GetRealPointer(userptr);
			if (GCLargeAlloc::IsLargeBlock(realptr))
				return GCLargeAlloc::IsMarkedThenMakeQueued(realptr);
			else
				return GCAlloc::IsMarkedThenMakeQueued(realptr);
		}

		// Used heavily by GC::Free.
		REALLY_INLINE bool IsQueued(const void* userptr)
		{
			const void* realptr = GetRealPointer(userptr);
			if (GCLargeAlloc::IsLargeBlock(realptr))
				return GCLargeAlloc::IsQueued(realptr);
			else
				return GCAlloc::IsQueued(realptr);
		}

		const static int16_t kSizeClasses[kNumSizeClasses];		
		const static uint8_t kSizeClassIndex[246];

		void *m_contextVars[GCV_COUNT];

		// bitmap for what pages are in use, 2 bits for every page
		// 0 - not in use
		// 1 - used by GCAlloc
		// 3 - used by GCLargeAlloc

		uintptr_t memStart;
		uintptr_t memEnd;

		/**
		 * The bitmap for what pages are in use.  Any access to either the
		 * pageMap pointer or the bitmap requires pageMapLock.
		 *
		 * (Note: A better synchronization scheme might be to use atomic
		 * operations to read and write the pageMap pointer, writing it only
		 * from within m_lock; and then using atomic read and write
		 * operations--on Intel x86, these are just ordinary reads and
		 * writes--to access the bitmap, with writes again only from within
		 * m_lock.  This would require reallocating the pageMap more often,
		 * but at least write barriers wouldn't have to acquire the spinlock.)
		 */
		unsigned char *pageMap;

		inline int GetPageMapValue(uintptr_t addr) const
		{
			return GetPageMapValueAlreadyLocked(addr);
		}

		int GetPageMapValueAlreadyLocked(uintptr_t addr) const;

		/**
		 * Set the pageMap bits for the given address.  Those bits must be
		 * zero beforehand.
		 */
		void SetPageMapValue(uintptr_t addr, int val);

		/**
		 * Zero out the pageMap bits for the given address.
		 */
		void ClearPageMapValue(uintptr_t addr);

		void MarkGCPages(void *item, uint32_t numpages, int val);
		void UnmarkGCPages(void *item, uint32_t numpages);
		
		/**
		 * Mark a region of memory, this will search all memory pointed to recursively
		 * and mark any GC Objects it finds
		 */
		void ConservativeMarkRegion(const void *base, size_t bytes);

		GCAlloc *containsPointersAllocs[kNumSizeClasses];
		GCAlloc *containsPointersRCAllocs[kNumSizeClasses];
		GCAlloc *noPointersAllocs[kNumSizeClasses];
		GCLargeAlloc *largeAlloc;
		GCHeap *heap;

	private:

#ifdef _DEBUG
		public:
#endif

		void ClearMarks();
#ifdef _DEBUG
		private:
#endif


#ifdef _DEBUG
		public:
		// Sometimes useful for mutator to call this.  Returns true if it succeeded, false if there was
		// a mark stack overflow.
		bool Trace(const void *stackStart=NULL, uint32_t stackSize=0);
		private:
#endif

		void Finalize();
		void Sweep(bool force=false);
		void ForceSweep() { Sweep(true); }
		void MarkAllRoots(GCStack<GCWorkItem>& work);
		void Mark(GCStack<GCWorkItem> &work);
		void MarkQueueAndStack(GCStack<GCWorkItem> &work, bool scanStack=true);
		void MarkItem(GCStack<GCWorkItem> &work)
		{
			GCWorkItem workitem = work.Pop();
			MarkItem(workitem, work);
		}
		void MarkItem(GCWorkItem &workitem, GCStack<GCWorkItem> &work);

		/**
		 * True during the sweep phase of collection.  Several things have to
		 * behave a little differently during this phase.  For example,
		 * GC::Free() does nothing during sweep phase; otherwise finalizers
		 * could be called twice.
		 *
		 * Also, Collect() uses this to protect itself from recursive calls
		 * (from badly behaved finalizers).
		 */
		bool collecting;
 
		bool finalizedValue;

		void AddToSmallEmptyBlockList(GCAlloc::GCBlock *b)
		{
			b->next = smallEmptyPageList;
			smallEmptyPageList = b;
		}

		/**
		 * List of pages to be swept, built up in Finalize.
		 */
		GCAlloc::GCBlock *smallEmptyPageList;
		
		void AddToLargeEmptyBlockList(GCLargeAlloc::LargeBlock *lb)
		{
			lb->next = largeEmptyPageList;
			largeEmptyPageList = lb;
		}

		/**
		 * List of pages to be swept, built up in Finalize.
		 */
		GCLargeAlloc::LargeBlock *largeEmptyPageList;
		
		vmpi_spin_lock_t const m_rootListLock;

		GCRoot *m_roots;
		void AddRoot(GCRoot *root);
		void RemoveRoot(GCRoot *root);
		
		/**
		 * Points to the head of a linked list of callback objects.
		 */
		GCCallback *m_callbacks;
		void AddCallback(GCCallback *cb);
		void RemoveCallback(GCCallback *cb);

		// Deferred ref counting implementation
		ZCT zct;
		void AddToZCT(RCObject *obj) { zct.Add(obj); }
		// public for one hack from Interval.cpp - no one else should call
		// this out of the GC
public:
		void RemoveFromZCT(RCObject *obj) { zct.Remove(obj); }
private:

		static const void *Pointer(const void *p) { return (const void*)(((uintptr_t)p)&~7); }

public:
		void DumpMemoryInfo();
#ifdef MMGC_MEMORY_PROFILER
		void DumpPauseInfo();
#endif
		
private:

		void PushWorkItem(GCStack<GCWorkItem> &stack, GCWorkItem item);		// defined in GC.cpp, always inlined in callers there

#ifdef _DEBUG		
		void CheckFreelist(GCAlloc *gca);
		void CheckFreelists();


		int m_gcLastStackTrace;

		/**
		 * Used by FindUnmarkedPointers.
		 */
		void UnmarkedScan(const void *mem, size_t size);

		/**
		 * Find unmarked pointers in the entire heap.
		 */
		void FindUnmarkedPointers();

		// methods for incremental verification

		/**
		 * Scan a region of memory for white pointers. Used by
		 * FindMissingWriteBarriers.
		 */
		void WhitePointerScan(const void *mem, size_t size);

		/**
		 * Scan all GC memory (skipping roots). If a GC object is black make sure
		 * it has no pointers to white objects.
		 */
		void FindMissingWriteBarriers();

#endif

public:
#ifdef MMGC_MEMORY_INFO
		typedef void (*pDumpBackCallbackProc)(void* pContext, void *obj, const char *type );
		void DumpBackPointerChain(void *o, pDumpBackCallbackProc p = NULL, void *context = NULL);

		// debugging routine that records who marked who, can be used to
		// answer the question, how did I get marked?  also could be used to
		// find false positives by verifying the back pointer chain gets back
		// to a GC root
		static void WriteBackPointer(const void *item, const void *container, size_t itemSize);
#endif
#ifdef _DEBUG
		// Dump a list of objects that have pointers to the given location.
		void WhosPointingAtMe(void* me, int recurseDepth=0, int currentDepth=0);

		/**
		 * Used by WhosPointingAtMe.
		 */
    	void ProbeForMatch(const void *mem, size_t size, uintptr_t value, int recurseDepth, int currentDepth);
#endif

		void DumpAlloc(GCAlloc *a, size_t& internal_waste, size_t& overhead);
		
		//This method returns the number bytes allocated for GC objects
		size_t GetBytesInUse();
		
		//This method is for more fine grained allocation details
		//It reports the total number of bytes requested (i.e. ask size) and
		//the number of bytes actually allocated.  The latter is the same
		//number as reported by GetBytesInUse()
		void GetUsageInfo(size_t& totalAskSize, size_t& totalAllocated);

		size_t GetNumBlocks() { return (size_t)policy.blocksOwnedByGC(); }

		virtual void memoryStatusChange(MemoryStatus oldStatus, MemoryStatus newStatus);

		/* A portable replacement for alloca().
		 *
		 * Memory is allocated from the heap and not from the stack.  It is freed in 
		 * one of two ways: If the function returns normally then an auto_ptr like
		 * mechanism frees the memory.  If the function leaves by throwing an exception
		 * (or if one of its callees throws an exception) then the exception
		 * handling mechanism in Exception.{h,cpp} frees the memory by releasing 
		 * everything that is still allocated that was allocated since the exception
		 * handler was erected.
		 *
		 * The auto_ptr mechanism, based on the class MMgc::GC::AllocaAutoPtr, cannot be
		 * circumvented, as allocaPush() takes a reference to such an object as an argument.
		 *
		 * Typical usage:
		 *
		 *    MMgc::GC::AllocaAutoPtr _ptr;                      // by convention prefixed by "_"
		 *    int* ptr = (int*)core->allocaPush(_ptr, nbytes);  // by convention same name, no "_"
		 *
		 * In practice the VMPI_alloca() macro, defined in avmbuild.h, should be used so that
		 * real alloca() can be used on platforms where that makes sense.
		 *
		 * Benchmarks suggest that the performance differences from using this mechanism
		 * instead of real alloca() are slight to nonexistent, and that the heap allocation
		 * sometimes provides a performance improvement.
		 */
	public:
		struct AllocaStackSegment
		{
			void* start;				// first address; also, the RCRoot pointer
			void* limit;				// address past data
			void* top;					// address past live if this segment is not the top
			AllocaStackSegment* prev;	// segments further from the top
		};
		
		void allocaInit();
		void allocaShutdown();
		void allocaPopToSlow(void* top);
		void* allocaPushSlow(size_t nbytes);
		void pushAllocaSegment(size_t nbytes);
		void popAllocaSegment();

		AllocaStackSegment* top_segment;// segment at the stack top
		void* stacktop;					// current first free word in top_segment
#ifdef _DEBUG
		size_t stackdepth;				// useful to have for debugging
#endif
		
	public:
		/* See documentation above */
		
		class AllocaAutoPtr
		{
			friend class GC;
		public:
			AllocaAutoPtr() : gc(NULL), unwindPtr(NULL) {}  // initialization of 'gc' to pacify gcc
			~AllocaAutoPtr() { if (unwindPtr) gc->allocaPopTo(unwindPtr); }
		private:
			GC* gc;
			void* unwindPtr;
		};
				
		inline void* allocaTop() 
		{
			return stacktop;
		}
		
		inline void allocaPopTo(void* top)
		{ 
			if (top >= top_segment->start && top <= top_segment->limit)
				stacktop = top;
			else
				allocaPopToSlow(top);
		}
	
		inline void* allocaPush(size_t nbytes, AllocaAutoPtr& x) 
		{
			GCAssert(x.unwindPtr == NULL);
			x.gc = this;
			x.unwindPtr = stacktop;
			nbytes = (nbytes + 7) & ~7;
			if ((char*)stacktop + nbytes <= top_segment->limit) {
				stacktop = (char*)stacktop + nbytes;
				return x.unwindPtr;
			}
			return allocaPushSlow(nbytes);
		}
	};

	// helper class to wipe out vtable pointer of members for DRC
	class Cleaner
	{
	public:
		Cleaner() {}
		// don't let myself move between objects
		Cleaner& operator=(const Cleaner& /*rhs*/) { return *this; }
		void set(const void * _v, size_t _size) { this->v = (int*)_v; this->size = _size; }
		~Cleaner() { 
			if(v) 
				VMPI_memset(v, 0, size);
			v = 0; 
			size = 0;
		}
		int *v;
		size_t size;
	};

	inline GCWorkItem::GCWorkItem(const void *p, uint32_t s, bool isGCItem)
		: ptr(p)
		, _size(s | uint32_t(isGCItem))
	{
#ifdef _DEBUG
		if (IsGCItem()) {
			GCAssert(GC::GetGC(p)->FindBeginning(p) == p);
		}
#endif
	}

	/**
	 * Stack object that takes care of many things including defining stack
	 * boundaries, doing low stack queued collections/ZCT reaps and stack cleaning
	 */
	class GCAutoEnter
	{
	public:
		GCAutoEnter(GC *gc);
		~GCAutoEnter();
		void Destroy() { gc = NULL; }
	private:
		GC* gc;
	};

	/**
	 * Undoes the affects of GCAutoEnter on entry and reapplies them upon exit
	 */
	class GCAutoEnterPause
	{
	public:
		GCAutoEnterPause(GC *gc);
		~GCAutoEnterPause();
	private:
		GC* gc;
		GCAutoEnter *enterSave;
	};
}

#endif /* __GC__ */
