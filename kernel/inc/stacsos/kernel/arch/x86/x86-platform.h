/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

#include <stacsos/kernel/arch/platform.h>

namespace stacsos::kernel::arch::x86
{
class ioapic;

// Given that DEFINE_SINGLETON is used, is this class considered to only have one instance
// which is static throughout the duration of the program?
//
// It is safe to assume that there is only 1 IOAPIC because there is only 
// one [multi-core] CPU in the system.
class x86_platform : public platform
{
public:
	DEFINE_SINGLETON(x86_platform);

	x86_platform()
		: ioapic_(nullptr)
	{}

	// Probes ACPI tables.
	virtual void probe() override;

	ioapic* get_ioapic() const
	{
		if (!ioapic_)
		{
			panic("no ioapic registered");
		}

		return ioapic_;
	}

	void set_ioapic(ioapic* i) { ioapic_ = i; }

private:
	ioapic* ioapic_;
};
} // namespace stacsos::kernel::arch::x86
