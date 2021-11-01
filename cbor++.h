/*
 * cbor++ - CBOR C++ library
 *
 * Copyright 2021 Patrick Oppenlander <patrick.oppenlander@gmail.com>
 *
 * SPDX-License-Identifier: 0BSD
 *
 * See RFC8949.
 */
#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <iterator>
#include <span>

namespace cbor {

/*
 * unreachable - tell the compiler about unreachable code paths
 */
#define unreachable __builtin_unreachable

/*
 * literals
 */
inline namespace literals {

constexpr
std::byte
operator""_b(unsigned long long v)
{
	return static_cast<std::byte>(v);
}

}

/*
 * type - C++(ish) mapping of CBOR data type
 */
enum class type {
	int32,
	int64,
	bytes,
	string,
	array,
	map,
	tag,
	uint32,
	uint64,
	fp32,
	fp64,
	boolean,
	null,
	undefined,
	indefinite_break,
};

/*
 * tag - some CBOR tag values
 *
 * See https://www.iana.org/assignments/cbor-tags.
 */
enum class tag : uint64_t {
	date_time_string = 0,		    /* string, RFC8949 3.4.1 */
	date_time_epoch = 1,		    /* int or float, RFC8949 3.4.2 */
	pos_bignum = 2,			    /* bytes, RFC8949 3.4.3 */
	neg_bignum = 3,			    /* bytes, RFC8949 3.4.3 */
	invalid_1 = 65535,		    /* always invalid */
	invalid_2 = 4294967295,		    /* always invalid */
	invalid_3 = 18446744073709551615u,  /* always invalid */
};

/*
 * byteswap - byteswap a value
 *
 * Unfortunately current versions of gcc (11.2) and clang (13.0.0) generally
 * don't optimise std::reverse down to byteswap instructions, and std::byteswap
 * isn't available until c++23.
 *
 * There are countless ways to write this function. The following tries to use
 * as much standard c++ as possible and avoid casting and undefined behaviour.
 */
inline
auto
byteswap(auto v)
{
	auto vb = as_writable_bytes(std::span(&v, 1));
	std::span<std::byte> tb;
	union {
		uint16_t t16;
		uint32_t t32;
		uint64_t t64;
	};

	static_assert(sizeof v == 2 || sizeof v == 4 || sizeof v == 8,
		      "Are you sure you want to byteswap that?");

	switch (sizeof v) {
	case 2:
		tb = as_writable_bytes(std::span(&t16, 1));
		std::copy(begin(vb), end(vb), begin(tb));
		t16 = __builtin_bswap16(t16);
		break;
	case 4:
		tb = as_writable_bytes(std::span(&t32, 1));
		std::copy(begin(vb), end(vb), begin(tb));
		t32 = __builtin_bswap32(t32);
		break;
	case 8:
		tb = as_writable_bytes(std::span(&t64, 1));
		std::copy(begin(vb), end(vb), begin(tb));
		t64 = __builtin_bswap64(t64);
		break;
	}

	std::copy(begin(tb), end(tb), begin(vb));
	return v;
}

/*
 * write_be - write a big endian value to byte iterator 'p'
 */
inline
void
write_be(auto p, auto v)
{
	auto vb = as_writable_bytes(std::span(&v, 1));
	if (std::endian::native != std::endian::big) {
		/* equivalent to std::reverse(begin(vb), end(vb)) but
		 * optimises properly */
		v = byteswap(v);
	}
	std::copy(begin(vb), end(vb), p);
}

/*
 * read_be - read a big endian value from byte iterator 'p'
 */
template<class T>
T
read_be(auto p)
{
	T v;
	auto vb = as_writable_bytes(std::span(&v, 1));
	std::copy(p, p + sizeof v, begin(vb));
	if (std::endian::native != std::endian::big) {
		/* equivalent to std::reverse(begin(vb), end(vb)) but
		 * optimises properly */
		v = byteswap(v);
	}
	return v;
}

/*
 * ih - definitions & functions related to the CBOR item head
 *
 * A CBOR item head is made up of an inital byte containing a 3-bit major type
 * and 5 bits of additional information. This is followed by 0, 1, 2, 4 or 8
 * additional bytes.
 */
namespace ih {

/*
 * major - item head major type
 */
enum class major {
	posint = 0,
	negint = 1,
	bytes = 2,
	utf8 = 3,
	array = 4,
	map = 5,
	tag = 6,
	special = 7,
};

/* Aligning these improves code generation */
static_assert(static_cast<int>(major::bytes) == static_cast<int>(type::bytes));
static_assert(static_cast<int>(major::utf8) == static_cast<int>(type::string));
static_assert(static_cast<int>(major::array) == static_cast<int>(type::array));
static_assert(static_cast<int>(major::map) == static_cast<int>(type::map));
static_assert(static_cast<int>(major::tag) == static_cast<int>(type::tag));

/*
 * ai - item head additional information values
 */
enum class ai {
	/* 0 - 23 directly encoded value */
	byte = 24,
	word = 25,
	dword = 26,
	qword = 27,
	/* 28 - 30 reserved */
	indefinite = 31,    /* only valid for bytes, utf8 and special */
};

/*
 * special - CBOR special data item type
 *
 * Stored in additional information of data item head.
 */
enum class special {
	bool_false = 20,
	bool_true = 21,
	null = 22,
	undefined = 23,
	extended = 24,
	fp16 = 25,
	fp32 = 26,
	fp64 = 27,
	indefinite_break = 31,
};

/*
 * make - create an item head
 */
constexpr
std::byte
make(major m, auto a)
{
	/* when this function is always evaluated in a constexpr context
	 * this won't throw -  it will fail to compile */
	if (static_cast<int>(m) > 7)
		throw std::invalid_argument("invalid major");
	if (static_cast<int>(a) > 31)
		throw std::invalid_argument("invalid additional information");
	return static_cast<std::byte>(m) << 5 |
	    static_cast<std::byte>(a);
}

/*
 * some useful item head constants
 */
constexpr auto fp16 = make(major::special, special::fp16);
constexpr auto fp32 = make(major::special, special::fp32);
constexpr auto fp64 = make(major::special, special::fp64);
constexpr auto null = make(major::special, special::null);
constexpr auto bool_false = make(major::special, special::bool_false);
constexpr auto bool_true = make(major::special, special::bool_true);

/*
 * get_major - get major type from CBOR item head at 'p'
 */
inline
major
get_major(auto p)
{
	static_assert(sizeof *p == 1);

	return static_cast<major>(static_cast<int>(*p) >> 5);
}

/*
 * get_ai - get additional information from CBOR item head at 'p'
 */
inline
unsigned
get_ai(auto p)
{
	static_assert(sizeof *p == 1);

	return static_cast<unsigned>(*p) & 0x1f;
}

/*
 * get_arg_size - get size of argument from CBOR item head at 'p'
 */
inline
unsigned
get_arg_size(auto p)
{
	static_assert(sizeof *p == 1);

	switch (get_ai(p)) {
	case 0 ... 23:
		return 0;
	case static_cast<int>(ai::byte):
		return 1;
	case 25:
		return 2;
	case 26:
		return 4;
	case 27:
		return 8;
	case 28 ... 30:
		throw std::runtime_error("invalid additional information");
	case 31:
		return 0;
	default:
		unreachable();
	}
}

/*
 * get_ih_size - get size (in bytes) of CBOR item head at 'p'
 */
inline
unsigned
get_ih_size(auto p)
{
	static_assert(sizeof *p == 1);

	return 1 + get_arg_size(p);
}

/*
 * get_arg - get argument of CBOR item head at 'p'
 */
inline
uint64_t
get_arg(auto p)
{
	auto ai = get_ai(p);
	switch (ai) {
	case 0 ... 23:
		return ai;
	case 24:
		return static_cast<uint8_t>(p[1]);
	case 25:
		return read_be<uint16_t>(p + 1);
	case 26:
		return read_be<uint32_t>(p + 1);
	case 27:
		return read_be<uint64_t>(p + 1);
	case 28 ... 30:
		throw std::runtime_error("invalid short count");
	case 31:
		return 31;
	default:
		unreachable();
	}
}

/*
 * get_data_size - get data size of CBOR item at 'p'
 */
inline
uint64_t
get_data_size(auto p)
{
	switch (get_major(p)) {
	case major::bytes:
	case major::utf8:
		return get_arg(p);
	default:
		return 0;
	}
}

/*
 * get_size - get size of CBOR item at 'p'
 */
inline
uint64_t
get_size(auto p)
{
	return get_ih_size(p) + get_data_size(p);
}

}

/*
 * data_item
 */
template<class I>
class data_item {
public:
	data_item(I p)
	: p_{p}
	{ }

