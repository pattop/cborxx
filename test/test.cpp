#include "../cbor++.h"

#include <gtest/gtest.h>

using namespace cbor::literals;

/*
 * https://stackoverflow.com/questions/39049803/google-test-cant-find-user-provided-equality-operator
 *
 * explains the reasons for this nasty hack.
 */
namespace testing::internal {

template<class T, std::size_t N>
bool
operator==(const std::vector<T> &l, const std::array<T, N> &r)
{
	if (size(l) != size(r))
		return false;
	return std::equal(begin(l), end(l), begin(r));
}

}

TEST(codec, null)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(nullptr);

	/* verify */
	std::array exp{
		0xf6_b		// null
	};
	ASSERT_EQ(buf, exp);

	/* type checks */
	EXPECT_EQ(c[0].type(), cbor::type::null);
	EXPECT_THROW(c[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(c[0].get<int>(), std::runtime_error);
	EXPECT_THROW(c[0].get<float>(), std::runtime_error);
}

TEST(codec, bool_true)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(true);

	/* verify */
	std::array exp{
		0xf5_b		// true
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_TRUE(c[0].get<bool>());

	/* type checks */
	EXPECT_EQ(c[0].type(), cbor::type::boolean);
	EXPECT_THROW(c[0].get<int>(), std::runtime_error);
	EXPECT_THROW(c[0].get<float>(), std::runtime_error);

}

TEST(codec, bool_false)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(false);

	/* verify */
	std::array exp{
		0xf4_b		// false
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_FALSE(c[0].get<bool>());

	/* type checks */
	EXPECT_EQ(c[0].type(), cbor::type::boolean);
	EXPECT_THROW(c[0].get<int>(), std::runtime_error);
	EXPECT_THROW(c[0].get<float>(), std::runtime_error);
}

TEST(codec, fp_nan)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(std::numeric_limits<float>::quiet_NaN());
	c.push_back(std::numeric_limits<double>::quiet_NaN());

	/* verify */
	std::array exp{
		0xf9_b, 0x7e_b, 0x00_b,	    // fp16 NaN
		0xf9_b, 0x7e_b, 0x00_b	    // fp16 NaN
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_TRUE(std::isnan(c[0].get<float>()));
	EXPECT_TRUE(std::isnan(c[1].get<double>()));

	/* type checks */
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_EQ(c[i].type(), cbor::type::fp32);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(c[i].get<int>(), std::runtime_error);
	}
}

TEST(codec, fp_positive_infinity)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(std::numeric_limits<float>::infinity());
	c.push_back(std::numeric_limits<double>::infinity());

	/* verify */
	std::array exp{
		0xf9_b, 0x7c_b, 0x00_b,	    // fp16 +infinity
		0xf9_b, 0x7c_b, 0x00_b	    // fp16 +infinity
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_GT(c[0].get<float>(), 0);
	EXPECT_TRUE(std::isinf(c[0].get<float>()));
	EXPECT_GT(c[1].get<double>(), 0);
	EXPECT_TRUE(std::isinf(c[0].get<double>()));

	/* type checks */
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_EQ(c[i].type(), cbor::type::fp32);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(c[i].get<int>(), std::runtime_error);
	}
}

TEST(codec, negative_infinity)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(-std::numeric_limits<float>::infinity());
	c.push_back(-std::numeric_limits<double>::infinity());

	/* verify */
	std::array exp{
		0xf9_b, 0xfc_b, 0x00_b,	    // fp16 -infinity
		0xf9_b, 0xfc_b, 0x00_b	    // fp16 -infinity
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_LT(c[0].get<float>(), 0);
	EXPECT_TRUE(std::isinf(c[0].get<float>()));
	EXPECT_LT(c[1].get<double>(), 0);
	EXPECT_TRUE(std::isinf(c[0].get<double>()));

	/* type checks */
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_EQ(c[i].type(), cbor::type::fp32);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(c[i].get<int>(), std::runtime_error);
	}
}

TEST(codec, int_inline)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(0);
	c.push_back(23);
	c.push_back(-1);
	c.push_back(-24);

	/* verify */
	std::array exp{
		0x00_b,		    // unsigned(0)
		0x17_b,		    // unsigned(23)
		0x20_b,		    // negative(0)
		0x37_b,		    // negative(23)
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_EQ(c[0].get<int>(), 0);
	EXPECT_EQ(c[1].get<int>(), 23);
	EXPECT_EQ(c[2].get<int>(), -1);
	EXPECT_EQ(c[3].get<int>(), -24);

	/* type checks */
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_EQ(c[i].type(), cbor::type::int32);
		EXPECT_THROW(c[i].get<float>(), std::runtime_error);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
	}
}

