/* ============================================================================
 *  Action.c: ROM controller functionality.
 *
 *  ROMSIM: ROM device SIMulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#include "Actions.h"
#include "Cart.h"
#include "Common.h"
#include "Controller.h"
#include "Definitions.h"
#include "Externs.h"

#ifdef __cplusplus
#include <cassert>
#include <cstring>
#else
#include <assert.h>
#include <string.h>
#endif

/* ============================================================================
 *  PIHandleDMARead: Invoked when PI_RD_LEN_REG is written.
 *
 *  PI_CART_ADDR_REG = Cart (source) address.
 *  PI_DRAM_ADDR_REG = DRAM (target) address.
 *  PI_WR_LEN_REG = Transfer size.
 * ========================================================================= */
void PIHandleDMARead(struct ROMController *controller) {
  uint32_t dest = controller->regs[PI_CART_ADDR_REG] & 0xFFFFFFF;
  uint32_t source = controller->regs[PI_DRAM_ADDR_REG] & 0x7FFFFF;
  uint32_t length = (controller->regs[PI_RD_LEN_REG] & 0xFFFFFF) + 1;

  if (length & 7)
    length = (length + 7) & ~7;

  if (dest & 0x08000000) {
    debug("DMA | Request: Write to SRAM.");
    dest &= 0x7FFF;

    if (dest + length > sizeof(controller->sram)) {
      length = sizeof(controller->sram) - dest;

      debug("DMA | Copy would overflow SRAM bounds; trimming.");
    }

    debugarg("DMA | DEST   : [0x%.8x].", dest);
    debugarg("DMA | SOURCE : [0x%.8x].", source);
    debugarg("DMA | LENGTH : [0x%.8x].", length);

    DMAFromDRAM(controller->bus, controller->sram + dest, source, length);
  }

  else if (!(dest & 0x06000000)) {
    debug("DMA | Request: Write to cart; ignoring.");
  }

  controller->regs[PI_DRAM_ADDR_REG] += length;
  controller->regs[PI_CART_ADDR_REG] += length;
  controller->regs[PI_STATUS_REG] &= ~0x1;
  controller->regs[PI_STATUS_REG] |= 0x8;

  BusRaiseRCPInterrupt(controller->bus, MI_INTR_PI);
}

/* ============================================================================
 *  PIHandleDMAWrite: Invoked when PI_WR_LEN_REG is written.
 *
 *  PI_CART_ADDR_REG = Cart (source) address.
 *  PI_DRAM_ADDR_REG = DRAM (target) address.
 *  PI_WR_LEN_REG = Transfer size.
 * ========================================================================= */
void PIHandleDMAWrite(struct ROMController *controller) {
  uint32_t dest = controller->regs[PI_DRAM_ADDR_REG] & 0x7FFFFF;
  uint32_t source = controller->regs[PI_CART_ADDR_REG] & 0xFFFFFFF;
  uint32_t length = (controller->regs[PI_WR_LEN_REG] & 0xFFFFFF) + 1;

  if (length & 7)
    length = (length + 7) & ~7;

  if (source & 0x08000000) {
    debug("DMA | Request: Read from SRAM.");
    source &= 0x7FFF;

    if (source + length > sizeof(controller->sram)) {
      length = sizeof(controller->sram) - source;

      debug("DMA | Copy would overflow SRAM bounds; trimming.");
    }

    debugarg("DMA | DEST   : [0x%.8x].", dest);
    debugarg("DMA | SOURCE : [0x%.8x].", source);
    debugarg("DMA | LENGTH : [0x%.8x].", length);

    DMAToDRAM(controller->bus, dest, controller->sram + source, length);
  }

  else if (!(source & 0x06000000)) {
    debug("DMA | Request: Read from cart.");

    if (source + length > controller->cart->size) {
      length = controller->cart->size - source;

      debug("DMA | Copy would overflow cart bounds; trimming.");
    }

    debugarg("DMA | DEST   : [0x%.8x].", dest);
    debugarg("DMA | SOURCE : [0x%.8x].", source);
    debugarg("DMA | LENGTH : [0x%.8x].", length);

    DMAToDRAM(controller->bus, dest, controller->cart->rom + source, length);
  }

  controller->regs[PI_DRAM_ADDR_REG] += length;
  controller->regs[PI_CART_ADDR_REG] += length;
  controller->regs[PI_STATUS_REG] &= ~0x1;
  controller->regs[PI_STATUS_REG] |= 0x8;

  BusRaiseRCPInterrupt(controller->bus, MI_INTR_PI);
}

/* ============================================================================
 *  PIHandleStatusWrite: Invoked when PI_STATUS_REG is written.
 *
 *  [0]: Reset controller.
 *  [1]: Clear interrupt.
 * ========================================================================= */
void PIHandleStatusWrite(struct ROMController *controller) {
  uint32_t status = controller->regs[PI_STATUS_REG];
  bool resetController = status & 1;
  bool clearInterrupt = status & 2;

  if (resetController)
    controller->regs[PI_STATUS_REG] = 0;

  if (clearInterrupt) {
    BusClearRCPInterrupt(controller->bus, MI_INTR_PI);
    controller->regs[PI_STATUS_REG] &= ~0x8;
  }
}

/* ============================================================================
 *  ReadSRAMFile: Reads the contents SRAM file into the controller.
 * ========================================================================= */
int
ReadSRAMFile(struct ROMController *controller) {
  size_t cur = 0;

  if (!controller->sramFile)
    return -1;

  rewind(controller->sramFile);

  while (cur < sizeof(controller->sram)) {
    size_t remaining = sizeof(controller->sram) - cur;
    size_t ret;

    if ((ret = fread(controller->sram + cur, 1,
      remaining, controller->sramFile)) == 0 &&
      ferror(controller->sramFile))
      return -1;

    /* Ignore invalid sized files. */
    if (feof(controller->sramFile)) {
      memset(controller->sram, 0, sizeof(controller->sram));
      printf("SRAM: Ignoring short SRAM file.\n");
      return 0;
    }

    cur += ret;
  }

  return 0;
}

/* ============================================================================
 *  SetSRAMFile: Sets the backing file for SRAM saves.
 * ========================================================================= */
void
SetSRAMFile(struct ROMController *controller, const char *filename) {
  if (controller->sramFile != NULL)
    fclose(controller->sramFile);

  /* Try opening with rb+ first, then wb+ iff we fail. */
  if ((controller->sramFile = fopen(filename, "rb+")) == NULL) {
    controller->sramFile = fopen(filename, "wb+");
    return;
  }

  ReadSRAMFile(controller);
}

/* ============================================================================
 *  WriteSRAMFile: Dumps the contents of the SRAM to the backing file.
 * ========================================================================= */
int
WriteSRAMFile(struct ROMController *controller) {
  size_t cur = 0;

  if (!controller->sramFile)
    return -1;

  rewind(controller->sramFile);

  while (cur < sizeof(controller->sram)) {
    size_t remaining = sizeof(controller->sram) - cur;
    size_t ret;

    if ((ret = fwrite(controller->sram + cur, 1,
      remaining, controller->sramFile)) == 0 &&
      ferror(controller->sramFile))
      return -1;

    cur += ret;
  }

  return 0;
}

