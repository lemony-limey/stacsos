/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

namespace stacsos::kernel::arch
{
class platform;
}

namespace stacsos::kernel::dev
{
class device;
class device_manager;
class system_bus;

class bus
{
	friend class device;
	friend class system_bus;
	friend class arch::platform;

public:
	bus(bus & parent)
		: parent_(&parent)
	{}

	virtual void probe() = 0;

private:
	bus()
		: parent_(nullptr)
	{}

	bus* parent_;
};

// The root bus of the system?
// When is this actually used though? Other buses have this listed as the parent
// bus, but is probe actually ever called? If so, it does nothing.
class system_bus : public bus
{
	friend class device_manager;

	virtual void probe() override { }

private:
	system_bus()
		: bus()
	{}
};
} // namespace stacsos::kernel::dev
