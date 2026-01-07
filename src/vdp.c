/******************************************************************************/
/* vdp.c (faux graphics chip)                                                 */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "vdp.h"

unsigned short  G_vdp_pals[VDP_PALS_SIZE];
short           G_vdp_pal_count;

unsigned char   G_vdp_cells[VDP_CELLS_SIZE];
short           G_vdp_cell_count;

struct sprite_def G_vdp_sprite_defs[VDP_NUM_SPRITE_DEFS];
short             G_vdp_sprite_def_count;

/* layers */
#define VDP_LAYER_W 512
#define VDP_LAYER_H 512

#define VDP_LAYER_SIZE (VDP_LAYER_W * VDP_LAYER_H)

static unsigned short S_vdp_layer_sky[VDP_LAYER_SIZE];
static unsigned short S_vdp_layer_bg[VDP_LAYER_SIZE];
static unsigned short S_vdp_layer_all[VDP_LAYER_SIZE];

/* framebuffer */
unsigned char G_vdp_fb_rgb[3 * VDP_FRAMEBUFFER_SIZE];

/* expand from 4 bits to 8 bits, then multiply by 7/8 (this lowers    */
/* the contrast, so we have room for the brightness adjustment later) */
static unsigned char  S_vdp_rgb_exp_table[16] = 
                      {   0,  15,  30,  45,  60,  74,  89, 104, 
                        119, 134, 149, 164, 178, 193, 208, 223
                      };

/******************************************************************************/
/* vdp_reset()                                                                */
/******************************************************************************/
int vdp_reset()
{
  int k;

  /* palettes */
  for (k = 0; k < VDP_PALS_SIZE; k++)
    G_vdp_pals[k] = 0x0000;

  G_vdp_pal_count = 0;

  /* cells */
  for (k = 0; k < VDP_CELLS_SIZE; k++)
    G_vdp_cells[k] = 0;

  G_vdp_cell_count = 0;

  /* definitions */
  for (k = 0; k < VDP_NUM_SPRITE_DEFS; k++)
  {
    G_vdp_sprite_defs[k].cell_index = 0x0000;
    G_vdp_sprite_defs[k].dim_frames_anim = 0x0000;
  }

  G_vdp_sprite_def_count = 0;

  /* layers */
  for (k = 0; k < VDP_LAYER_SIZE; k++)
  {
    S_vdp_layer_sky[k] = 0x0000;
    S_vdp_layer_bg[k] = 0x0000;
    S_vdp_layer_all[k] = 0x0000;
  }

  /* framebuffer */
  for (k = 0; k < VDP_FRAMEBUFFER_SIZE; k++)
  {
    G_vdp_fb_rgb[3 * k + 0] = 0;
    G_vdp_fb_rgb[3 * k + 1] = 0;
    G_vdp_fb_rgb[3 * k + 2] = 0;
  }

  return 0;
}

/******************************************************************************/
/* vdp_draw_frame()                                                           */
/******************************************************************************/
int vdp_draw_frame()
{
  int k;
  int m;

  unsigned char r;
  unsigned char g;
  unsigned char b;

  unsigned short val;

  struct sprite_def* spdf;

  int num_rows;
  int num_columns;
  int num_cells;

  int thing_pos_x;
  int thing_pos_y;

  int pal_addr;
  int pal_offset;

  int cell_addr;
  int cell_offset;

  int pixel_addr;
  int pixel_offset;

  /* clear output framebuffer */
  for (k = 0; k < VDP_FRAMEBUFFER_SIZE; k++)
  {
    G_vdp_fb_rgb[3 * k + 0] = 0;
    G_vdp_fb_rgb[3 * k + 1] = 0;
    G_vdp_fb_rgb[3 * k + 2] = 0;
  }

  /* test: draw the test sprite */
  spdf = &G_vdp_sprite_defs[0];

  num_rows = (spdf->dim_frames_anim & 0x0003) + 1;
  num_columns = ((spdf->dim_frames_anim >> 2) & 0x0003) + 1;
  num_cells = num_rows * num_columns;

  thing_pos_x = 64;
  thing_pos_y = 32;

  pal_addr = VDP_COLORS_PER_PAL * 0;
  
  for (k = 0; k < num_cells; k++)
  {
    /* determine cell & pixel positions */
    cell_addr = VDP_BYTES_PER_CELL * (spdf->cell_index + k);

    pixel_addr = VDP_FRAMEBUFFER_W * thing_pos_y + thing_pos_x;
    pixel_addr += VDP_FRAMEBUFFER_W * VDP_CELL_W_H * (k / num_columns);
    pixel_addr += VDP_CELL_W_H * (k % num_columns);

    for (m = 0; m < VDP_PIXELS_PER_CELL; m++)
    {
      /* determine cell & pixel offsets */
      cell_offset = m / 2;

      pixel_offset = VDP_FRAMEBUFFER_W * (m / VDP_CELL_W_H);
      pixel_offset += m % VDP_CELL_W_H;

      /* read palette offset from cell */
      if (m % 2 == 0)
        pal_offset = (G_vdp_cells[cell_addr + cell_offset] >> 4) & 0x0F;
      else
        pal_offset = G_vdp_cells[cell_addr + cell_offset] & 0x0F;

      /* write pixel to the frame buffer */
      if (pal_offset == 0)
        continue;

      val = G_vdp_pals[pal_addr + pal_offset];

      r = S_vdp_rgb_exp_table[(val >> 8) & 0x0F];
      g = S_vdp_rgb_exp_table[(val >> 4) & 0x0F];
      b = S_vdp_rgb_exp_table[val & 0x0F];

      G_vdp_fb_rgb[3 * (pixel_addr + pixel_offset) + 0] = r;
      G_vdp_fb_rgb[3 * (pixel_addr + pixel_offset) + 1] = g;
      G_vdp_fb_rgb[3 * (pixel_addr + pixel_offset) + 2] = b;
    }
  }

  return 0;
}

