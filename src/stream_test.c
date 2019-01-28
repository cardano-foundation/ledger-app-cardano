#ifdef DEVEL

#include "stream.h"
#include <os.h>
#include <stdbool.h>
#include "assert.h"
#include "test_utils.h"
#include "state.h"

static ins_tests_context_t* ctx = &(instructionState.testsContext);

void _run_stream_test(stream_t* s)
{
	PRINTF("run_stream_test\n");
	stream_init(s);
	// empty
	{
		EXPECT_EQ(stream_availableBytes(s), 0);

		EXPECT_EQ(stream_unusedBytes(s), STREAM_BUFFER_SIZE);

		EXPECT_THROWS( stream_peekByte(s), ERR_NOT_ENOUGH_INPUT);
	}


	// [1, 2, 3, 4]
	const uint8_t data[] = {1, 2, 3, 4};
	stream_appendData(s, data, 4);
	{

		EXPECT_EQ(stream_availableBytes(s), 4);

		EXPECT_EQ(stream_unusedBytes(s), STREAM_BUFFER_SIZE - 4);

		EXPECT_EQ(stream_peekByte(s), 1);
	}

	// [3, 4]
	stream_advancePos(s, 2);
	{

		EXPECT_EQ(stream_availableBytes(s), 2);

		EXPECT_EQ(stream_peekByte(s), 3);

	}

	// no-op from external point of view
	stream_shift(s);
	{

		EXPECT_EQ(stream_availableBytes(s), 2);

		EXPECT_EQ(stream_peekByte(s), 3);
	}

	// [3, 4, 1, 2, 3, 4]
	stream_appendData(s, data, 4);
	{

		EXPECT_EQ(stream_availableBytes(s), 6);

		EXPECT_EQ(stream_peekByte(s), 3);
	}

	// test excess read
	{
		EXPECT_THROWS(stream_advancePos(s, 10), ERR_NOT_ENOUGH_INPUT);

		EXPECT_EQ(stream_peekByte(s), 3);
	}

	// consume rest
	{
		stream_advancePos(s, 1);
		EXPECT_EQ(stream_peekByte(s), 4);

		stream_advancePos(s, 1);
		EXPECT_EQ(stream_peekByte(s), 1);

		stream_advancePos(s, 1);
		EXPECT_EQ(stream_peekByte(s), 2);

		stream_advancePos(s, 1);
		EXPECT_EQ(stream_peekByte(s), 3);

		stream_advancePos(s, 1);
		EXPECT_EQ(stream_peekByte(s), 4);

		stream_advancePos(s, 1);
		EXPECT_THROWS(stream_peekByte(s), ERR_NOT_ENOUGH_INPUT);
	}
}

void run_stream_test()
{
	_run_stream_test(&ctx->s);
}

#endif
