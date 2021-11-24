#define CBORXX_DEBUG
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

	/* encode */
	cbor::codec e(buf);
	e.push_back(nullptr);

	/* verify */
	const std::array exp{
		0xf6_b		// null
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::null);
	EXPECT_THROW(d[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(d[0].get<int>(), std::runtime_error);
	EXPECT_THROW(d[0].get<float>(), std::runtime_error);
	EXPECT_THROW(d[0].get_tag(), std::runtime_error);
	EXPECT_THROW(d[0].get_bytes(), std::runtime_error);
	EXPECT_THROW(d[0].get_string(), std::runtime_error);
}

TEST(codec, undefined)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(cbor::undefined());

	/* verify */
	const std::array exp{
		0xf7_b		// undefined
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::undefined);
	EXPECT_THROW(d[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(d[0].get<int>(), std::runtime_error);
	EXPECT_THROW(d[0].get<float>(), std::runtime_error);
	EXPECT_THROW(d[0].get_tag(), std::runtime_error);
	EXPECT_THROW(d[0].get_bytes(), std::runtime_error);
	EXPECT_THROW(d[0].get_string(), std::runtime_error);
}

TEST(codec, bool_true)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(true);

	/* verify */
	const std::array exp{
		0xf5_b		// true
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_TRUE(d[0].get<bool>());

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::boolean);
	EXPECT_THROW(d[0].get<int>(), std::runtime_error);
	EXPECT_THROW(d[0].get<float>(), std::runtime_error);
	EXPECT_THROW(d[0].get_tag(), std::runtime_error);
	EXPECT_THROW(d[0].get_bytes(), std::runtime_error);
	EXPECT_THROW(d[0].get_string(), std::runtime_error);
}

TEST(codec, bool_false)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(false);

	/* verify */
	const std::array exp{
		0xf4_b		// false
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_FALSE(d[0].get<bool>());

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::boolean);
	EXPECT_THROW(d[0].get<int>(), std::runtime_error);
	EXPECT_THROW(d[0].get<float>(), std::runtime_error);
	EXPECT_THROW(d[0].get_tag(), std::runtime_error);
	EXPECT_THROW(d[0].get_bytes(), std::runtime_error);
	EXPECT_THROW(d[0].get_string(), std::runtime_error);
}