	template<class T>
	requires std::is_integral_v<T>
	T
	get() const
	{
		/* booleans */
		if (std::is_same_v<T, bool>) {
			switch (*p_) {
			case ih::bool_false:
				return false;
			case ih::bool_true:
				return true;
			default:
				throw std::runtime_error("data is not boolean");
			}
		}

		/* integers */
		T v;
		switch (ih::get_major(p_)) {
		case ih::major::posint:
			v = ih::get_arg(p_);
			if (static_cast<uint64_t>(v) != ih::get_arg(p_))
				throw std::range_error("integer overflow");
			return v;
		case ih::major::negint:
			if (ih::get_arg(p_) > std::numeric_limits<int64_t>::max())
				throw std::range_error("integer overflow");
			v = -1 - static_cast<int64_t>(ih::get_arg(p_));
			if (static_cast<int64_t>(v) !=
			    -1 - static_cast<int64_t>(ih::get_arg(p_)))
				throw std::range_error("integer overflow");
			return v;
		default:
			throw std::runtime_error("data is not integral");
		}
	}

	template<class T>
	requires std::is_floating_point_v<T>
	T
	get() const
	{
		T v;
		switch (*p_) {
		case ih::fp16:
			if (p_[2] != 0x00_b)
				throw std::runtime_error("float16 not supported");
			if (p_[1] == 0x7e_b)
				return std::numeric_limits<T>::quiet_NaN();
			if (p_[1] == 0x7c_b)
				return std::numeric_limits<T>::infinity();
			if (p_[1] == 0xfc_b)
				return -std::numeric_limits<T>::infinity();
			throw std::runtime_error("float16 not supported");
		case ih::fp32:
			return read_be<float>(p_);
		case ih::fp64:
			v = read_be<double>(p_);
			if (v != read_be<double>(p_))
				throw std::range_error("lossy float conversion");
			return v;
		default:
			throw std::runtime_error("data is not floating point");
		}
	}

