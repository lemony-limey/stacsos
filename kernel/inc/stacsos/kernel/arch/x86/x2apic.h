/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

#include <stacsos/kernel/arch/x86/msr.h>
#include <stacsos/kernel/arch/x86/x2apic-structures.h>

namespace stacsos::kernel::arch::x86
{
class x86_core;

class x2apic
{
public:
	x2apic(x86_core & owner)
		: owner_(owner)
		, timer_frequency_(0)
	{}

	void init();

	// Argument is the local vector table register, which is an integer enum value used in the switch statement to determine 
	// which of the 6 LVT registers (there are 7 total, but the LVTError register is not included below) should have its
	// interrupts masked.
	//
	// The interrupts are masked by setting bit 16 (zero-indexed, so the 17th bit).
	void mask_interrupts(x2apic_lvts lvt)
	{
		switch (lvt)
		{
		case x2apic_lvts::error:
			// Bit 16 is the LVT interrupt mask bit.
			msr::write(msr_indicies::X2APIC_LVT_ERROR, msr::read(msr_indicies::X2APIC_LVT_ERROR) | 0x00010000);
			break;
		case x2apic_lvts::lint0:
			msr::write(msr_indicies::X2APIC_LVT_LINT0, msr::read(msr_indicies::X2APIC_LVT_LINT0) | 0x00010000);
			break;
		case x2apic_lvts::lint1:
			msr::write(msr_indicies::X2APIC_LVT_LINT1, msr::read(msr_indicies::X2APIC_LVT_LINT1) | 0x00010000);
			break;
		// PMU = Power Management Unit
		case x2apic_lvts::pmu:
			msr::write(msr_indicies::X2APIC_LVT_PERFMON, msr::read(msr_indicies::X2APIC_LVT_PERFMON) | 0x00010000);
			break;
		case x2apic_lvts::timer:
			msr::write(msr_indicies::X2APIC_LVT_TIMER, msr::read(msr_indicies::X2APIC_LVT_TIMER) | 0x00010000);
			break;
		case x2apic_lvts::thermal:
			msr::write(msr_indicies::X2APIC_LVT_THERMAL, msr::read(msr_indicies::X2APIC_LVT_THERMAL) | 0x00010000);
			break;
		}
	}

	/// @brief Same as mask_interrupts(), but the interrupts are unmasked.
	/// @param lvt The LVT register that interrupts should be unmasked for.
	void unmask_interrupts(x2apic_lvts lvt)
	{
		switch (lvt)
		{
		case x2apic_lvts::error:
			msr::write(msr_indicies::X2APIC_LVT_ERROR, msr::read(msr_indicies::X2APIC_LVT_ERROR) & ~0x00010000);
			break;
		case x2apic_lvts::lint0:
			msr::write(msr_indicies::X2APIC_LVT_LINT0, msr::read(msr_indicies::X2APIC_LVT_LINT0) & ~0x00010000);
			break;
		case x2apic_lvts::lint1:
			msr::write(msr_indicies::X2APIC_LVT_LINT1, msr::read(msr_indicies::X2APIC_LVT_LINT1) & ~0x00010000);
			break;
		case x2apic_lvts::pmu:
			msr::write(msr_indicies::X2APIC_LVT_PERFMON, msr::read(msr_indicies::X2APIC_LVT_PERFMON) & ~0x00010000);
			break;
		case x2apic_lvts::timer:
			msr::write(msr_indicies::X2APIC_LVT_TIMER, msr::read(msr_indicies::X2APIC_LVT_TIMER) & ~0x00010000);
			break;
		case x2apic_lvts::thermal:
			msr::write(msr_indicies::X2APIC_LVT_THERMAL, msr::read(msr_indicies::X2APIC_LVT_THERMAL) & ~0x00010000);
			break;
		}
	}

	// Indicates the servicing of the current interrupt has completed and LAPIC can serve next interrupt from
	// the ISR register. Intel SDM Vol. 3, 13.8.5.
	// TODO: Has no effect if bit 12 of the spurious interrupt vector is set?
	void eoi() { msrs::x2apic_eoi = 0; }

	void set_timer_irq(u8 irq)
	{
		// Read the LVT timer register.
		u64 lvt = msr::read(msr_indicies::X2APIC_LVT_TIMER);
		// Least significant 8 bits in the negated mask will be 0, so these low 8 bits will be cleared in
		// the value of the lvt variable, which is the value just read in from the LVT_TIMER MSR.
		lvt &= ~0xffull;
		// Set the low 8 bits corresponding to the IQR value.
		// The & 0xff means any bits higher are ignored.
		lvt |= ((u64)irq) & 0xff;
		// Write the changes back to the LVT timer register.
		msr::write(msr_indicies::X2APIC_LVT_TIMER, lvt);
	}

	// Set the divide configuration register.
	void set_timer_divide(u8 v) { msr::write(msr_indicies::X2APIC_TIMER_DCR, v); }

	// If timer initial count is set to 0, the LAPIC timer is disabled.
	// If the initial count is modified, the timer restarts.
	void set_timer_initial_count(u32 v) { msr::write(msr_indicies::X2APIC_TIMER_ICR, v); }

	void set_timer_periodic()
	{
		u64 lvt = msr::read(msr_indicies::X2APIC_LVT_TIMER);
		lvt |= 0x00020000;
		msr::write(msr_indicies::X2APIC_LVT_TIMER, lvt);
	}

	void set_timer_one_shot()
	{
		u64 lvt = msr::read(msr_indicies::X2APIC_LVT_TIMER);
		lvt &= ~0x00020000;
		msr::write(msr_indicies::X2APIC_LVT_TIMER, lvt);
	}

	u32 get_timer_current_count() { return msr::read(msr_indicies::X2APIC_TIMER_CCR); }

	u64 get_timer_frequency() const { return timer_frequency_; }

	// Set Interrupt Command Register.
	// TODO: What does the ICR do? Get a description.
	void set_icr(const x2apic_icr & icr) { msr::write(msr_indicies::X2APIC_ICR, icr.bits); }

	void activate_periodic_tick(u64 frequency)
	{
		set_timer_periodic();
		// 3 = 0b0011, which is divide by 16 mode for the DCR.
		set_timer_divide(3);
		// Bitshift right by 4 is sort of equivalent to dividing by 16.
		set_timer_initial_count((timer_frequency_ >> 4) / frequency);
	}

	// What is remote init?
	// Most likely for other cores.
	void send_remote_init(u32 target)
	{
		x2apic_icr v;

		v.destination = target;
		v.delivery_mode = icr_delivery_mode::init;
		v.trigger_mode = icr_trigger_mode::level;
		v.level = icr_level::assert;

		// dprintf("icr: %016lx\n", v.bits);
		set_icr(v);
	}

	// What is remote SIPI?
	// Is this what is needed to start another core? Most likely.
	void send_remote_sipi(u32 target, u8 pfn)
	{
		x2apic_icr v;

		v.destination = target;
		v.vector = pfn;
		v.delivery_mode = icr_delivery_mode::startup;
		v.trigger_mode = icr_trigger_mode::level;

		// dprintf("icr: %016lx\n", v.bits);
		set_icr(v);
	}

	x86_core & owner() const { return owner_; }

private:
	x86_core & owner_;
	u64 timer_frequency_;

	void calibrate_timer();
};
} // namespace stacsos::kernel::arch::x86