TEST(codec, fp_nan)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(
		std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN()
	);

	/* verify */
	const std::array exp{
		0xf9_b, 0x7e_b, 0x00_b,	    // fp16 NaN
		0xf9_b, 0x7e_b, 0x00_b	    // fp16 NaN
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_TRUE(std::isnan(d[0].get<float>()));
	EXPECT_TRUE(std::isnan(d[1].get<double>()));

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::fp32);
	EXPECT_THROW(d[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(d[0].get<int>(), std::runtime_error);
	EXPECT_THROW(d[0].get_tag(), std::runtime_error);
	EXPECT_THROW(d[0].get_bytes(), std::runtime_error);
	EXPECT_THROW(d[0].get_string(), std::runtime_error);
}

TEST(codec, fp_positive_infinity)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<double>::infinity()
	);

	/* verify */
	const std::array exp{
		0xf9_b, 0x7c_b, 0x00_b,	    // fp16 +infinity
		0xf9_b, 0x7c_b, 0x00_b	    // fp16 +infinity
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_GT(d[0].get<float>(), 0);
	EXPECT_TRUE(std::isinf(d[0].get<float>()));
	EXPECT_GT(d[1].get<double>(), 0);
	EXPECT_TRUE(std::isinf(d[0].get<double>()));

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::fp32);
	EXPECT_THROW(d[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(d[0].get<int>(), std::runtime_error);
	EXPECT_THROW(d[0].get_tag(), std::runtime_error);
	EXPECT_THROW(d[0].get_bytes(), std::runtime_error);
	EXPECT_THROW(d[0].get_string(), std::runtime_error);
}

TEST(codec, fp_negative_infinity)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<double>::infinity()
	);

	/* verify */
	const std::array exp{
		0xf9_b, 0xfc_b, 0x00_b,	    // fp16 -infinity
		0xf9_b, 0xfc_b, 0x00_b	    // fp16 -infinity
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_LT(d[0].get<float>(), 0);
	EXPECT_TRUE(std::isinf(d[0].get<float>()));
	EXPECT_LT(d[1].get<double>(), 0);
	EXPECT_TRUE(std::isinf(d[0].get<double>()));

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::fp32);
	EXPECT_THROW(d[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(d[0].get<int>(), std::runtime_error);
	EXPECT_THROW(d[0].get_tag(), std::runtime_error);
	EXPECT_THROW(d[0].get_bytes(), std::runtime_error);
	EXPECT_THROW(d[0].get_string(), std::runtime_error);
}

TEST(codec, int_inline)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(0, 23, -1, -24);

	/* verify */
	const std::array exp{
		0x00_b,		    // unsigned(0)
		0x17_b,		    // unsigned(23)
		0x20_b,		    // negative(0)
		0x37_b,		    // negative(23)
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_EQ(d[0].get<int>(), 0);
	EXPECT_EQ(d[1].get<int>(), 23);
	EXPECT_EQ(d[2].get<int>(), -1);
	EXPECT_EQ(d[3].get<int>(), -24);

	/* type checks */
	for (size_t i = 0; i < size(d); ++i) {
		EXPECT_EQ(d[i].type(), cbor::type::int32);
		EXPECT_THROW(d[i].get<float>(), std::runtime_error);
		EXPECT_THROW(d[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(d[i].get_tag(), std::runtime_error);
		EXPECT_THROW(d[i].get_bytes(), std::runtime_error);
		EXPECT_THROW(d[i].get_string(), std::runtime_error);
	}
}

TEST(codec, int_byte)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(24, 255, -25, -256);

	/* verify */
	const std::array exp{
		0x18_b, 0x18_b,	    // unsigned(24)
		0x18_b, 0xff_b,	    // unsigned(255)
		0x38_b, 0x18_b,	    // negative(24)
		0x38_b, 0xff_b	    // negative(255)
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_EQ(d[0].get<int>(), 24);
	EXPECT_EQ(d[1].get<int>(), 255);
	EXPECT_EQ(d[2].get<int>(), -25);
	EXPECT_EQ(d[3].get<int>(), -256);

	/* type checks */
	EXPECT_THROW(d[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[3].get<int8_t>(), std::range_error);
	for (size_t i = 0; i < size(d); ++i) {
		EXPECT_EQ(d[i].type(), cbor::type::int32);
		EXPECT_THROW(d[i].get<float>(), std::runtime_error);
		EXPECT_THROW(d[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(d[i].get_tag(), std::runtime_error);
		EXPECT_THROW(d[i].get_bytes(), std::runtime_error);
		EXPECT_THROW(d[i].get_string(), std::runtime_error);
	}
}

TEST(codec, int_word)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(256, 65535, -257, -65536);

	/* verify */
	const std::array exp{
		0x19_b, 0x01_b, 0x00_b,	    // unsigned(256)
		0x19_b, 0xff_b, 0xff_b,	    // unsigned(65535)
		0x39_b, 0x01_b, 0x00_b,	    // negative(256)
		0x39_b, 0xff_b, 0xff_b	    // negative(65536)
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_EQ(d[0].get<int>(), 256);
	EXPECT_EQ(d[1].get<int>(), 65535);
	EXPECT_EQ(d[2].get<int>(), -257);
	EXPECT_EQ(d[3].get<int>(), -65536);

	/* type checks */
	EXPECT_THROW(d[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[3].get<int16_t>(), std::range_error);
	for (size_t i = 0; i < size(d); ++i) {
		EXPECT_EQ(d[i].type(), cbor::type::int32);
		EXPECT_THROW(d[i].get<float>(), std::runtime_error);
		EXPECT_THROW(d[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(d[i].get_tag(), std::runtime_error);
		EXPECT_THROW(d[i].get_bytes(), std::runtime_error);
		EXPECT_THROW(d[i].get_string(), std::runtime_error);
	}
}

TEST(codec, int_dword)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(65536, 4294967295, -65537, -2147483648, -4294967296);

	/* verify */
	const std::array exp{
		0x1a_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b,	    // unsigned(65536)
		0x1a_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b,	    // unsigned(4294967295)
		0x3a_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b,	    // negative(65536)
		0x3a_b, 0x7F_b, 0xff_b, 0xff_b, 0xff_b,	    // negative(2147483647)
		0x3a_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b	    // negative(4294967295)
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_EQ(d[0].get<int>(), 65536);
	EXPECT_EQ(d[1].get<uint32_t>(), 4294967295);
	EXPECT_EQ(d[2].get<int>(), -65537);
	EXPECT_EQ(d[3].get<int>(), -2147483648);
	EXPECT_EQ(d[4].get<int64_t>(), -4294967296);

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::int32);
	EXPECT_EQ(d[1].type(), cbor::type::uint32);
	EXPECT_EQ(d[2].type(), cbor::type::int32);
	EXPECT_EQ(d[3].type(), cbor::type::int32);
	EXPECT_EQ(d[4].type(), cbor::type::int64);
	EXPECT_THROW(d[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[4].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[4].get<int32_t>(), std::range_error);
	for (size_t i = 0; i < size(d); ++i) {
		EXPECT_THROW(d[i].get<float>(), std::runtime_error);
		EXPECT_THROW(d[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(d[i].get_tag(), std::runtime_error);
		EXPECT_THROW(d[i].get_bytes(), std::runtime_error);
		EXPECT_THROW(d[i].get_string(), std::runtime_error);
	}
}

TEST(codec, int_qword)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(
		4294967296,
		18446744073709551615u,
		-4294967297,
		-9223372036854775807 - 1 // big integer literals are fun
	);

	/* verify */
	const std::array exp{
		0x1b_b, 0x00_b, 0x00_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	    // unsigned(4294967296)
		0x1b_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b,	    // unsigned(18446744073709551615)
		0x3b_b, 0x00_b, 0x00_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	    // negative(4294967296)
		0x3b_b, 0x7f_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b	    // negative(9223372036854775807)
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_EQ(d[0].get<int64_t>(), 4294967296);
	EXPECT_EQ(d[1].get<uint64_t>(), 18446744073709551615u);
	EXPECT_EQ(d[2].get<int64_t>(), -4294967297);
	EXPECT_EQ(d[3].get<int64_t>(), -9223372036854775807 - 1);

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::int64);
	EXPECT_EQ(d[1].type(), cbor::type::uint64);
	EXPECT_EQ(d[2].type(), cbor::type::int64);
	EXPECT_EQ(d[3].type(), cbor::type::int64);
	EXPECT_THROW(d[2].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[3].get<unsigned>(), std::range_error);
	EXPECT_THROW(d[3].get<int32_t>(), std::range_error);
	for (size_t i = 0; i < size(d); ++i) {
		EXPECT_THROW(d[i].get<float>(), std::runtime_error);
		EXPECT_THROW(d[i].get<bool>(), std::runtime_error);
		EXPECT_THROW(d[i].get_tag(), std::runtime_error);
		EXPECT_THROW(d[i].get_bytes(), std::runtime_error);
		EXPECT_THROW(d[i].get_string(), std::runtime_error);
	}
}

TEST(codec, int_qword_overflow)
{
	/* CBOR can encode 64-bit -ve numbers which don't fit in int64_t */
	const std::array buf {
		0x3b_b, 0x80_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	    // negative(9223372036854775808)
		0x3b_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b	    // negative(18446744073709551615)
	};
	const cbor::codec d(buf);

	/* overflow checks */
	EXPECT_THROW(d[0].get<int64_t>(), std::range_error);
	EXPECT_THROW(d[1].get<int64_t>(), std::range_error);
}

TEST(codec, bignum)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	std::array int256{
		0xdd_b, 0xf7_b, 0xff_b, 0x5e_b, 0xbd_b, 0x9d_b, 0x66_b, 0xce_b,
		0x16_b, 0x14_b, 0x66_b, 0xc1_b, 0xc0_b, 0x26_b, 0x24_b, 0x30_b,
		0xfa_b, 0x04_b, 0xde_b, 0x32_b, 0xb0_b, 0xe4_b, 0x20_b, 0xee_b,
		0x3f_b, 0x48_b, 0x9e_b, 0x2e_b, 0x21_b, 0x12_b, 0xe3_b, 0x86_b
	};
	e.push_back(cbor::tagged{cbor::tag::pos_bignum, int256});

	/* verify */
	const std::array exp{
		0xc2_b,			// tag(2)
		0x58_b, 0x20_b,		// bytes(32)
		0xdd_b, 0xf7_b, 0xff_b, 0x5e_b, 0xbd_b, 0x9d_b, 0x66_b, 0xce_b,
		0x16_b, 0x14_b, 0x66_b, 0xc1_b, 0xc0_b, 0x26_b, 0x24_b, 0x30_b,
		0xfa_b, 0x04_b, 0xde_b, 0x32_b, 0xb0_b, 0xe4_b, 0x20_b, 0xee_b,
		0x3f_b, 0x48_b, 0x9e_b, 0x2e_b, 0x21_b, 0x12_b, 0xe3_b, 0x86_b
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_EQ(d[0].get_tag(), cbor::tag::pos_bignum);
	EXPECT_EQ(d[0].untag().get_bytes(), int256);

	/* type checks */
	EXPECT_EQ(d[0].type(), cbor::type::tag);
	EXPECT_EQ(d[0].untag().type(), cbor::type::bytes);
}

TEST(codec, tags)
{
	std::vector<std::byte> buf;

	const std::array tags{
		0_tag, 23_tag, 24_tag, 255_tag, 256_tag, 65534_tag, 65536_tag,
		4294967294_tag, 4294967296_tag, 18446744073709551614_tag
	};

	/* encode */
	cbor::codec e(buf);
	e.push_back(
		cbor::tagged{tags[0], cbor::tagged{tags[1], cbor::tagged{tags[2],
		cbor::tagged{tags[3], cbor::tagged{tags[4], cbor::tagged{tags[5],
		cbor::tagged{tags[6], cbor::tagged{tags[7], cbor::tagged{tags[8],
		cbor::tagged{tags[9], 0}}}}}}}}}});
	EXPECT_THROW(e.push_back(cbor::tagged{cbor::tag::invalid_1, 0}), std::invalid_argument);
	EXPECT_THROW(e.push_back(cbor::tagged{cbor::tag::invalid_2, 0}), std::invalid_argument);
	EXPECT_THROW(e.push_back(cbor::tagged{cbor::tag::invalid_3, 0}), std::invalid_argument);

	/* verify */
	const std::array exp{
		0xc0_b,                                     // tag(0)
		0xd7_b,					    // tag(23)
		0xd8_b, 0x18_b,                             // tag(24)
		0xd8_b, 0xff_b,				    // tag(255)
		0xd9_b, 0x01_b, 0x00_b,			    // tag(256)
		0xd9_b, 0xff_b, 0xfe_b,			    // tag(65534)
		0xda_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b,	    // tag(65536)
		0xda_b, 0xff_b, 0xff_b, 0xff_b, 0xfe_b,	    // tag(4294967294)
		0xdb_b, 0x00_b, 0x00_b, 0x00_b, 0x01_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,	// tag(4294967296)
		0xdb_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xff_b, 0xfe_b,	// tag(18446744073709551614)
		0x00_b
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	auto it = begin(d);
	for (auto i = 0u; i < size(tags); ++i, it = it->untag().data())
		EXPECT_EQ(it->get_tag(), tags[i]);
	EXPECT_EQ(it->get<int>(), 0);

	/* type checks */
	it = begin(d);
	for (auto i = 0u; i < size(tags); ++i, it = it->untag().data()) {
		EXPECT_EQ(it->type(), cbor::type::tag);
		EXPECT_THROW(it->get<bool>(), std::runtime_error);
		EXPECT_THROW(it->get<int>(), std::runtime_error);
		EXPECT_THROW(it->get<float>(), std::runtime_error);
		EXPECT_THROW(it->get_bytes(), std::runtime_error);
		EXPECT_THROW(it->get_string(), std::runtime_error);
	}
}

TEST(codec, bytes)
{
	std::vector<std::byte> buf;

	std::array v{
		0xca_b, 0xfe_b, 0xbe_b, 0xef_b
	};

	/* encode */
	cbor::codec e(buf);
	e.push_back(v);

	/* verify */
	const std::array exp{
		0x44_b,	    // bytes(4)
		0xca_b, 0xfe_b, 0xbe_b, 0xef_b,
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_EQ(d[0].get_bytes(), v);
}

TEST(codec, string)
{
	std::vector<std::byte> buf;

	const char *v = "foo";

	/* encode */
	cbor::codec e(buf);
	e.push_back(v);

	/* verify */
	const std::array exp{
		0x63_b,				// text(3)
		    0x66_b, 0x6f_b, 0x6f_b,	// "foo"
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	EXPECT_EQ(d[0].get_string(), v);
	EXPECT_THROW(d[0].get<bool>(), std::runtime_error);
	EXPECT_THROW(d[0].get<int>(), std::runtime_error);
	EXPECT_THROW(d[0].get<float>(), std::runtime_error);
	EXPECT_THROW(d[0].get_tag(), std::runtime_error);
	EXPECT_THROW(d[0].get_bytes(), std::runtime_error);
}

TEST(codec, iterator)
{
	/* some basic iterator tests */
	const std::array cbor{
		0x00_b,			    // unsigned(0)
		0xf6_b,			    // null
		0xf9_b, 0x7e_b, 0x00_b,	    // NaN
		0x63_b,			    // text(3)
		    0x66_b, 0x6f_b, 0x6f_b, // "foo"
		0xfb_b, 0x40_b, 0x09_b, 0x21_b, 0xf9_b, 0xf0_b, 0x1b_b, 0x86_b, 0x6e_b // 3.14159
	};
	const cbor::codec d(cbor);

	auto i = begin(d);
	EXPECT_EQ(i++->get<int>(), 0);
	EXPECT_TRUE(std::isnan((++i)->get<float>()));
	EXPECT_EQ(i[2]->get<double>(), 3.14159);
	EXPECT_EQ((i + 2)->get<double>(), 3.14159);
	EXPECT_EQ((i - 2)->get<int>(), 0);
	EXPECT_TRUE(std::isnan(i--->get<float>()));
	EXPECT_EQ((--i)->get<int>(), 0);
	EXPECT_EQ((*(i += 3)).get_string(), "foo");
	EXPECT_EQ((*(i -= 3)).get<int>(), 0);
	EXPECT_EQ(i + 5, end(d));
}

TEST(codec, assignment)
{
	std::vector<std::byte> buf;

	cbor::codec c(buf);
	c.push_back(0);
	c.push_back("foo");

	EXPECT_EQ(size(c), 2);
	EXPECT_EQ(c[0].get<int>(), 0);
	EXPECT_EQ(c[1].get_string(), "foo");

	c[0] = 1;

	EXPECT_EQ(size(c), 2);
	EXPECT_EQ(c[0].get<int>(), 1);
	EXPECT_EQ(c[1].get_string(), "foo");

	c[0] = c[1];

	EXPECT_EQ(size(c), 2);
	EXPECT_EQ(c[0].get_string(), "foo");
	EXPECT_EQ(c[1].get_string(), "foo");
}

TEST(codec, array)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(cbor::array{0, 1, 2, 3});
	auto ea = e[0].get_array();
	for (int i = 4; i < 32; ++i)
		ea.push_back(i);
	ea.push_back(cbor::array{"foo", "bar"});
	ea.push_back("baz", nullptr);

	/* verify */
	const std::array exp{
		0x98_b, 0x23_b,		    // array(34)
		    0x00_b,			// unsigned(0)
		    0x01_b,			// unsigned(1)
		    0x02_b,			// unsigned(2)
		    0x03_b,			// unsigned(3)
		    0x04_b,			// unsigned(4)
		    0x05_b,			// unsigned(5)
		    0x06_b,			// unsigned(6)
		    0x07_b,			// unsigned(7)
		    0x08_b,			// unsigned(8)
		    0x09_b,			// unsigned(9)
		    0x0a_b,			// unsigned(10)
		    0x0b_b,			// unsigned(11)
		    0x0c_b,			// unsigned(12)
		    0x0d_b,			// unsigned(13)
		    0x0e_b,			// unsigned(14)
		    0x0f_b,			// unsigned(15)
		    0x10_b,			// unsigned(16)
		    0x11_b,			// unsigned(17)
		    0x12_b,			// unsigned(18)
		    0x13_b,			// unsigned(19)
		    0x14_b,			// unsigned(20)
		    0x15_b,			// unsigned(21)
		    0x16_b,			// unsigned(22)
		    0x17_b,			// unsigned(23)
		    0x18_b, 0x18_b,		// unsigned(24)
		    0x18_b, 0x19_b,		// unsigned(25)
		    0x18_b, 0x1a_b,		// unsigned(26)
		    0x18_b, 0x1b_b,		// unsigned(27)
		    0x18_b, 0x1c_b,		// unsigned(28)
		    0x18_b, 0x1d_b,		// unsigned(29)
		    0x18_b, 0x1e_b,		// unsigned(30)
		    0x18_b, 0x1f_b,		// unsigned(31)
		    0x82_b,			// array(2)
			0x63_b,			// text(3)
				0x66_b, 0x6F_b, 0x6F_b, // "foo"
			0x63_b,
				0x62_b, 0x61_b, 0x72_b, // "bar"
		    0x63_b,			// text(3)
			0x62_b, 0x61_b, 0x7a_b,	    // "baz"
		    0xf6_b			// null
	};
	EXPECT_EQ(buf, exp);

	/* decode */
	const cbor::codec d(exp);
	const auto &da = d[0].get_array();
	for (auto i = 0; i < 32; ++i)
		EXPECT_EQ(da[i].get<int>(), i);
	EXPECT_EQ(da[32].get_array()[0].get_string(), "foo");
	EXPECT_EQ(da[32].get_array()[1].get_string(), "bar");
	EXPECT_EQ(da[33].get_string(), "baz");
	EXPECT_EQ(da[34].type(), cbor::type::null);

	for (int idx = 0; const auto &i : da) {
		switch (idx) {
		case 0 ... 31:
			EXPECT_EQ(i->get<int>(), idx);
			break;
		case 32:
			EXPECT_EQ(i->get_array()[0].get_string(), "foo");
			EXPECT_EQ(i->get_array()[1].get_string(), "bar");
			break;
		case 33:
			EXPECT_EQ(i->get_string(), "baz");
			break;
		case 34:
			EXPECT_EQ(i->type(), cbor::type::null);
			break;
		}
		EXPECT_LT(idx, 35);
		++idx;
	}
}

TEST(codec, encode_map)
{
	std::vector<std::byte> buf;

	/* encode */
	cbor::codec e(buf);
	e.push_back(
		cbor::map{
			{0, "foo"},
			{"bar", 1},
			{"baz", cbor::map{
				{-1, nullptr}
			}
		}
	});

	std::array exp{
		0xa3_b,		// map(3)
		    0x00_b,	    // unsigned(0)
		    0x63_b,	    // text(3)
			0x66_b, 0x6f_b, 0x6f_b, // "foo"
		    0x63_b,	    // text(3)
			0x62_b, 0x61_b, 0x72_b, // "bar"
		    0x01_b,	    // unsigned(1)
		    0x63_b,	    // text(3)
			0x62_b, 0x61_b, 0x7a_b, // "baz"
		    0xa1_b,	    // map(1)
		       0x20_b,	    // negative(0)
		       0xf6_b	    // primitive(22)
	};
	EXPECT_EQ(buf, exp);
}
