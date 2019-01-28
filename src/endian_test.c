#ifdef DEVEL

#include "endian.h"
#include "assert.h"
#include "test_utils.h"
#include <os.h>

void run_read_test()
{
	const uint8_t* buffer = (uint8_t* )"\x47\x11\x22\x33\x44\x55\x66\x77\x88\x47";


	EXPECT_EQ(u1be_read(buffer), 0x47);
	EXPECT_EQ(u1be_read(buffer + 1), 0x11);
	EXPECT_EQ(u1be_read(buffer + 2), 0x22);

	EXPECT_EQ(u2be_read(buffer), 0x4711);
	EXPECT_EQ(u2be_read(buffer + 1), 0x1122);
	EXPECT_EQ(u2be_read(buffer + 2), 0x2233);

	EXPECT_EQ(u4be_read(buffer), 0x47112233);
	EXPECT_EQ(u4be_read(buffer + 1), 0x11223344);
	EXPECT_EQ(u4be_read(buffer + 2), 0x22334455);

	EXPECT_EQ(u8be_read(buffer), 0x4711223344556677);
	EXPECT_EQ(u8be_read(buffer + 1), 0x1122334455667788);
	EXPECT_EQ(u8be_read(buffer + 2), 0x2233445566778847);
}

void run_read_write_test()
{
	uint8_t buffer[10] = {
		0x44, // prefix
		0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47,
		0x77 // postfix
	};

	u1be_write(buffer + 1, 0x74);
	EXPECT_EQ(u1be_read(buffer), 0x44);
	EXPECT_EQ(u1be_read(buffer + 1), 0x74);
	EXPECT_EQ(u1be_read(buffer + 2), 0x47);


	u2be_write(buffer + 1, 0x2345);
	EXPECT_EQ(u1be_read(buffer), 0x44);
	EXPECT_EQ(u2be_read(buffer + 1), 0x2345);
	EXPECT_EQ(u1be_read(buffer + 3), 0x47);

	u4be_write(buffer + 1, 0x32547698);
	EXPECT_EQ(u1be_read(buffer), 0x44);
	EXPECT_EQ(u4be_read(buffer + 1), 0x32547698);
	EXPECT_EQ(u1be_read(buffer + 5), 0x47);

	u8be_write(buffer + 1, 0xa0b1c2d300112233);
	EXPECT_EQ(u1be_read(buffer), 0x44);
	EXPECT_EQ(u8be_read(buffer + 1), 0xa0b1c2d300112233);
	EXPECT_EQ(u1be_read(buffer + 9), 0x77);
}

void run_endian_test()
{
	PRINTF("run_endian_test\n");

	run_read_test();
	run_read_write_test();
}

#endif
