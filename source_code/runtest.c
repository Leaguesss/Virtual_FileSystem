#include <stdio.h>
#include <string.h>

#define TEST(x) test(x, #x)
#include "myfilesystem.h"
#include "selffunctions.h"

/* You are free to modify any part of this file. The only requirement is that when it is run, all your tests are automatically executed */

/* Some example unit test functions */

int test_contents(char* expect, char* actual) {
    
    FILE * expect_file = fopen(expect,"rb+");
    FILE * actual_file = fopen(actual,"rb+");
    unsigned int expect_size = fsize(expect);
    unsigned int actual_size = fsize(actual);
    
    uint8_t * expect_array = malloc(expect_size);
    uint8_t * actual_array = malloc(actual_size);
    fread(expect_array,expect_size,1,expect_file);
    fread(actual_array,actual_size,1,actual_file);
    
    fclose(expect_file);
    fclose(actual_file);
    
    if(expect_size != actual_size){
        fprintf(stderr,"file:%s expect size: %d but actual size: %d\n",expect,expect_size,actual_size);
        free(expect_array);
        free(actual_array);
        return -1;
    }
    for(unsigned int i = 0;i<expect_size;i++) {
        if(expect_array[i] != actual_array[i]) {
            fprintf(stderr,"file:%s At offset %d differ, expect %d(dec) but actual %d\n",expect,i,expect_array[i],actual_array[i]);
            free(expect_array);
            free(actual_array);
            return -1;
        }
    }
    free(expect_array);
    free(actual_array);
    return 0;
}

int bad_file() {
    void * helper = init_fs("asda", "sdfd", "ggs", 1);
    close_fs(helper);
    if(!helper){
        //fprintf(stderr,"Wrong file input.\n");
        return 0;
    }
    //never reach
    return -1;
}
int no_operation() {
    void * helper = init_fs("testcase/01_f1", "testcase/01_f2", "testcase/01_f3", 1);
    close_fs(helper);
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    int result = test_contents("expect/01_f1","testcase/01_f1");
    if(result!=0){
        return result;
    }
    result = test_contents("expect/01_f2","testcase/01_f2");
    return result;
}
int create_file_success() {
    void * helper = init_fs("testcase/02_f1", "testcase/02_f2", "testcase/02_f3", 1); 
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    int ret = create_file("test_file", 500, helper);
    close_fs(helper);
    
    if(ret == 0) {
        
        return ret;
    }
    
    int result = test_contents("expect/02_f1","testcase/02_f1");
    if(result!=0){
        return result;
    }
    result = test_contents("expect/02_f2","testcase/02_f2");
    if(result!=0){
        return result;
    }
    return ret;
}

int create_file_already_exists() {
    void * helper = init_fs("testcase/02_f1", "testcase/02_f2", "testcase/02_f3", 1);
    
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    int ret = create_file("test_file", 200, helper);
    close_fs(helper);
    
    
    if(ret == 1){
        return 0;
    }
    
    int result = test_contents("expect/02_f1","testcase/02_f1");
    if(result!=0){
        return result;
    }
    result = test_contents("expect/02_f2","testcase/02_f2");
    if(result!=0){
        return result;
    }
    
    return ret;
}
int create_file_nospace(){
    void * helper = init_fs("testcase/01_f1", "testcase/01_f2", "testcase/01_f3", 1);
    
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    int ret = create_file("helasdas", 50000, helper);
    close_fs(helper);
    
    
    if(ret == 2){
        return 0;
    }
    
    int result = test_contents("expect/02_f1","testcase/02_f1");
    if(result!=0){
        return result;
    }
    result = test_contents("expect/02_f2","testcase/02_f2");
    if(result!=0){
        return result;
    }
    
    return ret;
}
int resize_file_success(){
    void * helper = init_fs("testcase/03_f1", "testcase/03_f2", "testcase/03_f3", 1);
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    int ret = resize_file("sssas.txt", 1024, helper);
    close_fs(helper);
    
    if(ret == 0) {
        return 0;
    }
    
    int result = test_contents("expect/03_f1","testcase/03_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/03_f2","testcase/03_f2");
    if(result!=0){
        return result;
    }

    return ret;
}

int resize_file_fail_nospace(){
    void * helper = init_fs("testcase/03_f1", "testcase/03_f2", "testcase/03_f3", 1);
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    int ret = resize_file("sssas.txt", 1025, helper);
    close_fs(helper);
    
    if(ret == 2) {
        return 0;
    }
    
    int result = test_contents("expect/03_f1","testcase/03_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/03_f2","testcase/03_f2");
    if(result!=0){
        return result;
    }
    
    return ret;
}

