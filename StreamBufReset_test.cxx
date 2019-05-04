#include "sys.h"
#include "debug.h"
#include "ThreadPermuter.h"

char memory_block[64];
char* const block_start = memory_block;
char* const epptr = block_start + sizeof(memory_block);

struct Atomic
{
 private:
  char* next_egptr;
  char* next_egptr2;
  char* last_gptr;
  bool need_reset;

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

  void set_last_gptr(char* val)
  {
    last_gptr = val;
    print();
    TPY;
  }

  bool read_need_reset()
  {
    bool val = need_reset;
    TPY;
    return val;
  }

  bool get_need_reset() const
  {
    return need_reset;
  }

  void set_need_reset(bool val)
  {
    need_reset = val;
    print();
    TPY;
  }

  void init()
  {
    next_egptr = block_start;
    next_egptr2 = block_start;
    last_gptr = block_start;
    need_reset = false;
    output_c = 'A';
    input_c = 'A';
  }

  virtual void print() const = 0;
};

struct PutState : public virtual Atomic
{
  char* pptr;

  PutState() : pptr(block_start) { }

  void write()
  {
    DoutEntering(dc::notice, "PutState:write()");
    if (pptr > block_start && pptr == read_last_gptr() && read_next_egptr() != nullptr)
    {
      Dout(dc::notice, "Resetting buffer because next_egptr(P/n) != nullptr and  buffer is empty (pptr(P/p) == last_gptr(G/E/l)):");
      // A value of nullptr means 'block_start', but will prevent the PutThread to write to it
      // until the GetThread did reset too. Nor will the PutThread reset again until that happened.
      Dout(dc::notice, "  next_egptr2 = block_start");
      set_next_egptr2(block_start);
      Dout(dc::notice, "  next_egptr(P/n) = 0");
      set_next_egptr(nullptr);
      Dout(dc::notice, "  pptr(P/p) = 0");
      pptr = block_start;
      print();
    }
    else if (pptr == epptr)
    {
      Dout(dc::notice, "Returning because the buffer is full (pptr(P/p) == epptr).");
      std::cout << "full" << std::endl;
      return;
    }
    TP_ASSERT(pptr == block_start || pptr == block_start + 32);
    std::memset(pptr, output_c, 32);
    std::cout << "<< " << output_c << std::endl;
    ++output_c;
    pptr += 32;
    Dout(dc::notice, "Wrote 32 bytes, pptr(P/p) is now " << (int)(pptr - block_start));
    print();
  }

  void sync()
  {
    DoutEntering(dc::notice, "PutState:sync()");
    Dout(dc::notice, "next_egptr(P) = pptr(P) (" << (int)(pptr - block_start) << ").");
    if (read_next_egptr() == nullptr)
      set_next_egptr2(pptr);
    else
      set_next_egptr(pptr);
  }
};

struct GetState : public virtual Atomic
{
  char* gptr;
  char* egptr;
  std::string m_output;

  GetState() : gptr(block_start), egptr(block_start) { }

  void read()
  {
    DoutEntering(dc::notice, "GetState:read()");
    Dout(dc::notice, "egptr(E/e) = next_egptr(P/n) (" << (int)(get_next_egptr() - block_start) << ").");
    egptr = get_next_egptr();
    print();
    TPY;
    // As long as the get area appear empty we are not allowed to do ANYTHING - not even reset the get area.
    // The reason for that is that setting next_egptr to block_start in the PutThread is the atomic(!) signal
    // that a reset is necessary; it has to be atomic, so no other variable can be used for that.
    if (gptr == egptr)
    {
      Dout(dc::notice, "Updating last_gptr(G/E/l) because there is nothing else to read (gptr(g) == egptr(E/e) == " << (int)(gptr - block_start) << ").");
      Dout(dc::notice, "  last_gptr(G/E/l) = gptr(g) (" << (int)(gptr - block_start) << ").");
      set_last_gptr(gptr);
      std::cout << "empty" << std::endl;
      return;
    }
    // If there WAS a reset then next_egptr was set to nullptr at the moment that last_gptr == gptr == egptr == last_egptr == pptr > block_start,
    // of those egptr was just reset to block_start, but gptr has not changed. Therefore the following condition will be true
    // if and only if the PutThread set next_egptr to block_start as part of a reset before we entered this function, and after
    // the entered the function the previous time.
    if (gptr > egptr)
    {
      Dout(dc::notice|continued_cf, "Resetting get area because gptr(g) > egptr(E/e) (" << (int)(gptr - block_start) << " > ");
      if (egptr == nullptr)
        Dout(dc::finish, "nullptr):");
      else
        Dout(dc::finish, (int)(egptr - block_start) << "):");
      Dout(dc::notice, "  gptr(g) = 0");
      gptr = block_start;
      Dout(dc::notice, "  last_gptr(G/E/l) = 0");
      set_last_gptr(block_start);
      Dout(dc::notice, "  next_egptr(P/n) = next_egptr2");
      set_next_egptr(read_next_egptr2());
      Dout(dc::notice, "  egptr(E/e) = next_egptr(P/n) (" << (int)(get_next_egptr() - block_start) << ").");
      egptr = get_next_egptr();
      print();
      TPY;
      // In most cases next_egptr will *still* be block_start, so don't even bother checking that now.
      std::cout << "reset" << std::endl;
      return;
    }

    TP_ASSERT(egptr - gptr >= 32);
    m_output += std::string(gptr, 32);
    std::cout << ">> " << *gptr << std::endl;
    TP_ASSERT(*gptr == input_c);
    ++input_c;
    gptr += 32;
    Dout(dc::notice, "Read 32 bytes, gptr(g) is now " << (int)(gptr - block_start));
    print();
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
    std::cout << "output = \"" << m_output << "\"." << std::endl;
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
  if (get_need_reset())
    str2 += " *";
  Dout(dc::notice, str1);
  Dout(dc::notice, str2);
}

void PutThread(PutState& state)
{
  state.write();
  state.sync();
  state.write();
  state.sync();
  state.write();
  state.sync();
  state.write();
  state.sync();
}

void GetThread(GetState& state)
{
  state.read();
  state.read();
  state.read();
  state.read();
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

  tp.run();

  Dout(dc::notice|flush_cf, "After: " << state);
}
