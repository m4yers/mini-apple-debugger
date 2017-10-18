#ifndef MACHTASK_HPP_3NTPLSFZ
#define MACHTASK_HPP_3NTPLSFZ

#include <cstring>
#include <mach/mach.h>
#include <streambuf>
#include <unistd.h>

#include <experimental/optional>
#include <vector>

#include "MAD/Debug.hpp"
#include "MAD/MachThread.hpp"

template <typename T> using Optional = std::experimental::optional<T>;

namespace mad {
class MachTask {
  pid_t PID;
  mach_port_t Port;
  bool Suspended;

public:
  MachTask() : PID(0), Port(0), Suspended(false){};

  bool Attach(pid_t PID);
  bool Detach();

  bool IsAttached() { return Port; }

  mach_port_t GetPort() { return Port; }

  vm_size_t GetPageSize();
  vm_size_t ReadMemory(vm_address_t address, vm_size_t size, void *data);
  vm_size_t WriteMemory(vm_address_t address, vm_offset_t data,
                        mach_msg_type_number_t count);

  bool Suspend();
  bool Resume();

  Optional<std::vector<MachThread>> GetThreads(bool Suspend = false);
};

// TODO: Allow to choose buffer size, e.g. half-page-size, quater-page-size etc
// up to pointer size, because it is the minimum we can read from task memory
// TODO : Add ostream part
#define MT_INVALID_PAGE ~0
class MachTaskMemoryStreamBuf : public std::streambuf {
  MachTask &Task;
  // In-memory start of this stream
  vm_address_t Base;
  // In-memory address, i.e. Base + Position
  vm_address_t Address;
  // Virtual start of this stream, i.e. Address - Base
  vm_address_t Position;
  vm_size_t PageSize;
  vm_size_t PageMask;
  // In-memory page start address
  vm_size_t PageStart;
  std::vector<char_type> PageBuffer;

private:
  bool UpdateBufferIfNeeded() {
    // First read
    if (PageStart == MT_INVALID_PAGE ||
        // Address does not fall into the buffered page
        (Address < PageStart || Address >= PageStart + PageSize)) {
      PageStart = Address & PageMask;
      return Task.ReadMemory(PageStart, PageSize, PageBuffer.data()) ==
             PageSize;
    }
    return true;
  }

  void SetPosition(vm_address_t count) {
    Address = Base + count;
    Position = count;
  }

  void AdvancePosition(long count = 1) {
    Address += count;
    Position += count;
  }

public:
  MachTaskMemoryStreamBuf(MachTask &task, vm_address_t base)
      : Task(task), Base(base), Address(Base), Position(0),
        PageSize(Task.GetPageSize()), PageMask(~(PageSize - 1)),
        PageStart(MT_INVALID_PAGE), PageBuffer(PageSize, 0) {}

  //-----------------------------------------------------------------------------
  // Positioning
  //-----------------------------------------------------------------------------
  pos_type seekpos(pos_type pos,
                   std::ios_base::openmode which = std::ios_base::in |
                                                   std::ios_base::out) {
    SetPosition(pos);
    return Position;
  }
  pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                   std::ios_base::openmode which = std::ios_base::in |
                                                   std::ios_base::out) {
    switch (dir) {
    case std::ios_base::beg:
      SetPosition(off);
      break;
    case std::ios_base::end:
      pos_type(off_type(-1));
      break;
    case std::ios_base::cur:
      AdvancePosition(off);
      break;
    }
    return Position;
  }

  //-----------------------------------------------------------------------------
  // Get Area
  //-----------------------------------------------------------------------------
  int_type underflow() {
    if (!UpdateBufferIfNeeded()) {
      return traits_type::eof();
    }
    return traits_type::to_int_type(PageBuffer[Address % PageSize]);
  }

  int_type uflow() {
    AdvancePosition();
    if (!UpdateBufferIfNeeded()) {
      return traits_type::eof();
    }
    return traits_type::to_int_type(PageBuffer[Address % PageSize]);
  }

  std::streamsize xsgetn(char_type *s, std::streamsize count) {
    // TODO: Handle page boundries
    if (!UpdateBufferIfNeeded()) {
      return 0;
    }
    memcpy(s, PageBuffer.data() + (Address % PageSize), count);
    AdvancePosition(count);
    return count;
  }

  //-----------------------------------------------------------------------------
  // Putback
  //-----------------------------------------------------------------------------
  int_type pbackfail(int_type ch) {
    if (Address <= PageSize) {
      return traits_type::eof();
    }
    AdvancePosition(-1);
    if (!UpdateBufferIfNeeded() ||
        // We do not modify the buffer this way...
        (ch != traits_type::eof() && ch != PageBuffer[Address % PageSize])) {
      return traits_type::eof();
    }
    return traits_type::to_int_type(PageBuffer[Address % PageSize]);
  }
};
} // namespace mad

#endif /* end of include guard: MACHTASK_HPP_3NTPLSFZ */
