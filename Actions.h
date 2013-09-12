/* ============================================================================
 *  Action.h: ROM controller functionality.
 *
 *  ROMSIM: ROM device SIMulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#ifndef __ROM__ACTION_H__
#define __ROM__ACTION_H__
#include "Common.h"
#include "Controller.h"

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

void DMAToDRAM(struct BusController *, uint32_t, const void *, size_t);

void PIHandleDMARead(struct ROMController *);
void PIHandleDMAWrite(struct ROMController *);
void PIHandleStatusWrite(struct ROMController *);

#endif

