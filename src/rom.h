/******************************************************************************/
/* rom.h (faux game cartridge)                                                */
/******************************************************************************/

#ifndef ROM_H
#define ROM_H

extern unsigned char G_rom_data[];
extern unsigned long G_rom_size;

/* function declarations */
int rom_clear();
int rom_validate();

int rom_read_file_bytes(unsigned short file_index, 
                        unsigned char* data, unsigned long max_bytes);

int rom_read_file_words(unsigned short file_index, 
                        unsigned short* data, unsigned long max_words);

int rom_load(char* filename);

#endif

