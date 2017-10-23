#include <mach-o/fat.h>

#include "MAD/UniversalBinary.hpp"
#include "MAD/Error.hpp"

using namespace mad;

bool UniversalBinary::Parse() {
  fat_header header;
  uint64_t fatptr = 0;
  Input.seekg(fatptr);
  Input.read((char *)&header, sizeof(fat_header));

  if (header.magic != FAT_MAGIC && header.magic != FAT_CIGAM) {
    Error Err(MAD_ERROR_PARSER);
    Err.Log("Not a FAT binary", Path);
    return false;
  }

  fatptr += sizeof(fat_header);
  PRINT_DEBUG("FOUND ", header.nfat_arch, " ARCHS");

  for (uint32_t i = 0; i < header.nfat_arch; ++i) {
    fat_arch arch;
    Input.seekg(fatptr);
    Input.read((char *)&arch, sizeof(fat_arch));
    fatptr += sizeof(fat_arch);

    PRINT_DEBUG("FOUND CPUTYPE: ", arch.cputype);

    // Remove capability bits
    arch.cpusubtype &= ~CPU_SUBTYPE_MASK;

    Input.seekg(arch.offset);
    ObjectFile object(Path, Input);
    object.Parse();
    ObjectFiles.insert({arch.cputype, std::move(object)});
  }

  return true;
}
