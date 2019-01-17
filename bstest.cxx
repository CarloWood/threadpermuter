#include "sys.h"
#include "debug.h"
#include "utils/BitSet.h"
#include <iostream>

using namespace utils;
using namespace bitset;
using mask_t = uint32_t;

constexpr BitSetPOD<mask_t> test_value1 = { 0x80000700 };
constexpr BitSetPOD<mask_t> test_value2 = { 0x00005009 };

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  // Test Index.

  mask_t const mask = 0x80005709;

  Index i = index_pre_begin;
  while (i.next_bit_in(mask), i != index_end<mask_t>)
    Dout(dc::notice, "Found index " << (int)i());

  i = index_end<mask_t>;
  while (i.prev_bit_in(mask), i != index_pre_begin)
    Dout(dc::notice, "Found index " << (int)i());

  // Test BitSet.

  constexpr BitSetPOD<mask_t> cbs1 = { mask };
  constexpr BitSetPOD<mask_t> cbs2 = test_value1|test_value2;
  static_assert(cbs1 == cbs2, "*Twilight tune*");

  BitSet<mask_t> bs1{mask};
  BitSet<mask_t> const bs2 = test_value1|test_value2;

  if (bs1 != bs2)
    std::cerr << "WTF" << std::endl;

  Dout(dc::notice, "Forward iterating:");
  for (auto m : bs2)
    Dout(dc::notice, "Found bit " << std::hex << m);
}
