#include "sys.h"
#include "debug.h"
#include "ThreadPermuter.h"

char memory_block[32];
char* const block_start = memory_block;
char* const epptr = block_start + sizeof(memory_block);

struct Atomic
{
 private:
  char* next_egptr;
  char* next_egptr2;
  char* last_gptr;

 protected:
  char output_c;
  char input_c;

 public:
  Atomic() { init(); }

  char* read_next_egptr()
  {
    char* val = next_egptr;
    TPY;
    return val;
  }

  char* get_next_egptr() const
  {
    return next_egptr;
  }

  void set_next_egptr(char* val)
  {
    next_egptr =val;
    print();
    TPY;
  }

  char* read_next_egptr2()
  {
    char* val = next_egptr2;
    TPY;
    return val;
  }

  char* get_next_egptr2() const
  {
    return next_egptr2;
  }

  void set_next_egptr2(char* val)
  {
    next_egptr2 =val;
    print();
    TPY;
  }

  char* read_last_gptr()
  {
    char* val = last_gptr;
    TPY;
    return val;
  }

  char* get_last_gptr() const
  {
    return last_gptr;
  }

  void set_last_gptr(char* val, bool block)
  {
    block = block && last_gptr == val;
    last_gptr = val;
    print();
    if (block)
      TPB;
    else
      TPY;
  }

  void init()
  {
    next_egptr = block_start;
    next_egptr2 = block_start;
    last_gptr = block_start;
    output_c = 'A';
    input_c = 'A';
  }

  virtual void print() const = 0;
};

struct PutState : public virtual Atomic
{
  char* pptr;

  PutState() : pptr(block_start) { }

  bool write()
  {
    DoutEntering(dc::notice, "PutState:write()");
    if (pptr != block_start &&                  // Don't start a reset cycle when pptr is already at the start of the block ;).
        read_next_egptr() != nullptr &&         // If next_egptr is nullptr then the put area was reset, but the get area wasn't yet; don't reset again until it was.
                                                // This read must be acquire to make sure the write to last_gptr is visible too.
        pptr == read_last_gptr())               // If this happens while next_egptr != nullptr then the buffer is truely empty (gptr == pptr).
    {
      Dout(dc::notice, "Resetting buffer because next_egptr(P/n) != nullptr and  buffer is empty (pptr(P/p) == last_gptr(G/E/l)):");
      // Next simulate a sync() where next_egptr is set to the beginning on the block.
      // However, set next_egptr to nullptr to signal the GetThread that a reset is needed.
      Dout(dc::notice, "  next_egptr2 = block_start");
      set_next_egptr2(block_start);             // Initialize next_egptr2 that the GetThread will use once it reset itself.
                                                // This value will not be read by the GetThread until after it sees next_egptr to be nullptr.
                                                // Therefore this write can be relaxed.
      Dout(dc::notice, "  next_egptr(P/n) = 0");
      // A value of nullptr means 'block_start', but will prevent the PutThread to write to it
      // until the GetThread did reset too. Nor will the PutThread reset again until that happened.
      set_next_egptr(nullptr);                  // Atomically signal the GetThread that it must reset.
                                                // This write must be release to flush the write of next_egptr2.
      // The actual reset of the put area.
      Dout(dc::notice, "  pptr(P/p) = 0");
      pptr = block_start;                       // Reset ourselves.
      print();
    }
    else if (pptr == epptr)                     // Buffer full? Do nothing in this test.
    {
      Dout(dc::notice, "Returning because the buffer is full (pptr(P/p) == epptr).");
      TPB;
      return false;
    }
    // Actually write data to the buffer.
    TP_ASSERT(pptr == block_start || pptr == block_start + 32);
    std::memset(pptr, output_c, 32);
    ++output_c;
    pptr += 32;
    Dout(dc::notice, "Wrote 32 bytes, pptr(P/p) is now " << (int)(pptr - block_start));
    print();
    return true;
  }

  void sync()
  {
    DoutEntering(dc::notice, "PutState:sync()");
    // While next_egptr is nullptr, do not change its value - update next_egptr2 instead.
    Dout(dc::notice, "next_egptr2(P) = pptr(P) (" << (int)(pptr - block_start) << ").");
    set_next_egptr2(pptr);              // seq_cst
    if (read_next_egptr() != nullptr)   // seq_cst
    {
      Dout(dc::notice, "next_egptr(P) = pptr(P) (" << (int)(pptr - block_start) << ").");
      set_next_egptr(pptr);             // release
    }
  }
};

struct GetState : public virtual Atomic
{
  char* gptr;
  char* egptr;
  std::string m_output;

  GetState() : gptr(block_start), egptr(block_start) { }