	template<class T>
	requires std::is_same_v<T, cbor::tag>
	cbor::tag
	get() const
	{
		if (ih::get_major(p_) != ih::major::tag)
			throw std::runtime_error("data is not a tag");
		return static_cast<cbor::tag>(ih::get_arg(p_));
	}

	std::span<const typename std::iterator_traits<I>::value_type>
	get_bytes() const
	{
		if (ih::get_major(p_) != ih::major::bytes)
			throw std::runtime_error("data is not bytes");
		auto ih_sz = ih::get_ih_size(p_);
		return {p_ + ih_sz, p_ + ih_sz + ih::get_data_size(p_)};
	}

	cbor::type
	type() const
	{
		switch (ih::get_major(p_)) {
		case ih::major::posint: {
			auto sz = ih::get_arg_size(p_);
			auto val = ih::get_arg(p_);
			if (sz < 4)
				return cbor::type::int32;
			if (sz == 4)
				return val > std::numeric_limits<int32_t>::max()
				    ? cbor::type::uint32 : cbor::type::int32;
			return val > std::numeric_limits<int64_t>::max()
			    ? cbor::type::uint64 : cbor::type::int64;
		}
		case ih::major::negint: {
			auto sz = ih::get_arg_size(p_);
			auto val = ih::get_arg(p_);
			if (sz < 4)
				return cbor::type::int32;
			return val > std::numeric_limits<int32_t>::max() ?
			    cbor::type::int64 : cbor::type::int32;
		}
		case ih::major::bytes:
			return cbor::type::bytes;
		case ih::major::utf8:
			return cbor::type::string;
		case ih::major::array:
			return cbor::type::array;
		case ih::major::map:
			return cbor::type::map;
		case ih::major::tag:
			return cbor::type::tag;
		case ih::major::special:
			switch (static_cast<ih::special>(ih::get_ai(p_))) {
			case ih::special::bool_false:
			case ih::special::bool_true:
				return cbor::type::boolean;
			case ih::special::null:
				return cbor::type::null;
			case ih::special::undefined:
				return cbor::type::undefined;
			case ih::special::extended:
				throw std::runtime_error("extended special types are not supported");
			case ih::special::fp16:
			case ih::special::fp32:
				return cbor::type::fp32;
			case ih::special::fp64:
				return cbor::type::fp64;
			case ih::special::indefinite_break:
				return cbor::type::indefinite_break;
			default:
				throw std::runtime_error("invalid special type");
			}
		default:
			unreachable();
		}
	}

private:
	I p_;
};

/*
 * codec - raw CBOR codec
 *
 * Indexes refer to CBOR data items.
 * Iterators iterate CBOR data items.
 *
 * This is for low level manipulation of CBOR encoded data.
 */
template<class S>
requires std::is_same_v<typename S::value_type, std::byte>
class codec {
public:
	using reference = data_item<typename S::iterator>;
	using const_reference = const data_item<typename S::const_iterator>;
	using difference_type = std::ptrdiff_t;
	using size_type = std::size_t;

