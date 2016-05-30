#include "elf.h"
#include "fs.h"
#include "kmem_cache.h"
#include "stdio.h"
#include "string.h"
#include "paging.h"
#include "threads.h"

void elf_entry_wrap_irq(void *arg);


void elf_entry_wrap(void* arg) {
    struct page* stack = alloc_pages(0);

    phys_t pml4_phys = load_pml4();
    pte_t* pml4 = (pte_t*) va(pml4_phys);

    struct pt_iter iter;
    struct pt_iter* slot = pt_iter_set(&(iter), pml4, 0);

    pte_t *pt = slot->pt[slot->level];
    DBG_ASSERT(slot->level == 0);
    phys_t p_stack_addr = page_paddr(stack);
    pt[slot->idx[slot->level]] = p_stack_addr | PTE_PT_FLAGS | PTE_PRESENT;
    store_pml4(pml4_phys);

    uint64_t *stack_begin = get_current_thread()->rsp_start + THREAD_STACK_SIZE;

    *(--stack_begin) = USER_DATA; //ss
    *(--stack_begin) = (uint64_t) ((char*) slot->addr + PAGE_SIZE); //rsp
    *(--stack_begin) = 0; //rflags
    *(--stack_begin) = USER_CODE; //cs
    *(--stack_begin) = (uint64_t) arg; // return address 
}

__asm__("elf_entry_wrap_irq: \
        call elf_entry_wrap; \
        iretq;");

void run_elf(const char *name) {
    file_desc_t *elf = open(name);
    elf_hdr_t header;
    read(elf, (char*) &header, sizeof(elf_hdr_t));
    if (memcmp((char*) header.e_ident, ELF_MAGIC, 4)) {
        printf("Invalid magic: %x%x%x%x", header.e_ident[0], header.e_ident[1], header.e_ident[2], header.e_ident[3]);
        return;
    }
    if (header.e_ident[ELF_CLASS] != ELF_CLASS64) {
        printf("Incorrect class: %d\n", header.e_ident[ELF_CLASS]);
        return;
    } 
    if (header.e_ident[ELF_DATA] != ELF_DATA2LSB) {
        printf("Incorrect order of bytes: %d\n", header.e_ident[ELF_DATA]);
        return;
    }
    if (header.e_type != ELF_EXEC) {
        printf("Incorrect type: %d\n", header.e_type);
        return;
    }


    uint64_t program_header_addr = header.e_phoff;
    for (int i = 0; i < header.e_phnum; i++) {
        seek(elf, program_header_addr);
        elf_phdr_t program_header; 
        read(elf, (char*) &program_header, sizeof(elf_phdr_t));
        program_header_addr += header.e_phentsize;
        if (program_header.p_type != PT_LOAD) 
            continue;

        uint64_t vaddr_start_aligned = (program_header.p_vaddr / PAGE_SIZE) * PAGE_SIZE;
        uint64_t vaddr_end_aligned = ((program_header.p_vaddr + program_header.p_memsz + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

        phys_t pml4_phys = load_pml4();
        pte_t* pml4 = (pte_t*) va(pml4_phys);
        pt_populate_range(pml4, vaddr_start_aligned, vaddr_end_aligned); 

        struct pt_iter iter;
        for_each_slot_in_range(pml4, vaddr_start_aligned, vaddr_end_aligned, iter) {
            DBG_ASSERT(iter.level == 0);
            pte_t *pt = iter.pt[iter.level];
            pt[iter.idx[iter.level]] = page_paddr(alloc_pages(0)) | PTE_PT_FLAGS | PTE_PRESENT;
        }
        
        store_pml4(pml4_phys); 

        seek(elf, program_header.p_offset);
        read(elf, (char*) program_header.p_vaddr, program_header.p_filesz);
    }

    close(elf);

    thread_start(thread_create(&elf_entry_wrap_irq, (void*) header.e_entry));
}
