/**
  ******************************************************************************
  * @file    syscalls.c
  * @brief   newlib 系统调用桩（裸机 / 无半主机）。
  *          - _write：把 printf 等输出重定向到 USART1；
  *          - _sbrk：malloc 的堆内存管理；
  *          - 其余为最小桩函数。
  *          若改用其他串口，修改 _write 中的 USART1 即可。
  ******************************************************************************
  */
#include "stm32f10x.h"
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

extern int errno;

/* _write: 输出到 USART1 */
int _write(int file, char *data, int len)
{
    (void)file;
    int n;
    for (n = 0; n < len; n++) {
        USART_SendData(USART1, (uint8_t)data[n]);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) {}
    }
    return len;
}

/* _sbrk: 堆分配，堆底由链接脚本 _end 给出 */
extern char _end;
char *_sbrk(int incr)
{
    static char *heap_end = 0;
    char *prev;
    if (heap_end == 0) {
        heap_end = &_end;
    }
    prev = heap_end;
    heap_end += incr;
    return prev;
}

/* ---- 其余最小桩函数 ---- */
int _read(int file, char *ptr, int len)
{
    (void)file; (void)ptr; (void)len;
    return 0;
}

int _close(int file)
{
    (void)file;
    errno = EBADF;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int offset, int whence)
{
    (void)file; (void)offset; (void)whence;
    return 0;
}

void _exit(int status)
{
    (void)status;
    while (1) {}
}

int _kill(int pid, int sig)
{
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}
