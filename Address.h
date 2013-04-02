/* ============================================================================
 *  Address.h: Device address list.
 *
 *  ROMSIM: ROM device SIMulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#ifndef __ROM__ADDRESS_H__
#define __ROM__ADDRESS_H__

/* Parallel Interface Registers. */
#define PI_REGS_BASE_ADDRESS      0x04600000
#define PI_REGS_ADDRESS_LEN       0x00000034

/* ROM Cartridge Interface. */
#define ROM_CART_BASE_ADDRESS     0x10000000
#define ROM_CART_ADDRESS_LEN      0x0FC00000

#endif

