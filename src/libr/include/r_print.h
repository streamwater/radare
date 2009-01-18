#ifndef _INCLUDE_PRINT_R_
#define _INCLUDE_PRINT_R_

#include "r_types.h"

#define R_PRINT_FLAGS_COLOR   0x00000001
#define R_PRINT_FLAGS_ADDRMOD 0x00000002

void r_print_set_flags(int flags);
void r_print_addr(u64 addr);
void r_print_hexdump(u64 addr, u8 *buf, int len, int step, int columns, int header);

#endif
