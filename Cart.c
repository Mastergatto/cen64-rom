/* ============================================================================
 *  Cart.c: Cartridge interface.
 *
 *  ROMSIM: ROM device SIMulator.
 *  Copyright (C) 2013, Tyler J. Stachecki.
 *  All rights reserved.
 *
 *  This file is subject to the terms and conditions defined in
 *  file 'LICENSE', which is part of this source code package.
 * ========================================================================= */
#include "Address.h"
#include "Cart.h"
#include "Controller.h"

#ifdef __cplusplus
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#else
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifdef MMAP_ROM_IMAGE
#include <sys/mman.h>
#endif

#define CRC_CIC_NUS_6101 0x6170A4A1
#define CRC_CIC_NUS_6102 0x90BB6CB5
#define CRC_CIC_NUS_6103 0x0B050EE0
#define CRC_CIC_NUS_6105 0x98BC2C86
#define CRC_CIC_NUS_6106 0xACC8580A

enum CICSEED {
  SEED_CIC_NUS_6101 = 0x00063F3F,
  SEED_CIC_NUS_6102 = 0x00023F3F,
  SEED_CIC_NUS_6103 = 0x0002783F,
  SEED_CIC_NUS_6105 = 0x0002913F,
  SEED_CIC_NUS_6106 = 0x0002853F
};

static uint32_t CRC32(const uint8_t *, size_t);
static void InitCart(struct Cart *, FILE *, const uint8_t *, size_t);

/* ============================================================================
 *  CartRead: Read from Cart.
 * ========================================================================= */
int
CartRead(void *_controller, uint32_t address, void *_data) {
	struct Cart *cart = ((struct ROMController*) _controller)->cart;
	uint32_t *data = (uint32_t*) _data;
  uint32_t word;

  address = address - ROM_CART_BASE_ADDRESS;

  memcpy(&word, cart->rom + address, sizeof(word));
  *data = ByteOrderSwap32(word);

  return 0;
}

/* ============================================================================
 *  CartWrite: Write to Cart.
 * ========================================================================= */
int
CartWrite(void *unused(controller),
  uint32_t debugonly(address), void *unused(data)) {
  debugarg("CartWrite: Detected write [0x%.8x]", address);

  return 0;
}

/* ============================================================================
 *  Run the reference implementation of CRC32.
 * ========================================================================= */
static uint32_t CRC32(const uint8_t *data, size_t size) {
  uint32_t table[256];
  unsigned n, k;
  uint32_t c;

  for (n = 0; n < 256; n++) {
    c = (uint32_t) n;

    for (k = 0; k < 8; k++) {
      if (c & 1)
        c = 0xEDB88320L ^ (c >> 1);
      else
        c = c >> 1;
    }

    table[n] = c;
  }

  c = 0L ^ 0xFFFFFFFF;

  for (n = 0; n < size; n++)
    c = table[(c ^ data[n]) & 0xFF] ^ (c >> 8);

  return c ^ 0xFFFFFFFF;
}

/* ============================================================================
 *  CreateCart: Creates a new Cart.
 * ========================================================================= */
struct Cart *
CreateCart(const char *filename) {
  size_t allocSize = sizeof(struct Cart);

  struct Cart *cart;
  uint8_t *romImage;
  FILE *romFile;
  long romSize;

  if ((romFile = fopen(filename, "r")) == NULL) {
    debug("Failed to open ROM image.");
    return NULL;
  }

  if (fseek(romFile, 0, SEEK_END) == -1 || (romSize = ftell(romFile)) == -1) {
    debug("Failed to determine ROM size.");

    fclose(romFile);
    return NULL;
  }

#ifndef MMAP_ROM_IMAGE
  allocSize += romSize;
#endif

  /* Allocate memory for cart metadata and image. */
  if ((cart = (struct Cart*) malloc(allocSize)) == NULL) {
    debug("Failed to allocate memory for ROM.");

    fclose(romFile);
    return NULL;
  }

#ifndef MMAP_ROM_IMAGE
  romImage = (uint8_t*) cart + sizeof(*cart);

  rewind(romFile);
  if (fread(romImage, romSize, 1, romFile) != 1) {
#else
  int fd = fileno(romFile);

  /* Map the file directly into memory. */
  if ((romImage = (uint8_t*) mmap(NULL, romSize,
    PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
#endif

    debug("Failed to load ROM image.");

    free(cart);
    cart = NULL;
  }

  if (cart != NULL)
    InitCart(cart, romFile, romImage, romSize);

  fclose(romFile);
  return cart;
}

/* ============================================================================
 *  DestroyCart: Deallocates memory reserved for a Cart. 
 * ========================================================================= */
void
DestroyCart(struct Cart *cart) {
#ifdef MMAP_ROM_IMAGE
  munmap((void*) cart->rom, cart->size);
#endif

  free(cart);
}

/* ============================================================================
 *  GetCICSeed: Returns the proper CIC seed value depending on the cart header.
 * ========================================================================= */
uint32_t
GetCICSeed(const struct ROMController *controller) {
  uint32_t crc = CRC32(controller->cart->rom + 0x40, 4096 - 0x40);

  switch(crc) {
    case CRC_CIC_NUS_6101:
      debug("Detected: CIC-NUS-6101.");
      return (uint32_t) SEED_CIC_NUS_6101;

    case CRC_CIC_NUS_6102:
      debug("Detected: CIC-NUS-6102.");
      return (uint32_t) SEED_CIC_NUS_6102;

    case CRC_CIC_NUS_6103:
      debug("Detected: CIC-NUS-6103.");
      return (uint32_t) SEED_CIC_NUS_6103;

    case CRC_CIC_NUS_6105:
      debug("Detected: CIC-NUS-6105.");
      return (uint32_t) SEED_CIC_NUS_6105;

    case CRC_CIC_NUS_6106:
      debug("Detected: CIC-NUS-6106.");
      return (uint32_t) SEED_CIC_NUS_6106;

    default:
      debugarg("Unknown CIC/CRC [0x%.8x]", crc);
  }

  return 0;
}

/* ============================================================================
 *  InitCart: Initializes the Cart.
 * ========================================================================= */
void
GetROMTitle(const struct ROMController *controller, ROMTitle title) {
  memcpy(title, controller->cart->rom + 0x20, 20);
  title[20] = '\0';
}

/* ============================================================================
 *  InitCart: Initializes the Cart.
 * ========================================================================= */
static void
InitCart(struct Cart *cart, FILE *file, const uint8_t *rom, size_t size) {
  debug("Preparing the image.");
  memset(cart, 0, sizeof(*cart));

  cart->file = file;
  cart->rom = rom;
  cart->size = size;
}

