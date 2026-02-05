/******************************************************************************/
/* rom.c (faux game cartridge)                                                */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "rom.h"

#define ROM_MAX_BYTES (4 * 1024 * 1024) /* 4 MB */

/* file table format:                                     */
/* number of entries (16 bit value) -> 2 bytes            */
/* each entry (24 bit address & size) -> 6 bytes          */
/* addresses are relative (from start of the file table)  */
#define ROM_TABLE_COUNT_BYTES 2
#define ROM_TABLE_ENTRY_BYTES 6
#define ROM_ENTRY_ADDR_OFFSET 0
#define ROM_ENTRY_SIZE_OFFSET 3

#define ROM_FILE_TABLE_SIZE(num_entries)                                       \
  (ROM_TABLE_COUNT_BYTES + (ROM_TABLE_ENTRY_BYTES * num_entries))

#define ROM_FILE_ENTRY_LOC(entry)                                              \
  (ROM_TABLE_COUNT_BYTES + (ROM_TABLE_ENTRY_BYTES * (entry)) + 0)

#define ROM_FILE_ADDR_LOC(entry)                                               \
  (ROM_TABLE_COUNT_BYTES + (ROM_TABLE_ENTRY_BYTES * (entry)) + ROM_ENTRY_ADDR_OFFSET)

#define ROM_FILE_SIZE_LOC(entry)                                               \
  (ROM_TABLE_COUNT_BYTES + (ROM_TABLE_ENTRY_BYTES * (entry)) + ROM_ENTRY_SIZE_OFFSET)

#define ROM_READ_16BE(val, addr)                                               \
  (val) = (G_rom_data[(addr) + 0] << 8) & 0xFF00;                              \
  (val) |= G_rom_data[(addr) + 1] & 0x00FF;

#define ROM_READ_24BE(val, addr)                                               \
  (val) =  (G_rom_data[(addr) + 0] << 16) & 0xFF0000;                          \
  (val) |= (G_rom_data[(addr) + 1] << 8) & 0x00FF00;                           \
  (val) |=  G_rom_data[(addr) + 2] & 0x0000FF;

unsigned char G_rom_data[ROM_MAX_BYTES];
unsigned long G_rom_size;

/* file pointer variables */
static unsigned long S_rom_fp_addr;
static unsigned long S_rom_fp_offset;
static unsigned long S_rom_fp_size;

/******************************************************************************/
/* rom_clear()                                                                */
/******************************************************************************/
int rom_clear()
{
  int k;

  /* zero out the bytes */
  for (k = 0; k < ROM_MAX_BYTES; k++)
    G_rom_data[k] = 0x00;

  G_rom_size = 0;

  /* reset file pointer variables */
  S_rom_fp_addr = 0;
  S_rom_fp_offset = 0;
  S_rom_fp_size = 0;

  return 0;
}

/******************************************************************************/
/* rom_validate()                                                             */
/******************************************************************************/
int rom_validate()
{
  int k;
  int m;

  unsigned short num_folders;
  unsigned short num_files;

  unsigned long folder_addr;
  unsigned long folder_size;

  unsigned long file_addr;
  unsigned long file_size;

  unsigned long rom_accum;
  unsigned long folder_accum;

  /* make sure the file table is accurate */
  ROM_READ_16BE(num_folders, 0)

  if (num_folders != ROM_NUM_FOLDERS)
    return 1;

  /* the rom accumulator adds up the folder sizes,  */
  /* and should end up equaling the size of the rom */
  rom_accum = ROM_FILE_TABLE_SIZE(ROM_NUM_FOLDERS);

  for (k = 0; k < ROM_NUM_FOLDERS; k++)
  {
    ROM_READ_24BE(folder_addr, ROM_FILE_ADDR_LOC(k))
    ROM_READ_24BE(folder_size, ROM_FILE_SIZE_LOC(k))

    if (folder_addr != rom_accum)
      return 1; 

    ROM_READ_16BE(num_files, folder_addr + 0)

    /* the folder accumulator adds up the file sizes,     */
    /* and should end up equaling the size of the folder  */
    folder_accum = ROM_FILE_TABLE_SIZE(num_files);

    for (m = 0; m < num_files; m++)
    {
      ROM_READ_24BE(file_addr, folder_addr + ROM_FILE_ADDR_LOC(m))
      ROM_READ_24BE(file_size, folder_addr + ROM_FILE_SIZE_LOC(m))

      if (file_addr != folder_accum)
        return 1;

      folder_accum += file_size;
    }

    if (folder_accum != folder_size)
      return 1;

    rom_accum += folder_size;
  }

  if (rom_accum != G_rom_size)
    return 1;

  return 0;
}

