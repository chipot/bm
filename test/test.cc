#define BOOST_TEST_MODULE binmatch
#include <boost/test/unit_test.hpp>
#include "bin.hh"

BOOST_AUTO_TEST_SUITE(bin_match)

BOOST_AUTO_TEST_SUITE(binfield)
BOOST_AUTO_TEST_CASE(simple)
{
  unsigned char const tab[1] = {1};
  std::bitset<8> b;
  auto bin = make_binary(b);

  BOOST_CHECK_EQUAL(bin.match(tab), true);
  BOOST_CHECK_EQUAL(b.to_string(), "00000001");
}

BOOST_AUTO_TEST_CASE(small)
{
  unsigned char const tab[1] = {42};
  std::bitset<4> b1;
  std::bitset<4> b2;
  auto bin = make_binary(make_bitfield(b1, b2));

  BOOST_CHECK_EQUAL(bin.match(tab), true);
  BOOST_CHECK_EQUAL(b1.to_string(), "1010");
  BOOST_CHECK_EQUAL(b2.to_string(), "0010");
}

BOOST_AUTO_TEST_CASE(bits_test)
{
  {
    unsigned char const tab[1] = {0x2a};
    std::bitset<4> b1;
    bits<4> b{b1};

    auto mk = [&] (size_t offset)
    {
      b1 = decltype(b1){};
      BOOST_CHECK_EQUAL(b.match(tab, offset), true);
    };

    mk(0);
    BOOST_CHECK_EQUAL(b1.to_string(), "1010");
    mk(3);
    BOOST_CHECK_EQUAL(b1.to_string(), "0101");
    mk(4);
    BOOST_CHECK_EQUAL(b1.to_string(), "0010");
  }

  {
    unsigned char const tab[2] = {0x00, 0xFF};
    std::bitset<4> b1;
    bits<4> b{b1};

    auto mk = [&] (size_t offset)
    {
      std::stringstream ss;
      ss << "trying offset: " << offset;
      b1 = decltype(b1){};
      BOOST_CHECK_EQUAL(b.match(tab, offset), true);
    };

    mk(1);
    BOOST_CHECK_EQUAL(b1.to_string(), "0000");
    mk(2);
    BOOST_CHECK_EQUAL(b1.to_string(), "0000");
    mk(3);
    BOOST_CHECK_EQUAL(b1.to_string(), "0000");
    mk(4);
    BOOST_CHECK_EQUAL(b1.to_string(), "0000");
    mk(5);
    BOOST_CHECK_EQUAL(b1.to_string(), "1000");
    mk(6);
    BOOST_CHECK_EQUAL(b1.to_string(), "1100");
    mk(7);
    BOOST_CHECK_EQUAL(b1.to_string(), "1110");
    mk(8);
    BOOST_CHECK_EQUAL(b1.to_string(), "1111");
    mk(9);
    BOOST_CHECK_EQUAL(b1.to_string(), "1111");
    mk(10);
    BOOST_CHECK_EQUAL(b1.to_string(), "1111");
  }

  {
    unsigned char const tab[3] = {0x00, 0xFF, 0x2a};
    std::bitset<16> b1;
    bits<16> b{b1};

    auto mk = [&] (size_t offset)
    {
      b1 = decltype(b1){};
      BOOST_CHECK_EQUAL(b.match(tab, offset), true);
    };

    mk(4);
    BOOST_CHECK_EQUAL(b1.to_string(), "1010111111110000");
  }
  {
    unsigned char tab[24] = {};
    std::bitset<130> b1;
    bits<130> b{b1};

    std::memset(static_cast<void*>(tab), 0xFF, sizeof(tab));
    auto mk = [&] (size_t offset)
    {
      b1 = decltype(b1){};
      BOOST_CHECK_EQUAL(b.match(tab, offset), true);
    };

    mk(0);
    BOOST_CHECK_EQUAL(b1.to_string(), "111111111111111111111111111111111111111\
111111111111111111111111111111111111111111111111111111111111111111111111111111\
1111111111111");
  }
}

BOOST_AUTO_TEST_CASE(bit_field_test)
{
  {
    std::bitset<4> b1;
    std::bitset<4> b2;

    bit_field<4, 4> b{bits<4>{b1}, bits<4>{b2}};
    BOOST_CHECK_EQUAL(b.incr(), 1);
  }

  {
    std::bitset<8> b1;
    std::bitset<8> b2;

    bit_field<8, 8> b{bits<8>{b1}, bits<8>{b2}};
    BOOST_CHECK_EQUAL(b.incr(), 2);
  }

  {
    std::bitset<4> b1;
    std::bitset<4> b2;
    std::bitset<8> b3;

    bit_field<4, 4, 8> b{bits<4>{b1}, bits<4>{b2},bits<8>{b3}};
    BOOST_CHECK_EQUAL(b.incr(), 2);
  }

  {
    unsigned char const tab[1] = {42};

    std::bitset<4> b1;
    std::bitset<4> b2;
    //std::bitset<2> b3;

    auto bf = make_bitfield(b1, b2);

    BOOST_CHECK_EQUAL(bf.incr(), 1);
    BOOST_CHECK_EQUAL(bf.match(tab), true);
    BOOST_CHECK_EQUAL(b1.to_string(), "1010");
    BOOST_CHECK_EQUAL(b2.to_string(), "0010");
  }
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
