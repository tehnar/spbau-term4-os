#include <stdint.h>
#include <stddef.h>
#include "stdio.h"
#include "uart.h"

static void put(char **buf_ptr, int *len_ptr, char c) {
    if (*len_ptr < 0)
        putc(c);
    else {
        if (*len_ptr == 0)
            return;
        if (*len_ptr == 1)
            **buf_ptr = 0;
        else {
            **buf_ptr = c;
            (*buf_ptr)++;
        }
        (*len_ptr)--;
    }    
}

int puts(const char *s) {
    while (*s) {
        putc(*s);
        s++;
    }    
    putc('\n');
    return 1;
}

static int print_unsigned_number(uintmax_t number, unsigned int base, int pad_with_zeroes, char **buf_ptr, int *len_ptr) {
    if (number == 0) {
        put(buf_ptr, len_ptr, '0');
        return 1;
    }
    uintmax_t m = 1;
    while (number / m >= base) {
        m *= base;
    }
    if (pad_with_zeroes && base == 16) {
        m = 1ull<<(sizeof(uintmax_t) * 8 - 4);
    }        
    int count = 0;
    while (m) {
        count++;
        int x = number / m;
        if (x <= 9)
            put(buf_ptr, len_ptr, x + '0');
        else
            put(buf_ptr, len_ptr, 'a' + x - 10);
        number -= x * m;
        m /= base;            
    }
    return count;
}

static int vprint_number(va_list args, int len_modifier, int base, int is_unsigned, int pad_with_zeroes, char **buf_ptr, int *len_ptr) {
    int count = 0;
    if (!is_unsigned && len_modifier != 3) {
        intmax_t number;
        switch(len_modifier) {
            case -2:
                number = (char) va_arg(args, int);
                break;

            case -1:
                number = (short int) va_arg(args, int);
                break;

            case 0:
                number = va_arg(args, int);
                break;

            case 1:
                number = va_arg(args, long int);
                break;

            case 2:
                number = va_arg(args, long long int);
                break;
        }
        if (number < 0) {
            put(buf_ptr, len_ptr, '-');
            count++;
            number = -number;
        }

        count += print_unsigned_number(number, base, pad_with_zeroes, buf_ptr, len_ptr); 
    } else {
        uintmax_t number;
        switch(len_modifier) {
            case -2:
                number = (unsigned char) va_arg(args, int);
                break;

            case -1:
                number = (unsigned short int) va_arg(args, int);
                break;

            case 0:
                number = va_arg(args, unsigned int);
                break;

            case 1:
                number = va_arg(args, unsigned long int);
                break;

            case 2:
                number = va_arg(args, unsigned long long int);
                break;

            case 3:
                number = va_arg(args, size_t);        
                break;
        }
        count += print_unsigned_number(number, base, pad_with_zeroes, buf_ptr, len_ptr); 
    }
    return count;
}

static int vprint(const char *format, va_list args, char *buf, int len) {
    int count = 0;
    while (*format) {
        char cur_c = *(format++);
        if (cur_c != '%') {
            put(&buf, &len, cur_c);
            count++;
            continue;
        }
        cur_c = *(format++);
        int len_modifier = 0;
        if (cur_c == 'h') {
            len_modifier = -1;                
            if (*format == 'h') {
                format++;
                len_modifier = -2;
            }
            cur_c = *(format++);
        } else if (cur_c == 'l') {
            len_modifier = 1;
            if (*format == 'l') {
                format++;
                len_modifier = 2;
            }
            cur_c = *(format++);
        } else if (cur_c == 'z') {
            len_modifier = 3;
            cur_c = *(format++);
        }
        switch (cur_c) {
            case 'd':
            case 'i':
                count += vprint_number(args, len_modifier, 10, 0, 0, &buf, &len);
                break;
            
            case 'u':
                count += vprint_number(args, len_modifier, 10, 1, 0, &buf, &len);
                break;
            
            case 'o':
                count += vprint_number(args, len_modifier, 8, 1, 0, &buf, &len);
                break;

            case 'x':
                count += vprint_number(args, len_modifier, 16, 1, 0, &buf, &len);
                break;
            
            case 'c':
                ;
                int c = va_arg(args, int); 
                put(&buf, &len, c);
                count++;
                break;

            case 's':
                ;
                char *s = va_arg(args, char*);
                while (*s) {
                    put(&buf, &len, *s);
                    s++;
                    count++;
                }
                break;

            case 'p':
                ;
                void *ptr = va_arg(args, void*);
                count += print_unsigned_number((uint64_t) ptr, 16, 1, &buf, &len);
                break;            
        }
    }
    return count;
}


int vprintf(const char *format, va_list args) {
    return vprint(format, args, NULL, -1);
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprint(format, args, NULL, -1);
    va_end(args);
    return result;
}

int snprintf(char *buf, int size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprint(format, args, buf, size);
    va_end(args);
    return result;
}

int vsnprintf(char *buf, int size, const char *format, va_list args) {
    return vprint(format, args, buf, size);
}
