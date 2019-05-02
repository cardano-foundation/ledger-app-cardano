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

#include "common.h"
#include "base58.h"

static const uint32_t MAX_BUFFER_SIZE = 124;

static const char BASE58ALPHABET[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

size_t encode_base58(
        const uint8_t* inBuffer, size_t inSize,
        char* outStr, size_t outMaxSize
)
{
	uint8_t tmpBuffer[MAX_BUFFER_SIZE];
	uint8_t buffer[MAX_BUFFER_SIZE * 2];
	size_t startAt;
	size_t zeroCount = 0;

	ASSERT(inSize <= SIZEOF(tmpBuffer));
	ASSERT(outMaxSize < BUFFER_SIZE_PARANOIA);

	os_memmove(tmpBuffer, inBuffer, inSize);

	while ((zeroCount < inSize) && (tmpBuffer[zeroCount] == 0)) {
		++zeroCount;
	}
	size_t j = 2 * inSize;
	startAt = zeroCount;

	while (startAt < inSize) {
		unsigned short remainder = 0;
		size_t divLoop;
		for (divLoop = startAt; divLoop < inSize; divLoop++) {
			unsigned short digit256 = (unsigned short)(tmpBuffer[divLoop] & 0xff);
			unsigned short tmpDiv = remainder * 256 + digit256;
			tmpBuffer[divLoop] = (unsigned char)(tmpDiv / 58);
			remainder = (tmpDiv % 58);
		}
		if (tmpBuffer[startAt] == 0) {
			++startAt;
		}
		ASSERT((0 < j) && (j <= SIZEOF(buffer)));
		buffer[--j] = BASE58ALPHABET[remainder];
	}
	while ((j < (2 * inSize)) && (buffer[j] == BASE58ALPHABET[0])) {
		++j;
	}
	while (zeroCount-- > 0) {
		ASSERT((0 < j) && (j <= SIZEOF(buffer)));
		buffer[--j] = BASE58ALPHABET[0];
	}
	size_t outSize = 2 * inSize - j;

	ASSERT(outSize < outMaxSize);

	os_memmove(outStr, (buffer + j), outSize);
	outStr[outSize] = 0;
	return outSize;
}
