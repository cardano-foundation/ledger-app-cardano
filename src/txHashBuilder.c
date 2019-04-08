#include "common.h"
#include "txHashBuilder.h"
#include "hash.h"
#include "cbor.h"
#include "cardano.h"
#include "crc32.h"
#include "bufView.h"

// Syntactic sugar
#define BUILDER_APPEND_CBOR(type, value) \
	blake2b_256_append_cbor(&builder->txHash, type, value)

#define BUILDER_APPEND_DATA(buffer, bufferSize) \
	blake2b_256_append(&builder->txHash, buffer, bufferSize)


void blake2b_256_append_cbor(
        blake2b_256_context_t* hashCtx,
        uint8_t type, uint64_t value
)
{
	uint8_t buffer[10];
	size_t size = cbor_writeToken(type, value, buffer, SIZEOF(buffer));
	blake2b_256_append(hashCtx, buffer, size);
}

void txHashBuilder_init(tx_hash_builder_t* builder)
{
	blake2b_256_init(&builder->txHash);
	{
		// main preamble
		BUILDER_APPEND_CBOR(CBOR_TYPE_ARRAY, 3);
	}
	builder->state = TX_HASH_BUILDER_INIT;
}

void txHashBuilder_enterInputs(tx_hash_builder_t* builder)
{
	ASSERT(builder->state == TX_HASH_BUILDER_INIT);
	{
		// Enter inputs
		BUILDER_APPEND_CBOR(CBOR_TYPE_ARRAY_INDEF, 0);
	}
	builder->state = TX_HASH_BUILDER_IN_INPUTS;
}

size_t cborEncodeUtxoInner(
        const uint8_t* utxoHashBuffer, size_t utxoHashSize,
        uint32_t utxoIndex,
        uint8_t* outBuffer, size_t outSize
)
{
	ASSERT(utxoHashSize == 32);
	ASSERT(outSize < BUFFER_SIZE_PARANOIA);

	write_view_t out = make_write_view(outBuffer, outBuffer + outSize);


	// Array(2)[
	//   Bytes(32)[tx hash],
	//   Unsigned[output number]
	// ]
	{
		view_appendToken(&out, CBOR_TYPE_ARRAY, 2);
		{
			// Note: No tag because hash is not cbor
			view_appendToken(&out, CBOR_TYPE_BYTES, utxoHashSize);
			view_appendData(&out, utxoHashBuffer, utxoHashSize);
		} {
			view_appendToken(&out, CBOR_TYPE_UNSIGNED, utxoIndex);
		}
	}

	return view_processedSize(&out);
}

void txHashBuilder_addUtxoInput(
        tx_hash_builder_t* builder,
        const uint8_t* utxoHashBuffer, size_t utxoHashSize,
        uint32_t utxoIndex
)
{
	ASSERT(builder->state == TX_HASH_BUILDER_IN_INPUTS);

	// Array(2)[
	//    Unsigned[0],
	//    Tag(24):Bytes[utxo cbor]
	// ]
	{
		BUILDER_APPEND_CBOR(CBOR_TYPE_ARRAY, 2);
		{
			BUILDER_APPEND_CBOR(CBOR_TYPE_UNSIGNED, CARDANO_INPUT_TYPE_UTXO);

		} {
			BUILDER_APPEND_CBOR(CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);

			// We need about 1 + 2 + 32 + 1 + 0...8
			uint8_t tmpBuffer[50];
			size_t tmpSize = cborEncodeUtxoInner(
			                         utxoHashBuffer, utxoHashSize,
			                         utxoIndex,
			                         tmpBuffer, SIZEOF(tmpBuffer)
			                 );

			ASSERT(tmpSize <= SIZEOF(tmpBuffer));

			BUILDER_APPEND_CBOR(CBOR_TYPE_BYTES, tmpSize);
			BUILDER_APPEND_DATA(tmpBuffer, tmpSize);
		}
	}
}

void txHashBuilder_enterOutputs(tx_hash_builder_t* builder)
{
	ASSERT(builder->state == TX_HASH_BUILDER_IN_INPUTS);
	{
		// End inputs
		BUILDER_APPEND_CBOR(CBOR_TYPE_INDEF_END, 0);
		// Enter outputs
		BUILDER_APPEND_CBOR(CBOR_TYPE_ARRAY_INDEF, 0);
	}
	builder->state = TX_HASH_BUILDER_IN_OUTPUTS;
}

void txHashBuilder_addOutput(
        tx_hash_builder_t* builder,
        const uint8_t* rawAddressBuffer, size_t rawAddressSize,
        uint64_t amount
)
{
	ASSERT(builder->state == TX_HASH_BUILDER_IN_OUTPUTS);

	// Array(2)[
	//   Array(2)[
	//      Tag(24):Bytes[raw address],
	//      Unsigned[checksum]
	//   ],
	//   Unsigned[amount]
	// ]
	{
		BUILDER_APPEND_CBOR(CBOR_TYPE_ARRAY, 2);
		{
			// Note(ppershing): this reimplements cborPackRawAddressWithChecksum
			// without requiring temporary stack

			BUILDER_APPEND_CBOR(CBOR_TYPE_ARRAY, 2);
			{
				BUILDER_APPEND_CBOR(CBOR_TYPE_TAG, CBOR_TAG_EMBEDDED_CBOR_BYTE_STRING);
				BUILDER_APPEND_CBOR(CBOR_TYPE_BYTES, rawAddressSize);
				BUILDER_APPEND_DATA(rawAddressBuffer, rawAddressSize);
			} {
				uint32_t checksum = crc32(rawAddressBuffer, rawAddressSize);
				BUILDER_APPEND_CBOR(CBOR_TYPE_UNSIGNED, checksum);
			}
		} {
			// address output amount
			BUILDER_APPEND_CBOR(CBOR_TYPE_UNSIGNED, amount);
		}
	}
}

void txHashBuilder_enterMetadata(tx_hash_builder_t* builder)
{
	ASSERT(builder->state == TX_HASH_BUILDER_IN_OUTPUTS);
	{
		// End outputs
		BUILDER_APPEND_CBOR(CBOR_TYPE_INDEF_END, 0);
		// Enter attributes
		BUILDER_APPEND_CBOR(CBOR_TYPE_MAP, 0);
		// End of attributes
	}
	builder->state = TX_HASH_BUILDER_IN_METADATA;
}


void txHashBuilder_finalize(tx_hash_builder_t* builder, uint8_t* outBuffer, size_t outSize)
{
	ASSERT(builder->state == TX_HASH_BUILDER_IN_METADATA);
	ASSERT(outSize == 32);
	{
		blake2b_256_finalize(&builder->txHash, outBuffer, outSize);
	}
	builder->state = TX_HASH_BUILDER_FINISHED;
}
