/******************************************************************************/
/* rom.c (faux game cartridge)                                                */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "rom.h"

/* chunk table format                         */
/* 1) number of chunks (2 bytes)              */
/* 2) the chunk table entries (6 bytes each)  */
/*    a) chunk address (3 bytes)              */
/*    b) chunk size (3 bytes)                 */

#define ROM_MAX_CHUNKS 65535

#define ROM_CHUNK_TABLE_COUNT_BYTES  2

#define ROM_CHUNK_ENTRY_ADDR_OFFSET  0
#define ROM_CHUNK_ENTRY_ADDR_BYTES   3

#define ROM_CHUNK_ENTRY_SIZE_OFFSET  3
#define ROM_CHUNK_ENTRY_SIZE_BYTES   3

#define ROM_CHUNK_TABLE_ENTRY_BYTES  6

#define ROM_CHUNK_TABLE_SIZE(num_entries)                                      \
  (ROM_CHUNK_TABLE_COUNT_BYTES + (ROM_CHUNK_TABLE_ENTRY_BYTES * num_entries))

#define ROM_CHUNK_ENTRY_LOC(entry_index)                                       \
  (ROM_CHUNK_TABLE_COUNT_BYTES + (ROM_CHUNK_TABLE_ENTRY_BYTES * (entry_index)))

#define ROM_CHUNK_ADDR_LOC(entry_index)                                        \
  (ROM_CHUNK_ENTRY_LOC(entry_index) + ROM_CHUNK_ENTRY_ADDR_OFFSET)

#define ROM_CHUNK_SIZE_LOC(entry_index)                                        \
  (ROM_CHUNK_ENTRY_LOC(entry_index) + ROM_CHUNK_ENTRY_SIZE_OFFSET)

/* big endian read macros */

#define ROM_READ_BYTE(val, addr)                                               \
  (val) = G_rom_data[(addr) + 0] & 0xFF;

#define ROM_READ_16BE(val, addr)                                               \
  (val) = (G_rom_data[(addr) + 0] << 8) & 0xFF00;                              \
  (val) |= G_rom_data[(addr) + 1] & 0x00FF;

#define ROM_READ_24BE(val, addr)                                               \
  (val) =  (G_rom_data[(addr) + 0] << 16) & 0xFF0000;                          \
  (val) |= (G_rom_data[(addr) + 1] << 8) & 0x00FF00;                           \
  (val) |=  G_rom_data[(addr) + 2] & 0x0000FF;

/* the rom! */

#define ROM_MAX_BYTES (4 * 1024 * 1024) /* 4 MB */

unsigned char G_rom_data[ROM_MAX_BYTES];
unsigned long G_rom_size;

/******************************************************************************/
/* rom_clear()                                                                */
/******************************************************************************/
int rom_clear()
{
  unsigned long k;

  for (k = 0; k < ROM_MAX_BYTES; k++)
    G_rom_data[k] = 0x00;

  G_rom_size = 0;

  return 0;
}

/******************************************************************************/
/* rom_validate()                                                             */
/******************************************************************************/
int rom_validate()
{
  unsigned short k;

  unsigned short num_chunks;

  unsigned long  data_block_addr;
  unsigned long  data_block_size;

  unsigned long  chunk_addr;
  unsigned long  chunk_size;
  unsigned long  chunk_accum;

  /* make sure rom size is valid */
  if (G_rom_size > ROM_MAX_BYTES)
    return 1;

  /* obtain chunk table size */
  if (G_rom_size >= ROM_CHUNK_TABLE_COUNT_BYTES)
  {
    ROM_READ_16BE(num_chunks, 0)
  }
  else
    return 1;

  /* obtain data block size */
  data_block_addr = ROM_CHUNK_TABLE_SIZE(num_chunks);

  if (G_rom_size >= data_block_addr)
    data_block_size = G_rom_size - data_block_addr;
  else
    return 1;

  /* validate chunk table */
  chunk_accum = 0;

  for (k = 0; k < num_chunks; k++)
  {
    ROM_READ_24BE(chunk_addr, ROM_CHUNK_ADDR_LOC(k))
    ROM_READ_24BE(chunk_size, ROM_CHUNK_SIZE_LOC(k))

    if (chunk_accum != chunk_addr)
      return 1;

    if (chunk_size == 0)
      return 1;

    if (chunk_addr >= data_block_size)
      return 1;

    chunk_accum += chunk_size;
  }

  if (chunk_accum != data_block_size)
    return 1;

  return 0;
}

