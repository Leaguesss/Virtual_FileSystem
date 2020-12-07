#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "myfilesystem.h"
#include "selffunctions.h"
pthread_mutex_t create_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resize_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t repack_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t delete_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t read_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t htree_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hblock_mutex = PTHREAD_MUTEX_INITIALIZER;

void * init_fs(char * f1, char * f2, char * f3, int n_processors) {
    struct threefile *tmp = (struct threefile *)malloc(sizeof(struct threefile));
    tmp->f1 = fopen(f1,"rb+");
    tmp->f2 = fopen(f2,"rb+");
    tmp->f3 = fopen(f3,"rb+");
    
    if((tmp->f1 == NULL) || (tmp->f2 == NULL) || (tmp->f3 == NULL)){
        free(tmp);
        return NULL;
    } else{
        tmp->f1_size = fsize(f1);
        tmp->total_blocks = (tmp->f1_size/256*2)-1;
        tmp->f2_size = fsize(f2);
        tmp->levels = 0;
        for(unsigned int i = tmp->f1_size/256;i>=1;i/=2){
            tmp->levels += 1;
        }
    }
    
    return tmp;
}

void close_fs(void * helper) {
    struct threefile *tmp = (struct threefile*)helper;
    if(tmp != NULL){
        fclose(tmp->f1);
        fclose(tmp->f2);
        fclose(tmp->f3);
    }
    free(tmp);
    return;
}


