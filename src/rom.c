/******************************************************************************/
/* rom.c (faux game cartridge)                                                */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rom.h"

#include "vdp.h"

/* big endian read / write macros */

#define ROM_READ_16BE(val, buf)                                                \
  (val) =  (buf[0] << 8) & 0xFF00;                                             \
  (val) |=  buf[1]       & 0x00FF;

#define ROM_READ_24BE(val, buf)                                                \
  (val) =  (buf[0] << 16) & 0xFF0000;                                          \
  (val) |= (buf[1] << 8)  & 0x00FF00;                                          \
  (val) |=  buf[2]        & 0x0000FF;

#define ROM_READ_32BE(val, buf)                                                \
  (val) =  (buf[0] << 24) & 0xFF000000;                                        \
  (val) |= (buf[1] << 16) & 0x00FF0000;                                        \
  (val) |= (buf[2] << 8)  & 0x0000FF00;                                        \
  (val) |=  buf[3]        & 0x000000FF;

/* magic number macros */

#define ROM_MAGIC_IS(c_1, c_2, c_3, c_4)                                       \
  ((magic[0] == c_1) &&                                                        \
   (magic[1] == c_2) &&                                                        \
   (magic[2] == c_3) &&                                                        \
   (magic[3] == c_4))

#define ROM_MAGIC_IS_NOT(c_1, c_2, c_3, c_4)                                   \
  (!(ROM_MAGIC_IS(c_1, c_2, c_3, c_4)))

/******************************************************************************/
/* rom_load()                                                                 */
/******************************************************************************/
int rom_load(char* filename)
{
  unsigned long k;

  FILE* fp;

  char magic[4];

  unsigned char buf[4];

  unsigned long num_words;
  unsigned long num_bytes;

  /* make sure filename is valid */
  if (filename == NULL)
    return 1;

  /* open the rom file */
  fp = fopen(filename, "rb");

  if (fp == NULL)
    return 1;

  /* read cart header */
  if (fread(magic, sizeof(char), 4, fp) < 4)
    return 1;

  if (ROM_MAGIC_IS_NOT('K', 'U', 'N', 'O'))
    return 1;

  if (fread(magic, sizeof(char), 4, fp) < 4)
    return 1;

  if (ROM_MAGIC_IS_NOT('I', 'C', 'H', 'I'))
    return 1;

  if (fread(magic, sizeof(char), 4, fp) < 4)
    return 1;

  if (ROM_MAGIC_IS_NOT('C', 'A', 'R', 'T'))
    return 1;

  /* reset vdp buffers */
  vdp_reset();

  /* read vdp nametable */
  if (fread(buf, sizeof(unsigned char), 3, fp) < 3)
    return 1;

  ROM_READ_24BE(num_words, buf)

  if (num_words > VDP_NAMETABLE_SIZE)
    return 1;

  for (k = 0; k < num_words; k++)
  {
    if (fread(buf, sizeof(unsigned char), 2, fp) < 2)
      return 1;

    ROM_READ_16BE(G_vdp_entries[k], buf)
  }

  /* read vdp palettes */
  if (fread(buf, sizeof(unsigned char), 3, fp) < 3)
    return 1;

  ROM_READ_24BE(num_words, buf)

  if (num_words > VDP_ROM_PALS_SIZE)
    return 1;

  for (k = 0; k < num_words; k++)
  {
    if (fread(buf, sizeof(unsigned char), 2, fp) < 2)
      return 1;

    ROM_READ_16BE(G_vdp_pals[k], buf)
  }

  /* read vdp cells */
  if (fread(buf, sizeof(unsigned char), 3, fp) < 3)
    return 1;

  ROM_READ_24BE(num_bytes, buf)

  if (num_bytes > VDP_ROM_CELLS_SIZE)
    return 1;

  if (fread(G_vdp_cells, sizeof(unsigned char), num_bytes, fp) < num_bytes)
    return 1;

  /* close the file */
  fclose(fp);

  return 0;
}

