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

/* function declarations */
int vdp_reset();

int vdp_load_sprite(unsigned short sprite_number);

int vdp_draw_frame();

#endif