int create_file(char * filename, size_t length, void * helper) {
    pthread_mutex_lock(&create_mutex);
    struct threefile *tmp = (struct threefile*)helper;

    int boolean = find_offset(filename,tmp);
    if(boolean == 1){ //found in the file dir
        pthread_mutex_unlock(&create_mutex);
        return 1;
    }
    unsigned int left_space = caclulate_the_f1_space(tmp);
    if(left_space < length){ // no space overall
        pthread_mutex_unlock(&create_mutex);
        return 2;

    }
    
    FILE* f2 = tmp->f2;
    rewind(f2);
    unsigned int f2_size = tmp->f2_size;
    unsigned int f1_size = tmp->f1_size;
    
    
    uint8_t *copied_f2 = malloc(tmp->f2_size);
	fread(copied_f2,tmp->f2_size,1,f2);

    uint8_t to_clean[72];
    memset(to_clean,0,72);


    unsigned int smallest_offset = f1_size + 1;
    unsigned int smallest_length = f1_size + 1;
    unsigned int position_clean = f2_size + 1;
    unsigned int temp_position = 0;

    while(temp_position<f2_size){ // find the first smallest offset 

        unsigned int temp_length = 0;
        unsigned int temp_offset = 0;
        uint8_t file_exits;
        
        memcpy(&file_exits,copied_f2+temp_position,1);
        
        memcpy(&temp_offset,copied_f2+temp_position+64,4);

        memcpy(&temp_length,copied_f2+temp_position+68,4);

        
        if((file_exits != '\0')&&(temp_offset<smallest_offset)) {
            smallest_offset = temp_offset;
            smallest_length = temp_length;
            position_clean = temp_position;

        }

        temp_position += 72;

    }


    int empty_bool = 0;


    if(smallest_length== (f1_size + 1)){ // f2 is empty
        empty_bool=1;
    } else{
        memcpy(copied_f2+position_clean,to_clean,72);
    }


    
    FILE* file_data = tmp->f1;
    rewind(file_data);
    FILE* fp = fopen(filename,"wb+");
    uint8_t temp[length];
    memset(temp,0,length);
    fwrite(temp , 1 , length , fp );
    fclose(fp);


    unsigned int f2_position = 0;

    while(f2_position<f2_size){ // find the first avaiable slot in dir.
        uint8_t file_exits;
        fseek(f2,f2_position,SEEK_SET);
        fread(&file_exits,1,1,f2);
        if(file_exits == '\0'){
            break;
        }

        f2_position+= 72;
    }

    if(empty_bool==1) {    // if the f2 is  empty
        smallest_offset = 0;

        fseek(f2, 0, SEEK_SET); 
        fwrite(filename,strlen(filename)+1,1,f2); //write filename into f2


        fseek(f2, 64, SEEK_SET);
        fwrite(&smallest_offset,4,1,f2); //write offset into f2

        fseek(f2, 68, SEEK_SET);
        fwrite(&length,4,1,f2); //write file length into f2

        fseek(file_data,smallest_offset,SEEK_SET);
        fwrite(temp , 1 ,length, file_data );
        
        blocks_changed(smallest_offset,smallest_offset+length,tmp); // to caclute the changed blocks
        if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks) {
            compute_hash_tree(tmp);
        }else{
            unsigned int left = tmp->left_block_offset;
            unsigned int right = tmp->right_block_offset;
            compute_hashblock_writein(left,right,tmp);

        }
        

    }else {      // if the f2 is not empty

        if(length <= smallest_offset) {  // check from zero bytes to smallest is enough.
            fseek(f2, f2_position, SEEK_SET); 
            fwrite(filename,strlen(filename)+1,1,f2); //write filename into f2

            fseek(f2, f2_position+64, SEEK_SET);
            int zero = 0;
            fwrite(&zero,4,1,f2); //write offset into f2

            fseek(f2, f2_position+68, SEEK_SET);
            fwrite(&length,4,1,f2); //write file length into f2

            fseek(file_data,smallest_offset,SEEK_SET);
            fwrite(temp , 1 ,length, file_data );
            
            blocks_changed(smallest_offset,smallest_offset+length,tmp); // to caclute the changed blocks
            if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks) {
                compute_hash_tree(tmp);
            }else{
                unsigned int left = tmp->left_block_offset;
                unsigned int right = tmp->right_block_offset;
                compute_hashblock_writein(left,right,tmp);

            }
            
        }else{
            unsigned int smallest_final= f1_size + 1; //smallest final offset
            int no_space  = 0;

            while(1){

                unsigned int smallest_offset2 = f1_size + 1;
                unsigned int smallest_length2 = f1_size + 1;
                unsigned int positionto_clean = f2_size + 1;
                unsigned int temp_position2 = 0;

                while(temp_position2<f2_size){  //find the smallest
                    unsigned int temp_length2 = 0;
                    unsigned int temp_offset2 = 0;
                    uint8_t file_exits;
                    
                    memcpy(&file_exits,copied_f2+temp_position2,1);

                    memcpy(&temp_offset2,copied_f2+temp_position2+64,4);

                    memcpy(&temp_length2,copied_f2+temp_position2+68,4);

                    if((file_exits != '\0')&&(temp_offset2<smallest_offset2)) { // every time update the smaller offset
                        smallest_offset2 = temp_offset2;
                        smallest_length2 = temp_length2;
                        positionto_clean = temp_position2;

                    }

                    temp_position2 += 72;
                }
                if((smallest_length2==(f1_size+1))&& (smallest_offset2 ==(f1_size+1)) ){ //no item find anymore above while loop
                    no_space = 1;
                    break;
                }

                int compare = smallest_offset2 - smallest_offset-smallest_length;

                if(compare >= (unsigned int)length){ // smallest offset and enough space found
                    smallest_final = smallest_offset+smallest_length;
                    break;

                }//else continue while loop this space is not enough
                
        

                memcpy(copied_f2+positionto_clean,to_clean,72);//clear smallest everytime;
                

                smallest_offset = smallest_offset2; //update it 
                smallest_length = smallest_length2;
                

            }

            
            if(no_space==0) { //find enough space to fill in
                
                fseek(f2, f2_position, SEEK_SET); 
                fwrite(filename,strlen(filename)+1,1,f2); //write filename into f2

                fseek(f2, f2_position+68, SEEK_SET);
                fwrite(&length,4,1,f2); //write file length into f2

                fseek(f2, f2_position+64, SEEK_SET);
                fwrite(&smallest_final,4,1,f2); //write offset into f2
                

                fseek(file_data,smallest_final,SEEK_SET);
                fwrite(temp , 1 ,length, file_data ); // write content into f1
                
                blocks_changed(smallest_final,smallest_final+length,tmp); // to caclute the changed blocks
                if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks) {
                    compute_hash_tree(tmp);
                }else{
                    unsigned int left = tmp->left_block_offset;
                    unsigned int right = tmp->right_block_offset;
                    compute_hashblock_writein(left,right,tmp);

                }

            } else{ // otherwise repack it put it at the last 


                repack(tmp);

                fseek(f2, f2_position, SEEK_SET); 
                fwrite(filename,strlen(filename)+1,1,f2); //write filename into f2

                fseek(f2, f2_position+68, SEEK_SET);
                fwrite(&length,4,1,f2); //write file length into f2
                
                fseek(f2, f2_position+64, SEEK_SET);
                fwrite(&tmp->total_f1_space,4,1,f2); //write offset into f2
                
                fseek(file_data,tmp->total_f1_space,SEEK_SET);
                fwrite(temp , 1 ,length, file_data ); // write content into f1

                //combo here
                
                rewind(f2);
                rewind(file_data);
                free(copied_f2);
                
                blocks_changed(tmp->total_f1_space,tmp->total_f1_space+length,tmp); // to caclute the changed blocks
                if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks) {
                    compute_hash_tree(tmp);
                }else{
                    unsigned int left = tmp->left_block_offset;
                    unsigned int right = tmp->right_block_offset;
                    compute_hashblock_writein(left,right,tmp);

                }
                pthread_mutex_unlock(&create_mutex);
                return 0;

            }


        }


    }
    rewind(f2);
    rewind(file_data);
    free(copied_f2);
    pthread_mutex_unlock(&create_mutex);
    return 0;

}


