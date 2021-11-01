#include "../cbor++.h"

#include <vector>

using namespace cbor::literals;

/*
 * https://stackoverflow.com/questions/39049803/google-test-cant-find-user-provided-equality-operator
 *
 * explains the reasons for this nasty hack.
 */
namespace testing::internal {

/*
 * Teach C++ how to compare things like vector<byte> and array<byte>
 */
template<class L, class R>
requires requires (L l, R r) {
	size(l);
	size(r);
	begin(l);
	begin(r);
	end(l);
} && std::is_same_v<typename L::value_type, typename R::value_type>
bool
operator==(const L &l, const R &r)
{
	if (size(l) != size(r))
		return false;
	return std::equal(begin(l), end(l), begin(r));
}

}

#include <gtest/gtest.h>

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
	EXPECT_THROW(c[0].get<cbor::tag>(), std::runtime_error);
	EXPECT_THROW(c[0].get_bytes(), std::runtime_error);
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
	EXPECT_THROW(c[0].get<cbor::tag>(), std::runtime_error);
	EXPECT_THROW(c[0].get_bytes(), std::runtime_error);
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
	EXPECT_THROW(c[0].get<cbor::tag>(), std::runtime_error);
	EXPECT_THROW(c[0].get_bytes(), std::runtime_error);
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
	EXPECT_EQ(c[0].type(), cbor::type::fp32);
	EXPECT_THROW(c[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(c[0].get<int>(), std::runtime_error);
	EXPECT_THROW(c[0].get<cbor::tag>(), std::runtime_error);
	EXPECT_THROW(c[0].get_bytes(), std::runtime_error);
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
	EXPECT_EQ(c[0].type(), cbor::type::fp32);
	EXPECT_THROW(c[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(c[0].get<int>(), std::runtime_error);
	EXPECT_THROW(c[0].get<cbor::tag>(), std::runtime_error);
	EXPECT_THROW(c[0].get_bytes(), std::runtime_error);
}

TEST(codec, fp_negative_infinity)
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
	EXPECT_EQ(c[0].type(), cbor::type::fp32);
	EXPECT_THROW(c[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(c[0].get<int>(), std::runtime_error);
	EXPECT_THROW(c[0].get<cbor::tag>(), std::runtime_error);
	EXPECT_THROW(c[0].get_bytes(), std::runtime_error);
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
		EXPECT_THROW(c[i].get<cbor::tag>(), std::runtime_error);
		EXPECT_THROW(c[i].get_bytes(), std::runtime_error);
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
		EXPECT_THROW(c[i].get<cbor::tag>(), std::runtime_error);
		EXPECT_THROW(c[i].get_bytes(), std::runtime_error);
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
		EXPECT_THROW(c[i].get<cbor::tag>(), std::runtime_error);
		EXPECT_THROW(c[i].get_bytes(), std::runtime_error);
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
		EXPECT_THROW(c[i].get<cbor::tag>(), std::runtime_error);
		EXPECT_THROW(c[i].get_bytes(), std::runtime_error);
	}
}

TEST(codec, int_qword)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	c.push_back(4294967296);
	c.push_back(18446744073709551615u);
	c.push_back(-4294967297);
	c.push_back(-9223372036854775807 - 1);	// big integer literals are fun

	/* verify */
	std::array exp{
		0x1b_b, 0x00_b, 0x00_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	    // unsigned(4294967296)
		0x1b_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b,	    // unsigned(18446744073709551615)
		0x3b_b, 0x00_b, 0x00_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	    // negative(4294967296)
		0x3b_b, 0x7f_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b	    // negative(9223372036854775807)
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_EQ(c[0].get<int64_t>(), 4294967296);
	EXPECT_EQ(c[1].get<uint64_t>(), 18446744073709551615u);
	EXPECT_EQ(c[2].get<int64_t>(), -4294967297);
	EXPECT_EQ(c[3].get<int64_t>(), -9223372036854775807 - 1);

	/* type checks */
	EXPECT_EQ(c[0].type(), cbor::type::int64);
	EXPECT_EQ(c[1].type(), cbor::type::uint64);
	EXPECT_EQ(c[2].type(), cbor::type::int64);
	EXPECT_EQ(c[3].type(), cbor::type::int64);
	EXPECT_THROW(c[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(c[3].get<int32_t>(), std::range_error);
	for (size_t i = 0; i < size(c); ++i) {
		EXPECT_THROW(c[i].get<float>(), std::runtime_error);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(c[i].get<cbor::tag>(), std::runtime_error);
		EXPECT_THROW(c[i].get_bytes(), std::runtime_error);
	}
}

TEST(codec, int_qword_overflow)
{
	/* CBOR can encode 64-bit -ve numbers which don't fit in int64_t */
	std::array buf {
		0x3b_b, 0x80_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	    // negative(9223372036854775808)
		0x3b_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b	    // negative(18446744073709551615)
	};
	cbor::codec c(buf);

	/* overflow checks */
	EXPECT_THROW(c[0].get<int64_t>(), std::range_error);
	EXPECT_THROW(c[1].get<int64_t>(), std::range_error);
}

TEST(codec, bignum)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	/* encode */
	std::array int256{
		0xdd_b, 0xf7_b, 0xff_b, 0x5e_b, 0xbd_b, 0x9d_b, 0x66_b, 0xce_b,
		0x16_b, 0x14_b, 0x66_b, 0xc1_b, 0xc0_b, 0x26_b, 0x24_b, 0x30_b,
		0xfa_b, 0x04_b, 0xde_b, 0x32_b, 0xb0_b, 0xe4_b, 0x20_b, 0xee_b,
		0x3f_b, 0x48_b, 0x9e_b, 0x2e_b, 0x21_b, 0x12_b, 0xe3_b, 0x86_b
	};
	c.push_back(cbor::tag::pos_bignum);
	c.push_back(int256);

	/* verify */
	std::array exp{
		0xc2_b,			// tag(2)
		0x58_b, 0x20_b,		// bytes(32)
		0xdd_b, 0xf7_b, 0xff_b, 0x5e_b, 0xbd_b, 0x9d_b, 0x66_b, 0xce_b,
		0x16_b, 0x14_b, 0x66_b, 0xc1_b, 0xc0_b, 0x26_b, 0x24_b, 0x30_b,
		0xfa_b, 0x04_b, 0xde_b, 0x32_b, 0xb0_b, 0xe4_b, 0x20_b, 0xee_b,
		0x3f_b, 0x48_b, 0x9e_b, 0x2e_b, 0x21_b, 0x12_b, 0xe3_b, 0x86_b
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	EXPECT_EQ(c[0].get<cbor::tag>(), cbor::tag::pos_bignum);
	EXPECT_EQ(c[1].get_bytes(), int256);

	/* type checks */
	EXPECT_EQ(c[0].type(), cbor::type::tag);
	EXPECT_EQ(c[1].type(), cbor::type::bytes);
}

TEST(codec, tags)
{
	std::vector<std::byte> buf;
	cbor::codec c(buf);

	std::array tags = {
		static_cast<cbor::tag>(0),
		static_cast<cbor::tag>(23),
		static_cast<cbor::tag>(24),
		static_cast<cbor::tag>(255),
		static_cast<cbor::tag>(65536),
		static_cast<cbor::tag>(4294967296),
	};
	std::array invalid_tags =  {
		cbor::tag::invalid_1,
		cbor::tag::invalid_2,
		cbor::tag::invalid_3,
	};

	/* encode */
	for (auto &t : tags)
		c.push_back(t);
	for (auto &t : invalid_tags)
		EXPECT_THROW(c.push_back(t), std::invalid_argument);
	c.push_back(0);

	/* verify */
	std::array exp{
		0xc0_b,                                     // tag(0)
		0xd7_b,					    // tag(23)
		0xd8_b, 0x18_b,                             // tag(24)
		0xd8_b, 0xff_b,				    // tag(255)
		0xda_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b,	    // tag(65536)
		0xdb_b, 0x00_b, 0x00_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	// tag(4294967296)
		0x00_b
	};
	ASSERT_EQ(buf, exp);

	/* decode */
	for (auto i = 0u; i < size(tags); ++i)
		EXPECT_EQ(c[i].get<cbor::tag>(), tags[i]);
	EXPECT_EQ(c[size(tags)].get<int>(), 0);

	/* type checks */
	for (auto i = 0u; i < size(tags); ++i) {
		EXPECT_EQ(c[i].type(), cbor::type::tag);
		EXPECT_THROW(c[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(c[i].get<int>(), std::runtime_error);
		EXPECT_THROW(c[i].get<float>(), std::runtime_error);
		EXPECT_THROW(c[i].get_bytes(), std::runtime_error);
	}
}
