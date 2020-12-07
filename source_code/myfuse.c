/* Do not change! */
#define FUSE_USE_VERSION 29
#define _FILE_OFFSET_BITS 64
/******************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fuse.h>

#include "myfilesystem.h"


char * file_data_file_name = NULL;
char * directory_table_file_name = NULL;
char * hash_data_file_name = NULL;

int myfuse_getattr(const char * path, struct stat * result) {

    // MODIFY THIS FUNCTION
    memset(result, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        result->st_mode = S_IFDIR;
    } else {
        result->st_mode = S_IFREG;
		result->st_size = file_size((char*)path,fuse_get_context()->private_data);
    }
    return 0;
}

int myfuse_readdir(const char * path, void * buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {
    // MODIFY THIS FUNCTION
     

    if (strcmp(path, "/") == 0) {
        filler(buf, "test_file", NULL, 0);
    }else{
        filler( buf, path, NULL, 0 ); 
    }
    return 0;
}

int myfuse_unlink(const char * path){
    // FILL OUT

    int result = delete_file((char *)(path+1),fuse_get_context()->private_data);
    return -result;
    
}


int myfuse_rename(const char *oldname, const char *newname){
    // FILL OUT
    int result = rename_file((char *)oldname,(char *)newname,fuse_get_context()->private_data);
    return -result;
}

int myfuse_truncate(const char *path, off_t length){
    // FILL OUT

    int result = resize_file((char *)(path+1),length,fuse_get_context()->private_data);
    return -result;
}

int myfuse_open(const char * path, struct fuse_file_info * fi){
    // FILL OUT
    return 0;
}

int myfuse_read(const char * path, char * buf, size_t length, off_t offset, struct fuse_file_info * fi){
    // FILL OUT
    int result = read_file((char *)(path+1),offset,length,buf,fuse_get_context()->private_data);
    return -result;
}

int myfuse_write(const char * path, const char * buf, size_t length, off_t offset, struct fuse_file_info * fi){
    // FILL OUT

    int result = write_file((char *)(path+1),offset,length,(void *)buf,fuse_get_context()->private_data);
    return -result;
}

int myfuse_release(const char * path, struct fuse_file_info * fi){
    // FILL OUT
}

void * myfuse_init(struct fuse_conn_info * st){
    // FILL OUT
    return init_fs(file_data_file_name, directory_table_file_name,hash_data_file_name, 1);
}

void myfuse_destroy(void * s){
    // FILL OUT
    close_fs(s);
}

int myfuse_create(const char * path, mode_t mode, struct fuse_file_info * fi){
    // FILL OUT
    int result = create_file((char*)(path+1),0,fuse_get_context()->private_data);
    return -result;
}

struct fuse_operations operations = {
    .getattr = myfuse_getattr,
    .readdir = myfuse_readdir,
    .unlink = myfuse_unlink,
    .rename = myfuse_rename,
    .truncate = myfuse_truncate,
    .open = myfuse_open,
    .read = myfuse_read,
    .write = myfuse_write,
    .release = myfuse_release,
    .init = myfuse_init,
    .destroy = myfuse_destroy,
    .create = myfuse_create
    /* FILL OUT BELOW FUNCTION POINTERS
    */
};

int main(int argc, char * argv[]) {
    // MODIFY (OPTIONAL)
    if (argc >= 5) {
        if (strcmp(argv[argc-4], "--files") == 0) {
            file_data_file_name = argv[argc-3];
            directory_table_file_name = argv[argc-2];
            hash_data_file_name = argv[argc-1];
            argc -= 4;
        }
    }
    // After this point, you have access to file_data_file_name, directory_table_file_name and hash_data_file_name
    int ret = fuse_main(argc, argv, &operations, NULL);
    return ret;
}

