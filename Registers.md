/* ============================================================================
 *  Registers.md: PI registers.
 *
 *  ROMSIM: ROM device SIMulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#ifndef PI_REGISTER_LIST
#define PI_REGISTER_LIST \
  X(PI_DRAM_ADDR_REG) \
  X(PI_CART_ADDR_REG) \
  X(PI_RD_LEN_REG) \
  X(PI_WR_LEN_REG) \
  X(PI_STATUS_REG) \
  X(PI_BSD_DOM1_LAT_REG) \
  X(PI_BSD_DOM1_PWD_REG) \
  X(PI_BSD_DOM1_PGS_REG) \
  X(PI_BSD_DOM1_RLS_REG) \
  X(PI_BSD_DOM2_LAT_REG) \
  X(PI_BSD_DOM2_PWD_REG) \
  X(PI_BSD_DOM2_PGS_REG) \
  X(PI_BSD_DOM2_RLS_REG)
#endif

PI_REGISTER_LIST

