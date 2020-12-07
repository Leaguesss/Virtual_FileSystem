#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "selffunctions.h"
#include "myfilesystem.h"
unsigned int fsize(char * filename){  // given filename, find the size of it
    FILE* fp = fopen(filename,"rb+");
    fseek(fp, 0, SEEK_END);
    unsigned int len1 = ftell(fp);
    fclose(fp);
    return len1;
}


int find_offset(char *filename,void* helper){ // given the filename. find the offset on directory. also could check whether exist or not
    struct threefile *tmp = (struct threefile*)helper;
    FILE* fp = tmp->f2;
    rewind(fp);
    unsigned int position = 0;
    unsigned int f2_size = tmp->f2_size;
    
    while(position < f2_size){ // check file whether is exists and record the offset+length
        char buffer[64];
        fseek(fp,position,SEEK_SET);
        fread(buffer,64,1,fp);
        if(strcmp(buffer,filename)==0){
           
            fseek(fp,position+64,SEEK_SET);
            fread(&tmp->file_offset,4,1,fp);
            fseek(fp,position+68,SEEK_SET);
            fread(&tmp->file_length,4,1,fp);
            tmp->f2_found_position = position+72; //store next position
            rewind(fp);
            return 1;
            }
        position += 72;
        }
    rewind(fp);
    return 0;
}


int caclulate_the_f1_space(void* helper){ //cacluate the left space for f1

    struct threefile *tmp = (struct threefile*)helper;
    FILE* f2 = tmp->f2;
    rewind(f2);
    unsigned int f2_size = tmp->f2_size; 
    unsigned int position = 0;

    unsigned int total = 0;
    
    while(position<f2_size) { 
        char file_exits;
        int s = 0;
        fseek(f2,position,SEEK_SET);
        fread(&file_exits,1,1,f2);
        if(file_exits != '\0'){
            fseek(f2,position+68,SEEK_SET);
            fread(&s,4,1,f2);
        }
        

        total += s;

        position += 72;
    }

    
    tmp->total_f1_space = total;

    unsigned int f1_size = tmp->f1_size;
    rewind(f2);
    return f1_size-total;
   
}
void blocks_changed(unsigned int left_offset, unsigned int right_offset, void * helper){ //check how many blocks changed for f1 
    struct threefile *tmp = (struct threefile*)helper;
    tmp->left_block_offset = left_offset/256;
    unsigned int right = right_offset/256;
    if(right*256 != right_offset) {
        right += 1;
    }
    tmp->right_block_offset = right;
    tmp->changed_blocks = right - tmp->left_block_offset + 1;
  
  
}
void compute_hashblock_writein(unsigned int left,unsigned int right,void * helper){ // write in f3
    struct threefile *tmp = (struct threefile*)helper;
    if(left == right) {
        compute_hash_block(left,tmp);
        return;
    }
    
    for(unsigned int i = left;i<right;i++){
        compute_hash_block(i,tmp);
    }
  
  
}
int verify_hashblock(unsigned int block_offset, void * helper){ // for read use
    struct threefile *tmp = (struct threefile*)helper;

    FILE* f1 = tmp->f1;
    FILE* f3 = tmp->f3;
    rewind(f1);
    rewind(f3);
    

    unsigned int f1_size = tmp->f1_size;
    
    int total_levels = 0; //levels
    for(int i = f1_size/256; i>=1 ; i /= 2){
        total_levels += 1;
    }
    
    uint8_t block[256];
    fseek(f1,block_offset*256,SEEK_SET);
    fread(block,256,1,f1);
    
    uint8_t output[16];
    fletcher(block,256,output);
    
    unsigned int level = total_levels - 1; //including 0 level
    
    unsigned int offset = ((1<<level)-1+block_offset)*16;
    block_offset = offset/16;
    
    uint8_t true_hash[16];
    fseek(f3,offset,SEEK_SET);
    fread(true_hash,16,1,f3);
    
    unsigned int result = memcmp(output, true_hash, 16);
    
    if(result!=0){
        rewind(f1);
        rewind(f3);
        return 0;
    } 
        
    
    while(level>0) {

        unsigned int offset = block_offset*16;
        unsigned int upper_partent_offset;

        if(block_offset%2==0) {
            upper_partent_offset = (block_offset/2-1)*16;
        }else{
            upper_partent_offset = block_offset/2*16;
        }
        
        if(block_offset%2==0){
            
            uint8_t temp[32];
            fseek(f3,offset-16,SEEK_SET);
            fread(temp,32,1,f3);
            
            uint8_t output[16];
            fletcher(temp,32,output);
        
            uint8_t true_hash[16];
            fseek(f3,upper_partent_offset,SEEK_SET);
            fread(true_hash,16,1,f3);
            
            unsigned int result = memcmp(output, true_hash, 16);
            if(result!=0){
                rewind(f1);
                rewind(f3);
                return 0;
            } 
            
            block_offset = block_offset/2-1;
            
        } else{
            
            uint8_t temp[32];
            fseek(f3,offset,SEEK_SET);
            fread(temp,32,1,f3);
            
            uint8_t output[16];
            fletcher(temp,32,output);
            
            uint8_t true_hash[16];
            fseek(f3,upper_partent_offset,SEEK_SET);
            fread(true_hash,16,1,f3);
            
            unsigned int result = memcmp(output, true_hash, 16);
            if(result!=0){
                rewind(f1);
                rewind(f3);
                return 0;
            }
            
            block_offset = block_offset/2;
            
        }
        
        level -= 1;
    }
    rewind(f1);
    rewind(f3);
    return 1;
}

