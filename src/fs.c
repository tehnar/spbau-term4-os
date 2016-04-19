#include "fs.h"
#include "string.h"
#include "kmem_cache.h"
#include "stdio.h"

static file_t root;

void init_fs() {
    root.type = DIRECTORY;
    root.dir_files_head = LIST_HEAD_INIT(root.dir_files_head);
    create_spinlock(&root.lock);
    root.name[0] = '/';
}

static size_t get_last_separator(char *name) {
    size_t len = strlen(name);
    if (len == 0 || name[0] != '/') {
        return -1;
    }
    len--; // ignoring slash as the last character
    while (1) {
        len--;
        if (name[len] == '/') {
            return len;
        }
    }
}

static file_desc_t *make_file_desc(file_t *file) {
    file_desc_t *desc = (file_desc_t*) kmem_alloc(sizeof(file_desc_t));
    desc->file = file;
    desc->cur_offset = 0;
    desc->cur_chunk = file->chunk_head.next;
    return desc;
}

static file_desc_t *create_file(file_t *dir, char *name) {
    file_t *file = (file_t*) kmem_alloc(sizeof(file_t)); 
    strcpy(file->name, name); 
    size_t len = strlen(name);
    if (name[len - 1] == '/') {
        file->name[len - 1] = 0;
    }
    file->type = FILE;
    file->size = 0;
    create_spinlock(&file->lock);

    file_chunk_t *chunk = (file_chunk_t*) kmem_alloc(sizeof(file_chunk_t));
    file->chunk_head = LIST_HEAD_INIT(file->chunk_head);
    list_add(&chunk->head, &file->chunk_head);

    dir_files_t* node = (dir_files_t*) kmem_alloc(sizeof(dir_files_t));
    node->file = file;
    list_add(&node->head, &dir->dir_files_head); 

    return make_file_desc(file);
}

file_t *readdir(char *name) {
    if (strcmp(name, "/") == 0) {
        return &root;
    }
    int pos = get_last_separator(name);

    if (pos < 0) {
        return NULL;
    }
      
    char parent_dir_name[MAX_FILE_NAME];
    strncpy(parent_dir_name, name, pos + 1);
    parent_dir_name[pos + 1] = 0;
    file_t* parent_dir = readdir(parent_dir_name);

    lock(&parent_dir->lock);
    for (list_head_t* node = list_first(&parent_dir->dir_files_head); 
            node != &parent_dir->dir_files_head; node = node->next) {
        dir_files_t *entry = (dir_files_t*) node;
        if (strcmp(entry->file->name, name) == 0 && entry->file->type == DIRECTORY) {
            unlock(&parent_dir->lock);
            return entry->file;
        } 
    } 
    unlock(&parent_dir->lock);
    return NULL;
}

int mkdir(char *name) {
    int pos = get_last_separator(name);
    if (pos < 0) {
       return 0;
    }

    char parent_dir_name[MAX_FILE_NAME];
    strncpy(parent_dir_name, name, pos + 1);
    parent_dir_name[pos + 1] = 0;
    file_t *parent_dir = readdir(parent_dir_name);
    lock(&parent_dir->lock);

    dir_files_t *node = (dir_files_t*) kmem_alloc(sizeof(dir_files_t));
    list_add((list_head_t*) node, &parent_dir->dir_files_head);

    node->file = (file_t*) kmem_alloc(sizeof(file_t));
    node->file->type = DIRECTORY;
    create_spinlock(&node->file->lock);
    node->file->dir_files_head = LIST_HEAD_INIT(node->file->dir_files_head);
    strcpy(node->file->name, name);
    size_t len = strlen(name);
    if (name[len - 1] != '/') {
        node->file->name[len] = '/';
        node->file->name[len + 1] = 0;
    }
    unlock(&parent_dir->lock);
    return 1;
}

file_desc_t* open(char* name) {
    int pos = get_last_separator(name);
    if (pos < 0) {
        return NULL;
    }

    char parent_dir_name[MAX_FILE_NAME];
    strncpy(parent_dir_name, name, pos + 1);
    parent_dir_name[pos + 1] = 0;
    file_t *parent_dir = readdir(parent_dir_name);
    lock(&parent_dir->lock);

    for (list_head_t* node = list_first(&parent_dir->dir_files_head); 
            node != &parent_dir->dir_files_head; node = node->next) {
        dir_files_t *entry = (dir_files_t*) node;
        if (strcmp(entry->file->name, name) == 0) {
            unlock(&parent_dir->lock);
            return make_file_desc(entry->file);
        } 
    } 
    
    file_desc_t *desc = create_file(parent_dir, name);
    unlock(&parent_dir->lock);
    return desc;
}

void seek(file_desc_t *file, uint64_t offset) {
    lock(&file->file->lock);
    while (file->cur_offset / CHUNK_SIZE > offset / CHUNK_SIZE) {
        file->cur_chunk = file->cur_chunk->prev;        
        size_t offset_change = file->cur_offset % CHUNK_SIZE;
        if (offset_change == 0) {
            offset_change = CHUNK_SIZE;
        } 
        file->cur_offset -= offset_change;
    }    
    while (file->cur_offset / CHUNK_SIZE < offset / CHUNK_SIZE) {
        file->cur_chunk = file->cur_chunk->next;        
        file->cur_offset += CHUNK_SIZE - file->cur_offset % CHUNK_SIZE;
    }    
    file->cur_offset = offset;
    unlock(&file->file->lock);
}

size_t read(file_desc_t *file, char *buffer, size_t count) {
    lock(&file->file->lock);
    size_t read_bytes = 0;
    size_t chunk_offset = file->cur_offset % CHUNK_SIZE;
    while (read_bytes != count && file->cur_offset != file->file->size) {
        file_chunk_t *chunk = (file_chunk_t*) file->cur_chunk;
        *(buffer++) = chunk->data[chunk_offset++];
        file->cur_offset++;
        read_bytes++;
        if (chunk_offset == CHUNK_SIZE) {
            if (file->cur_chunk->next == &file->file->chunk_head) {
                unlock(&file->file->lock);
                return read_bytes; // EOF reached
            }
            file->cur_chunk = file->cur_chunk->next;
            chunk_offset = 0;
        }
    }
    unlock(&file->file->lock);
    return read_bytes;
} 

size_t write(file_desc_t *file, char *buffer, size_t count) {
    lock(&file->file->lock);
    size_t write_bytes = 0;
    size_t chunk_offset = file->cur_offset % CHUNK_SIZE;
    while (write_bytes != count) {
        file_chunk_t *chunk = (file_chunk_t*) file->cur_chunk;
        chunk->data[chunk_offset++] = *(buffer++);
        file->cur_offset++;
        if (file->cur_offset > file->file->size) {
            file->file->size = file->cur_offset;
        }
        write_bytes++;
        if (chunk_offset == CHUNK_SIZE) {
            if (file->cur_chunk->next == &file->file->chunk_head) {
                list_head_t *node = (list_head_t*) kmem_alloc(sizeof(file_chunk_t));
                list_add(node, file->cur_chunk);
            }
            file->cur_chunk = file->cur_chunk->next;
            chunk_offset = 0;
        }
    }
    unlock(&file->file->lock);
    return write_bytes;
}

void close(file_desc_t *desc) {
    kmem_free(desc);
}
