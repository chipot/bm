/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Pichot
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
**/

#pragma once
#ifndef BIN_ZATJQ4PK
# define BIN_ZATJQ4PK

#include <arpa/inet.h>

#include <iostream>
#include <stdexcept>
#include <tuple>
#include <cstring>
#include <bitset>
#include <cassert>

enum class endian : int
{
  LITTLE = 0,
  BIG,
  MIDDLE,
  NATIVE,
  NETWORK = BIG,
};

namespace detail {

    template <template <size_t, size_t> class Traits, size_t Sum, size_t ...List>
    struct foldl;

    template <template <size_t, size_t> class Traits, size_t Sum, size_t Value, size_t...List>
    struct foldl<Traits, Sum, Value, List...>
    {
      static constexpr size_t value = foldl<Traits, Traits<Value, Sum>::value, List...>::value;
    };

    template <template <size_t, size_t> class Traits, size_t Sum>
    struct foldl<Traits, Sum>
    {
      static constexpr size_t value = Sum;
    };

    template <size_t Value, size_t Sum>
    struct sum
    {
      static constexpr size_t value = Value + Sum;
    };

    template <size_t i, size_t end>
    struct match {
      template <typename T>
      static bool make(unsigned char const *memory, T& values)
      {
        auto& v = std::get<i>(values);
        if (v.match(memory))
        {
          size_t incr = v.incr();
          if (incr == 0)
            return true;
          return match<i + 1, end>::make(memory + incr, values);
        }
        else
          return false;
      }
    };

    template <size_t end>
    struct match<end, end> {
      template <typename T>
      static bool make(unsigned char const *memory, T& values)
      {
        return true;
      }
    };
} /* detail */

class base_binary
{
public:

  virtual bool match(unsigned char const* memory) = 0;
};

template <typename ...Pattern>
class binary: public base_binary
{
  std::tuple<Pattern...> _values;
  using value_type = std::tuple<Pattern...>;

public:
  template <typename ...T>
  binary(T && ... values)
    : _values{std::forward<T>(values)...}
  {
  }

  bool match(unsigned char const *memory)
  {
    return detail::template match<0, sizeof...(Pattern)>::make(memory, this->_values);
  }
};

template <>
class binary<> : public base_binary
{
  unsigned char const* _data = nullptr;
public:
  binary()
  {
  }

  bool match(unsigned char const *memory)
  {
    this->_data = memory;
    return true;
  }

  size_t incr() const
  {
    return 0;
  }

  unsigned char const* payload() const
  {
    return this->_data;
  }
};

template <size_t nbits>
class bits
{
  std::bitset<nbits>& _value;
  template <size_t ...> friend class bit_field;
public:
  bits(std::bitset<nbits>& value)
    : _value{value}
  {
  }

  bool match(unsigned char const* memory, size_t offset = 0) const
  {
    // We need N full bytes
    size_t bit_total = nbits + offset;
    size_t needed = ((bit_total) / 8);
    size_t remain = ((bit_total) % 8);

    if (bit_total > 8)
      needed += 1;

    if (needed == 0)
      needed = 1;

    for (size_t sbit = 0, i = 0; i < needed; i++)
    {
      unsigned char const byte = memory[i];

      for (size_t bit = offset;
           bit < (sizeof(byte) * 8) && sbit < nbits;
           ++bit, ++sbit)
      {
        this->_value.set(sbit, bool(byte & (1 << bit)));
      }
      offset = 0;
    }
    return true;
  }

  size_t bitsize() const
  {
    return nbits;
  }

  size_t incr() const
  {
    return nbits / 8;
  }
};

template <size_t ...BitSets>
class bit_field
{
  static constexpr size_t bitsize = detail::foldl<detail::sum, 0, BitSets...>::value;

  static_assert(bitsize % 8 == 0, "bit field not aligned on a octet");

  using tuple_type = std::tuple<bits<BitSets>...>;
  tuple_type _bits;

  template <size_t Idx, size_t Max>
  struct bitfield_match
  {
    template <typename T>
    static bool make(std::bitset<bitsize> const& allbits, T& t, size_t off = 0)
    {
      auto& current = std::get<Idx>(t)._value;

      assert(off < bitsize);

      for (size_t i = 0; i < current.size(); ++i, ++off)
      {
        current[i] = allbits.test(off);
      }
      return bitfield_match<Idx + 1, Max>::make(allbits, t, off);
    }
  };

  template <size_t Max>
  struct bitfield_match<Max, Max>
  {
    template <typename T>
    static bool make(std::bitset<bitsize> const& allbits, T& t, size_t off = 0)
    {
      return true;
    }
  };

public:
  bit_field(bits<BitSets>&& ...args)
    : _bits{args...}
  {
  }

