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
#include <variant>
#include <vector>

#if defined(CBORXX_DEBUG)
#define CBORXX_ASSERT(x) assert(x)
#else
#define CBORXX_ASSERT(x)
#endif

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
 * undefined - unique type for representing CBOR undefined items
 */
struct undefined {};

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

constexpr
tag
make_tag(unsigned long long v)
{
	auto t = static_cast<tag>(v);
	CBORXX_ASSERT(t != tag::invalid_1);
	CBORXX_ASSERT(t != tag::invalid_2);
	CBORXX_ASSERT(t != tag::invalid_3);
	return t;
}

inline namespace literals {

constexpr
tag
operator""_tag(unsigned long long v)
{
	return make_tag(v);
}

}

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
	/* when this function is evaluated in a constexpr context this won't
	 * throw - it will fail to compile */
	if (static_cast<int>(m) > 7)
		throw std::invalid_argument("invalid major");
	if (static_cast<int>(a) > 31)
		throw std::invalid_argument("invalid additional information");
	return static_cast<std::byte>(m) << 5 |
	    static_cast<std::byte>(a);
}

template<class Arg>
requires std::is_unsigned_v<Arg>
constexpr
std::span<std::byte>
make(major m, Arg a, std::span<std::byte> b)
{
	assert(size(b) >= 9);

	if (a <= 23) {
		b[0] = make(m, static_cast<ai>(a));
		return b.first(1);
	} else if (a <= std::numeric_limits<uint8_t>::max()) {
		b[0] = ih::make(m, ih::ai::byte);
		b[1] = static_cast<std::byte>(a);
		return b.first(2);
	} else if (a <= std::numeric_limits<uint16_t>::max()) {
		b[0] = ih::make(m, ih::ai::word);
		write_be(&b[1], static_cast<uint16_t>(a));
		return b.first(3);
	} else if (a <= std::numeric_limits<uint32_t>::max()) {
		b[0] = ih::make(m, ih::ai::dword);
		write_be(&b[1], static_cast<uint32_t>(a));
		return b.first(5);
	} else {
		b[0] = ih::make(m, ih::ai::qword);
		write_be(&b[1], a);
		return b.first(9);
	}
}

/*
 * some useful item head constants
 */
constexpr auto fp16 = make(major::special, special::fp16);
constexpr auto fp32 = make(major::special, special::fp32);
constexpr auto fp64 = make(major::special, special::fp64);
constexpr auto null = make(major::special, special::null);
constexpr auto undefined = make(major::special, special::undefined);
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
ai
get_ai(auto p)
{
	static_assert(sizeof *p == 1);

	return static_cast<ai>(static_cast<unsigned>(*p) & 0x1f);
}

/*
 * get_arg_size - get size of argument from CBOR item head at 'p'
 */
