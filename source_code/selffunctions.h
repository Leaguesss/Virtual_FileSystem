#ifndef SELFFUNCTIONS_H
#define SELFFUNCTIONS_H

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct threefile{ //helper
        FILE * f1;
        FILE * f2;
        FILE * f3;
        unsigned int file_offset; // for resize_file()
        unsigned int file_length; // for resize_file()
        unsigned int f2_found_position; // for resize_file()
        unsigned int total_f1_space; // for caclulate_the_f1_space()
        unsigned int repack_last_position; //for resize last position.
        unsigned int f1_size;  //f1 total size
        unsigned int f2_size; //f2 total size
        unsigned int left_block_offset; // for block or hash tree change
        unsigned int right_block_offset; // for block or hash tree change
        unsigned int changed_blocks; // changed blocks left - right +1. including 0.
        unsigned int total_blocks; // total blocks
        unsigned int levels; //block levels
        

    };

unsigned int fsize(char * filename);  // given filename, find the size of it

int find_offset(char *filename,void* helper); // given the filename. find the offset on directory. also could check whether exist or not


int caclulate_the_f1_space(void* helper); //cacluate the left space for f1

void blocks_changed(unsigned int left_offset, unsigned int right_offset, void * helper); //check how many blocks changed for f1 

void compute_hashblock_writein(unsigned int left,unsigned int right,void * helper); // write in f3
    
int verify_hashblock(unsigned int block_offset, void * helper); // for read use

#endif