int resize_file_fail_notfound(){
    void * helper = init_fs("testcase/03_f1", "testcase/03_f2", "testcase/03_f3", 1);
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    int ret = resize_file("asdasd.txt", 1025, helper);
    close_fs(helper);
    
    if(ret == 1) {
        return 0;
    }
    
    int result = test_contents("expect/03_f1","testcase/03_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/03_f2","testcase/03_f2");
    if(result!=0){
        return result;
    }
    
    return ret;
}

int test_repack() {
    void * helper = init_fs("testcase/04_f1", "testcase/04_f2", "testcase/04_f3", 1);
    
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    repack(helper);
    close_fs(helper);
    
    int result = test_contents("expect/04_f1","testcase/04_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/04_f2","testcase/04_f2");
    if(result!=0){
        return result;
    }
    
    return 0;
}

int delete_file_success() {
    void * helper = init_fs("testcase/05_f1", "testcase/05_f2", "testcase/05_f3", 1);
    
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    
    int ret = delete_file("pqwe.txt", helper);
    close_fs(helper);
    
    if(ret == 0) {
        return 0;
    }
    
    int result = test_contents("expect/05_f1","testcase/05_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/05_f2","testcase/05_f2");
    if(result!=0){
        return result;
    }
    
    return 1;
}

int delete_file_notfount() {
    void * helper = init_fs("testcase/05_f1", "testcase/05_f2", "testcase/05_f3", 1);
    
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    
    int ret = delete_file("asdasd.txt", helper);
    close_fs(helper);
    
    if(ret == 1) {
        return 0;
    }
    
    int result = test_contents("expect/05_f1","testcase/05_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/05_f2","testcase/05_f2");
    if(result!=0){
        return result;
    }
    
    return 0;
}
int rename_success() {
    void * helper = init_fs("testcase/03_f1", "testcase/03_f2", "testcase/03_f3", 1);
    
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    
    int ret = rename_file("sssas.txt","hello.txt", helper);
    
    close_fs(helper);
    
    if(ret == 0) {
        return 0;
    }
    
    int result = test_contents("expect/03_f1","testcase/03_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/03_f2","testcase/03_f2");
    if(result!=0){
        return result;
    }
    
    return ret;
}

int rename_fail() {
    void * helper = init_fs("testcase/03_f1", "testcase/03_f2", "testcase/03_f3", 1);
    
    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    
    int ret = rename_file("asdasd.txt","hello.txt", helper);
    
    close_fs(helper);
    
    if(ret == 1) {
        return 0;
    }
    
    int result = test_contents("expect/03_f1","testcase/03_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/03_f2","testcase/03_f2");
    if(result!=0){
        return result;
    }
    
    return ret;
}

int read_file_combo() {
    void * helper = init_fs("testcase/06_f1", "testcase/06_f2", "testcase/06_f3", 1);

    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }

    char buf[20];
    
    int ret3 = read_file("notfount.txt", 5, 50, buf, helper);
    int ret2 = read_file("today.txt", 5, 50, buf, helper);
    int ret = read_file("today.txt", 5, 10, buf, helper);
    close_fs(helper);
    if((ret2 == 2) && (ret == 0) &&(ret3 == 1)) {
        return 0;
    }
    
    int result = test_contents("expect/06_f1","testcase/06_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/06_f2","testcase/06_f2");
    if(result!=0){
        return result;
    }

    result = test_contents("expect/06_f3","testcase/06_f3");
    if(result!=0){
        return result;
    }
    
    return 0;
}

int write_file_combo() {
    void * helper = init_fs("testcase/06_f1", "testcase/06_f2", "testcase/06_f3", 1);

    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    } 

    char buf[] = "testscombo";
    
    int ret = write_file("today.txt", 180, 50, buf, helper); 
    int ret2 = write_file("not found", 18, 2, buf, helper);
    int ret3 = write_file("today.txt", 18, 12345, buf, helper);
    int ret4 = write_file("today.txt", 18, 5, buf, helper);
    close_fs(helper);
    if((ret2 == 1)&& (ret ==2)&&(ret3 == 3) &&(ret4 == 0)) {
        return 0;
    }
    
    int result = test_contents("expect/06_f1","testcase/06_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/06_f2","testcase/06_f2");
    if(result!=0){
        return result;
    }
    
    return 0;
}

int file_size_combo() {
    
    void * helper = init_fs("testcase/06_f1", "testcase/06_f2", "testcase/06_f3", 1); 

    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }
    
    ssize_t ret = file_size("today.txt", helper);
    
    ssize_t ret2 = file_size("not found", helper);
    
    close_fs(helper);
    if((ret2 == -1)&& (ret == 43)) {
        return 0;
    }
    
    int result = test_contents("expect/06_f1","testcase/06_f1");
    if(result!=0){
        return result;
    }
    
    result = test_contents("expect/06_f2","testcase/06_f2");
    if(result!=0){
        return result;
    }
    
    return 0;
    
}

