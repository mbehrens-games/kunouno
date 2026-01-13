/******************************************************************************/
/* vdp.c (faux graphics chip)                                                 */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vdp.h"

#include "rom.h"

/* palettes */
unsigned short G_vdp_sprite_pals[VDP_PALS_SIZE];
short          G_vdp_sprite_pal_count;

unsigned short G_vdp_tile_pals[VDP_PALS_SIZE];
short          G_vdp_tile_pal_count;

/* cells */
unsigned char  G_vdp_sprite_cells[VDP_CELLS_SIZE];
short          G_vdp_sprite_cell_count;

unsigned char  G_vdp_tile_cells[VDP_CELLS_SIZE];
short          G_vdp_tile_cell_count;

/* table entries */
unsigned short G_vdp_sprite_entries[VDP_ENTRIES_SIZE];
short          G_vdp_sprite_entry_count;

unsigned short G_vdp_tile_entries[VDP_ENTRIES_SIZE];
short          G_vdp_tile_entry_count;

/* framebuffer */
unsigned short G_vdp_fb_rgb[VDP_SCREEN_SIZE];

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

  /* palettes */
  for (k = 0; k < VDP_PALS_SIZE; k++)
  {
    G_vdp_sprite_pals[k] = 0x0000;
    G_vdp_tile_pals[k] = 0x0000;
  }

  G_vdp_sprite_pal_count = 0;
  G_vdp_tile_pal_count = 0;

  /* cells */
  for (k = 0; k < VDP_CELLS_SIZE; k++)
  {
    G_vdp_sprite_cells[k] = 0x00;
    G_vdp_tile_cells[k] = 0x00;
  }

  G_vdp_sprite_cell_count = 0;
  G_vdp_tile_cell_count = 0;

  /* table entries */
  for (k = 0; k < VDP_ENTRIES_SIZE; k++)
  {
    G_vdp_sprite_entries[k] = 0x0000;
    G_vdp_tile_entries[k] = 0x0000;
  }

  G_vdp_sprite_entry_count = 0;
  G_vdp_tile_entry_count = 0;

  /* framebuffer */
  for (k = 0; k < VDP_SCREEN_SIZE; k++)
    G_vdp_fb_rgb[k] = 0x0000;

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
int vdp_load_sprite(unsigned short sprite_index)
{
  int k;

  unsigned long  abs_addr;
  unsigned long  num_bytes;

  unsigned short num_rows;
  unsigned short num_columns;
  unsigned short num_frames;
  unsigned short delay_time;

  unsigned short pal_index;
  unsigned short cell_index;
  unsigned short entry_index;

  unsigned short pal_colors[VDP_COLORS_PER_PAL];

  unsigned short num_cells;

  /* get the file address and size in the rom */
  rom_lookup_file(ROM_FOLDER_SPRITES, sprite_index, &abs_addr, &num_bytes);

  printf("Sprite Absolute Address: %ld, Size: %ld\n", abs_addr, num_bytes);

  /* read sprite header */
  num_columns = ((G_rom_data[abs_addr + 0] >> 4) & 0x0F) + 1;
  num_rows = (G_rom_data[abs_addr + 0] & 0x0F) + 1;
  num_frames = ((G_rom_data[abs_addr + 1] >> 4) & 0x0F) + 1;

  delay_time = (G_rom_data[abs_addr + 2] << 8) & 0xFF00;
  delay_time |= G_rom_data[abs_addr + 3] & 0x00FF;
  delay_time *= 10;

  printf( "Sprite Num Frames: %d, Columns: %d, Rows: %d, Delay: %d\n", 
          num_frames, num_columns, num_rows, delay_time);

  /* read palette */
  for (k = 0; k < VDP_COLORS_PER_PAL; k++)
  {
    pal_colors[k] = (G_rom_data[abs_addr + 4 + (2 * k + 0)] << 8) & 0x7F00;
    pal_colors[k] |= G_rom_data[abs_addr + 4 + (2 * k + 1)] & 0x00FF;

    printf( "Palette Color %d: %d (%d, %d, %d)\n", k, pal_colors[k], 
            (pal_colors[k] >> 10) & 0x1F, (pal_colors[k] >> 5) & 0x1F, pal_colors[k] & 0x1F);
  }

  /* copy palette */
  if ((G_vdp_sprite_pal_count + 1) >= VDP_NUM_PALS)
    return 1;

  pal_index = G_vdp_sprite_pal_count;

  memcpy( &G_vdp_sprite_pals[pal_index * VDP_COLORS_PER_PAL + 0], 
          &pal_colors[0], 
          sizeof(unsigned short) * VDP_COLORS_PER_PAL);

  G_vdp_sprite_pal_count += 1;

  /* copy cells */
  num_cells = num_columns * num_rows * num_frames;

  if ((G_vdp_sprite_cell_count + num_cells) >= VDP_NUM_CELLS)
    return 1;

  cell_index = G_vdp_sprite_cell_count;

  memcpy( &G_vdp_sprite_cells[cell_index * VDP_BYTES_PER_CELL + 0], 
          &G_rom_data[abs_addr + 4 + (2 * 16) + 0], 
          num_cells * VDP_BYTES_PER_CELL);

  G_vdp_sprite_cell_count += num_cells;

  /* create table entry */
  if ((G_vdp_sprite_entry_count + 1) >= VDP_NUM_ENTRIES)
    return 1;

  entry_index = G_vdp_sprite_entry_count;

  G_vdp_sprite_entries[entry_index * VDP_VALS_PER_ENTRY + 0] = ((num_columns - 1) << 12) & 0xF000;
  G_vdp_sprite_entries[entry_index * VDP_VALS_PER_ENTRY + 0] |= ((num_rows - 1) << 8) & 0x0F00;
  G_vdp_sprite_entries[entry_index * VDP_VALS_PER_ENTRY + 0] |= ((num_frames - 1) << 4) & 0x00F0;

  G_vdp_sprite_entries[entry_index * VDP_VALS_PER_ENTRY + 1] = delay_time;

  G_vdp_sprite_entries[entry_index * VDP_VALS_PER_ENTRY + 2] = (pal_index << 12) & 0xF000;
  G_vdp_sprite_entries[entry_index * VDP_VALS_PER_ENTRY + 2] |= cell_index & 0x0FFF;

  G_vdp_sprite_entry_count += 1;

  return 0;
}