	/*
	 * iterators
	 */
	template<bool const__>
	class itr__ {
		friend class codec;

	public:
		using container = std::conditional_t<const__, const codec, codec>;
		using difference_type = codec<S>::difference_type;
		using reference = std::conditional_t<const__,
						     codec<S>::const_reference,
						     codec<S>::reference>;
		using iterator_category = std::random_access_iterator_tag;

		itr__()
		: it_{}
		, c_{}
		{ }

		itr__(size_type it, container *c)
		: it_{it}
		, c_{c}
		{ }

		itr__(const itr__ &) = default;
		~itr__() = default;
		itr__ &operator=(const itr__ &) = default;

		/* iterator->const_iterator conversion */
		template<bool c = const__, class = std::enable_if_t<c>>
		itr__(const itr__<false> &r)
		: it_{r.it_}
		, c_{r.c_}
		{ }

		std::strong_ordering
		operator<=>(const itr__ &r) const
		{
			return it_ <=> r.it_;
		}

		bool
		operator==(const itr__ &r) const
		{
			return it_ == r.it_;
		}

		itr__ &
		operator++()
		{
			++it_;
			return *this;
		}

		itr__
		operator++(int)
		{
			auto tmp{*this};
			++it_;
			return tmp;
		}

		itr__ &
		operator--()
		{
			--it_;
			return *this;
		}

		itr__
		operator--(int)
		{
			auto tmp{*this};
			--it_;
			return tmp;
		}

		itr__ &
		operator+=(size_type d)
		{
			it_ += d;
			return *this;
		}

		itr__
		operator+(size_type d) const
		{
			auto tmp{*this};
			return tmp += d;
		}

		friend
		itr__
		operator+(size_type d, const itr__ &i)
		{
			return i + d;
		}

		itr__ &
		operator-=(size_type d)
		{
			it_ -= d;
			return *this;
		}

		itr__
		operator-(size_type d) const
		{
			auto tmp{*this};
			return tmp -= d;
		}

		difference_type
		operator-(const itr__<true> &r) const
		{
			return it_ - r.it_;
		}

		reference
		operator*() const
		{
			return c_->ref(it_);
		}

		reference
		operator[](size_type d) const
		{
			return c_->ref(it_ + d);
		}

	private:
		size_type it_;
		container *c_;
	};
	typedef itr__<false> iterator;
	typedef itr__<true> const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	/*
	 * Constructor for CBOR codec using storage container S.
	 */
	codec(S &s)
	: s_{s}
	{ }

	/* codec &operator=(const codec&); */
	/* bool operator==(const codec&) const; */
	/* bool operator!=(const codec&) const; */
	/* iterator begin() */
	/* const_iterator begin() const */
	/* const_iterator cbegin() const */
	/* iterator end() */
	/* const_iterator end() const */
	/* const_iterator cend() const */
	/* reverse_iterator rbegin() */
	/* const_reverse_iterator rbegin() const */
	/* const_reverse_iterator crbegin() const */
	/* reverse_iterator rend() */
	/* const_reverse_iterator rend() const */
	/* const_reverse_iterator crend() const */
	/* reference front() */
	/* const_reference front() const */
	/* reference back() */
	/* const_reference back() const */
	/* template<typename ...A> void emplace_front(A &&...a) */
	/* template<typename ...A> void emplace_back(A &&...a) */
	/* void push_front(const T &v) */
	/* void push_front(T &&v) */

	void
	push_back(const auto &v)
	{
		encode(end(s_), v);
	}

	/* void push_back(const T &v) */
	/* void push_back(T &&v) */
	/* void pop_front() */
	/* void pop_back() */

	reference
	operator[](size_type i)
	{
#warning fixme: stupid version just to get tests running
		auto p = begin(s_);
		auto len = ih::get_size(p);
		for (size_type n = 0; n < i; ++n)
			len = ih::get_size(p += len);
		return {p};
	}

