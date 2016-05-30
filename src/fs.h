#ifndef FS_H
#define FS_H

#define MAX_FILE_NAME 256
#define CHUNK_SIZE ((1<<12) - sizeof(list_head_t))

#include "list.h"
#include "threads.h"

enum file_type {
    FILE,
    DIRECTORY
};

struct file_chunk {
    list_head_t head;
    char data[CHUNK_SIZE];
};

typedef struct file_chunk file_chunk_t;

struct dir_files;

typedef struct dir_files dir_files_t;

struct file {
    list_head_t chunk_head;
    spinlock_t lock;
    uint64_t size;
    char name[MAX_FILE_NAME]; 
    enum file_type type;

    list_head_t dir_files_head;
};

typedef struct file file_t;

struct file_desc {
    list_head_t *cur_chunk;
    file_t *file;
    uint64_t cur_offset;
};

typedef struct file_desc file_desc_t;

struct dir_files {
    list_head_t head;
    file_t *file;
};


void init_fs();
file_desc_t *open(const char *name);
void close(file_desc_t *file);
void seek(file_desc_t *file, uint64_t offset);
size_t read(file_desc_t *file, char *buffer, size_t count);
size_t write(file_desc_t *file, char *buffer, size_t count);
int mkdir(char *name);
file_t *readdir(char *name);

#endif /* FS_H */
