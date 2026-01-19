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

/* sprites */
#define VDP_VALS_PER_SPRITE 5
#define VDP_NUM_SPRITES     1024
#define VDP_SPRITES_SIZE    (VDP_VALS_PER_SPRITE * VDP_NUM_SPRITES)

static unsigned short S_vdp_sprites[VDP_SPRITES_SIZE];
static short          S_vdp_sprite_count;

/* tiles */
#define VDP_VALS_PER_TILE   5
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
/* vdp_load_palette_from_last_sprite()                                        */
/******************************************************************************/
int vdp_load_palette_from_last_sprite()
{
  int k;

  unsigned long  rom_addr;
  unsigned long  num_bytes;

  unsigned short sprite_index;
  unsigned short sprite_addr;

  unsigned short pal_number;
  unsigned short pal_index;
  unsigned short pal_addr;

  unsigned short pal_colors[VDP_COLORS_PER_PAL];

  /* determine index of last sprite */
  if (S_vdp_sprite_count == 0)
    return 1;

  sprite_index = S_vdp_sprite_count - 1;
  sprite_addr = sprite_index * VDP_VALS_PER_SPRITE;

  /* obtain palette number (in the rom) from the sprite */
  pal_number = S_vdp_sprites[sprite_addr + 2];

  /* check if this palette has already been loaded to cram */
  for (k = 0; k < sprite_index; k++)
  {
    if (pal_number == S_vdp_sprites[k * VDP_VALS_PER_SPRITE + 2])
    {
      pal_index = k;
      S_vdp_sprites[sprite_addr + 4] &= 0x0FFF;
      S_vdp_sprites[sprite_addr + 4] |= (pal_index << 12) & 0xF000;
      return 0;
    }
  }

  /* get the file address and size in the rom */
  if (rom_lookup_file(ROM_FOLDER_PALS, pal_number, &rom_addr, &num_bytes))
    return 1;

  /* read the palette colors from the rom */
  for (k = 0; k < VDP_COLORS_PER_PAL; k++)
  {
    pal_colors[k] = (G_rom_data[rom_addr + (2 * k + 0)] << 8) & 0x7F00;
    pal_colors[k] |= G_rom_data[rom_addr + (2 * k + 1)] & 0x00FF;
  }

  /* copy palette to cram */
  if ((S_vdp_pal_count + 1) >= VDP_NUM_PALS)
    return 1;

  pal_index = S_vdp_pal_count;
  pal_addr = pal_index * VDP_COLORS_PER_PAL;

  memcpy( &S_vdp_pals[pal_addr], &pal_colors[0], 
          sizeof(unsigned short) * VDP_COLORS_PER_PAL);

  S_vdp_pal_count += 1;

  /* save palette index in sprite and return */
  S_vdp_sprites[sprite_addr + 4] &= 0x0FFF;
  S_vdp_sprites[sprite_addr + 4] |= (pal_index << 12) & 0xF000;

  return 0;
}