int resize_file(char * filename, size_t length, void * helper) {
    pthread_mutex_lock(&resize_mutex);
    struct threefile *tmp = (struct threefile*)helper;
    unsigned int boolean = find_offset(filename,tmp);

    if(boolean == 0){ // not found return 0 
        pthread_mutex_unlock(&resize_mutex);
        return 1;
    }
    
    
    unsigned int position_offset = tmp->file_offset;
    unsigned int old_length = tmp->file_length;
    unsigned int left_space = caclulate_the_f1_space(tmp);
    left_space += old_length;
    
    if (length > left_space){ //no space overall
        pthread_mutex_unlock(&resize_mutex);
        return 2;
    }

    unsigned int f2_size = tmp->f2_size;
    unsigned int f1_size = tmp->f1_size;
    
    FILE* f1 = tmp->f1;
    rewind(f1);
    FILE* f2 = tmp->f2;
    rewind(f2);

    if(length > old_length){
        unsigned int temp_offset=0;
        unsigned int next_length=0;
        unsigned int next_offset = f1_size+1;
        uint8_t file_exits;
        unsigned int f2_position = 0;
        while(f2_position<f2_size){
            fseek(f2,f2_position,SEEK_SET);
            fread(&file_exits,1,1,f2);
            
            fseek(f2,f2_position+64,SEEK_SET);
            fread(&temp_offset,4,1,f2); //update next_offset

            fseek(f2,f2_position+68,SEEK_SET);
            fread(&next_length,4,1,f2);
            
            if((file_exits != '\0') && (temp_offset>=(position_offset+old_length))){
                if(temp_offset<next_offset){
                    next_offset = temp_offset;
                }
            }

            f2_position += 72;
        }
        
            //no next offset found           // left space is enough to fill new length
        if((next_offset == f1_size+1)&&(f1_size >=(position_offset+length))) {
            // printf("1111\n");
            uint8_t to_clean[length-old_length];
            memset(to_clean,0,length-old_length);
                
            tmp->repack_last_position = position_offset+old_length;
            fseek(f1,position_offset+old_length,SEEK_SET);
            fwrite(to_clean,1,length-old_length,f1); //fill the new space with zero byte


            fseek(f2,tmp->f2_found_position-4,SEEK_SET);  //update f2 length
            fwrite(&length,4,1,f2);
            
            
            rewind(f2);
            rewind(f1);
            
            blocks_changed(position_offset+old_length,position_offset+length,tmp);
            if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks){
                compute_hash_tree(tmp);
            }else{
                unsigned int left = tmp->left_block_offset;
                unsigned int right = tmp->right_block_offset;
                compute_hashblock_writein(left,right,tmp);
            }
            pthread_mutex_unlock(&resize_mutex);
            return 0;

        }
        
        else if((next_offset > (position_offset+length)) && (next_offset != f1_size+1)){
            
            uint8_t to_clean[length-old_length];
            memset(to_clean,0,length-old_length);

            fseek(f1,position_offset+old_length,SEEK_SET);
            fwrite(to_clean,1,length-old_length,f1); //fill the new space with zero byte
            
            fseek(f2,tmp->f2_found_position-4,SEEK_SET);  //update f2 length
            fwrite(&length,4,1,f2);
            
 
            rewind(f2);
            rewind(f1);
            blocks_changed(position_offset+old_length,position_offset+length,tmp);
            if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks){
                compute_hash_tree(tmp);
            }else{
                unsigned int left = tmp->left_block_offset;
                unsigned int right = tmp->right_block_offset;
                compute_hashblock_writein(left,right,tmp);
            }
            pthread_mutex_unlock(&resize_mutex);
            return 0;
        }
        //repack
        
        uint8_t fname[64];
        fseek(f2,tmp->f2_found_position-72,SEEK_SET);
        fread(fname,64,1,f2); //store the name

        uint8_t fcontent[old_length];
        fseek(f1,position_offset,SEEK_SET);
        fread(fcontent,old_length,1,f1); // store the file content


        uint8_t te[72];
        memset(te,0,72);
        fseek(f2,tmp->f2_found_position-72,SEEK_SET);
        fwrite(te,1,72,f2); // clear it in order to repack

        repack(tmp); //repack it

        fseek(f2,tmp->f2_found_position-72,SEEK_SET);
        fwrite(fname,1,64,f2);  // restore the fname


        fseek(f2,tmp->f2_found_position-4,SEEK_SET); // update the length in f2
        fwrite(&length,1,4,f2);

        fseek(f2,tmp->f2_found_position-8,SEEK_SET); // update the offset in f2
        fwrite(&tmp->repack_last_position,1,4,f2);

        fseek(f1,tmp->repack_last_position,SEEK_SET);
        fwrite(fcontent,1,old_length,f1); // restore the fcontent

        uint8_t to_clean[length-old_length];
        memset(to_clean,0,length-old_length);
        
		
        fseek(f1,tmp->repack_last_position+old_length,SEEK_SET);
        fwrite(to_clean,1,length-old_length,f1);   //fill the new space with zero bytes
        tmp->repack_last_position+=old_length;
        
        
        rewind(f2);
        rewind(f1);
        blocks_changed(tmp->repack_last_position,tmp->repack_last_position+length-old_length,tmp);
        if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks){
            compute_hash_tree(tmp);
        }else{
            unsigned int left = tmp->left_block_offset;
            unsigned int right = tmp->right_block_offset;
            compute_hashblock_writein(left,right,tmp);
        }
        pthread_mutex_unlock(&resize_mutex);
        return 0;
        
            

    }
    else {
        fseek(f2,tmp->f2_found_position-4,SEEK_SET);  //update file dir
        fwrite(&length,4,1,f2);
        rewind(f2);
        rewind(f1);
        pthread_mutex_unlock(&resize_mutex);
        return 0;

    }
    
};


