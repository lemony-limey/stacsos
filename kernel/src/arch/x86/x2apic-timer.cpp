/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/kernel/arch/x86/machine-context.h>
#include <stacsos/kernel/arch/x86/x2apic-timer.h>
#include <stacsos/kernel/arch/x86/x2apic.h>
#include <stacsos/kernel/arch/x86/x86-core.h>
#include <stacsos/kernel/debug.h>
#include <stacsos/kernel/sched/sleeper.h>

using namespace stacsos::kernel;
using namespace stacsos::kernel::arch::x86;
using namespace stacsos::kernel::sched;

// The function signature has to be this way because it is defined as a newtype alias
// in arch/x86/irq-manager.h
void x2apic_timer::timer_irq_handler(u8 irq, void* context, void* arg)
{
	// Converts the argument pointer set in x2apic_timer::init() back
	// into an x2apic_timer*.
	x2apic_timer* timer = (x2apic_timer*) arg;
	timer->lapic_.owner().update_clock();

	// Check all timer things that are waiting to be woken up.
	// This should use a priority queue, but doesn't.
	sleeper::get().check_wakeup();

	// This is how preemption is done. Switch to the next scheduled task here.
	timer->lapic_.owner().schedule();

	// Send the EOI signal.
	timer->lapic_.eoi();
}

void x2apic_timer::init()
{ 
	lapic_.set_timer_irq(
		// x2apic::owner() returns the x86_core that owns this x2apic.
		lapic_.owner()  
			// Get a handle to the irq_manager
			.irqmgr()   
			// Allocate the handler function that will be called. Passes pointer to this x2apic_timer
			// instance as an argument.
			.allocate_irq(timer_irq_handler, this)  
	);
}
