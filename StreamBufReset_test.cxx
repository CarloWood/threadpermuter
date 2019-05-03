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
  char* last_gptr;
  bool need_reset;

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
    next_egptr = nullptr;
    last_gptr = nullptr;
    need_reset = false;
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
    if (!read_need_reset() && pptr == read_last_gptr())
    {
      Dout(dc::notice, "pptr == last_gptr:");
      Dout(dc::notice, "  Setting need_reset to true.");
      set_need_reset(true);
      Dout(dc::notice, "  Setting next_egptr to block_start.");
      set_next_egptr(block_start);
      Dout(dc::notice, "  Setting pptr to block_start.");
      pptr = block_start;
      print();
    }
    else if (pptr == epptr)
    {
      Dout(dc::notice, "Returning because the buffer is full.");
      return;
    }
    Dout(dc::notice, "pptr == " << (int)(pptr - block_start));
    ASSERT(pptr == block_start || pptr == block_start + 32);
    pptr += 32;
    Dout(dc::notice, "Wrote 32 bytes, pptr is now " << (int)(pptr - block_start));
    print();
  }

  void flush()
  {
    DoutEntering(dc::notice, "PutState:flush()");
    Dout(dc::notice, "Copying pptr to next_egptr (" << (int)(pptr - block_start) << ").");
    set_next_egptr(pptr);
  }
};

struct GetState : public virtual Atomic
{
  char* gptr;
  char* egptr;

  GetState() : gptr(block_start), egptr(block_start) { }

  void read()
  {
    DoutEntering(dc::notice, "GetState:read()");
    Dout(dc::notice, "Copying next_egptr to egptr (" << (int)(get_next_egptr() - block_start) << ").");
    egptr = get_next_egptr();
    print();
    TPY;
    if (read_need_reset())
    {
      Dout(dc::notice, "need_reset is true:");
      Dout(dc::notice, "  Setting gptr to block_start.");
      gptr = block_start;
      Dout(dc::notice, "  Setting last_gptr to block_start.");
      set_last_gptr(block_start);
      Dout(dc::notice, "  Setting need_reset to false.");
      set_need_reset(false);
    }
    if (gptr == egptr)
    {
      Dout(dc::notice, "gptr == egptr (" << (int)(gptr - block_start) << ").");
      Dout(dc::notice, "  Setting last_gptr to gptr (" << (int)(gptr - block_start) << ").");
      set_last_gptr(gptr);
      return;
    }
    ASSERT(egptr - gptr >= 32);
    gptr += 32;
    Dout(dc::notice, "Read 32 bytes, gptr is now " << (int)(gptr - block_start));
    print();
  }
};

struct State : public PutState, public GetState
{
  State() { }

  void init()
  {
    gptr = egptr = pptr = block_start;
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
  std::string str1 = "                 ";
  std::string str2 = "================ ";
  int gptr_offset = (gptr - block_start) / 4;
  int egptr_offset = (egptr - block_start) / 4;
  int pptr_offset = (pptr - block_start) / 4;
  str1[gptr_offset] = 'g';
  str1[egptr_offset + (egptr_offset == gptr_offset ? 1 : 0)] = 'e';
  str2[pptr_offset] = 'p';
  if (get_next_egptr())
  {
    int next_egptr_offset = (get_next_egptr() - block_start) / 4;
    str2[next_egptr_offset] = pptr_offset == next_egptr_offset ? 'P' : 'n';
  }
  if (get_last_gptr())
  {
    int last_gptr_offset = (get_last_gptr() - block_start) / 4;
    str1[last_gptr_offset] = gptr_offset == last_gptr_offset ? 'G' : egptr_offset == last_gptr_offset ? 'E' : 'l';
  }
  if (get_need_reset())
    str2 += " *";
  Dout(dc::notice, str1);
  Dout(dc::notice, str2);
}

void PutThread(PutState& state)
{
  state.write();
  state.flush();
  state.write();
  state.flush();
  state.write();
  state.flush();
  state.write();
  state.flush();
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