int test_fletcher() {
    uint8_t * temp = (uint8_t*)"thisisforhash";
    uint8_t output[16];
    fletcher(temp,13,output);
    FILE *fp = fopen("testcase/fletcher_value","wb+");
    fwrite(output,1,16,fp);
    fclose(fp);

    int result = test_contents("expect/fletcher_value","testcase/fletcher_value");
    if(result!=0){
        return result;
    }

    return 0;

}

int test_compute_hash_tree() {
    void * helper = init_fs("testcase/07_f1", "testcase/07_f2", "testcase/07_f3", 1); 

    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }

    compute_hash_tree(helper);
    close_fs(helper);

    int result = test_contents("expect/07_f1","testcase/07_f1");
    if(result!=0){
        return result;
    }

    result = test_contents("expect/07_f2","testcase/07_f2");
    if(result!=0){
        return result;
    }

    result = test_contents("expect/07_f3","testcase/07_f3");
    if(result!=0){
        return result;
    }

    return 0;

}

int test_compute_hash_block() {
    void * helper = init_fs("testcase/07_f1", "testcase/07_f2", "testcase/07_f3", 1);

    if(!helper){
        fprintf(stderr,"Wrong file input.\n");
        return -1;
    }



    compute_hash_block(0,helper); //f3 won't change because f1 doesn't change
    compute_hash_block(1,helper);

    close_fs(helper);

    int result = test_contents("expect/07_f1","testcase/07_f1");
    if(result!=0){
        return result;
    }

    result = test_contents("expect/07_f2","testcase/07_f2");
    if(result!=0){
        return result;
    }

    result = test_contents("expect/07_f3","testcase/07_f3");
    if(result!=0){
        return result;
    }

    return 0;


}

int test_hash_read() {
    void * helper = init_fs("testcase/07_f1", "testcase/07_f2", "testcase/07_f3", 1);

    char buf[40];
    int ret = read_file("asdsment.xsxs",2,40,buf,helper);
    close_fs(helper);

    if(ret == 0 ) {
        return 0;
    }

    return ret;


}
int big_combo() {
    void * helper = init_fs("testcase/08_f1", "testcase/08_f2", "testcase/08_f3", 1);
    create_file("hello.txt",10,helper);
    resize_file("today.txt",100,helper); //not found
    create_file("today.txt",100,helper);
    resize_file("today.txt",10,helper);
    delete_file("today.txt",helper);
    char * buf = "thereis14words";
    repack(helper);
    write_file("hello.txt",0,14,buf,helper);

    char compare_buf[14];

    read_file("hello.txt",0,14,compare_buf,helper);
    for(int i = 0;i<14;i++) {
        if(compare_buf[i] != buf[i]) {
            fprintf(stderr,"read or write wrong\n");
            return -1;
        }
    }

    compute_hash_tree(helper);
    compute_hash_block(3,helper);
    close_fs(helper);

    int result = test_contents("expect/08_f1","testcase/08_f1");
    if(result!=0){
        return result;
    }

    result = test_contents("expect/08_f2","testcase/08_f2");
    if(result!=0){
        return result;
    }

    result = test_contents("expect/08_f3","testcase/08_f3");
    if(result!=0){
        return result;
    }
    
    return 0;

}
/****************************/

/* Helper function */
void test(int (*test_function) (), char * function_name) {
    int ret = test_function();
    if (ret == 0) {
        printf("Passed %s\n", function_name);
    } else {
        printf("Failed %s returned %d\n", function_name, ret);
    }
}
/************************/

int main(int argc, char * argv[]) {
    
    // You can use the TEST macro as TEST(x) to run a test function named "x"
    TEST(bad_file);
    TEST(no_operation);
    TEST(create_file_success);
    TEST(create_file_already_exists);
    TEST(create_file_nospace);
    TEST(resize_file_success);
    TEST(resize_file_fail_nospace);
    TEST(resize_file_fail_notfound);
    TEST(test_repack);
    TEST(delete_file_success);
    TEST(delete_file_notfount);
    TEST(rename_success);
    TEST(rename_fail);
    TEST(read_file_combo);
    TEST(write_file_combo);
    TEST(file_size_combo);
    TEST(test_fletcher);
    TEST(test_compute_hash_tree);
    TEST(test_compute_hash_block);
    TEST(test_hash_read);
    TEST(big_combo);
    
    return 0;
}
