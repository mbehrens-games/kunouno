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
#define VDP_NUM_PALS        16
#define VDP_PALS_SIZE       (VDP_COLORS_PER_PAL * VDP_NUM_PALS)

static unsigned short S_vdp_pals[VDP_PALS_SIZE];
static short          S_vdp_pal_count;

/* cells */
#define VDP_CELL_W_H        8
#define VDP_PIXELS_PER_CELL (VDP_CELL_W_H * VDP_CELL_W_H)  /* 8x8 */
#define VDP_BYTES_PER_CELL  (VDP_PIXELS_PER_CELL / 2)

#define VDP_NUM_CELLS       (4 * 1024)
#define VDP_CELLS_SIZE      (VDP_BYTES_PER_CELL * VDP_NUM_CELLS)  /* 128 KB */

static unsigned char  S_vdp_cells[VDP_CELLS_SIZE];
static short          S_vdp_cell_count;

/* table entries */
#define VDP_VALS_PER_SPRITE 3
#define VDP_NUM_SPRITES     1024
#define VDP_SPRITES_SIZE    (VDP_VALS_PER_SPRITE * VDP_NUM_SPRITES)

static unsigned short S_vdp_sprites[VDP_SPRITES_SIZE];
static short          S_vdp_sprite_count;

/* tiles */
#define VDP_VALS_PER_TILE   3
#define VDP_NUM_TILES       1024
#define VDP_TILES_SIZE      (VDP_VALS_PER_TILE * VDP_NUM_TILES)

static unsigned short S_vdp_tiles[VDP_TILES_SIZE];
static short          S_vdp_tile_count;

/* layers */
#define VDP_LAYER_W 512
#define VDP_LAYER_H 512

#define VDP_LAYER_SIZE (VDP_LAYER_W * VDP_LAYER_H)

static unsigned short S_vdp_layer_sky[VDP_LAYER_SIZE];
static unsigned short S_vdp_layer_bg[VDP_LAYER_SIZE];

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
  for (k = 0; k < VDP_PALS_SIZE; k++)
    S_vdp_pals[k] = 0x0000;

  S_vdp_pal_count = 0;

  /* cells */
  for (k = 0; k < VDP_CELLS_SIZE; k++)
    S_vdp_cells[k] = 0x00;

  S_vdp_cell_count = 0;

  /* sprites */
  for (k = 0; k < VDP_SPRITES_SIZE; k++)
    S_vdp_sprites[k] = 0x00;

  S_vdp_sprite_count = 0;

  /* tiles */
  for (k = 0; k < VDP_TILES_SIZE; k++)
    S_vdp_tiles[k] = 0x00;

  S_vdp_tile_count = 0;

  /* layers */
  for (k = 0; k < VDP_LAYER_SIZE; k++)
  {
    S_vdp_layer_sky[k] = 0x0000;
    S_vdp_layer_bg[k] = 0x0000;
  }

  return 0;
}

