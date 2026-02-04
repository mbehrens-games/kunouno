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

/* palettes */
#define VDP_COLORS_PER_PAL  16

static unsigned short S_vdp_BG1_palette[VDP_COLORS_PER_PAL];
static unsigned short S_vdp_BG2_palette[VDP_COLORS_PER_PAL];
static unsigned short S_vdp_SP1_palette[VDP_COLORS_PER_PAL];
static unsigned short S_vdp_SP2_palette[VDP_COLORS_PER_PAL];

/* vram */
#define VDP_CELL_W_H        8
#define VDP_PIXELS_PER_CELL (VDP_CELL_W_H * VDP_CELL_W_H)
#define VDP_BYTES_PER_CELL  (VDP_PIXELS_PER_CELL / 2)

#define VDP_VRAM_SIZE (1 << 16) /* 64 K (2048 cells, 32 bytes each) */

#define VDP_MAX_CELLS (VDP_VRAM_SIZE / VDP_BYTES_PER_CELL) /* 2048 */

static unsigned char S_vdp_BG1_cells[VDP_VRAM_SIZE];
static unsigned char S_vdp_BG2_cells[VDP_VRAM_SIZE];
static unsigned char S_vdp_SP1_cells[VDP_VRAM_SIZE];
static unsigned char S_vdp_SP2_cells[VDP_VRAM_SIZE];

static unsigned short S_vdp_BG1_num_cells;
static unsigned short S_vdp_BG2_num_cells;
static unsigned short S_vdp_SP1_num_cells;
static unsigned short S_vdp_SP2_num_cells;

/* nametables */
#define VDP_NAME_ENTRY_SIZE 2
#define VDP_NAME_TABLE_SIZE (VDP_NAME_ENTRY_SIZE * VDP_MAX_CELLS)

static unsigned short S_vdp_BG1_tiles[VDP_NAME_TABLE_SIZE];
static unsigned short S_vdp_BG2_tiles[VDP_NAME_TABLE_SIZE];

static unsigned short S_vdp_SP1_sprites[VDP_NAME_TABLE_SIZE];
static unsigned short S_vdp_SP2_sprites[VDP_NAME_TABLE_SIZE];

static unsigned short S_vdp_BG1_num_tiles;
static unsigned short S_vdp_BG2_num_tiles;

static unsigned short S_vdp_SP1_num_sprites;
static unsigned short S_vdp_SP2_num_sprites;

/* registers */

/* add in later! */

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

  /* palettes */
  for (k = 0; k < VDP_COLORS_PER_PAL; k++)
  {
    S_vdp_BG1_palette[k] = 0x0000;
    S_vdp_BG2_palette[k] = 0x0000;
    S_vdp_SP1_palette[k] = 0x0000;
    S_vdp_SP2_palette[k] = 0x0000;
  }

  /* cells */
  for (k = 0; k < VDP_VRAM_SIZE; k++)
  {
    S_vdp_BG1_cells[k] = 0x00;
    S_vdp_BG2_cells[k] = 0x00;
    S_vdp_SP1_cells[k] = 0x00;
    S_vdp_SP2_cells[k] = 0x00;
  }

  S_vdp_BG1_num_cells = 0;
  S_vdp_BG2_num_cells = 0;
  S_vdp_SP1_num_cells = 0;
  S_vdp_SP2_num_cells = 0;

  /* nametables */
  for (k = 0; k < VDP_NAME_TABLE_SIZE; k++)
  {
    S_vdp_BG1_tiles[k] = 0x0000;
    S_vdp_BG2_tiles[k] = 0x0000;
  }

  S_vdp_BG1_num_tiles = 0;
  S_vdp_BG2_num_tiles = 0;

  for (k = 0; k < VDP_NAME_TABLE_SIZE; k++)
  {
    S_vdp_SP1_sprites[k] = 0x0000;
    S_vdp_SP2_sprites[k] = 0x0000;
  }

  S_vdp_SP1_num_sprites = 0;
  S_vdp_SP2_num_sprites = 0;

  /* layers */
  for (k = 0; k < VDP_LAYER_SIZE; k++)
  {
    S_vdp_BG1_layer[k] = 0x0000;
    S_vdp_BG2_layer[k] = 0x0000;
  }

  return 0;
}

/******************************************************************************/
/* vdp_load_sprite_file()                                                     */
/******************************************************************************/
int vdp_load_sprite_file(unsigned short file_number)
{
  /* lookup file */
  if (rom_lookup_file(ROM_FOLDER_SPRITES, file_number))
    return 1;

  /* palette */
  if (rom_read_words_from_file(&S_vdp_SP1_palette[0], VDP_COLORS_PER_PAL))
    return 1;

  /* uncompressed size, compressed size, then the nametable */
  if (rom_read_words_from_file(&S_vdp_SP1_num_sprites, 1))
    return 1;

  if (rom_read_words_from_file(&S_vdp_SP1_num_sprites, 1))
    return 1;

  if (rom_read_words_from_file(&S_vdp_SP1_sprites[0], S_vdp_SP1_num_sprites * VDP_NAME_ENTRY_SIZE))
    return 1;

  /* uncompressed size, compressed size, then the cells */
  if (rom_read_words_from_file(&S_vdp_SP1_num_cells, 1))
    return 1;

  if (rom_read_words_from_file(&S_vdp_SP1_num_cells, 1))
    return 1;

  if (rom_read_bytes_from_file(&S_vdp_SP1_cells[0], S_vdp_SP1_num_cells * VDP_BYTES_PER_CELL))
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

  unsigned short cell_addr;
  unsigned short cell_offset;

  unsigned long  pixel_addr;
  unsigned long  pixel_offset;

  /* clear framebuffer */
  for (m = 0; m < VDP_SCREEN_SIZE; m++)
    G_vdp_fb_rgb[m] = 0x0000;

  /* test: draw the test sprite */
  nametable_index = 0;

  val = S_vdp_SP1_sprites[nametable_index * VDP_NAME_ENTRY_SIZE + 0];

  num_columns = ((val >> 12) & 0x000F) + 1;
  num_rows = ((val >> 8) & 0x000F) + 1;
  num_frames = ((val >> 5) & 0x0007) + 1;

  val = S_vdp_SP1_sprites[nametable_index * VDP_NAME_ENTRY_SIZE + 1];

  cell_addr = (val & 0x07FF) * VDP_BYTES_PER_CELL;

  pal_addr = 0;
  
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
        pal_offset = (S_vdp_SP1_cells[cell_addr + cell_offset] >> 4) & 0x0F;
      else
        pal_offset = S_vdp_SP1_cells[cell_addr + cell_offset] & 0x0F;

      /* write pixel to the frame buffer */
      if (pal_offset == 0)
        continue;

      val = S_vdp_SP1_palette[pal_addr + pal_offset];

      G_vdp_fb_rgb[pixel_addr + pixel_offset] = val;
    }
  }

  return 0;
}

