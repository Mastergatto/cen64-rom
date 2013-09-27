/* ============================================================================
 *  Controller.h: ROM controller.
 *
 *  ROMSIM: ROM device SIMulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#ifndef __ROM__CONTROLLER_H__
#define __ROM__CONTROLLER_H__
#include "Address.h"
#include "Cart.h"
#include "Common.h"

enum PIRegister {
#define X(reg) reg,
#include "Registers.md"
#undef X
  NUM_PI_REGISTERS
};

#ifndef NDEBUG
extern const char *PIRegisterMnemonics[NUM_PI_REGISTERS];
#endif

struct BusController;

struct ROMController {
  struct BusController *bus;
  struct Cart *cart;
  FILE *sramFile;

  uint32_t regs[NUM_PI_REGISTERS];
  uint8_t sram[32768];
};

struct ROMController *CreateROM(void);
void DestroyROM(struct ROMController *);

#endif