/******************************************************************************/
/* rom_lookup_file()                                                          */
/******************************************************************************/
int rom_lookup_file(int folder_index, unsigned short file_index)
{
  unsigned long folder_addr;
  unsigned long file_addr;
  unsigned long file_size;

  unsigned short num_files;

  /* check input variables */
  if (folder_index >= ROM_NUM_FOLDERS)
    return 1;

  /* read folder address from top level table */
  ROM_READ_24BE(folder_addr, ROM_FILE_ADDR_LOC(folder_index))

  /* read number of files */
  ROM_READ_16BE(num_files, folder_addr)

  /* make sure the index is valid */
  if (file_index >= num_files) 
    return 1;

  /* get file address and size */
  ROM_READ_24BE(file_addr, folder_addr + ROM_FILE_ADDR_LOC(file_index))
  ROM_READ_24BE(file_size, folder_addr + ROM_FILE_SIZE_LOC(file_index))

  /* set values of file pointer variables */
  S_rom_fp_addr = folder_addr + file_addr;
  S_rom_fp_offset = 0;
  S_rom_fp_size = file_size;

  return 0;
}

/******************************************************************************/
/* rom_read_bytes_from_file()                                                 */
/******************************************************************************/
int rom_read_bytes_from_file(unsigned char* data, unsigned long num_bytes)
{
  /* check input variables and file pointer variables */
  if ((S_rom_fp_addr == 0) || (S_rom_fp_size == 0))
    return 1;

  if (S_rom_fp_offset == S_rom_fp_size)
    return 1;

  if ((S_rom_fp_offset + num_bytes) > S_rom_fp_size)
    return 1;

  if (data == NULL)
    return 1;

  if (num_bytes == 0)
    return 0;

  /* read the bytes */
  memcpy(data, &G_rom_data[S_rom_fp_addr + S_rom_fp_offset], num_bytes);

  /* update file pointer offset */
  S_rom_fp_offset += num_bytes;

  if (S_rom_fp_offset > S_rom_fp_size)
    S_rom_fp_offset = S_rom_fp_size;

  return 0;
}

/******************************************************************************/
/* rom_read_words_from_file()                                                 */
/******************************************************************************/
int rom_read_words_from_file(unsigned short* data, unsigned long num_words)
{
  unsigned long k;

  /* check input variables and file pointer variables */
  if ((S_rom_fp_addr == 0) || (S_rom_fp_size == 0))
    return 1;

  if (S_rom_fp_offset == S_rom_fp_size)
    return 1;

  if ((S_rom_fp_offset + (2 * num_words)) > S_rom_fp_size)
    return 1;

  if (data == NULL)
    return 1;

  if (num_words == 0)
    return 0;

  /* read the words */
  for (k = 0; k < num_words; k++)
  {
    ROM_READ_16BE(data[k], S_rom_fp_addr + S_rom_fp_offset + 2 * k)
  }

  /* update file pointer offset */
  S_rom_fp_offset += 2 * num_words;

  if (S_rom_fp_offset > S_rom_fp_size)
    S_rom_fp_offset = S_rom_fp_size;

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
  if (fread(&G_rom_data[0], sizeof(unsigned char), rom_bytes, fp) < rom_bytes)
    return 1;

  G_rom_size = rom_bytes;

  /* close the file */
  fclose(fp);

  /* make sure the rom is valid */
  if (rom_validate())
    return 1;

  return 0;
}