	/* const_reference operator[](size_type i) const */
	/* reference at(size_type); */
	/* const_reference at(size_type) const; */
	/* template<class ...A> iterator emplace(const_iterator pos, A &&...a) */
	/* iterator insert(const_iterator pos, const T &v) */
	/* iterator insert(const_iterator pos, T &&v) */
	/* iterator insert(const_iterator pos, size_type n, const T &v) */
	/* template<class I> iterator insert(const_iterator pos, I b, I e) */
	/* iterator insert(const_iterator pos, std::initializer_list<T> l) */
	/* iterator erase(const_iterator pos) */
	/* iterator erase(const_iterator begin, const_iterator end) */

	void
	clear()
	{
		return s_.clear();
	}

	/* template<class it> void assign(iter, iter); */
	/* void assign(std::initializer_list<T>); */
	/* void assign(size_type, const T&); */
	/* void swap(codec &); */

	size_type
	size() const
	{
#warning fixme: stupid version just to get tests running
		size_type n = 0;
		if (s_.empty())
			return 0;
		for (auto p = begin(s_); p != end(s_); p += ih::get_size(p))
			++n;
		return n;
	}

	/* size_type max_size() const */

	bool empty() const
	{
		return s_.empty();
	}

private:
	template<class U>
	requires std::is_unsigned_v<U>
	typename S::iterator
	encode_ih(typename S::iterator p, ih::major m, U arg)
	{
		if (arg <= 23)
			return s_.insert(p, ih::make(m, arg)) + 1;
		else if (arg <= std::numeric_limits<uint8_t>::max()) {
			std::array<std::byte, 2> b{
				ih::make(m, ih::ai::byte),
				static_cast<std::byte>(arg)};
			return s_.insert(p, begin(b), end(b)) + std::size(b);
		} else if (arg <= std::numeric_limits<uint16_t>::max()) {
			std::array<std::byte, 3> b{ih::make(m, ih::ai::word)};
			write_be(&b[1], static_cast<uint16_t>(arg));
			return s_.insert(p, begin(b), end(b)) + std::size(b);
		} else if (arg <= std::numeric_limits<uint32_t>::max()) {
			std::array<std::byte, 5> b{ih::make(m, ih::ai::dword)};
			write_be(&b[1], static_cast<uint32_t>(arg));
			return s_.insert(p, begin(b), end(b)) + std::size(b);
		} else {
			std::array<std::byte, 9> b{ih::make(m, ih::ai::qword)};
			write_be(&b[1], arg);
			return s_.insert(p, begin(b), end(b)) + std::size(b);
		}
	}

	template<class T>
	requires std::is_floating_point_v<T>
	void
	encode(typename S::iterator p, T v)
	{
		/* nan is encoded as 2-byte float */
		if (std::isnan(v)) {
			auto b = {ih::fp16, 0x7e_b, 0x00_b};
			s_.insert(p, begin(b), end(b));
			return;
		}

		/* infinity is encoded as 2-byte float */
		if (std::isinf(v)) {
			auto b = {ih::fp16, v > 0 ? 0x7c_b : 0xfc_b, 0x00_b};
			s_.insert(p, begin(b), end(b));
			return;
		}

		/* encode doubles as float when no data is lost */
		if (std::is_same_v<T, double> && v == static_cast<float>(v)) {
			encode(p, static_cast<float>(v));
			return;
		}

		std::array<std::byte, sizeof v + 1> b{
				  sizeof v == 4 ? ih::fp32 : ih::fp32};
		write_be(&b[1], &v);
		s_.insert(p, begin(b), end(b));
	}

	template<class T>
	requires std::is_integral_v<T>
	void
	encode(typename S::iterator p, T v)
	{
		encode_ih(p, v < 0 ? ih::major::negint : ih::major::posint,
			  std::make_unsigned_t<T>(v < 0 ? -v - 1 : v));
	}

	void
	encode(typename S::iterator p, std::nullptr_t)
	{
		s_.insert(p, ih::null);
	}

	void
	encode(typename S::iterator p, bool v)
	{
		s_.insert(p, v ? ih::bool_true : ih::bool_false);
	}

	void
	encode(typename S::iterator p, tag t)
	{
		if (t == tag::invalid_1 || t == tag::invalid_2 ||
		    t == tag::invalid_3)
			throw std::invalid_argument("tag value is invalid");
		encode_ih(p, ih::major::tag,
			  static_cast<std::underlying_type_t<tag>>(t));
	}

	void
	encode(typename S::iterator p, std::span<const typename S::value_type> v)
	{
		s_.insert(encode_ih(p, ih::major::bytes, std::size(v)),
			  begin(v), end(v));
	}

	S &s_;
};

}