/******************************************************************************/
/* vdp_draw_frame()                                                           */
/******************************************************************************/
int vdp_draw_frame()
{
  int k;
  int m;

  unsigned short val;

  unsigned short num_rows;
  unsigned short num_columns;
  unsigned short num_frames;
  unsigned short num_cells;

  unsigned short thing_pos_x;
  unsigned short thing_pos_y;

  unsigned short pal_addr;
  unsigned short pal_offset;

  unsigned long cell_addr;
  unsigned long cell_offset;

  unsigned long pixel_addr;
  unsigned long pixel_offset;

  unsigned short sprite_index;
  unsigned short pal_index;
  unsigned short cell_index;

  /* clear framebuffer */
  for (k = 0; k < VDP_SCREEN_SIZE; k++)
    G_vdp_fb_rgb[k] = 0x0000;

  /* test: draw the test sprite */
  sprite_index = 0;

  val = G_vdp_sprite_entries[sprite_index * VDP_VALS_PER_ENTRY + 0];

  num_columns = ((val >> 12) & 0x000F) + 1;
  num_rows = ((val >> 8) & 0x000F) + 1;
  num_frames = ((val >> 4) & 0x000F) + 1;

  num_cells = num_rows * num_columns;

  val = G_vdp_sprite_entries[sprite_index * VDP_VALS_PER_ENTRY + 2];

  pal_index = (val >> 12) & 0x000F;
  cell_index = val & 0x0FFF;

  num_cells = num_rows * num_columns;

  thing_pos_x = 128;
  thing_pos_y = 64;

  pal_addr = VDP_COLORS_PER_PAL * pal_index;
  
  for (k = 0; k < num_cells; k++)
  {
    /* determine cell & pixel positions */
    cell_addr = VDP_BYTES_PER_CELL * (cell_index + k);

    pixel_addr = VDP_SCREEN_W * thing_pos_y + thing_pos_x;
    pixel_addr += VDP_SCREEN_W * VDP_CELL_W_H * (k / num_columns);
    pixel_addr += VDP_CELL_W_H * (k % num_columns);

    for (m = 0; m < VDP_PIXELS_PER_CELL; m++)
    {
      /* determine cell & pixel offsets */
      cell_offset = m / 2;

      pixel_offset = VDP_SCREEN_W * (m / VDP_CELL_W_H);
      pixel_offset += m % VDP_CELL_W_H;

      /* read palette offset from cell */
      if (m % 2 == 0)
        pal_offset = (G_vdp_sprite_cells[cell_addr + cell_offset] >> 4) & 0x0F;
      else
        pal_offset = G_vdp_sprite_cells[cell_addr + cell_offset] & 0x0F;

      /* write pixel to the frame buffer */
      if (pal_offset == 0)
        continue;

      val = G_vdp_sprite_pals[pal_addr + pal_offset];

      G_vdp_fb_rgb[pixel_addr + pixel_offset] = val;
    }
  }

  return 0;
}

