/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/kernel/arch/core-manager.h>
#include <stacsos/kernel/arch/core.h>
#include <stacsos/kernel/arch/timer.h>
#include <stacsos/kernel/arch/x86/cregs.h>
#include <stacsos/kernel/arch/x86/machine-context.h>
#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/mem/memory-manager.h>
#include <stacsos/kernel/mem/page-allocator.h>
#include <stacsos/kernel/sched/schedulable-entity.h>

using namespace stacsos::kernel::arch;
using namespace stacsos::kernel::arch::x86;
using namespace stacsos::kernel::sched;
using namespace stacsos::kernel::mem;
using namespace stacsos::kernel;

core & core::this_core()
{
	auto & c = core_manager::get().get_core(core::this_core_id());
	assert(&c != nullptr);

	return c;
}

static void idle_thread()
{
	while (true) {
		__relax();
	}
}

// Is this an intrinsic?
extern "C" __noreturn void x86_return_to_task();

// starts running the scheduler on that core.
void core::run()  // run_scheduler would be a better name
{
	// This routine MUST be called from the same core, i.e. the core that is executing must call the run for
	// the core object that represents it.
	if (core::this_core_id() != id())
	{
		panic("run must be called on executing core");
	}

	// Initialise the IDLE thread, for doing nothing when there are no tasks to run.
	void* idle_thread_stack = memory_manager::get()
		.pgalloc()
		.allocate_pages(0, page_allocation_flags::zero)
		.to_page()
		.base_address_ptr();

	// A zeroed page is allocated for the stack.
	idle_thread_.mcontext = (machine_context *)idle_thread_stack;
	// The idle thread runs in kernel mode.
	idle_thread_.mcontext->cs = KERNEL_CODE_SEGMENT_SELECTOR;
	idle_thread_.mcontext->ss = KERNEL_DATA_SEGMENT_SELECTOR;
	idle_thread_.mcontext->rflags = 0x202; // RSVD | IF. RSVD = . IF = .
	// idle_thread() spins forever. %rip points here as all the intructions in this function will be executing relative to here.
	idle_thread_.mcontext->rip = (u64)idle_thread; 
	// The top of the stack is the starting position, because the stack grows down.
	idle_thread_.mcontext->rsp = (u64)idle_thread_stack + PAGE_SIZE;  
	// GS holds a pointer to the current TCB in stacsos.
	idle_thread_.mcontext->gs = (u64)&idle_thread_;  
	// What is "effective" cr3? Is this virtual, or is it "effective" because it's what the process sees?
	idle_thread_.cr3 = memory_manager::get()
		.root_address_space()
		.pgtable()
		.effective_cr3();  
	// In this case, the same address that rsp points to, because this is operating in kernel mode.
	idle_thread_.kernel_stack = (u64)idle_thread_stack + PAGE_SIZE;  

	set_current_tcb(&idle_thread_);

	dprintf("core [%d]: run\n", id());
	local_timer().start(100); // 100 Hz for preemption, uses LAPIC timer because it is in periodic mode.

	// This will also enable interrupts, because the IF flag is set in rflags.
	x86_return_to_task();  // returns into the first thread
	__unreachable();
}

/// @brief Schedule a new task. This is called during preemptive multitasking.
///  The task that should be switched to is determined by the scheduling algorithm.
void core::schedule()
{
	tcb* current = get_current_tcb();
	u64 now = __builtin_ia32_rdtsc();

	// If the current tcb address in GS is not null, update the timer stats.
	// This is used in scheduling later on.
	if (current)
	{
		// Update the current task's runtime.
		u64 delta = now - current->start_time;
		current->run_time += delta;
	}

	// Select the next task for execution
	// TODO: Check task quantum expiry
	tcb* next = sched_alg_->select_next_task(current);
	if (!next)
	{
		next = &idle_thread_;
	}

	// Update the next task's start time
	next->start_time = now;

	// Activate the task.
	set_current_tcb(next);
}

// Essentially refreshes the time to represent the current moment, now.
void core::update_clock()
{
	// Update the internal clock
	u64 now = __builtin_ia32_rdtsc();
	clock_ += now - last_clock_;

	last_clock_ = now;
}
