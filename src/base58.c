/*******************************************************************************
*   Ripple Wallet
*   (c) 2017 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

// This code is slightly modified version of Ripple's code

#include "base58.h"
#include "assert.h"
#include <os.h>

static const uint8_t MAX_BUFFER_SIZE = 124;

static const unsigned char BASE58ALPHABET[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

unsigned char encode_base58(const unsigned char *in, unsigned char length,
                            unsigned char *out, unsigned char maxoutlen)
{
	unsigned char tmp[MAX_BUFFER_SIZE];
	unsigned char buffer[MAX_BUFFER_SIZE * 2];
	unsigned char j;
	unsigned char startAt;
	unsigned char zeroCount = 0;

	ASSERT(length <= MAX_BUFFER_SIZE);

	os_memmove(tmp, in, length);

	while ((zeroCount < length) && (tmp[zeroCount] == 0)) {
		++zeroCount;
	}
	j = 2 * length;
	startAt = zeroCount;

	while (startAt < length) {
		unsigned short remainder = 0;
		unsigned char divLoop;
		for (divLoop = startAt; divLoop < length; divLoop++) {
			unsigned short digit256 = (unsigned short)(tmp[divLoop] & 0xff);
			unsigned short tmpDiv = remainder * 256 + digit256;
			tmp[divLoop] = (unsigned char)(tmpDiv / 58);
			remainder = (tmpDiv % 58);
		}
		if (tmp[startAt] == 0) {
			++startAt;
		}
		buffer[--j] = BASE58ALPHABET[remainder];
	}
	while ((j < (2 * length)) && (buffer[j] == BASE58ALPHABET[0])) {
		++j;
	}
	while (zeroCount-- > 0) {
		buffer[--j] = BASE58ALPHABET[0];
	}
	length = 2 * length - j;

	ASSERT(length <= maxoutlen);

	os_memmove(out, (buffer + j), length);
	return length;
}
