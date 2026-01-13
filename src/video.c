/*******************************************************************************
** video.c (sdl video code)
*******************************************************************************/

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>

#include "video.h"

#include "vdp.h"

/* sdl window, renderer, etc */
static SDL_Window*   S_video_sdl_window;
static SDL_Renderer* S_video_sdl_renderer;
static SDL_Texture*  S_video_sdl_frame_texture;

static int S_video_window_size;

/* window size lut */
static short  S_video_window_size_table[2 * VIDEO_NUM_WINDOW_SIZES] = 
              {  854,  480, 
                1067,  600, 
                1280,  720, 
                1366,  768, 
                1920, 1080 
              };

#define VIDEO_FB_TEXTURE_W 512
#define VIDEO_FB_TEXTURE_H 256

/*******************************************************************************
** video_init()
*******************************************************************************/
short int video_init()
{
  /* initialize pointers to null */
  S_video_sdl_window = NULL;
  S_video_sdl_renderer = NULL;
  S_video_sdl_frame_texture = NULL;

  /* set default window size */
  S_video_window_size = VIDEO_WINDOW_SIZE_480P;

  /* create the window */
  S_video_sdl_window = SDL_CreateWindow("KUNO-1",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        S_video_window_size_table[2 * S_video_window_size + 0], 
                                        S_video_window_size_table[2 * S_video_window_size + 1],
                                        SDL_WINDOW_SHOWN);

  if (S_video_sdl_window == NULL)
  {
    printf("Failed to create window: %s\n", SDL_GetError());
    return 1;
  }

  /* create the renderer */
  S_video_sdl_renderer = SDL_CreateRenderer(S_video_sdl_window, 
                                            -1, 
                                            0);

  if (S_video_sdl_renderer == NULL)
  {
    printf("Failed to create renderer: %s\n", SDL_GetError());
    return 1;
  }

  /* set screen size and upscaling mode */
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

/*
  SDL_RenderSetLogicalSize( S_video_sdl_renderer, 
                            VDP_SCREEN_W, 
                            VDP_SCREEN_H);
*/

  /* create the framebuffer texture */
  S_video_sdl_frame_texture = SDL_CreateTexture(S_video_sdl_renderer,
                                                SDL_PIXELFORMAT_RGB555,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                VIDEO_FB_TEXTURE_W, 
                                                VIDEO_FB_TEXTURE_H);

  if (S_video_sdl_frame_texture == NULL)
  {
    printf("Failed to create framebuffer texture: %s\n", SDL_GetError());
    return 1;
  }

  /* initialize window to black */
  SDL_SetRenderDrawColor(S_video_sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(S_video_sdl_renderer);
  SDL_RenderPresent(S_video_sdl_renderer);

  return 0;
}

/*******************************************************************************
** video_deinit()
*******************************************************************************/
short int video_deinit()
{
  SDL_DestroyTexture(S_video_sdl_frame_texture);
  SDL_DestroyRenderer(S_video_sdl_renderer);
  SDL_DestroyWindow(S_video_sdl_window);

  return 0;
}

/*******************************************************************************
** video_display_frame()
*******************************************************************************/
short int video_display_frame()
{
  SDL_Rect screen_rect;

  /* setup rectangle */
  screen_rect.x = 0;
  screen_rect.y = 0;
  screen_rect.w = VDP_SCREEN_W;
  screen_rect.h = VDP_SCREEN_H;

  /* clear screen */
  SDL_SetRenderDrawColor(S_video_sdl_renderer, 0, 0, 0, 255);
  SDL_RenderClear(S_video_sdl_renderer);

  /* copy the framebuffer to the texture and draw it on screen */
  SDL_UpdateTexture(S_video_sdl_frame_texture, 
                    &screen_rect, 
                    G_vdp_fb_rgb, 
                    VDP_SCREEN_W * sizeof (unsigned short));

  SDL_RenderCopy( S_video_sdl_renderer, 
                  S_video_sdl_frame_texture, 
                  &screen_rect, 
                  NULL);

  SDL_RenderPresent(S_video_sdl_renderer);

  return 0;
}

/*******************************************************************************
** video_resize_window()
*******************************************************************************/
short int video_resize_window(int size)
{
  int index;

  SDL_DisplayMode mode;

  /* make sure the window size is valid */
  if ((size < 0) || (size >= VIDEO_NUM_WINDOW_SIZES))
    return 1;

  /* determine display resolution */
  index = SDL_GetWindowDisplayIndex(S_video_sdl_window);
  SDL_GetDesktopDisplayMode(index, &mode);

  /* determine if the window will fit on-screen */
  if ((mode.w < S_video_window_size_table[2 * size + 0]) || 
      (mode.h < S_video_window_size_table[2 * size + 1]))
  {
    return 1;
  }

  /* set the new window size */
  S_video_window_size = size;

  /* resize the window if not in fullscreen */
  SDL_SetWindowSize(S_video_sdl_window, 
                    S_video_window_size_table[2 * S_video_window_size + 0], 
                    S_video_window_size_table[2 * S_video_window_size + 1]);

  SDL_SetWindowPosition(S_video_sdl_window, 
                        SDL_WINDOWPOS_CENTERED, 
                        SDL_WINDOWPOS_CENTERED);

  return 0;
}

/*******************************************************************************
** video_increase_window_size()
*******************************************************************************/
short int video_increase_window_size()
{
  if (S_video_window_size < VIDEO_NUM_WINDOW_SIZES - 1)
    video_resize_window(S_video_window_size + 1);

  return 0;
}

/*******************************************************************************
** video_decrease_window_size()
*******************************************************************************/
short int video_decrease_window_size()
{
  if (S_video_window_size > 0)
    video_resize_window(S_video_window_size - 1);

  return 0;
}

