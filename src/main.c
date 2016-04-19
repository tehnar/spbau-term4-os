#include "pic.h"
#include "interrupt.h"
#include "uart.h"
#include "timer.h"
#include "stdio.h"
#include "misc.h"
#include "memory.h"
#include "balloc.h"
#include "paging.h"
#include "kmem_cache.h"
#include "threads.h"
#include "fs.h"
#include "initramfs.h"
#include "string.h"

void print_dir(char *name) {
    file_t *dir = readdir(name);
    DBG_ASSERT(dir);
    printf("Contents of %s:\n", dir->name);
    for (list_head_t *node = dir->dir_files_head.next; node != &dir->dir_files_head; node = node->next) {
        file_t *file = ((dir_files_t*) node)->file;
        switch (file->type) {
            case FILE:
                printf("FILE name=%s size=%llu\n", file->name, file->size);
                break;
            case DIRECTORY:
                printf("DIRECTORY name=%s\n", file->name);
                break;
            default:
                DBG_ASSERT(0);
        }
    }
    puts("============");            
}

void test_initramfs_contents() {
    print_dir("/");
    print_dir("/test/");
    print_dir("/test/empty_dir/");
    print_dir("/test/tree/");

    file_desc_t *file = open("/test/tree/b/b");
    char buffer[1024];
    DBG_ASSERT(read(file, buffer, 1) == 0); // empty file
    close(file);

    file = open("/test/tree/a/a");
    DBG_ASSERT(read(file, buffer, 1024) == 20);
    DBG_ASSERT(!memcmp(buffer, "aaaaaaaaaaaaaaaaaaaa", 20));
    close(file);

    file = open("/test/long.txt");
    DBG_ASSERT(read(file, buffer, 9) == 9);
    DBG_ASSERT(!memcmp(buffer, "123456789", 9));
    seek(file, 2001);
    DBG_ASSERT(read(file, buffer, 9) == 9);
    DBG_ASSERT(!memcmp(buffer, "23456789\n", 9));
    seek(file, 0);
    DBG_ASSERT(read(file, buffer, 20) == 20);
    DBG_ASSERT(!memcmp(buffer, "123456789\n123456789\n", 20));
    close(file);
}

void fill_file(file_desc_t *desc, char *str, int cnt) {
    size_t len = strlen(str);
    for (int i = 0; i < cnt; i++) {
        DBG_ASSERT(write(desc, str, len) == len);
    }
}

void expect_read(file_desc_t *desc, uint64_t offset, char *str) {
    char buffer[1024];
    size_t len = strlen(str);
    seek(desc, offset);
    DBG_ASSERT(read(desc, buffer, len) == len);
    DBG_ASSERT(!memcmp(str, buffer, len));
}

void test_fs() {
    DBG_ASSERT(mkdir("/1234/"));
    DBG_ASSERT(mkdir("/1234/12345"));    
    file_desc_t *desc;
    DBG_ASSERT(desc = open("/1234/test.txt"));
    fill_file(desc, "12345", 1024);
    close(desc);
    DBG_ASSERT(desc = open("/1234/test.txt"));
    expect_read(desc, 1003, "45123");
    expect_read(desc, 0, "123451234512345");
    close(desc);
    print_dir("/1234/");
    puts("(Expected file /1234/test.txt with size=5120 and directory /1234/12345)");
    DBG_ASSERT(desc = open("/1234/12345/empty"));
    char buffer[1024];
    DBG_ASSERT(!read(desc, buffer, 1024));
    close(desc);
    print_dir("/1234/12345/");
    puts("(Expected file empty with size=0)");
}

void main(void) { 
    pic_init();      
    uart_init();
    idt_init();
    setup_misc();
    setup_memory();
    setup_initramfs();        
    setup_buddy();
    setup_paging();
    setup_alloc();
    threads_init();
    interrupt_enable();
    init_fs();
    read_initramfs();
    
    test_initramfs_contents();
    test_fs();

    puts("Tests completed");
    while (1);
}

