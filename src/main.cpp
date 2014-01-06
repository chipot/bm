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

#include "bin.hh"

#include <iomanip>
#include <fstream>
#include <sstream>

unsigned char frame[] = {
  0x0, 0x0, 0x0, 0x0, 0x0, 0x1,
  0x0, 0x0, 0x1, 0x0, 0x0, 0x1,
  46, 0,

  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  //5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,

  //0, 0, 0, 6,
};

template <int i>
struct dump
{
  template <typename T, typename ...List>
  static void do_it(std::ostream& out, T && value, List &&... args)
  {
    out << ":" << std::setw(2) << std::setfill('0') << static_cast<int>(value);
    dump<i + 1>::do_it(out, std::forward<List>(args)...);
  }

  static void do_it(std::ostream&) { }
};

template <>
struct dump<0>
{
  template <typename T, typename ...List>
  static void do_it(std::ostream& out, T && value, List &&... args)
  {
    out << std::setw(2) << std::setfill('0') << static_cast<int>(value);
    dump<1>::do_it(out, std::forward<List>(args)...);
  }
};

std::string ddump(unsigned char *source)
{
  std::stringstream ss;
  ss << std::hex;
  dump<0>::do_it(ss, source[0], source[1], source[2], source[3], source[4], source[5]);
  return ss.str();
}

int
main(int argc, const char *argv[])
{
  unsigned char source[6] = {}, dest[6] = {};
  short header_size = 8;

  std::ifstream in{"out"};

  in.seekg(0, std::ios_base::end);
  size_t size = in.tellg();
  in.seekg(0);

  auto *data = new unsigned char[size];
  in.read(reinterpret_cast<char *>(data), size);
  store_binary p;

  auto b = make_binary(dest, source, 8, p);
  if (b.match(data))
  {
    std::cout << "OK" << std::endl;
  }
  else
  {
    std::cout << "KO" << std::endl;
    return 1;
  }

  std::cout << "source is: " << ddump(source) << std::endl;
  std::cout << "destination is: " << ddump(dest) << std::endl;
  std::cout << "payload size is: " << header_size << std::endl;
  std::cout << "payload at: " << (void*)p.payload() << std::endl;

  unsigned char *payload = p.payload();
  std::cout << "payload is: ";
  std::cout << std::hex;
  for (size_t i = 0; i < header_size; ++i)
  {
    if (i != 0)
      std::cout << ", ";
    std::cout << std::setw(2) << std::setfill('0') << int(payload[i]);
  }
  std::cout << std::dec << std::endl;
  return 0;
}
