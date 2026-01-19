/******************************************************************************/
/* rom.h (faux game cartridge)                                                */
/******************************************************************************/

#ifndef ROM_H
#define ROM_H

#define ROM_MAX_BYTES (4 * 1024 * 1024) /* 4 MB */

extern unsigned char G_rom_data[ROM_MAX_BYTES];
extern unsigned long G_rom_size;

enum
{
  ROM_FOLDER_PALS = 0, 
  ROM_FOLDER_CELLS, 
  ROM_FOLDER_SPRITES, 
  ROM_FOLDER_TILES, 
  ROM_NUM_FOLDERS
};

/* function declarations */
int rom_clear();
int rom_validate();

int rom_lookup_file(int folder, unsigned short file_number, 
                    unsigned long* rom_addr_cb, unsigned long* num_bytes_cb);

int rom_load(char* filename);

#endif