void repack(void * helper) {
    pthread_mutex_lock(&repack_mutex);
    struct threefile *tmp = (struct threefile*)helper;
    //Firstly repack the file dic
	
	FILE* f1 =tmp->f1;
    rewind(f1);
    FILE* f2 = tmp->f2;
    rewind(f2);
	
	uint8_t *copied_f2= malloc(tmp->f2_size);
	fread(copied_f2,tmp->f2_size,1,f2);
    
    unsigned int f2_size = tmp->f2_size;
    unsigned int f1_size = tmp->f1_size;
    
    
    uint8_t to_clean[72];
    memset(to_clean,0,72);
    
    unsigned int len = 0;
    while(1) {
        
        unsigned int offset = f1_size+1;
        unsigned int length = f1_size+1;
        unsigned int positioncopied_f2 = 0;
        
        unsigned int position2 = 0;
        
        while(position2<f2_size) {
            unsigned int offset_;
            unsigned int length_;
            uint8_t file_exits;
			
			memcpy(&file_exits,copied_f2+position2,1);

			memcpy(&offset_,copied_f2+position2+64,4);

            memcpy(&length_,copied_f2+position2+68,4);
			
            if((offset_<offset) && (file_exits != '\0')) {  //find the smallest
                offset = offset_;
                length = length_;
                positioncopied_f2 = position2;
            }
            position2 += 72;


        }

        if((offset==(f1_size+1))&&(length ==(f1_size+1))){ // no item find above while loop
            break;
        }
        
		memcpy(copied_f2+positioncopied_f2,to_clean,72);

        fseek(f2,positioncopied_f2+64,SEEK_SET); // update offset in real f2;
        fwrite(&len,1,4,f2);

        uint8_t content[length];
        fseek(f1,offset,SEEK_SET);
        fread(content,length,1,f1); //read contect from  f1,store.
        
        fseek(f1,len,SEEK_SET);        //write content in real f1; restore.
        fwrite(content,1,length,f1);

        len += length; //update length each time, make sure it's compact

    }
    
    free(copied_f2);
    tmp->repack_last_position = len;
   
    rewind(f1);
    rewind(f2);
    
    blocks_changed(0,tmp->repack_last_position,tmp);
    if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks){
        compute_hash_tree(tmp);
    }else{
        unsigned int left = tmp->left_block_offset;
        unsigned int right = tmp->right_block_offset;
        compute_hashblock_writein(left,right,tmp);
    } 
    pthread_mutex_unlock(&repack_mutex);
    return;
}

