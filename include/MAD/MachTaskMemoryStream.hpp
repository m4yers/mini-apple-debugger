#ifndef MACHTASKMEMORYSTREAM_HPP_8CBVRMN6
#define MACHTASKMEMORYSTREAM_HPP_8CBVRMN6

// System
#include <mach/mach.h>

// Std
#include <streambuf>

// MAD
#include <MAD/MachMemory.hpp>
#include <MAD/MachTask.hpp>

using namespace mad;

#define MTS_INVALID_PAGE ~0u

// TODO: Allow to choose buffer size, e.g. half-page-size, quater-page-size etc
// up to pointer size, because it is the minimum we can read from Task memory
// TODO : Add ostream part
class MachTaskMemoryStreamBuf : public std::streambuf {
  MachMemory &Memory;
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
    if (PageStart == MTS_INVALID_PAGE ||
        // Address does not fall into the buffered page
        (Address < PageStart || Address >= PageStart + PageSize)) {
      PageStart = Address & PageMask;
      return Memory.Read(PageStart, PageSize, PageBuffer.data()) == PageSize;
    }
    return true;
  }

  void SetPosition(vm_address_t Count) {
    Address = Base + Count;
    Position = Count;
  }

  void AdvancePosition(long Count = 1) {
    Address += Count;
    Position += Count;
  }

public:
  MachTaskMemoryStreamBuf(MachMemory &Memory, vm_address_t Base)
      : Memory(Memory), Base(Base), Address(Base), Position(0),
        PageSize(Memory.GetPageSize()), PageMask(~(PageSize - 1)),
        PageStart(MTS_INVALID_PAGE), PageBuffer(PageSize, 0) {}

  //----------------------------------------------------------------------------
  // Positioning
  //----------------------------------------------------------------------------
  pos_type seekpos(pos_type pos,
                   std::ios_base::openmode which = std::ios_base::in |
                                                   std::ios_base::out) {
    if (which & std::ios_base::in) {
      SetPosition(pos);
    }
    return Position;
  }
  pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                   std::ios_base::openmode which = std::ios_base::in |
                                                   std::ios_base::out) {
    if (which & std::ios_base::in) {
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
    }
    return Position;
  }

  //----------------------------------------------------------------------------
  // Get Area
  //----------------------------------------------------------------------------
  int_type underflow() {
    if (!UpdateBufferIfNeeded()) {
      return traits_type::eof();
    }
    return traits_type::to_int_type(PageBuffer[Address % PageSize]);
  }

  int_type uflow() {
    if (!UpdateBufferIfNeeded()) {
      return traits_type::eof();
    }
    auto Result = traits_type::to_int_type(PageBuffer[Address % PageSize]);
    AdvancePosition();
    return Result;
  }

  std::streamsize xsgetn(char_type *Out, std::streamsize Count) {
    if (!UpdateBufferIfNeeded()) {
      return 0;
    }

    auto Written = 0;

    // If the requested data sits on 2+ pages we handle this explicitly
    auto LastBytePageStart = (Address + Count - 1) & PageMask;
    if (LastBytePageStart != PageStart) {
      // Write what'Out left on the current page
      auto LeftHere = PageSize - (Address % PageSize);
      memcpy(Out, PageBuffer.data() + (Address % PageSize), LeftHere);
      AdvancePosition(LeftHere);
      Written += LeftHere;
      Count -= LeftHere;

      // If the read range occupies more than two pages we just read middle
      // pages sequentially.
      auto MorePages = (LastBytePageStart - PageStart) / PageSize;
      while (MorePages != 1) {
        if (!UpdateBufferIfNeeded()) {
          return Written;
        }
        memcpy(Out, PageBuffer.data(), PageSize);
        AdvancePosition(PageSize);
        Written += PageSize;
        Count -= PageSize;
        MorePages--;
      }

      if (!Count) {
        return Written;
      }
    }

    if (!UpdateBufferIfNeeded()) {
      return Written;
    }

    memcpy(Out, PageBuffer.data() + (Address % PageSize), Count);
    AdvancePosition(Count);
    Written += Count;

    return Written;
  }

  //----------------------------------------------------------------------------
  // Putback
  //----------------------------------------------------------------------------
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

class MachTaskMemoryStream : public std::iostream {
public:
  MachTaskMemoryStreamBuf Buffer;

public:
  MachTaskMemoryStream(MachMemory &Memory, vm_address_t Address)
      : std::iostream(&Buffer), Buffer(Memory, Address) {}
};

#endif /* end of include guard: MACHTASKMEMORYSTREAM_HPP_8CBVRMN6 */