  bool read()
  {
    DoutEntering(dc::notice, "GetState:read()");
    if (get_next_egptr())
      Dout(dc::notice, "egptr(E/e) = next_egptr(P/n) (" << (int)(get_next_egptr() - block_start) << ").");
    else
      Dout(dc::notice, "egptr(E/e) = next_egptr(P/n) (nullptr).");
    egptr = get_next_egptr();           // acquire, to also get the data that was written to the buffer.
    print();
    TPY;
    // If there WAS a reset then next_egptr was set to nullptr.
    if (egptr == nullptr)
    {
      Dout(dc::notice, "Resetting get area because egptr(E/e) == nullptr");
      Dout(dc::notice, "  gptr(g) = 0");
      gptr = block_start;
      Dout(dc::notice, "  last_gptr(G/E/l) = 0");
      set_last_gptr(block_start, false);        // relaxed, is synchronized by the release (seq_cst) of the set_next_egptr below.
      Dout(dc::notice, "  next_egptr(P/n) = block_start");
      set_next_egptr(block_start);              // seq_cst
      char* expected = block_start;
      for (;;)
      {
        Dout(dc::notice|continued_cf, "  n2 = next_egptr2  = ");
        char* n2 = get_next_egptr2();           // seq_cst
        Dout(dc::finish, (int)(n2 - block_start));
        print();
        TPY;
        // compare_exchange
        if (get_next_egptr() == expected)       // acquire
        {
          Dout(dc::notice, "  next_egptr(P/n) = n2");
          set_next_egptr(n2);                   // relaxed is ok (is read below in the same thread, and at the top of write(), but the delay is not important there).
          break;        // Success
        }
        else
        {
          expected = get_next_egptr();          // acquire (may actually be relaxed).
        }
        // Compare exchange failed.
      }
      Dout(dc::notice, "  egptr(E/e) = next_egptr(P/n) (" << (int)(get_next_egptr() - block_start) << ").");
      egptr = get_next_egptr();
      print();
      // In most cases next_egptr will *still* be block_start, but lets continue reading when possible.
      if (egptr == block_start)
      {
        TPB;
        return false;
      }
      TPY;
    }
    // If the get area is empty, do nothing - but copy the current value of gptr to last_gptr.
    // Note that we must reset (when next_egptr == nullptr) then this is never equal.
    else if (gptr == egptr)
    {
      Dout(dc::notice, "Updating last_gptr(G/E/l) because there is nothing else to read (gptr(g) == egptr(E/e) == " << (int)(gptr - block_start) << ").");
      Dout(dc::notice, "  last_gptr(G/E/l) = gptr(g) (" << (int)(gptr - block_start) << ").");
      set_last_gptr(gptr, egptr == get_next_egptr());   // relaxed, it doesn't really matter when the PutThread sees this.
      return false;
    }
    TP_ASSERT(egptr - gptr >= 32);
    m_output += std::string(gptr, 32);
    TP_ASSERT(*gptr == input_c);
    ++input_c;
    gptr += 32;
    Dout(dc::notice, "Read 32 bytes, gptr(g) is now " << (int)(gptr - block_start));
    print();
    return true;
  }
};

struct State : public PutState, public GetState
{
  State() { }

  void init()
  {
    gptr = egptr = pptr = block_start;
    m_output.clear();
    Atomic::init();
  }

  void on_permutation_begin()
  {
    DoutEntering(dc::notice, "State::on_permutation_begin()");
    init();
    print();
  }

  void on_permutation_end(std::string const& permutation)
  {
    DoutEntering(dc::notice, "State::on_permutation_end(\"" << permutation << "\")");
    Dout(dc::notice, "output = \"" << m_output << "\".");
  }

  friend std::ostream& operator<<(std::ostream& os, State const& state)
  {
    os << "pptr = " << (int)(state.pptr - block_start) << "; gptr = " << (int)(state.gptr - block_start) << "; egptr = " << (int)(state.egptr - block_start) <<
      "; next_egptr = " << (int)(state.get_next_egptr() - block_start) << "; last_gptr = " << (int)(state.get_last_gptr() - block_start);
    return os;
  }

  void print() const override;
};

void State::print() const
{
  std::string str1 = "                    ";
  std::string str2 = "  ================  ";
  int gptr_offset = (gptr - block_start) / 4 + 2;
  int pptr_offset = (pptr - block_start) / 4 + 2;
  str1[gptr_offset] = 'g';
  if (egptr == nullptr)
  {
    str1[0] = 'e';
  }
  else
  {
    int egptr_offset = (egptr - block_start) / 4 + 2;
    str1[egptr_offset + (egptr_offset == gptr_offset ? 1 : 0)] = 'e';
  }
  str2[pptr_offset] = 'p';
  if (get_next_egptr())
  {
    int next_egptr_offset = (get_next_egptr() - block_start) / 4 + 2;
    str2[next_egptr_offset] = pptr_offset == next_egptr_offset ? 'P' : 'n';
  }
  else
    str2[0] = 'n';
  int last_gptr_offset = (get_last_gptr() - block_start) / 4 + 2;
  str1[last_gptr_offset] = gptr == get_last_gptr() ? 'G' : egptr == get_last_gptr() ? 'E' : 'l';
  Dout(dc::notice, str1);
  Dout(dc::notice, str2);
}

void PutThread(PutState& state)
{
  int count = 0;
  do
  {
    count += state.write();
    state.sync();
  }
  while (count != 2);
}

void GetThread(GetState& state)
{
  int count = 0;
  do
  {
    count += state.read();
  }
  while (count != 2);
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  State state;

  ThreadPermuter::tests_type tests =
  {
    [&]{ Debug(libcw_do.margin().assign("PutThread  ")); PutThread(state); },
    [&]{ Debug(libcw_do.margin().assign("GetThread  ")); GetThread(state); }
  };

  ThreadPermuter tp(
      [&]{ state.on_permutation_begin(); },
      tests,
      [&](std::string const& permutation){ state.on_permutation_end(permutation); }
  );

  tp.run("00000010000001000001101");

  Dout(dc::notice|flush_cf, "After: " << state);
}
