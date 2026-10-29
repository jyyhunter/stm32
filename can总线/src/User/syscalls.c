/**
  ******************************************************************************
  * @file    syscalls.c
  * @brief   newlib / newlib-nano 系统调用桩，用于 arm-none-eabi-gcc + PlatformIO。
  *          - _write 重定向到 USART1（Serial_SendByte），使 printf/puts 直接输出；
  *          - _sbrk 提供最小堆（snprintf 等可能需要）；
  *          - 其余为最小桩函数，保证标准库正常链接。
  *          Keil 工程使用 MicroLib（fputc 重定向），不需要本文件；
  *          本文件仅在 PlatformIO 构建中参与编译。
  ******************************************************************************
  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include "Serial.h"

/* ---------------- printf / puts 输出重定向到 USART1 ---------------- */
int _write(int file, char *data, int len)
{
    (void)file;
    int i;
    for (i = 0; i < len; i++)
    {
        Serial_SendByte((uint8_t)data[i]);
    }
    return len;
}

/* ---------------- 堆支持（供 malloc / snprintf 等使用） ---------------- */
extern char _end;                 /* 由链接脚本在 .bss 之后定义 */

void *_sbrk(int incr)
{
    static unsigned char *heap = 0;
    unsigned char *prev_heap;
    register unsigned char *sp __asm("sp");

    if (heap == 0)
    {
        heap = (unsigned char *)&_end;
    }

    prev_heap = heap;
    /* 留出 128 字节，避免堆顶撞穿当前栈指针 */
    if (heap + incr > sp - 128)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap += incr;
    return (void *)prev_heap;
}

/* ---------------- 标准库所需的最小桩函数 ---------------- */
int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

void _exit(int status)
{
    (void)status;
    while (1)
    {
        /* 永不返回 */
    }
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}
