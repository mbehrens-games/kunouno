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

extern unsigned short G_vdp_nametable_buf[VDP_NAMETABLE_SIZE];
extern unsigned long  G_vdp_nametable_num_words;

/* palettes */
#define VDP_COLORS_PER_PAL  16

#define VDP_MAX_PALS        (1 << 8) /* 8 KB total size */ 
#define VDP_PALS_SIZE       (VDP_MAX_PALS * VDP_COLORS_PER_PAL)

extern unsigned short G_vdp_pals_buf[VDP_PALS_SIZE];
extern unsigned long  G_vdp_pals_num_words;

/* cells */
#define VDP_CELL_W_H        8
#define VDP_PIXELS_PER_CELL (VDP_CELL_W_H * VDP_CELL_W_H)
#define VDP_BYTES_PER_CELL  (VDP_PIXELS_PER_CELL / 2)

#define VDP_BANK_SIZE       (1 << 22) /* 4 MB total size */

extern unsigned char  G_vdp_bank_buf[VDP_BANK_SIZE];
extern unsigned long  G_vdp_bank_num_bytes;

/* function declarations */
int vdp_reset();

int vdp_draw_frame();

#endif

