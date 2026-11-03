/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

#include <stacsos/list.h>
#include <stacsos/kernel/lock.h>

namespace stacsos::kernel::sched
{
class thread;

template <bool AUTO_RESET> class event
{
public:
	event()
		: triggered_(false)
	{
	}

	void trigger();
	void wait();
	void reset() { triggered_ = false; }

private:
	bool           triggered_;
	spinlock_irq   lock_;
	list<thread *> wait_list_;
};

using auto_reset_event   = event<true>;
using manual_reset_event = event<false>;

} // namespace stacsos::kernel::sched
