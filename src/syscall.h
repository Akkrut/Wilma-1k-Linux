/* libc called in sick. ask the i386 kernel ourselves. */

#ifndef SYSCALL_H
#define SYSCALL_H

/* see if ESC or timer paperwork arrived. */
static inline int sys_read(int fd, void *buf, int count) {
    int ret;
    __asm__ volatile("int $0x80"
        : "=a"(ret)
        : "a"(3), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

/* throw bytes at whatever fd still answers. */
static inline int sys_write(int fd, const void *buf, int count) {
    int ret;
    __asm__ volatile("int $0x80"
        : "=a"(ret)
        : "a"(4), "b"(fd), "c"(buf), "d"(count)
    );
    return ret;
}

/* repaint the whole wall from offset zero. subtle. */
static inline void sys_pwrite64(int fd, const void *buf, int count) {
    __asm__ volatile("int $0x80"
        :
        : "a"(0xb5), "b"(fd), "c"(buf), "d"(count), "S"(0), "D"(0)
        : "memory"
    );
}

/* no return address, no problem. */
static inline void __attribute__((noreturn)) sys_exit(int status) {
    __asm__ volatile("int $0x80"
        :
        : "a"(1), "b"(status)
    );
    __builtin_unreachable();
}

#endif
