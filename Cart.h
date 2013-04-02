/* ============================================================================
 *  Cart.h: Cartridge interface.
 *
 *  ROMSIM: ROM device SIMulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#ifndef __ROM__CART_H__
#define __ROM__CART_H__
#include "Common.h"
#include <stdio.h>

struct Cart {
  FILE *file;
  const uint8_t *rom;
  unsigned size;
};

struct ROMController;
typedef char ROMTitle[32];

struct Cart *CreateCart(const char *);
void DestroyCart(struct Cart *);

uint32_t GetCICSeed(const struct ROMController *);
void GetROMTitle(const struct ROMController *, ROMTitle );

#endif