inline
unsigned
get_arg_size(auto p)
{
	static_assert(sizeof *p == 1);

	switch (static_cast<unsigned>(get_ai(p))) {
	case 0 ... 23:
		return 0;
	case static_cast<unsigned>(ai::byte):
		return 1;
	case static_cast<unsigned>(ai::word):
		return 2;
	case static_cast<unsigned>(ai::dword):
		return 4;
	case static_cast<unsigned>(ai::qword):
		return 8;
	case 28 ... 30:
		throw std::runtime_error("invalid additional information");
	case static_cast<unsigned>(ai::indefinite):
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
	auto ai = static_cast<unsigned>(get_ai(p));
	switch (ai) {
	case 0 ... 23:
		return ai;
	case static_cast<unsigned>(ai::byte):
		return static_cast<uint8_t>(p[1]);
	case static_cast<unsigned>(ai::word):
		return read_be<uint16_t>(p + 1);
	case static_cast<unsigned>(ai::dword):
		return read_be<uint32_t>(p + 1);
	case static_cast<unsigned>(ai::qword):
		return read_be<uint64_t>(p + 1);
	case 28 ... 30:
		throw std::runtime_error("invalid additional information");
	case static_cast<unsigned>(ai::indefinite):
		throw std::runtime_error("indefinite argument");
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
 * Convenience types for initialisation of cbor objects
 *
 * These exist purely to support list-style initialisation. See unit tests for
 * example usage.
 *
 * Storing the items in a vector is unavoidable due to the limited lifetime
 * of initializer list array temporaries.
 */
struct array;
struct map;
struct tagged;
using item = std::variant<
	std::span<const std::byte>,
	std::string_view,
	int64_t,
	uint64_t,
	std::nullptr_t,
	array,
	map,
	tagged,
	double,
	bool,
	undefined
>;

struct array {
	array(std::initializer_list<item> init)
	: init_(init)
	{ }

	const std::vector<item> init_;
};

struct map {
	map(std::initializer_list<std::pair<item, item>> init)
	: init_(init)
	{ }

	const std::vector<std::pair<item, item>> init_;
};

struct tagged {
	tagged(tag t, const item &i)
	: tag_(t)
	, item_{i}
	{ }

	const tag tag_;
	const std::vector<item> item_;
};

/*
 * codec - raw CBOR codec
 *
 * Indexes refer to CBOR data items.
 * Iterators iterate CBOR data items.
 *
 * This is for low level manipulation of CBOR encoded data.
 */
template<class> class data_item;
template<class S>
requires std::is_same_v<typename S::value_type, std::byte>
class codec {
	template<class> friend class data_item;
	template<class> friend class array_item;
	template<class> friend class map_item;

public:
	template<bool> class itr__;
	using iterator = itr__<false>;
	using const_iterator = itr__<true>;

	using reference = data_item<iterator>;
	using const_reference = const data_item<const_iterator>;
	using difference_type = std::ptrdiff_t;
	using size_type = std::size_t;
	using data_iterator = typename S::iterator;
	using const_data_iterator = typename S::const_iterator;

	/*
	 * iterators
	 */
	template<bool const__>
	class itr__ {
		friend class codec;
		template<class> friend class data_item;
		template<class> friend class array_item;
		template<class> friend class map_item;

	public:
		using container = std::conditional_t<const__, const codec, codec>;
		using difference_type = codec<S>::difference_type;
		using reference = std::conditional_t<const__,
						     codec<S>::const_reference,
						     codec<S>::reference>;
		using iterator_category = std::random_access_iterator_tag;
		using data_iterator = std::conditional_t<const__,
						codec<S>::const_data_iterator,
						codec<S>::data_iterator>;

		itr__()
		: c_{}
		{ }

		itr__(data_iterator data, size_type cont, size_type idx,
		      container *c)
		: data_{data}
		, cont_{cont}
		, idx_{idx}
		, c_{c}
		{ }

		itr__(const itr__ &) = default;
		~itr__() = default;
		itr__ &operator=(const itr__ &) = default;

		/* iterator->const_iterator conversion */
		itr__(const itr__<false> &r)
		requires const__
		: data_{r.data_}
		, cont_{r.cont_}
		, idx_{r.idx_}
		, c_{r.c_}
		{ }

		std::strong_ordering
		operator<=>(const itr__ &r) const
		{
			CBORXX_ASSERT(c_ && c_ == r.c_);
			return data_ <=> r.data_;
		}

		bool
		operator==(const itr__ &r) const
		{
			CBORXX_ASSERT(c_ && c_ == r.c_);
			return data_ == r.data_;
		}

		itr__ &
		operator++()
		{
			CBORXX_ASSERT(c_);
			return *this = c_->next(*this);
		}

		itr__
		operator++(int)
		{
			auto tmp = *this;
			++*this;
			return tmp;
		}

		itr__ &
		operator--()
		{
			CBORXX_ASSERT(c_);
			return *this = c_->prev(*this);
		}

		itr__
		operator--(int)
		{
			auto tmp = *this;
			--*this;
			return tmp;
		}

		itr__ &
		operator+=(size_type d)
		{
			CBORXX_ASSERT(c_);
			return *this = c_->next(*this, d);
		}

		itr__
		operator+(size_type d) const
		{
			auto tmp = *this;
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
			CBORXX_ASSERT(c_);
			return *this = c_->prev(*this, d);
		}

		itr__
		operator-(size_type d) const
		{
			auto tmp = *this;
			return tmp -= d;
		}

		difference_type
		operator-(const itr__<true> &r) const
		{
			CBORXX_ASSERT(c_ && c_ == r.c_);
			CBORXX_ASSERT(cont_ == r.cont_);
			return idx_ - r.idx_;
		}

		reference
		operator*() const
		{
			CBORXX_ASSERT(c_);
			return *this;
		}

		reference
		operator->() const
		{
			CBORXX_ASSERT(c_);
			return *this;
		}

		reference
		operator[](size_type d) const
		{
			CBORXX_ASSERT(c_);
			return *this + d;
		}

	private:
		itr__
		enter_container() const
		{
			return {data_ + ih::get_size(data_), cont_ + 1, 0, c_};
		}

		itr__
		next_item() const
		{
			return {data_ + ih::get_size(data_), cont_, idx_ + 1, c_};
		}

		data_iterator data_;
		size_type cont_;
		size_type idx_;
		container *c_;
	};

	/*
	 * Constructor for CBOR codec using storage container S.
	 */
	codec(S &s)
	: s_{s}
	{ }

	codec(const codec &) = delete;

	codec(codec &&) = delete;

	~codec() = default;

	codec &
	operator=(const codec&) = delete;

	codec &
	operator=(codec &&) = delete;

	/* bool operator==(const codec&) const; */
	/* bool operator!=(const codec&) const; */

	iterator
	begin()
	{
		return {std::begin(s_), 0, 0, this};
	}

	const_iterator
	begin() const
	{
		return {std::begin(s_), 0, 0, this};
	}

	const_iterator
	cbegin() const
	{
		return {std::begin(s_), 0, 0, this};
	}

	iterator
	end()
	{
		return {std::end(s_), 0, size(), this};
	}

	const_iterator
	end() const
	{
		return {std::end(s_), 0, size(), this};
	}

	const_iterator
	cend() const
	{
		return {std::end(s_), 0, size(), this};
	}

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
	push_back(const auto &...v)
	{
		encode_sequence(make_data_range(std::end(s_), 0), v...);
	}

	/* void pop_front() */
	/* void pop_back() */

	reference
	operator[](size_type i)
	{
		CBORXX_ASSERT(i < size());
		return next(begin(), i);
	}

	const_reference
	operator[](size_type i) const
	{
		CBORXX_ASSERT(i < size());
		return next(begin(), i);
	}

	/* reference at(size_type); */
	/* const_reference at(size_type) const; */
	/* template<class ...A> iterator emplace(const_iterator pos, A &&...a) */
	/* iterator insert(const_iterator pos, const T &v) */
	/* iterator insert(const_iterator pos, T &&v) */
	/* iterator insert(const_iterator pos, size_type n, const T &v) */
	/* template<class I> iterator insert(const_iterator pos, I b, I e) */
	/* iterator insert(const_iterator pos, std::initializer_list<T> l) */
	/* iterator erase(const_iterator pos) */

	iterator
	erase(const_iterator b, const_iterator e)
	{
		return {s_.erase(b.data_, e.data_), b.cont_, b.idx_, this};
	}

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
		/* TODO: cache size */
		auto it = begin();
		while (it.data_ != std::end(s_)) ++it;
		return it.idx_;
	}

	/* size_type max_size() const */

	bool
	empty() const
	{
		return s_.empty();
	}

	/*
	 * Extensions for CBOR codec
	 */

	/*
	 * replace - replace a cbor data item
	 */
	iterator
	replace(iterator p, const_iterator v)
	{
		/* this is tricky as erase invalidates 'v' and insert cannot
		 * take iterators into *this, so, take a copy for now */
		/* TODO: resize & overwrite existing item */
		S tmp{v.data_, next(v).data_};
		p = erase(p, next(p));
		return {s_.insert(p.data_, std::begin(tmp), std::end(tmp)),
			p.cont_, p.idx_, this};
	}

	iterator
	replace(iterator p, const item &v)
	{
		/* TODO: resize & overwrite existing item */
		p = erase(p, next(p));
		auto [it, sz] = encode_item(make_data_range(p.data_, 0), v);
		return {it, p.cont_, p.idx_, this};
	}

	/*
	 * next - get nth next cbor item at this level
	 */
	template<class I>
	requires std::is_convertible_v<I, const_iterator>
	I
	next(I it, size_type n = 1) const
	{
		/* TODO: optimise by caching item offsets? */
		while (n--) {
			CBORXX_ASSERT(it.data_ != std::end(s_));
			it = data_item(it).next().data();
		}
		return it;
	}

	/*
	 * prev - get nth previous cbor item at this level
	 */
	template<class I>
	requires std::is_convertible_v<I, const_iterator>
	I
	prev(I it, size_type n = 1) const
	{
		CBORXX_ASSERT(n <= it.idx_);
		auto tmp = begin();
		while (tmp.cont_ != it.cont_) tmp = tmp.next_item();
		return next(tmp, it.idx_ - n);
	}

private:
	template<class It>
	requires std::is_convertible_v<It, const_data_iterator>
	using data_range = std::pair<It, std::size_t>;

	/* REVISIT: clang 13.0.0 doesn't support CTAD for alias templates */
	template<class It>
	static
	data_range<It>
	make_data_range(It it, std::size_t sz)
	{
		return {it, sz};
	}

	template<class It>
	data_range<It>
	append(data_range<It> p, std::span<const typename S::value_type> v)
	{
		auto [it, sz] = p;
		return {s_.insert(it + sz, std::begin(v), std::end(v)) - sz,
			sz + std::size(v)};
	}

	template<class It>
	data_range<It>
	append(data_range<It> p, typename S::value_type v)
	{
		return append(p, std::span(&v, 1));
	}

	template<class It>
	data_range<It>
	append(data_range<It> p, std::initializer_list<typename S::value_type> v)
	{
		return append(p, std::span(std::begin(v), std::size(v)));
	}

	template<class It>
	data_range<It>
	replace(data_range<It> p, std::span<const std::byte> v)
	{
		auto [it, sz] = p;
		if (sz == std::size(v)) [[likely]];
		else if (sz < std::size(v))
			it = s_.insert(it, std::size(v) - sz, {});
		else if (sz > std::size(v)) [[unlikely]]
			it = s_.erase(it, it + (sz - std::size(v)));
		std::copy(std::begin(v), std::end(v), it);
		return {it, std::size(v)};
	}

	/*
	 * encode_ih - encode CBOR item head
	 */
	template<class It, class U>
	requires std::is_unsigned_v<U>
	data_range<It>
	encode_ih(data_range<It> p, ih::major m, U arg)
	{
		std::array<std::byte, 9> tmp;
		return append(p, ih::make(m, arg, tmp));
	}

	/*
	 * set_ih_arg
	 */
	template<class It, class U>
	requires std::is_unsigned_v<U>
	data_range<It>
	set_ih_arg(data_range<It> p, U arg)
	{
		std::array<std::byte, 9> tmp;
		auto n = ih::make(ih::get_major(p.first), arg, tmp);
		auto osz = ih::get_ih_size(p.first);
		auto [it, sz] = replace(make_data_range(p.first, osz), n);
		return {it, p.second - osz + sz};
	}

	template<class It>
	data_range<It>
	encode_item(data_range<It> p, const item &i)
	{
		std::visit([&](auto &v) { p = encode(p, v); }, i);
		return p;
	}

	template<class It>
	data_range<It>
	encode_sequence(data_range<It> p, const auto &...a)
	{
		((p = encode(p, a)), ...);
		return p;
	}

	template<class It, class T>
	requires std::is_floating_point_v<T>
	data_range<It>
	encode(data_range<It> p, T v)
	{
		/* nan is encoded as 2-byte float */
		if (std::isnan(v))
			return append(p, {ih::fp16, 0x7e_b, 0x00_b});
		/* infinity is encoded as 2-byte float */
		if (std::isinf(v))
			return append(p, {ih::fp16, v > 0 ? 0x7c_b : 0xfc_b, 0x00_b});

		/* encode doubles as float when no data is lost */
		if (std::is_same_v<T, double> && v == static_cast<float>(v))
			return encode(p, static_cast<float>(v));

		std::array<std::byte, sizeof v + 1> b{
				  sizeof v == 4 ? ih::fp32 : ih::fp32};
		write_be(&b[1], &v);
		return append(p, b);
	}

	template<class It, class T>
	requires std::is_integral_v<T>
	data_range<It>
	encode(data_range<It> p, T v)
	{
		return encode_ih(p, v < 0 ? ih::major::negint : ih::major::posint,
				 std::make_unsigned_t<T>(v < 0 ? -v - 1 : v));
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, std::nullptr_t)
	{
		return append(p, ih::null);
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, undefined)
	{
		return append(p, ih::undefined);
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, bool v)
	{
		return append(p, v ? ih::bool_true : ih::bool_false);
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, std::span<const typename S::value_type> v)
	{
		return append(encode_ih(p, ih::major::bytes, std::size(v)), v);
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, const char *v)
	{
		return encode(p, std::string_view(v));
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, std::string_view v)
	{
		auto vb = as_bytes(std::span(std::data(v), std::size(v)));
		return append(encode_ih(p, ih::major::utf8, std::size(vb)), vb);
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, const array &a)
	{
		p = encode_ih(p, ih::major::array, std::size(a.init_));
		for (const auto &i : a.init_)
			p = encode_item(p, i);
		return p;
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, const map &m)
	{
		p = encode_ih(p, ih::major::map, std::size(m.init_));
		for (const auto &i : m.init_)
			p = encode_item(encode_item(p, i.first), i.second);
		return p;
	}

	template<class It>
	data_range<It>
	encode(data_range<It> p, const tagged &t)
	{
		if (t.tag_ == tag::invalid_1 || t.tag_ == tag::invalid_2 ||
		    t.tag_ == tag::invalid_3)
			throw std::invalid_argument("tag value is invalid");
		auto tv = static_cast<std::underlying_type_t<tag>>(t.tag_);
		return encode_item(encode_ih(p, ih::major::tag, tv), t.item_[0]);
	}

	S &s_;
};

/*
 * array_item
 */
template<class I>
class array_item {
public:
	using iterator = typename I::container::iterator;
	using const_iterator = typename I::container::const_iterator;
	using size_type = std::size_t;

	array_item(I it)
	: it_{it}
	{ }

	array_item(const array_item &) = delete;

	array_item(array_item &&) = delete;

	~array_item() = default;

	iterator
	begin()
	requires std::is_same_v<I, iterator>
	{
		return it_.enter_container();
	}

	const_iterator
	begin() const
	{
		return it_.enter_container();
	}

	const_iterator
	cbegin() const
	{
		return it_.enter_container();
	}

	iterator
	end()
	requires std::is_same_v<I, iterator>
	{
		return it_.enter_container() + size();
	}

	const_iterator
	end() const
	{
		return it_.enter_container() + size();
	}

	const_iterator
	cend() const
	{
		return it_.enter_container() + size();
	}

	void
	push_back(const auto &...v)
	{
		/* modify the item head at it_ with new size */
		auto p = it_.c_->set_ih_arg(
			I::container::make_data_range(it_.data_, std::distance(it_.data_, end().data_)),
			size() + sizeof...(v));
		auto [it, sz] = it_.c_->encode_sequence(p, v...);
		/* set_ih_arg and encode_sequence both invalidate it_.data_ */
		it_.data_ = it;
	}

	data_item<I>
	operator[](size_type i) const
	{
		/* TODO: optimise by caching item offsets? */
		CBORXX_ASSERT(i < size());
		auto it = it_.enter_container();
		while (i--)
			it = data_item(it).next().data();
		return it;
	}

	size_type
	size() const
	{
		/* TODO: cache size? */
		/* TODO: indefinite arrays */
		return ih::get_arg(it_.data_);
	}

private:
	I it_;
};

/*
 * data_item
 */
template<class I>
class data_item {
public:
	data_item(I it)
	: it_{it}
	{ }

	data_item(const data_item &) = delete;

	data_item(data_item &&r) = delete;

	~data_item() = default;

	data_item&
	operator=(const data_item &i)
	{
		it_.c_->replace(it_, i.it_);
		return *this;
	}

	data_item&
	operator=(const item &i)
	{
		it_.c_->replace(it_, i);
		return *this;
	}

	/*
	 * get - get value of data item
	 */
	template<class T>
	requires std::is_integral_v<T>
	T
	get() const
	{
		auto d = it_.data_;

		/* booleans */
		if (std::is_same_v<T, bool>) {
			switch (*d) {
			case ih::bool_false:
				return false;
			case ih::bool_true:
				return true;
			default:
				throw std::runtime_error("item is not boolean");
			}
		}

		/* integers */
		T v;
		switch (ih::get_major(d)) {
		case ih::major::posint:
			v = ih::get_arg(d);
			if (static_cast<uint64_t>(v) != ih::get_arg(d))
				throw std::range_error("integer overflow");
			return v;
		case ih::major::negint:
			if (ih::get_arg(d) > std::numeric_limits<int64_t>::max())
				throw std::range_error("integer overflow");
			v = -1 - static_cast<int64_t>(ih::get_arg(d));
			if (static_cast<int64_t>(v) !=
			    -1 - static_cast<int64_t>(ih::get_arg(d)))
				throw std::range_error("integer overflow");
			return v;
		default:
			throw std::runtime_error("item is not integral");
		}
	}

	template<class T>
	requires std::is_floating_point_v<T>
	T
	get() const
	{
		T v;
		auto d = it_.data_;
		switch (*d) {
		case ih::fp16:
			if (d[2] != 0x00_b)
				throw std::runtime_error("float16 not supported");
			if (d[1] == 0x7e_b)
				return std::numeric_limits<T>::quiet_NaN();
			if (d[1] == 0x7c_b)
				return std::numeric_limits<T>::infinity();
			if (d[1] == 0xfc_b)
				return -std::numeric_limits<T>::infinity();
			throw std::runtime_error("float16 not supported");
		case ih::fp32:
			return read_be<float>(d + 1);
		case ih::fp64:
			if (auto vd = read_be<double>(d + 1); v = vd, v != vd)
				throw std::range_error("lossy float conversion");
			return v;
		default:
			throw std::runtime_error("item is not floating point");
		}
	}

	tag
	get_tag() const
	{
		auto d = it_.data_;
		if (ih::get_major(d) != ih::major::tag)
			throw std::runtime_error("item is not tagged");
		return static_cast<cbor::tag>(ih::get_arg(d));
	}

	std::span<const std::byte>
	get_bytes() const
	requires std::contiguous_iterator<typename I::data_iterator>
	{
		/* throws for indefinite size */
		auto d = std::to_address(it_.data_);
		if (ih::get_major(d) != ih::major::bytes)
			throw std::runtime_error("item is not bytes");
		auto ih_sz = ih::get_ih_size(d);
		return {d + ih_sz, d + ih_sz + ih::get_data_size(d)};
	}

	std::string_view
	get_string() const
	requires std::contiguous_iterator<typename I::data_iterator>
	{
		/* throws for indefinite size */
		auto d = std::to_address(it_.data_);
		if (ih::get_major(d) != ih::major::utf8)
			throw std::runtime_error("item is not a string");
		auto ih_sz = ih::get_ih_size(d);
		return {reinterpret_cast<const char *>(d + ih_sz),
			static_cast<std::string_view::size_type>(ih::get_data_size(d))};
	}

	array_item<I>
	get_array() const
	{
		if (ih::get_major(it_.data_) != ih::major::array)
			throw std::runtime_error("item is not an array");
		return it_;
	}

	/*
	 * type
	 */
	cbor::type
	type() const
	{
		auto d = it_.data_;
		switch (ih::get_major(d)) {
		case ih::major::posint: {
			auto sz = ih::get_arg_size(d);
			auto val = ih::get_arg(d);
			if (sz < 4)
				return cbor::type::int32;
			if (sz == 4)
				return val > std::numeric_limits<int32_t>::max()
				    ? cbor::type::uint32 : cbor::type::int32;
			return val > std::numeric_limits<int64_t>::max()
			    ? cbor::type::uint64 : cbor::type::int64;
		}
		case ih::major::negint: {
			auto sz = ih::get_arg_size(d);
			auto val = ih::get_arg(d);
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
			switch (static_cast<ih::special>(ih::get_ai(d))) {
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

	const data_item<I> *
	operator->() const
	{
		return this;
	}

	data_item<I> *
	operator->()
	{
		return this;
	}

	/*
	 * untag - get untagged data item
	 */
	data_item<I>
	untag() const
	{
		if (ih::get_major(it_.data_) != ih::major::tag)
			throw std::runtime_error("data is not tagged");
		return it_.enter_container();
	}

	/*
	 * next - return next data item at this level
	 */
	data_item<I>
	next() const
	{
		switch (ih::get_major(it_.data_)) {
		case ih::major::array:
			return array_item(it_).end();
		case ih::major::map:
			assert(!"todo");
		case ih::major::tag:
			return untag().next().it_;
		default:
			return it_.next_item();
		}
	}

	/*
	 * data - get raw cbor data
	 */
	I
	data() const
	{
		return it_;
	}

private:
	const I it_;
};

}
