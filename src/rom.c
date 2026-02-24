/******************************************************************************/
/* rom.c (faux game cartridge)                                                */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "rom.h"

/* file table format                          */
/* 1) number of files (2 bytes)               */
/* 2) the file table entries (12 bytes each)  */
/*    a) file address (3 bytes)               */
/*    b) file size (3 bytes)                  */
/*    c) file name (6 bytes)                  */

#define ROM_MAX_FILES 65535

#define ROM_FILE_TABLE_COUNT_BYTES  2

#define ROM_FILE_ENTRY_ADDR_OFFSET  0
#define ROM_FILE_ENTRY_ADDR_BYTES   3

#define ROM_FILE_ENTRY_SIZE_OFFSET  3
#define ROM_FILE_ENTRY_SIZE_BYTES   3

#define ROM_FILE_ENTRY_NAME_OFFSET  6
#define ROM_FILE_ENTRY_NAME_BYTES   6

#define ROM_FILE_TABLE_ENTRY_BYTES  12

#define ROM_FILE_TABLE_SIZE(num_entries)                                       \
  (ROM_FILE_TABLE_COUNT_BYTES + (ROM_FILE_TABLE_ENTRY_BYTES * num_entries))

#define ROM_FILE_ENTRY_LOC(entry_index)                                        \
  (ROM_FILE_TABLE_COUNT_BYTES + (ROM_FILE_TABLE_ENTRY_BYTES * (entry_index)))

#define ROM_FILE_ADDR_LOC(entry_index)                                         \
  (ROM_FILE_ENTRY_LOC(entry_index) + ROM_FILE_ENTRY_ADDR_OFFSET)

#define ROM_FILE_SIZE_LOC(entry_index)                                         \
  (ROM_FILE_ENTRY_LOC(entry_index) + ROM_FILE_ENTRY_SIZE_OFFSET)

#define ROM_FILE_NAME_LOC(entry_index)                                         \
  (ROM_FILE_ENTRY_LOC(entry_index) + ROM_FILE_ENTRY_NAME_OFFSET)

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

  unsigned short num_files;

  unsigned long  data_block_addr;
  unsigned long  data_block_size;

  unsigned long  file_addr;
  unsigned long  file_size;
  unsigned long  file_accum;

  /* make sure rom size is valid */
  if (G_rom_size > ROM_MAX_BYTES)
    return 1;

  /* obtain file table size */
  if (G_rom_size >= ROM_FILE_TABLE_COUNT_BYTES)
  {
    ROM_READ_16BE(num_files, 0)
  }
  else
    return 1;

  /* obtain data block size */
  data_block_addr = ROM_FILE_TABLE_SIZE(num_files);

  if (G_rom_size >= data_block_addr)
    data_block_size = G_rom_size - data_block_addr;
  else
    return 1;

  /* validate file table */
  file_accum = 0;

  for (k = 0; k < num_files; k++)
  {
    ROM_READ_24BE(file_addr, ROM_FILE_ADDR_LOC(k))
    ROM_READ_24BE(file_size, ROM_FILE_SIZE_LOC(k))

    if (file_accum != file_addr)
      return 1;

    if (file_size == 0)
      return 1;

    if (file_addr >= data_block_size)
      return 1;

    file_accum += file_size;
  }

  if (file_accum != data_block_size)
    return 1;

  return 0;
}

/******************************************************************************/
/* rom_read_file_bytes()                                                      */
/******************************************************************************/
int rom_read_file_bytes(unsigned short file_index, 
                        unsigned char* data, unsigned long max_bytes)
{
  unsigned short num_files;

  unsigned long  data_block_addr;
  unsigned long  file_addr;
  unsigned long  file_size;

  /* check input variables */
  if (data == NULL)
    return 1;

  if (max_bytes == 0)
    return 1;

  /* lookup file */
  ROM_READ_16BE(num_files, 0)

  if (file_index >= num_files)
    return 1;

  data_block_addr = ROM_FILE_TABLE_SIZE(num_files);

  ROM_READ_24BE(file_addr, ROM_FILE_ADDR_LOC(file_index))
  ROM_READ_24BE(file_size, ROM_FILE_SIZE_LOC(file_index))

  /* copy the data from the file */
  if (file_size <= max_bytes)
    memcpy(data, &G_rom_data[data_block_addr + file_addr], file_size);
  else
    memcpy(data, &G_rom_data[data_block_addr + file_addr], max_bytes);

#if 0
  printf("Bytes - File Index: %d, Addr: %ld, Size: %ld\n", file_index, file_addr, file_size);
#endif

  return 0;
}

/******************************************************************************/
/* rom_read_file_words()                                                      */
/******************************************************************************/
int rom_read_file_words(unsigned short file_index, 
                        unsigned short* data, unsigned long max_words)
{
  unsigned long  k;

  unsigned short num_files;

  unsigned long  data_block_addr;
  unsigned long  file_addr;
  unsigned long  file_size;

  /* check input variables */
  if (data == NULL)
    return 1;

  if (max_words == 0)
    return 1;

  /* lookup file */
  ROM_READ_16BE(num_files, 0)

  if (file_index >= num_files)
    return 1;

  data_block_addr = ROM_FILE_TABLE_SIZE(num_files);

  ROM_READ_24BE(file_addr, ROM_FILE_ADDR_LOC(file_index))
  ROM_READ_24BE(file_size, ROM_FILE_SIZE_LOC(file_index))

  if ((file_size % 2) != 0)
    return 1;

  /* copy the data from the file */
  if ((file_size / 2) <= max_words)
  {
    for (k = 0; k < (file_size / 2); k++)
    {
      ROM_READ_16BE(data[k], data_block_addr + file_addr + 2 * k)
    }
  }
  else
  {
    for (k = 0; k < max_words; k++)
    {
      ROM_READ_16BE(data[k], data_block_addr + file_addr + 2 * k)
    }
  }

#if 0
  printf("Words - File Index: %d, Addr: %ld, Size: %ld\n", file_index, file_addr, file_size);
#endif

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