int delete_file(char * filename, void * helper) {
    pthread_mutex_lock(&delete_mutex);
    struct threefile *tmp = (struct threefile*)helper;
    unsigned int boolean = find_offset(filename,tmp);

    if(boolean==0){
        pthread_mutex_unlock(&delete_mutex);
        return 1;
    }
    
    FILE* f2 = tmp->f2;
    unsigned int filename_position = tmp->f2_found_position;
    filename_position -= 72;

    uint8_t temp[72];
    memset(temp,0,72);
    
    fseek(f2,filename_position,SEEK_SET);
    fwrite(temp,1,72,f2);
    
    pthread_mutex_unlock(&delete_mutex);
    
    return 0;


}

int rename_file(char * oldname, char * newname, void * helper) {
    pthread_mutex_lock(&delete_mutex);
    struct threefile *tmp = (struct threefile*)helper;
    unsigned int boolean = find_offset(newname,tmp);
    if(boolean==1){
        pthread_mutex_unlock(&delete_mutex);
        return 1;
    }
    FILE* f2 = tmp->f2;
    unsigned int h = find_offset(oldname,tmp);
    if(h==0){
        pthread_mutex_unlock(&delete_mutex);
        return 1;
    }
    
    int filename_position = tmp->f2_found_position;
    filename_position -= 72;
    fseek(f2,filename_position,SEEK_SET);

    fwrite(newname,1,strlen(newname)+1,f2);
    pthread_mutex_unlock(&delete_mutex);
    return 0;
}



