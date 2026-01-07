/******************************************************************************/
/* kunouno (KUNO-1 faux game console) - No Shinobi Knows Me 2025              */
/******************************************************************************/

#include <SDL2/SDL.h>

#include <stdio.h>

#include "cart.h"

#include "vdp.h"
#include "video.h"

/*******************************************************************************
** main()
*******************************************************************************/
int main(int argc, char *argv[])
{
  SDL_Event event;
  Uint32    ticks_last_update;
  Uint32    ticks_current;

  /* initialize sdl */
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
  {
    fprintf(stdout, "Failed to initialize SDL: %s\n", SDL_GetError());
    return 0;
  }

  /* initialize video */
  if (video_init())
  {
    fprintf(stdout, "Failed to initialize video. Exiting...\n");
    goto cleanup_sdl;
  }

#if 0
  /* initialize audio */
  if (audio_init())
  {
    fprintf(stdout, "Error initializing audio device.\n");
    goto cleanup_video;
  }
#endif

  /* initialize graphics chip */
  vdp_reset();

  /* increase window size to 720p as test */
  video_increase_window_size();
  video_increase_window_size();

  /* load test cart file */
  if (cart_load("test.kn1"))
  {
    fprintf(stdout, "Failed to load test cart data. Exiting...\n");
    goto cleanup_all;
  }

  /* initialize ticks */
  ticks_current = SDL_GetTicks();
  ticks_last_update = ticks_current;

  /* main loop */
  while (1)
  {
    /* process sdl events */
    while (SDL_PollEvent(&event))
    {
      /* quit */
      if (event.type == SDL_QUIT)
      {
        goto cleanup_all;
      }

      /* window */
      if (event.type == SDL_WINDOWEVENT)
      {
        /* if window is closed, quit */
        if (event.window.event == SDL_WINDOWEVENT_CLOSE)
        {
          goto cleanup_all;
        }

#if 0
        /* if focus is lost, pause */
        if ((event.window.event == SDL_WINDOWEVENT_MINIMIZED) ||
            (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST))
        {
          G_program_flags |= PROGRAM_FLAG_WINDOW_MINIMIZED;
          audio_pause();
        }

        /* if focus is gained, unpause */
        if ((event.window.event == SDL_WINDOWEVENT_RESTORED) ||
            (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED))
        {
          G_program_flags &= ~PROGRAM_FLAG_WINDOW_MINIMIZED;
          audio_unpause();
          ticks_last_update = SDL_GetTicks();
        }
#endif
      }

#if 0
      /* keyboard (key down) */
      if (event.type == SDL_KEYDOWN)
      {
        if ((event.key.state == SDL_PRESSED) && (event.key.repeat == 0))
          controls_keyboard_key_pressed(event.key.keysym.scancode);
      }

      /* keyboard (key up) */
      if (event.type == SDL_KEYUP)
      {
        if ((event.key.state == SDL_RELEASED) && (event.key.repeat == 0))
          controls_keyboard_key_released(event.key.keysym.scancode);
      }

      /* mouse (button down) */
      if (event.type == SDL_MOUSEBUTTONDOWN)
      {
        if (event.button.state == SDL_PRESSED)
          controls_mouse_button_pressed(event.button.button, event.button.x, event.button.y);
      }

      /* mouse (button up) */
      if (event.type == SDL_MOUSEBUTTONUP)
      {
        if (event.button.state == SDL_RELEASED)
          controls_mouse_button_released(event.button.button);
      }

      /* mouse (cursor moved) */
      if (event.type == SDL_MOUSEMOTION)
      {
        if ((event.motion.xrel != 0) || (event.motion.yrel != 0))
          controls_mouse_cursor_moved(event.motion.x, event.motion.y);
      }

      /* mouse wheel (wheel up/down) */
      if (event.type == SDL_MOUSEWHEEL)
      {
        if (event.wheel.y != 0)
          controls_mouse_wheel_moved(event.wheel.y);
      }
#endif
    }

#if 0
    /* make sure the window is not minimized */
    if (G_program_flags & PROGRAM_FLAG_WINDOW_MINIMIZED)
      continue;
#endif

    /* update ticks */
    ticks_current = SDL_GetTicks();

    /* check for tick wraparound (~49 days) */
    if (ticks_current < ticks_last_update)
      ticks_last_update = 0;

    /* check if a new frame has elapsed */
    if ((ticks_current - ticks_last_update) >= (1000 / 60))
    {
#if 0
      /* advance frame */
      loop_advance_frame();

      /* generate samples for this frame */
      frame_generate(ticks_current - ticks_last_update);

      /* send samples to audio output */
      audio_queue_frame();

      /* quit */
      if (G_program_flags & PROGRAM_FLAG_QUIT)
      {
        goto cleanup_all;
      }
#endif

      /* update window */
      vdp_draw_frame();
      video_display_frame();

      /* store this update time */
      ticks_last_update = ticks_current;
    }
  }

  /* cleanup window and quit */
cleanup_all:
#if 0
  audio_deinit();
#endif
cleanup_video:
  video_deinit();
cleanup_sdl:
  SDL_Quit();

  return 0;
}

