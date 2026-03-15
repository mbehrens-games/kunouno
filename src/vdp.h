/******************************************************************************/
/* vdp.h (faux graphics chip)                                                 */
/******************************************************************************/

#ifndef VDP_H
#define VDP_H

/* framebuffer */
#define VDP_SCREEN_W 320
#define VDP_SCREEN_H 224

#define VDP_SCREEN_SIZE (VDP_SCREEN_W * VDP_SCREEN_H)

extern unsigned short G_vdp_fb_rgb[VDP_SCREEN_SIZE];

/* nametable */
#define VDP_ENTRY_SIZE      5
#define VDP_MAX_ENTRIES     (1 << 12)
#define VDP_NAMETABLE_SIZE  (VDP_ENTRY_SIZE * VDP_MAX_ENTRIES)

extern unsigned short G_vdp_entries[VDP_NAMETABLE_SIZE];

/* palettes */
#define VDP_COLORS_PER_PAL  16

#define VDP_ROM_MAX_PALS    (1 << 8) /* 8 KB total size */ 
#define VDP_ROM_PALS_SIZE   (VDP_ROM_MAX_PALS * VDP_COLORS_PER_PAL)

extern unsigned short G_vdp_pals[VDP_ROM_PALS_SIZE];

/* cells */
#define VDP_CELL_W_H          8
#define VDP_PIXELS_PER_CELL   (VDP_CELL_W_H * VDP_CELL_W_H)
#define VDP_BYTES_PER_CELL    (VDP_PIXELS_PER_CELL / 2)

#define VDP_ROM_MAX_CELLS     (1 << 16) /* 2 MB total size */
#define VDP_ROM_CELLS_SIZE    (VDP_ROM_MAX_CELLS * VDP_BYTES_PER_CELL)

#define VDP_CACHE_MAX_CELLS   (1 << 13) /* 256 KB total size */
#define VDP_CACHE_CELLS_SIZE  (VDP_CACHE_MAX_CELLS * VDP_BYTES_PER_CELL)

extern unsigned char G_vdp_cells[VDP_ROM_CELLS_SIZE];

/* function declarations */
int vdp_reset();

int vdp_draw_frame();

#endif