int read_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
    pthread_mutex_lock(&read_mutex);
    offset = (unsigned int) offset;
    count = (unsigned int) count;
    struct threefile *tmp = (struct threefile*)helper;
    unsigned int boolean = find_offset(filename,tmp);
    
    if (boolean == 0) {
        pthread_mutex_unlock(&read_mutex);
        return 1;
    }

    unsigned int length = tmp->file_length;
    unsigned int offset_position =tmp->file_offset;
    
    if(count+offset>length){
        pthread_mutex_unlock(&read_mutex);
        return 2;
    }
    
    unsigned int left_position = offset_position+offset;
    unsigned int right_position = left_position+count;
    unsigned int left;
    unsigned int right;
    left = left_position/256;
    right =  right_position/256;
	if(right*256!=right_position){
		right += 1;
	}

    for(int i = left;i<right;i++) {
        int result = verify_hashblock(i,tmp);
        if(result ==0) {
            pthread_mutex_unlock(&read_mutex);
            return 3;
        }
    
    }
    FILE* f1 = tmp->f1;
    uint8_t tmep[count];
    fseek(f1,offset_position+offset,SEEK_SET);
    fread(tmep,count,1,f1);
    memcpy(buf,tmep,count);
    
    
    rewind(f1);
    pthread_mutex_unlock(&read_mutex);
    return 0;
    
}
int write_file(char * filename, size_t offset, size_t count, void * buf, void * helper) {
    pthread_mutex_lock(&write_mutex);
    struct threefile *tmp = (struct threefile*)helper;
    //could calclute total in init.
    unsigned int boolean = find_offset(filename,tmp);
    if(boolean == 0){
        pthread_mutex_unlock(&write_mutex);
        return 1;
    }
    
    unsigned int length = tmp->file_length;
    unsigned int offset_position = tmp->file_offset;
    
    if((unsigned int)offset > length){
        pthread_mutex_unlock(&write_mutex);
        return 2;
 
    }
    if(count+offset > length) {
        
        unsigned int left_space = caclulate_the_f1_space(tmp);
        left_space += length;
        if(left_space < offset+count){
            pthread_mutex_unlock(&write_mutex);
            return 3;
        }
        
        FILE* f2 = tmp->f2;
        FILE* f1 = tmp->f1;
        rewind(f2);
        rewind(f1);
        unsigned int f2_position_for_filename = tmp->f2_found_position-72;
        uint8_t fname[64];
        unsigned int fname_offset;
        unsigned int fname_length;
        fseek(f2,f2_position_for_filename,SEEK_SET);
        fread(fname,64,1,f2);
        
        fseek(f2,f2_position_for_filename+64,SEEK_SET);
        fread(&fname_offset,4,1,f2);
        
        fseek(f2,f2_position_for_filename+68,SEEK_SET);
        fread(&fname_length,4,1,f2);
        
        uint8_t file_content[fname_length];
        fseek(f1,fname_offset,SEEK_SET);
        fread(file_content,fname_length,1,f1);
        delete_file(filename,tmp);
        
        repack(tmp);
        
        fseek(f2,f2_position_for_filename,SEEK_SET);  //write fname back
        fwrite(fname,1,64,f2);   
        
        fseek(f2,f2_position_for_filename+64,SEEK_SET); //write offset back
        fwrite(&tmp->repack_last_position,1,4,f2);
        
        fseek(f2,f2_position_for_filename+68,SEEK_SET); // write length back
        unsigned int offcount  = offset+count; 
        fwrite(&offcount,1,4,f2);
        
        fseek(f1,tmp->repack_last_position,SEEK_SET);
        fwrite(file_content,1,offset,f1); //write file content back
		
        fseek(f1,tmp->repack_last_position+offset,SEEK_SET);
        fwrite(buf,1,count,f1);
        
        
        blocks_changed(tmp->repack_last_position,tmp->repack_last_position+offset+count,tmp);
        if((tmp->changed_blocks*tmp->levels) >= tmp->total_blocks){
            compute_hash_tree(tmp);
        }else{
            unsigned int left = tmp->left_block_offset;
            unsigned int right = tmp->right_block_offset;
            compute_hashblock_writein(left,right,tmp);
        }
        
        rewind(f2);
        rewind(f1);
        pthread_mutex_unlock(&write_mutex);
        return 0 ;
    }
    
    FILE* f1 =tmp->f1;
    rewind(f1);
    fseek(f1,offset_position+offset,SEEK_SET); //write buf in f1
    fwrite(buf,1,count,f1);
    
    compute_hash_tree(tmp);

    
    pthread_mutex_unlock(&write_mutex);
    return 0;
}


ssize_t file_size(char * filename, void * helper) {
    struct threefile *tmp = (struct threefile*)helper;
    unsigned int boolean = find_offset(filename,tmp);
    if (boolean==0){
        return -1;  
    }else{
        return tmp->file_length;
    }
}