/******************************************************************************/
/* vdp_load_cells_from_last_sprite()                                          */
/******************************************************************************/
int vdp_load_cells_from_last_sprite()
{
  int k;

  unsigned long  rom_addr;
  unsigned long  num_bytes;

  unsigned short sprite_index;
  unsigned short sprite_addr;

  unsigned short cell_number;
  unsigned short cell_index;
  unsigned short cell_addr;

  unsigned short num_rows;
  unsigned short num_columns;
  unsigned short num_frames;

  unsigned short num_cells;

  /* determine index of last sprite */
  if (S_vdp_sprite_count == 0)
    return 1;

  sprite_index = S_vdp_sprite_count - 1;
  sprite_addr = sprite_index * VDP_VALS_PER_SPRITE;

  /* obtain cell number (in the rom) from the sprite */
  cell_number = S_vdp_sprites[sprite_addr + 3];

  /* check if these cells have already been loaded to vram */
  for (k = 0; k < sprite_index; k++)
  {
    if (cell_number == S_vdp_sprites[k * VDP_VALS_PER_SPRITE + 3])
    {
      cell_index = k;
      S_vdp_sprites[sprite_addr + 4] &= 0xF000;
      S_vdp_sprites[sprite_addr + 4] |= cell_index & 0x0FFF;
      return 0;
    }
  }

  /* get the file address and size in the rom */
  if (rom_lookup_file(ROM_FOLDER_CELLS, cell_number, &rom_addr, &num_bytes))
    return 1;

  /* compute number of cells */
  num_columns = ((S_vdp_sprites[sprite_addr + 0] >> 6) & 0x03) + 1;
  num_rows =    ((S_vdp_sprites[sprite_addr + 0] >> 4) & 0x03) + 1;
  num_frames =   (S_vdp_sprites[sprite_addr + 0] & 0x0F) + 1;

  num_cells = num_columns * num_rows * num_frames;

  /* copy cells to vram */
  if ((S_vdp_cell_count + num_cells) >= VDP_NUM_CELLS)
    return 1;

  cell_index = S_vdp_cell_count;
  cell_addr = cell_index * VDP_BYTES_PER_CELL;

  memcpy( &S_vdp_cells[cell_addr], &G_rom_data[rom_addr], 
          num_cells * VDP_BYTES_PER_CELL);

  S_vdp_cell_count += num_cells;

  /* save cell index in sprite and return */
  S_vdp_sprites[sprite_addr + 4] &= 0xF000;
  S_vdp_sprites[sprite_addr + 4] |= cell_index & 0x0FFF;

  return 0;
}

/******************************************************************************/
/* vdp_load_sprite()                                                          */
/******************************************************************************/
int vdp_load_sprite(unsigned short sprite_number)
{
  unsigned short val;

  unsigned long  rom_addr;
  unsigned long  num_bytes;

  unsigned short sprite_index;
  unsigned short sprite_addr;

  /* get the file address and size in the rom */
  if (rom_lookup_file(ROM_FOLDER_SPRITES, sprite_number, &rom_addr, &num_bytes))
    return 1;

  /* copy sprite data to vram */
  if ((S_vdp_sprite_count + 1) >= VDP_NUM_SPRITES)
    return 1;

  sprite_index = S_vdp_sprite_count;
  sprite_addr = sprite_index * VDP_VALS_PER_SPRITE;

  val = (G_rom_data[rom_addr + 8] << 8) & 0xFF00;
  val |= G_rom_data[rom_addr + 9] & 0x00FF;
  S_vdp_sprites[sprite_addr + 0] = val;

  val = (G_rom_data[rom_addr + 10] << 8) & 0xFF00;
  val |= G_rom_data[rom_addr + 11] & 0x00FF;
  S_vdp_sprites[sprite_addr + 1] = val;

  val = (G_rom_data[rom_addr + 12] << 8) & 0xFF00;
  val |= G_rom_data[rom_addr + 13] & 0x00FF;
  S_vdp_sprites[sprite_addr + 2] = val;

  val = (G_rom_data[rom_addr + 14] << 8) & 0xFF00;
  val |= G_rom_data[rom_addr + 15] & 0x00FF;
  S_vdp_sprites[sprite_addr + 3] = val;

  S_vdp_sprite_count += 1;

  /* load the palette, if necessary */
  if (vdp_load_palette_from_last_sprite())
    return 1;

  /* load cells, if necessary */
  if (vdp_load_cells_from_last_sprite())
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

  unsigned short sprite_addr;

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
  sprite_addr = 0 * VDP_VALS_PER_SPRITE;

  val = S_vdp_sprites[sprite_addr + 0];

  num_columns = ((val >> 6) & 0x0003) + 1;
  num_rows = ((val >> 4) & 0x0003) + 1;
  num_frames = (val & 0x000F) + 1;

  val = S_vdp_sprites[sprite_addr + 1];

  val = S_vdp_sprites[sprite_addr + 4];

  pal_addr = VDP_COLORS_PER_PAL * ((val >> 12) & 0x000F);
  cell_addr = VDP_BYTES_PER_CELL * (val & 0x0FFF);
  
  thing_pos_x = 64;
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

