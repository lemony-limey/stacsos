/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

#include <stacsos/bitset.h>
#include <stacsos/kernel/arch/x86/dt.h>

namespace stacsos::kernel::arch::x86::irq
{
using irq_handler_fn = void (*)(u8 irq, void* ctx, void* arg);

template <int NR_IRQS> class irq_manager
{
public:
	// IRQ manager is a wrapper that contains the IDT, keeps track
	// of used IRQ values, handler functions and handler function
	// arguments.
	irq_manager(interrupt_descriptor_table<NR_IRQS> & idt)
		: idt_(idt)
	{}

	void initialise();

	u8 allocate_irq(irq_handler_fn handler, void* arg);
	void reserve_irq(u8 irq_number, irq_handler_fn handler, void* arg);
	void assign_irq(u8 irq_number, irq_handler_fn handler, void* arg);
	void handle_irq(u8 irq_number, void* mcontext) { handlers_[irq_number](irq_number, mcontext, handler_args_[irq_number]); }

private:
	// The IDT.
	interrupt_descriptor_table<NR_IRQS> & idt_;
	// Keeps track of which entries in the IDT have already been assigned.
	bitset<NR_IRQS> used_irq_;
	// The handler functions associated with each entry in the IDT.
	irq_handler_fn handlers_[NR_IRQS];
	void* handler_args_[NR_IRQS];
};
} // namespace stacsos::kernel::arch::x86::irq