void fletcher(uint8_t * buf, size_t length, uint8_t * output) {
    uint64_t a = 0;
    uint64_t b = 0;
    uint64_t c = 0;
    uint64_t d = 0;
    size_t new_length;
    if(length%4!=0){
        new_length = length/4+1;
        
    }else{
        new_length = length/4;
    }
    
    uint32_t arr[new_length];
    memset(arr,0,new_length);
    memcpy(arr,buf,length);
    
    for(int i =0;i<length/4;i++) {
        a = (a+ arr[i]) % (((uint64_t)1<<32) -1);
        b = (b+a) % (((uint64_t)1<<32) -1);
        c= (c+b) % (((uint64_t)1<<32) -1);
        d = (d+c) % (((uint64_t)1<<32) -1);
        
    }
    
    memcpy(output,&a,4);
    memcpy(output+4,&b,4);
    memcpy(output+8,&c,4);
    memcpy(output+12,&d,4);

    return;
}

void compute_hash_tree(void * helper) {
    pthread_mutex_lock(&htree_mutex);
    struct threefile *tmp = (struct threefile*)helper;
    FILE* f1 = tmp->f1;
    FILE* f3 = tmp->f3;
    rewind(f1);
    rewind(f3);
    
    unsigned int f1_size = tmp->f1_size;
    unsigned int total_levels = tmp->levels; //levels
    
    uint8_t hashed_value[tmp->total_blocks*16];
    
    uint8_t* buf = malloc(f1_size);
    fread(buf,f1_size,1,f1);
    
    unsigned int f1_position = 0;
    unsigned int position_of_lowest_hash_value = ((1<<(total_levels-1))-1)*16;

    while(f1_position < f1_size){ //calculate the base hashed value
        
        uint8_t buf_t[256];
        memcpy(buf_t,buf+f1_position,256);

        fletcher(buf_t,256,hashed_value+position_of_lowest_hash_value);

        position_of_lowest_hash_value+= 16;
        f1_position += 256;

    }
    free(buf);
    
    unsigned int level = total_levels - 1; // 0 takes one 

    while(level>0) { //calculate the parent hashed value
        unsigned int left_position = ((1<<level)-1)*16;
        unsigned int right_position = ((1<<(level+1))-2)*16;
        unsigned int upper_level_left = ((1<<(level-1))-1)*16;

        for(unsigned int i = left_position;i < right_position;i+=32){

            uint8_t temp[32];
            memcpy(temp,hashed_value+i,32);

            fletcher(temp,32,hashed_value+upper_level_left);
            
            upper_level_left+=16;
        }

        level -= 1;
    }
    
    fwrite(hashed_value,1,tmp->total_blocks*16,f3); //write back
    rewind(f1);
    rewind(f3);
    pthread_mutex_unlock(&htree_mutex);
    return;
}

void compute_hash_block(size_t block_offset, void * helper) {
    //resize, writefile,create_file,repack,write_file,write_file
    pthread_mutex_lock(&hblock_mutex);
    struct threefile *tmp = (struct threefile*)helper;
    FILE* f1 = tmp->f1;
    FILE* f3 = tmp->f3;
    rewind(f1);
    rewind(f3);    
    int total_levels = tmp->levels; //levels

    uint8_t block[256];
    fseek(f1,block_offset*256,SEEK_SET);
    fread(block,256,1,f1);

    uint8_t output[16];
    fletcher(block,256,output);
    
    unsigned int level = total_levels - 1; //including 0 level

    
    unsigned int offset = ((1<<level)-1+block_offset)*16;
    block_offset = offset/16;
    fseek(f3,offset,SEEK_SET);
    fwrite(output,1,16,f3);
    

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
            
            fseek(f3,upper_partent_offset,SEEK_SET);
            fwrite(output,1,16,f3);
            
            block_offset = block_offset/2-1;
            
        } else{
            uint8_t temp[32];
            fseek(f3,offset,SEEK_SET);
            fread(temp,32,1,f3);
            
            uint8_t output[16];
            fletcher(temp,32,output);
            
            fseek(f3,upper_partent_offset,SEEK_SET);
            fwrite(output,1,16,f3);
            
            block_offset = block_offset/2;
            
        }
        level -= 1;
    }
    
    rewind(f1);
    rewind(f3);
    pthread_mutex_unlock(&hblock_mutex);
    return;
}


