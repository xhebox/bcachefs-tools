// SPDX-License-Identifier: GPL-2.0

#include <linux/bitops.h>
#include <linux/string.h>
#include <asm/unaligned.h>

#include "varint.h"

/**
 * bch2_varint_encode - encode a variable length integer
 * @out - destination to encode to
 * @v	- unsigned integer to encode
 *
 * Returns the size in bytes of the encoded integer - at most 9 bytes
 */
int bch2_varint_encode(u8 *out, u64 v)
{
	unsigned bits = fls64(v|1);
	unsigned bytes = DIV_ROUND_UP(bits, 7);

	if (likely(bytes < 9)) {
		v <<= bytes;
		v |= ~(~0 << (bytes - 1));
		v = cpu_to_le64(v);
		memcpy(out, &v, bytes);
	} else {
		*out++ = 255;
		bytes = 9;
		put_unaligned_le64(v, out);
	}

	return bytes;
}

/**
 * bch2_varint_decode - encode a variable length integer
 * @in	- varint to decode
 * @end	- end of buffer to decode from
 * @out	- on success, decoded integer
 *
 * Returns the size in bytes of the decoded integer - or -1 on failure (would
 * have read past the end of the buffer)
 */
int bch2_varint_decode(const u8 *in, const u8 *end, u64 *out)
{
	unsigned bytes = likely(in < end)
		? ffz(*in & 255) + 1
		: 1;
	u64 v;

	if (unlikely(in + bytes > end))
		return -1;

	if (likely(bytes < 9)) {
		v = 0;
		memcpy(&v, in, bytes);
		v = le64_to_cpu(v);
		v >>= bytes;
	} else {
		v = get_unaligned_le64(++in);
	}

	*out = v;
	return bytes;
}

/**
 * bch2_varint_encode_fast - fast version of bch2_varint_encode
 *
 * This version assumes it's always safe to write 8 bytes to @out, even if the
 * encoded integer would be smaller.
 */
int bch2_varint_encode_fast(u8 *out, u64 v)
{
	unsigned bits = fls64(v|1);
	unsigned bytes = DIV_ROUND_UP(bits, 7);

	if (likely(bytes < 9)) {
		v <<= bytes;
		v |= ~(~0 << (bytes - 1));
	} else {
		*out++ = 255;
		bytes = 9;
	}

	put_unaligned_le64(v, out);
	return bytes;
}

/**
 * bch2_varint_decode_fast - fast version of bch2_varint_decode
 *
 * This version assumes that it is safe to read at most 8 bytes past the end of
 * @end (we still return an error if the varint extends past @end).
 */
int bch2_varint_decode_fast(const u8 *in, const u8 *end, u64 *out)
{
	u64 v = get_unaligned_le64(in);
	unsigned bytes = ffz(v & 255) + 1;

	if (unlikely(in + bytes > end))
		return -1;

	if (likely(bytes < 9)) {
		v >>= bytes;
		v &= ~(~0ULL << (7 * bytes));
	} else {
		v = get_unaligned_le64(++in);
	}

	*out = v;
	return bytes;
}
