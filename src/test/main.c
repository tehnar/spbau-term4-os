#include <stdint.h>
 
static void syscall(uint64_t type, uint64_t arg1)
{
    __asm__ ("movq %0, %%rdi\n\r"
             "movq %1, %%rsi\n\r"
             "int $239\n\r"
             :
             : "m"(type), "m"(arg1)
             : "rdi", "rsi", "memory");
}

int main() {
    syscall(1, (uint64_t)"Test\n");
    int x;
    __asm__ volatile ("mov %%cs, %0" : "=a"(x));
    if ((x&3) != 3) 
        syscall(1, (uint64_t) "Oooops, not in userspace!\n");
    else
        syscall(1, (uint64_t) "I'm in userspace!\n");

    while(1);
}