/******************************************************************************/
/* vdp_load_sprite()                                                          */
/******************************************************************************/
int vdp_load_sprite(unsigned short sprite_number)
{
  int k;

  unsigned short val;

  unsigned long  rom_addr;
  unsigned long  num_bytes;

  unsigned short num_rows;
  unsigned short num_columns;
  unsigned short num_frames;
  unsigned short delay_time;

  unsigned short num_cells;

  unsigned long  pal_addr;
  unsigned long  cell_addr;
  unsigned long  sprite_addr;

  unsigned short pal_colors[VDP_COLORS_PER_PAL];

  /* get the file address and size in the rom */
  rom_lookup_file(ROM_FOLDER_SPRITES, sprite_number, &rom_addr, &num_bytes);

  /* read sprite header */
  num_columns = ((G_rom_data[rom_addr + 0] >> 4) & 0x0F) + 1;
  num_rows = (G_rom_data[rom_addr + 0] & 0x0F) + 1;
  num_frames = ((G_rom_data[rom_addr + 1] >> 4) & 0x0F) + 1;

  delay_time = (G_rom_data[rom_addr + 2] << 8) & 0xFF00;
  delay_time |= G_rom_data[rom_addr + 3] & 0x00FF;
  delay_time *= 10;

  rom_addr += 4;
  num_bytes -= 4;

  /* read palette */
  for (k = 0; k < VDP_COLORS_PER_PAL; k++)
  {
    pal_colors[k] = (G_rom_data[rom_addr + (2 * k + 0)] << 8) & 0x7F00;
    pal_colors[k] |= G_rom_data[rom_addr + (2 * k + 1)] & 0x00FF;
  }

  rom_addr += 2 * VDP_COLORS_PER_PAL;
  num_bytes -= 2 * VDP_COLORS_PER_PAL;

  /* copy palette */
  if ((S_vdp_pal_count + 1) >= VDP_NUM_PALS)
    return 1;

  pal_addr = S_vdp_pal_count * VDP_COLORS_PER_PAL;

  memcpy( &S_vdp_pals[pal_addr], &pal_colors[0], 
          sizeof(unsigned short) * VDP_COLORS_PER_PAL);

  S_vdp_pal_count += 1;

  /* copy cells */
  num_cells = num_columns * num_rows * num_frames;

  if ((S_vdp_cell_count + num_cells) >= VDP_NUM_CELLS)
    return 1;

  cell_addr = S_vdp_cell_count * VDP_BYTES_PER_CELL;

  memcpy( &S_vdp_cells[cell_addr], &G_rom_data[rom_addr], 
          num_cells * VDP_BYTES_PER_CELL);

  S_vdp_cell_count += num_cells;

  /* create sprite table entry */
  if ((S_vdp_sprite_count + 1) >= VDP_NUM_SPRITES)
    return 1;

  sprite_addr = S_vdp_sprite_count * VDP_VALS_PER_SPRITE;

  val = ((num_columns - 1) << 12) & 0xF000;
  val |= ((num_rows - 1) << 8) & 0x0F00;
  val |= ((num_frames - 1) << 4) & 0x00F0;
  S_vdp_sprites[sprite_addr + 0] = val;

  val = delay_time;
  S_vdp_sprites[sprite_addr + 1] = val;

  val = ((pal_addr / VDP_COLORS_PER_PAL) << 12) & 0xF000;
  val |= (cell_addr / VDP_BYTES_PER_CELL) & 0x0FFF;
  S_vdp_sprites[sprite_addr + 2] = val;

  S_vdp_sprite_count += 1;

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

  unsigned long  sprite_addr;

  unsigned short num_rows;
  unsigned short num_columns;
  unsigned short num_frames;
  unsigned short delay_time;

  unsigned short thing_pos_x;
  unsigned short thing_pos_y;

  unsigned long  pal_addr;
  unsigned long  pal_offset;

  unsigned long  cell_addr;
  unsigned long  cell_offset;

  unsigned long  pixel_addr;
  unsigned long  pixel_offset;

  /* clear framebuffer */
  for (m = 0; m < VDP_SCREEN_SIZE; m++)
    G_vdp_fb_rgb[m] = 0x0000;

  /* test: draw the test sprite */
  sprite_addr = 0 * VDP_VALS_PER_SPRITE;

  val = S_vdp_sprites[sprite_addr + 0];

  num_columns = ((val >> 12) & 0x000F) + 1;
  num_rows = ((val >> 8) & 0x000F) + 1;
  num_frames = ((val >> 4) & 0x000F) + 1;

  val = S_vdp_sprites[sprite_addr + 1];

  delay_time = val;

  val = S_vdp_sprites[sprite_addr + 2];

  pal_addr = VDP_COLORS_PER_PAL * ((val >> 12) & 0x000F);
  cell_addr = VDP_BYTES_PER_CELL * (val & 0x0FFF);
  
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

