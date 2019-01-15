#include "sys.h"
#include "debug.h"
#include "utils/BitSet.h"
#include <iostream>

using namespace utils;

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  uint32_t const mask = 0x80005709;

  BitSetIndex i = index_pre_begin;
  while (i.next_bit_in(mask), i != index_end<uint32_t>)
    Dout(dc::notice, "Found index " << (int)i());

  i = index_end<uint32_t>;
  while (i.prev_bit_in(mask), i != index_pre_begin)
    Dout(dc::notice, "Found index " << (int)i());
}
