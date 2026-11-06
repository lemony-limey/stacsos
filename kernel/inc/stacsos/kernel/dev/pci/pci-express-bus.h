/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

#include <stacsos/kernel/dev/bus.h>

namespace stacsos::kernel::dev::pci
{
class pci_express_bus : public bus
{
public:
	// Base address comes from ACPI tables (MCFG)
	// first_bus_id = starting bus number
	// last_bus_id = ending bus number
	pci_express_bus(bus & parent, void* base, int first_bus_id, int last_bus_id)
		: bus(parent)
		, base_(base)
		, first_(first_bus_id)
		, last_(last_bus_id)
	{
	}

	virtual void probe() override;

private:
	void* base_;          // Base address parsed from the MCFG.
	int   first_, last_;  // starting and ending bus number
	void probe_bus(unsigned int bus);
	void probe_slot(unsigned int bus, unsigned int slot);
	void probe_func(unsigned int bus, unsigned int slot, unsigned int func);

	// Compute the base address of a device on the given bus in a given slot and function within the slot.
	// Why is it 20, 15 and 12?
	void* compute_base(unsigned int bus, unsigned int slot, unsigned int func)
	{
		return (void*)((uintptr_t)base_ + ((bus - first_) << 20 | slot << 15 | func << 12));
	}
};
} // namespace stacsos::kernel::dev::pci