/******************************************************************************/
/* rom_read_chunk_bytes()                                                     */
/******************************************************************************/
int rom_read_chunk_bytes( unsigned short chunk_index, 
                          unsigned char* data, unsigned long max_bytes)
{
  unsigned short num_chunks;

  unsigned long  data_block_addr;
  unsigned long  chunk_addr;
  unsigned long  chunk_size;

  /* check input variables */
  if (data == NULL)
    return 1;

  if (max_bytes == 0)
    return 1;

  /* lookup chunk */
  ROM_READ_16BE(num_chunks, 0)

  if (chunk_index >= num_chunks)
    return 1;

  data_block_addr = ROM_CHUNK_TABLE_SIZE(num_chunks);

  ROM_READ_24BE(chunk_addr, ROM_CHUNK_ADDR_LOC(chunk_index))
  ROM_READ_24BE(chunk_size, ROM_CHUNK_SIZE_LOC(chunk_index))

  /* copy the data from the chunk */
  if (chunk_size <= max_bytes)
    memcpy(data, &G_rom_data[data_block_addr + chunk_addr], chunk_size);
  else
    memcpy(data, &G_rom_data[data_block_addr + chunk_addr], max_bytes);

  return 0;
}

/******************************************************************************/
/* rom_read_chunk_words()                                                     */
/******************************************************************************/
int rom_read_chunk_words( unsigned short chunk_index, 
                          unsigned short* data, unsigned long max_words)
{
  unsigned long  k;

  unsigned short num_chunks;

  unsigned long  data_block_addr;
  unsigned long  chunk_addr;
  unsigned long  chunk_size;

  /* check input variables */
  if (data == NULL)
    return 1;

  if (max_words == 0)
    return 1;

  /* lookup chunk */
  ROM_READ_16BE(num_chunks, 0)

  if (chunk_index >= num_chunks)
    return 1;

  data_block_addr = ROM_CHUNK_TABLE_SIZE(num_chunks);

  ROM_READ_24BE(chunk_addr, ROM_CHUNK_ADDR_LOC(chunk_index))
  ROM_READ_24BE(chunk_size, ROM_CHUNK_SIZE_LOC(chunk_index))

  if ((chunk_size % 2) != 0)
    return 1;

  /* copy the data from the chunk */
  if ((chunk_size / 2) <= max_words)
  {
    for (k = 0; k < (chunk_size / 2); k++)
    {
      ROM_READ_16BE(data[k], data_block_addr + chunk_addr + 2 * k)
    }
  }
  else
  {
    for (k = 0; k < max_words; k++)
    {
      ROM_READ_16BE(data[k], data_block_addr + chunk_addr + 2 * k)
    }
  }

  return 0;
}

/******************************************************************************/
/* rom_load()                                                                 */
/******************************************************************************/
int rom_load(char* filename)
{
  FILE* fp;

  char signature[8];
  char type[4];

  struct stat file_stat;

  unsigned long rom_bytes;

  /* make sure filename is valid */
  if (filename == NULL)
    return 1;

  /* determine rom size */
  if (stat(filename, &file_stat) != 0)
    return 1;

  if (file_stat.st_size <= 12)
    return 1;

  rom_bytes = file_stat.st_size - 12;

  if (rom_bytes > ROM_MAX_BYTES)
    return 1;

  /* open the file */
  fp = fopen(filename, "rb");

  if (fp == NULL)
    return 1;

  /* read cart header */
  if (fread(signature, sizeof(char), 8, fp) < 8)
    return 1;

  if ((signature[0] != 'K') || 
      (signature[1] != 'U') || 
      (signature[2] != 'N') || 
      (signature[3] != 'O') || 
      (signature[4] != 'I') || 
      (signature[5] != 'C') || 
      (signature[6] != 'H') || 
      (signature[7] != 'I'))
  {
    return 1;
  }

  if (fread(type, sizeof(char), 4, fp) < 4)
    return 1;

  if ((type[0] != 'C') || 
      (type[1] != 'A') || 
      (type[2] != 'R') || 
      (type[3] != 'T')) 
  {
    return 1;
  }

  /* read rom data */
  if (fread(G_rom_data, sizeof(unsigned char), rom_bytes, fp) < rom_bytes)
    return 1;

  G_rom_size = rom_bytes;

  /* close the file */
  fclose(fp);

  /* make sure the rom is valid */
  if (rom_validate())
    return 1;

  return 0;
}

