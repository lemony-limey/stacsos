/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once
#include <stacsos/kernel/arch/x86/irq/irq-manager.h>

namespace stacsos::kernel::arch::x86
{
class x2apic;

// I/O APIC does not use define singleton, potentially because there could be multiple if this is used on a mult-CPU system?
// I/O APIC instances are only ever stored in x86_platform, which does use define_singleton.
class ioapic
{
public:
	ioapic(void* base_address)
		: base_address_((volatile u32*)base_address)
		, nr_irqs_(0)
	{}

	void initialise();

	void allocate_physical_irq(u32 phys_irq_nr, x86_core & target_core, irq::irq_handler_fn handler_fn, void* handler_fn_arg = nullptr);
	void map_physical_irq(u32 phys_irq_nr, x86_core & target_core, u8 target_irq_nr);

private:
	volatile u32* base_address_;
	u8 nr_irqs_;  // Total number supported?

	void write(u32 reg, u32 val)
	{
		// 
		base_address_[0] = reg;  // Put register index in IOREGSEL
		__sync_synchronize();    // a barrier, needed so that IOREGSEL write fully completes before IOREGWIN write begins.
		// u32 = 4 bytes. 4 lots of 4 bytes = 16 bytes. 0x10 = 16.
		// base_address_[4] = base_address_ + (4 * sizeof(u32)) = base_address_ + (4 * 4) = base_address_ + 16 = base_address_ + 0x10.
		base_address_[4] = val;  // Put value in IOREGWIN, which should be base_address_[0] + 0x10.
	}

	u32 read(u32 reg)
	{
		base_address_[0] = reg;
		__sync_synchronize();    // a barrier, needed so that IOREGSEL write fully completes before IOREGWIN read begins.
		return base_address_[4];
	}

	// Not saved anywhere by the I/O APIC, simply used to represent the parsed values and make it easier to access them.
	struct redirection_entry
	{
		union
		{
			struct
			{
				// BUG: Should this be two u32s, not 2 u64s?
				// Maybe not, because this allows them to be bitshifted.
				u64 low, high;
			} __packed;

			struct
			{
				u8 vector;
				u8 delivery_mode : 3;
				u8 destination_mode : 1; // 0 = Physical destination; 1 = Logical destination.
				u8 delivery_status : 1;  // 0 = IRQ is relaxed and waiting; 1 = Sent but waiting to be delivered.
				u8 pin_polarity : 1;  // 0 = active high; 1 = active low.
				u8 remote_irr : 1;
				u8 trigger_mode : 1;  // 0 = Edge; 1 = Level.
				u8 mask : 1;  // 1 means IRQ is masked.
				u8 destination;  // APIC ID of CPU that you want to receive the interrupt.
			} __packed;
		};
	} __packed;

	void store_redir_entry(u8 irq_index, const redirection_entry & e);
};
} // namespace stacsos::kernel::arch::x86
