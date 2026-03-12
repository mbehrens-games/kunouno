/******************************************************************************/
/* vdp.c (faux graphics chip)                                                 */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vdp.h"

#include "rom.h"

/* framebuffer */
unsigned short G_vdp_fb_rgb[VDP_SCREEN_SIZE];

/* nametable */
#define VDP_ENTRY_SIZE        5
#define VDP_MAX_ENTRIES       (1 << 12)
#define VDP_NAMETABLE_SIZE    (VDP_ENTRY_SIZE * VDP_MAX_ENTRIES)

static unsigned short S_vdp_nametable[VDP_NAMETABLE_SIZE];
static unsigned short S_vdp_num_entries;

/* palettes */
#define VDP_COLORS_PER_PAL    16

#define VDP_ROM_MAX_PALS      (1 << 8) /* 8 KB total size */ 
#define VDP_ROM_PALS_SIZE     (VDP_ROM_MAX_PALS * VDP_COLORS_PER_PAL)

static unsigned short S_vdp_pals[VDP_ROM_PALS_SIZE];
static unsigned short S_vdp_num_pals;

/* cells (vram) */
#define VDP_CELL_W_H          8
#define VDP_PIXELS_PER_CELL   (VDP_CELL_W_H * VDP_CELL_W_H)
#define VDP_BYTES_PER_CELL    (VDP_PIXELS_PER_CELL / 2)

#define VDP_ROM_MAX_CELLS     (1 << 16) /* 2 MB total size */
#define VDP_ROM_CELLS_SIZE    (VDP_ROM_MAX_CELLS * VDP_BYTES_PER_CELL)

#define VDP_CACHE_MAX_CELLS   (1 << 13) /* 256 KB total size */
#define VDP_CACHE_CELLS_SIZE  (VDP_CACHE_MAX_CELLS * VDP_BYTES_PER_CELL)

static unsigned char  S_vdp_cells[VDP_ROM_CELLS_SIZE];
static unsigned long  S_vdp_num_cells;

/* registers (add in later!) */

/* layers */
#define VDP_LAYER_W 512
#define VDP_LAYER_H 512

#define VDP_LAYER_SIZE (VDP_LAYER_W * VDP_LAYER_H)

static unsigned short S_vdp_BG1_layer[VDP_LAYER_SIZE];
static unsigned short S_vdp_BG2_layer[VDP_LAYER_SIZE];

/******************************************************************************/
/* vdp_reset()                                                                */
/******************************************************************************/
int vdp_reset()
{
  int k;

  /* framebuffer */
  for (k = 0; k < VDP_SCREEN_SIZE; k++)
    G_vdp_fb_rgb[k] = 0x0000;

  /* nametable */
  for (k = 0; k < VDP_NAMETABLE_SIZE; k++)
    S_vdp_nametable[k] = 0x0000;

  S_vdp_num_entries = 0;

  /* palettes */
  for (k = 0; k < VDP_ROM_PALS_SIZE; k++)
    S_vdp_pals[k] = 0x0000;

  S_vdp_num_pals = 0;

  /* cells */
  for (k = 0; k < VDP_ROM_CELLS_SIZE; k++)
    S_vdp_cells[k] = 0x00;

  S_vdp_num_cells = 0;

  /* layers */
  for (k = 0; k < VDP_LAYER_SIZE; k++)
  {
    S_vdp_BG1_layer[k] = 0x0000;
    S_vdp_BG2_layer[k] = 0x0000;
  }

  return 0;
}

/******************************************************************************/
/* vdp_load_rom_chunks()                                                      */
/******************************************************************************/
int vdp_load_rom_chunks()
{
  if (rom_read_chunk_words(0, S_vdp_nametable, VDP_NAMETABLE_SIZE))
    return 1;

  if (rom_read_chunk_words(1, S_vdp_pals, VDP_ROM_PALS_SIZE))
    return 1;

  if (rom_read_chunk_bytes(2, S_vdp_cells, VDP_ROM_CELLS_SIZE))
    return 1;

  return 0;
}

/******************************************************************************/
/* vdp_draw_frame()                                                           */
/******************************************************************************/
int vdp_draw_frame()
{
  int m;
  int n;

  unsigned short val;

  unsigned short nametable_index;

  unsigned short num_rows;
  unsigned short num_columns;
  unsigned short num_frames;
  unsigned short delay_time;

  unsigned short thing_pos_x;
  unsigned short thing_pos_y;

  unsigned short pal_addr;
  unsigned short pal_offset;

  unsigned long  cell_addr;
  unsigned long  cell_offset;

  unsigned long  pixel_addr;
  unsigned long  pixel_offset;

  /* clear framebuffer */
  for (m = 0; m < VDP_SCREEN_SIZE; m++)
    G_vdp_fb_rgb[m] = 0x0000;

  /* test: draw the test sprite */
  nametable_index = 0;

  val = S_vdp_nametable[VDP_ENTRY_SIZE * nametable_index + 0];

  num_columns = ((val >> 13) & 0x0003) + 1;
  num_rows = ((val >> 11) & 0x0003) + 1;
  num_frames = ((val >> 8) & 0x0007) + 1;

  val = S_vdp_nametable[VDP_ENTRY_SIZE * nametable_index + 1];

  pal_addr = (val & 0x00FF) * VDP_COLORS_PER_PAL;

  val = S_vdp_nametable[VDP_ENTRY_SIZE * nametable_index + 2];

  cell_addr = (val << 16) & 0x3F0000;

  val = S_vdp_nametable[VDP_ENTRY_SIZE * nametable_index + 3];

  cell_addr |= val & 0x00FFFF;

  cell_addr *= VDP_BYTES_PER_CELL;

  val = S_vdp_nametable[VDP_ENTRY_SIZE * nametable_index + 4];

  thing_pos_x = 128;
  thing_pos_y = 64;

  for (m = 0; m < num_rows * num_columns; m++)
  {
    /* determine pixel address */
    pixel_addr = VDP_SCREEN_W * thing_pos_y + thing_pos_x;
    pixel_addr += VDP_SCREEN_W * VDP_CELL_W_H * (m / num_columns);
    pixel_addr += VDP_CELL_W_H * (m % num_columns);

    for (n = 0; n < VDP_PIXELS_PER_CELL; n++)
    {
      /* determine cell & pixel offsets */
      cell_offset = (VDP_BYTES_PER_CELL * m) + (n / 2);

      pixel_offset = VDP_SCREEN_W * (n / VDP_CELL_W_H);
      pixel_offset += n % VDP_CELL_W_H;

      /* read palette offset from cell */
      if (n % 2 == 0)
        pal_offset = (S_vdp_cells[cell_addr + cell_offset] >> 4) & 0x0F;
      else
        pal_offset = S_vdp_cells[cell_addr + cell_offset] & 0x0F;

      /* write pixel to the frame buffer */
      if (pal_offset == 0)
        continue;

      val = S_vdp_pals[pal_addr + pal_offset];

      G_vdp_fb_rgb[pixel_addr + pixel_offset] = val;
    }
  }

  return 0;
}