  bool match(unsigned char const* memory) const
  {
    std::bitset<bitsize> stock;
    bits<bitsize> allbits{stock};
    if (allbits.match(memory) == false)
      return false;

    bitfield_match<0, std::tuple_size<tuple_type>::value>
      ::make(stock, this->_bits);
    return true;
  }

  size_t incr() const
  {
    // Minimum for bitsize is 8
    return bitsize / 8;
  }
};

template <typename T, enum endian E = endian::NATIVE>
class assign
{
  T& _target;

  T hton(T value)
  {
    switch (sizeof(T))
    {
      default:
        assert(false);
      case 4:
        return ntohl(value);
      case 2:
        return ntohs(value);
    }
  }


public:
  assign(T& target)
    : _target{target}
  {
  }

  bool match(unsigned char const *memory)
  {
    auto rhs = *reinterpret_cast<T const *>(memory);

    switch (E)
    {
      default:
      case endian::NATIVE:
        this->_target = rhs;
        break;
      case endian::NETWORK:
        this->_target = hton(rhs);
        break;
    }
    return true;
  }

  size_t incr() const
  {
    return sizeof(T);
  }
};

template <size_t n>
class bytes
{
  unsigned char *_target = nullptr;
public:
  bytes(unsigned char (&tab)[n])
    : _target{tab}
  {
  }

  bytes()
  {
  }

  template <typename T>
  bytes &operator = (T &target)
  {
    this->_target = reinterpret_cast<unsigned char *>(&target);
    return *this;
  }

  bool match(unsigned char const* memory)
  {
    std::memcpy(this->_target, memory, n);
    return true;
  }

  size_t incr() const
  {
    return n;
  }
};

template <typename T>
class value
{
  T _val;
public:
  value(T value)
    : _val{value}
  {

  }

  value(T && value)
    : _val{std::forward<T>(value)}
  {
  }

  bool match(unsigned char const* memory)
  {
    auto rhs = *reinterpret_cast<T const*>(memory);
    return rhs == this->_val;
  }

  size_t incr() const
  {
    return sizeof(T);
  }
};

namespace detail
{
    template <typename T, size_t i = 0>
    struct adapt
    {
      using type = typename std::remove_reference<T>::type &;
    };

    template <size_t i>
    struct adapt<int, i>
    {
      using type = value<int>;
    };

    template <size_t i>
    struct adapt<int&, i>
    {
      using type = assign<int>;
    };

    template <size_t i>
    struct adapt<unsigned int, i>
    {
      using type = value<unsigned int>;
    };

    template <size_t i>
    struct adapt<unsigned int&, i>
    {
      using type = assign<unsigned int>;
    };

    template <size_t i>
    struct adapt<char, i>
    {
      using type = value<char>;
    };

    template <size_t i>
    struct adapt<char&, i>
    {
      using type = assign<char>;
    };

    template <size_t i>
    struct adapt<unsigned char, i>
    {
      using type = value<unsigned char>;
    };

    template <size_t i>
    struct adapt<unsigned char&, i>
    {
      using type = assign<unsigned char>;
    };

    template <size_t i>
    struct adapt<short, i>
    {
      using type = value<short>;
    };

    template <size_t i>
    struct adapt<short&, i>
    {
      using type = assign<short>;
    };

    template <size_t i>
    struct adapt<unsigned short, i>
    {
      using type = value<unsigned short>;
    };

    template <size_t i>
    struct adapt<unsigned short&, i>
    {
      using type = assign<unsigned short>;
    };

    template <size_t i>
    struct adapt<unsigned char (&)[i]>
    {
      using type = bytes<i>;
    };

    template <size_t i>
    struct adapt<std::bitset<i>&>
    {
      using type = bits<i>;
    };
}

template <template <size_t> class ...BS, size_t ...Idx>
auto make_bitfield(BS<Idx> & ...args) -> bit_field<Idx...>
{
  return bit_field<Idx...>{typename detail::adapt<BS<Idx>>::type{args}...};
}

template <typename ...Pattern>
auto make_binary(Pattern && ... ps)
  -> binary<typename detail::adapt<Pattern>::type...>
{
  return binary<typename detail::adapt<Pattern>::type...>{
    ps...
  };
}

template <typename T>
auto make_value(T && val)
  -> decltype(value<typename std::remove_reference<T>::type>{val})
{
  return value<typename std::remove_reference<T>::type>{val};
}

using store_binary = binary<>;

#endif /* end of include guard: BIN_ZATJQ4PK */