TEST(codec, int_byte)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(24);
	c.push_back(255);
	c.push_back(-25);
	c.push_back(-256);

	/* verify */
	std::array exp{
		0x18_b, 0x18_b,	    // unsigned(24)
		0x18_b, 0xff_b,	    // unsigned(255)
		0x38_b, 0x18_b,	    // negative(24)
		0x38_b, 0xff_b	    // negative(255)
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_EQ(c[0].get<int>(), 24);
	EXPECT_EQ(c[1].get<int>(), 255);
	EXPECT_EQ(c[2].get<int>(), -25);
	EXPECT_EQ(c[3].get<int>(), -256);

	/* type checks */
	EXPECT_THROW(c[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<int8_t>(), std::range_error);
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_EQ(c[i].type(), cbor::type::int32);
		EXPECT_THROW(c[i].get<float>(), std::runtime_error);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
	}
}

TEST(codec, int_word)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(256);
	c.push_back(65535);
	c.push_back(-257);
	c.push_back(-65536);

	/* verify */
	std::array exp{
		0x19_b, 0x01_b, 0x00_b,	    // unsigned(256)
		0x19_b, 0xff_b, 0xff_b,	    // unsigned(65535)
		0x39_b, 0x01_b, 0x00_b,	    // negative(256)
		0x39_b, 0xff_b, 0xff_b	    // negative(65536)
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_EQ(c[0].get<int>(), 256);
	EXPECT_EQ(c[1].get<int>(), 65535);
	EXPECT_EQ(c[2].get<int>(), -257);
	EXPECT_EQ(c[3].get<int>(), -65536);

	/* type checks */
	EXPECT_THROW(c[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<int16_t>(), std::range_error);
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_EQ(c[i].type(), cbor::type::int32);
		EXPECT_THROW(c[i].get<float>(), std::runtime_error);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
	}
}

TEST(codec, int_dword)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(65536);
	c.push_back(4294967295);
	c.push_back(-65537);
	c.push_back(-2147483648);
	c.push_back(-4294967296);

	/* verify */
	std::array exp{
		0x1a_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b,	    // unsigned(65536)
		0x1a_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b,	    // unsigned(4294967295)
		0x3a_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b,	    // negative(65536)
		0x3a_b, 0x7F_b, 0xff_b, 0xff_b, 0xff_b,	    // negative(2147483647)
		0x3a_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b	    // negative(4294967295)
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_EQ(c[0].get<int>(), 65536);
	EXPECT_EQ(c[1].get<uint32_t>(), 4294967295);
	EXPECT_EQ(c[2].get<int>(), -65537);
	EXPECT_EQ(c[3].get<int>(), -2147483648);
	EXPECT_EQ(c[4].get<int64_t>(), -4294967296);

	/* type checks */
	EXPECT_EQ(c[0].type(), cbor::type::int32);
	EXPECT_EQ(c[1].type(), cbor::type::uint32);
	EXPECT_EQ(c[2].type(), cbor::type::int32);
	EXPECT_EQ(c[3].type(), cbor::type::int32);
	EXPECT_EQ(c[4].type(), cbor::type::int64);
	EXPECT_THROW(c[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[4].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[4].get<int32_t>(), std::range_error);
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_THROW(c[i].get<float>(), std::runtime_error);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
	}
}

TEST(codec, int_qword)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(4294967296);
	c.push_back(18446744073709551615ull);
	c.push_back(-4294967297);
	c.push_back(-9223372036854775808ull);

	/* verify */
	std::array exp{
		0x1b_b, 0x00_b, 0x00_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	    // unsigned(4294967296)
		0x1b_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b,	    // unsigned(18446744073709551615)
		0x3b_b, 0x00_b, 0x00_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	    // negative(4294967296)
		0x3b_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b	    // negative(9223372036854775807)
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_EQ(c[0].get<int64_t>(), 4294967296);
	EXPECT_EQ(c[1].get<uint64_t>(), 18446744073709551615ull);
	EXPECT_EQ(c[2].get<int64_t>(), -4294967297);
	EXPECT_EQ(c[3].get<int64_t>(), -9223372036854775808ull);

	/* type checks */
	EXPECT_EQ(c[0].type(), cbor::type::int32);
	EXPECT_EQ(c[1].type(), cbor::type::uint32);
	EXPECT_EQ(c[2].type(), cbor::type::int32);
	EXPECT_EQ(c[3].type(), cbor::type::int64);
	EXPECT_THROW(c[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<int32_t>(), std::range_error);
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_THROW(c[i].get<float>(), std::runtime_error);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
	}
}
