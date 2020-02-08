/* Copyright © 2018-2020 N. Van Bossuyt.                                      */
/* This code is licensed under the MIT License.                               */
/* See: LICENSE.md                                                            */

#include <libsystem/atomic.h>

#include "filesystem/Filesystem.h"
#include "serial.h"
#include "system.h"
#include "x86/Interrupts.h"

#define PORT_COM1 0x3f8

static void wait_write()
{
    while ((in8(PORT_COM1 + 5) & 0x20) == 0)
    { /* do nothing */
    }
}

void serial_putc(char c)
{
    wait_write();
    out8(PORT_COM1, c);
}

int serial_write(const char *buffer, uint size)
{
    atomic_begin();

    for (uint i = 0; i < size; i++)
    {
        serial_putc(buffer[i]);
    }

    atomic_end();

    return size;
}

/* --- Serial device  node -------------------------------------------------- */

static RingBuffer *serial_buffer;

uintptr_t serial_interrupt_handler(uintptr_t current_stack_pointer, InterruptStackFrame *stackframe)
{
    __unused(stackframe);

    char byte = in8(PORT_COM1);

    ringbuffer_write(serial_buffer, &byte, sizeof(byte));

    return current_stack_pointer;
}

bool serial_FsOperationCanRead(FsNode *node, FsHandle *handle)
{
    __unused(node);
    __unused(handle);

    // FIXME: make this atomic or something...
    return !ringbuffer_is_empty(serial_buffer);
}

static error_t serial_FsOperationRead(FsNode *node, FsHandle *handle, void *buffer, size_t size, size_t *readed)
{
    __unused(node);
    __unused(handle);

    // FIXME: use locks
    atomic_begin();
    *readed = ringbuffer_read(serial_buffer, buffer, size);
    atomic_end();

    return ERR_SUCCESS;
}

static error_t serial_FsOperationWrite(FsNode *node, FsHandle *handle, const void *buffer, size_t size, size_t *writen)
{
    __unused(node);
    __unused(handle);

    *writen = serial_write(buffer, size);

    return ERR_SUCCESS;
}

void serial_initialize(void)
{
    out8(PORT_COM1 + 2, 0);
    out8(PORT_COM1 + 3, 0x80);
    out8(PORT_COM1 + 0, 115200 / 9600);
    out8(PORT_COM1 + 1, 0);
    out8(PORT_COM1 + 3, 0x03);
    out8(PORT_COM1 + 4, 0);
    out8(PORT_COM1 + 1, 0x01);

    serial_buffer = ringbuffer_create(1024);

    interrupts_register_irq(4, serial_interrupt_handler);

    FsNode *serial_device = __create(FsNode);
    fsnode_init(serial_device, FSNODE_DEVICE);

    FSNODE(serial_device)->can_read = (FsOperationCanRead)serial_FsOperationCanRead;
    FSNODE(serial_device)->read = (FsOperationRead)serial_FsOperationRead;
    FSNODE(serial_device)->write = (FsOperationWrite)serial_FsOperationWrite;

    Path *serial_device_path = path("/dev/serial");
    filesystem_link_and_take_ref(serial_device_path, serial_device);
    path_delete(serial_device_path);
}