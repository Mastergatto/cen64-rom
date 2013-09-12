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
  uint32_t length = (controller->regs[PI_WR_LEN_REG] & 0xFFFFFF) + 1;

  if (length & 7)
    length = (length + 7) & ~7;

  if (dest & 0x08000000) {
    debug("DMA | Request: Write to SRAM.");
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
  }

  else if (!(source & 0x06000000)) {
    debug("DMA | Request: Read from cart.");

    if (source + length > controller->cart->size) {
      debug("DMA | Copy would overflow cart bounds; ignoring.");
      return;
    }

    /* TODO: Handle out-of-bounds cart reads properly. */
    assert((source + length) <= controller->cart->size);

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

