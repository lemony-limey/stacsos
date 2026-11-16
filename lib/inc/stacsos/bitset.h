/* SPDX-License-Identifier: MIT */

/* StACSOS - Utility Library
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#pragma once

#include <stacsos/memops.h>

namespace stacsos
{
template <size_t nr_bits, typename bit_word = unsigned long> class bitset
{
	static_assert(nr_bits != 0);
	static const size_t bits_per_word = sizeof(bit_word) * 8;
	static const size_t nr_words = (nr_bits + (bits_per_word - 1)) / bits_per_word;

public:
	struct bitref
	{
		bit_word & word;
		u64 offset;

		bitref & operator=(bool v)
		{
			word = (word & (~((bit_word)1 << offset))) | (((bit_word) !!v) << offset);
			return *this;
		}

		operator bool() const { return !!(word & ((bit_word)1 << offset)); }
	};

	bitref operator[](u64 index) { return bitref { words_[index / bits_per_word], index % bits_per_word }; }

	// Big-endian or little-endian?
	u64 find_first_zero()
	{
		for (u64 i = 0; i < nr_words; i++)
		{
			// Binary negation of the current u32. We need the cleared bits set, and vice versa, to look for the bits set.
			bit_word tmp = ~words_[i];

			// FFSLL = Find First bit Set in word (Long Long variant)
			int first_set = __builtin_ffsll(tmp);
			// 0 is returned if no bits are set in first_set. Otherwise, the position of the bit (i.e., the index + 1) is returned.
			if (first_set != 0)
			{
				// Return the index of the bit.
				return (u64)(first_set - 1) + (i * sizeof(bit_word) * 8);
			}
		}

		// Return the max value, which will be out of range.
		return ~0ull;
	}

	void bitset() { memops::bzero(words_, sizeof(words_)); }

private:
	// bit_word = unsigned long = u32
	bit_word words_[nr_words];
};
} // namespace stacsos
