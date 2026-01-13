/******************************************************************************/
/* vdp.h (faux graphics chip)                                                 */
/******************************************************************************/

#ifndef VDP_H
#define VDP_H

/* palettes */
#define VDP_COLORS_PER_PAL  16
#define VDP_NUM_PALS        16
#define VDP_PALS_SIZE       (VDP_COLORS_PER_PAL * VDP_NUM_PALS)

extern unsigned short G_vdp_sprite_pals[VDP_PALS_SIZE];
extern short          G_vdp_sprite_pal_count;

extern unsigned short G_vdp_tile_pals[VDP_PALS_SIZE];
extern short          G_vdp_tile_pal_count;

/* cells */
#define VDP_CELL_W_H        8
#define VDP_PIXELS_PER_CELL (VDP_CELL_W_H * VDP_CELL_W_H)  /* 8x8 */
#define VDP_BYTES_PER_CELL  (VDP_PIXELS_PER_CELL / 2)

#define VDP_NUM_CELLS       (2 * 1024)
#define VDP_CELLS_SIZE      (VDP_BYTES_PER_CELL * VDP_NUM_CELLS)  /* 64 K */

extern unsigned char  G_vdp_sprite_cells[VDP_CELLS_SIZE];
extern short          G_vdp_sprite_cell_count;

extern unsigned char  G_vdp_tile_cells[VDP_CELLS_SIZE];
extern short          G_vdp_tile_cell_count;

/* table entries */
#define VDP_VALS_PER_ENTRY  3
#define VDP_NUM_ENTRIES     1024
#define VDP_ENTRIES_SIZE    (VDP_VALS_PER_ENTRY * VDP_NUM_ENTRIES)

extern unsigned short G_vdp_sprite_entries[VDP_ENTRIES_SIZE];
extern short          G_vdp_sprite_entry_count;

extern unsigned short G_vdp_tile_entries[VDP_ENTRIES_SIZE];
extern short          G_vdp_tile_entry_count;

/* framebuffer */
#define VDP_SCREEN_W 320
#define VDP_SCREEN_H 224

#define VDP_SCREEN_SIZE (VDP_SCREEN_W * VDP_SCREEN_H)

extern unsigned short G_vdp_fb_rgb[VDP_SCREEN_SIZE];

/* function declarations */
int vdp_reset();

int vdp_load_sprite(unsigned short sprite_index);

int vdp_draw_frame();

#endif

