/******************************************************************************/
/* rom.h (faux game cartridge)                                                */
/******************************************************************************/

#ifndef ROM_H
#define ROM_H

extern unsigned char G_rom_data[];
extern unsigned long G_rom_size;

enum
{
  ROM_FOLDER_SPRITES = 0, 
  ROM_FOLDER_TILES, 
  ROM_NUM_FOLDERS
};

/* function declarations */
int rom_clear();
int rom_validate();

int rom_lookup_file(int folder_index, unsigned short file_index);

int rom_read_bytes_from_file(unsigned char* data, unsigned long num_bytes);

int rom_read_words_from_file(unsigned short* data, unsigned long num_words);

int rom_load(char* filename);

#endif

