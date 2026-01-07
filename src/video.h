/*******************************************************************************
** video.h (sdl video code)
*******************************************************************************/

#ifndef VIDEO_H
#define VIDEO_H

enum
{
  VIDEO_WINDOW_SIZE_480P = 0, 
  VIDEO_WINDOW_SIZE_600P, 
  VIDEO_WINDOW_SIZE_720P, 
  VIDEO_WINDOW_SIZE_768P, 
  VIDEO_WINDOW_SIZE_1080P, 
  VIDEO_NUM_WINDOW_SIZES 
};

/* function declarations */
short int video_init();
short int video_deinit();

short int video_display_frame();

short int video_increase_window_size();
short int video_decrease_window_size();

#endif

