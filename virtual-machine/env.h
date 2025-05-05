#pragma once

enum dolly_vm_syscall
{
    DOLLY_SYSCALL_EXIT = 0,
    DOLLY_SYSCALL_PRINT = 1
};

typedef enum dolly_vm_syscall dolly_vm_syscall;
