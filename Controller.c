/* ============================================================================
 *  Controller.c: ROM controller.
 *
 *  ROMSIM: ROM device SIMulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#include "Address.h"
#include "Actions.h"
#include "Cart.h"
#include "Common.h"
#include "Controller.h"

#ifdef __cplusplus
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

/* ============================================================================
 *  Mnemonics table.
 * ========================================================================= */
#ifndef NDEBUG
const char *PIRegisterMnemonics[NUM_PI_REGISTERS] = {
#define X(reg) #reg,
#include "Registers.md"
#undef X
};
#endif

static void InitROM(struct ROMController *);

/* ============================================================================
 *  ConnectROMToBus: Connects a ROM instance to a Bus instance.
 * ========================================================================= */
void
ConnectROMToBus(struct ROMController *rom, struct BusController *bus) {
  rom->bus = bus;
}

/* ============================================================================
 *  CreateROM: Creates and initializes an ROM instance.
 * ========================================================================= */
struct ROMController *
CreateROM(void) {
  size_t allocSize = sizeof(struct ROMController);
  struct ROMController *controller;

  if ((controller = (struct ROMController*) malloc(allocSize)) == NULL) {
    debug("Failed to allocate memory.");
    return NULL;
  }

  InitROM(controller);
  return controller;
}

/* ============================================================================
 *  DestroyROM: Releases any resources allocated for an ROM instance.
 * ========================================================================= */
void
DestroyROM(struct ROMController *controller) {
  if (controller->sramFile) {
    if (WriteSRAMFile(controller))
      printf("Failed to write the SRAM file.\n");
  }

  if (controller->cart)
    DestroyCart(controller->cart);

  free(controller);
}

/* ============================================================================
 *  InitROM: Initializes the ROM controller.
 * ========================================================================= */
static void
InitROM(struct ROMController *controller) {
  debug("Initializing Interface.");
  memset(controller, 0, sizeof(*controller));
}

/* ============================================================================
 *  InsertCart: Associates a cart with the controller.
 * ========================================================================= */
int
InsertCart(struct ROMController *controller, const char *filename) {
  ROMTitle debugonly(title);

  if (controller->cart != NULL)
    DestroyCart(controller->cart);

  if ((controller->cart = CreateCart(filename)) == NULL)
    return 1;

#ifndef NDEBUG
  GetROMTitle(controller, title);
  debugarg("Loaded: [%s]", title);
#endif

  return 0;
}

/* ============================================================================
 *  PIRegRead: Read from PI registers.
 * ========================================================================= */
int
PIRegRead(void *_pif, uint32_t address, void *_data) {
	struct ROMController *controller = (struct ROMController*) _pif;
	uint32_t *data = (uint32_t*) _data;

  address -= PI_REGS_BASE_ADDRESS;
  enum PIRegister reg = (enum PIRegister) (address / 4);

  debugarg("PIRegRead: Reading from register [%s].", PIRegisterMnemonics[reg]);

  if (reg == PI_STATUS_REG)
    *data = 0;
  else
    *data = controller->regs[reg];

  return 0;
}

/* ============================================================================
 *  PIRegWrite: Write to PI registers.
 * ========================================================================= */
int
PIRegWrite(void *_pif, uint32_t address, void *_data) {
	struct ROMController *controller = (struct ROMController*) _pif;
	uint32_t *data = (uint32_t*) _data;

  address -= PI_REGS_BASE_ADDRESS;
  enum PIRegister reg = (enum PIRegister) (address / 4);

  debugarg("PIRegWrite: Writing to register [%s].", PIRegisterMnemonics[reg]);

  controller->regs[reg] = *data;

  /* Action? */
  switch(reg) {
    case PI_STATUS_REG:
      PIHandleStatusWrite(controller);
      break;

    case PI_RD_LEN_REG:
      PIHandleDMARead(controller);
      break;

    case PI_WR_LEN_REG:
      PIHandleDMAWrite(controller);
      break;

    default:
      break;
  }

  return 0;
}

