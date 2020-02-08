#pragma once

#include <libsystem/runtime.h>

typedef struct __packed
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t intno, err;
    uint32_t eip, cs, eflags;
} InterruptStackFrame;

typedef uintptr_t (*IRQHandler)(uintptr_t current_stack_pointer, InterruptStackFrame *stackframe);

void interrupts_dump_stackframe(InterruptStackFrame *stackframe);

void interrupts_register_irq(int irq, IRQHandler handler);

void interrupts_initialize(void);
