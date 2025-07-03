#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void) { initlock(&tickslock, "time"); }

// set up to take exceptions and traps while in the kernel.
void trapinithart(void) { w_stvec((uint64)kernelvec); }

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void) {
    int which_dev = 0;

    // 判断 trap是来自于用户空间还是内核空间 $sstatus
    // 读 %sstatus 寄存器， 访问模式位
    if ((r_sstatus() & SSTATUS_SPP) != 0)
        panic("usertrap: not from user mode");

    // send interrupts and exceptions to kerneltrap(),
    // since we're now in the kernel.
    // 将kernelvec 函数指针 写入 $stvec 寄存器
    w_stvec((uint64)kernelvec); // $stvec 内核空间 trap 处理代码的位置
    
    // 获取进程控制块
    struct proc *p = myproc(); // $tp

    // save user program counter.
    // 保存 $secp 寄存器数据
    p->trapframe->epc = r_sepc();

    // $scause 判断 进入usertrap原因 8: 系统调用
    // 判断 $scause 寄存器数据 是否为8
    if (r_scause() == 8) {
        // system call
        // 检查用户进程是否被杀掉
        if (killed(p))
            exit(-1);

        // sepc points to the ecall instruction,
        // but we want to return to the next instruction.
        // PC 恢复为下一条指令 ecall之后的一条指令
        p->trapframe->epc += 4;

        // an interrupt will change sepc, scause, and sstatus,
        // so enable only now that we're done with those registers.
        intr_on(); // 有些系统调用需要许多时间处理, 使能中断 
        syscall();
    } else if ((which_dev = devintr()) != 0) {
        // ok
        if (which_dev == 2 && p->in_handler == 0) {
            p->ticks += 1;
            if ((p->ticks == p->interval) && (p->interval != 0)) {
                p->in_handler = 1;
                p->ticks = 0;

                p->saved_epc = p->trapframe->epc;
                p->saved_ra = p->trapframe->ra;
                p->saved_sp = p->trapframe->sp;
                p->saved_gp = p->trapframe->gp;
                p->saved_tp = p->trapframe->tp;

                p->saved_t0 = p->trapframe->t0;
                p->saved_t1 = p->trapframe->t1;
                p->saved_t2 = p->trapframe->t2;
                p->saved_t3 = p->trapframe->t3;
                p->saved_t4 = p->trapframe->t4;
                p->saved_t5 = p->trapframe->t5;
                p->saved_t6 = p->trapframe->t6;

                p->saved_s0 = p->trapframe->s0;
                p->saved_s1 = p->trapframe->s1;
                p->saved_s2 = p->trapframe->s2;
                p->saved_s3 = p->trapframe->s3;
                p->saved_s4 = p->trapframe->s4;
                p->saved_s5 = p->trapframe->s5;
                p->saved_s6 = p->trapframe->s6;
                p->saved_s7 = p->trapframe->s7;
                p->saved_s8 = p->trapframe->s8;
                p->saved_s9 = p->trapframe->s9;
                p->saved_s10 = p->trapframe->s10;
                p->saved_s11 = p->trapframe->s11;
                
                p->saved_a0 = p->trapframe->a0;
                p->saved_a1 = p->trapframe->a1;
                p->saved_a2 = p->trapframe->a2;
                p->saved_a3 = p->trapframe->a3;
                p->saved_a4 = p->trapframe->a4;
                p->saved_a5 = p->trapframe->a5;
                p->saved_a6 = p->trapframe->a6;
                p->saved_a7 = p->trapframe->a7;
                p->trapframe->epc = (uint64)p->handler;
            }
        }
    } else {
        printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
        printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
        setkilled(p);
    }

    if (killed(p))
        exit(-1);

    // give up the CPU if this is a timer interrupt.
    if (which_dev == 2)
        yield();

    usertrapret();
}

//
// return to user space
//
void usertrapret(void) {
    struct proc *p = myproc();

    // we're about to switch the destination of traps from
    // kerneltrap() to usertrap(), so turn off interrupts until
    // we're back in user space, where usertrap() is correct.
    intr_off(); // 关闭中断 防止更新 $stvec 寄存器时候 内核出错

    // send syscalls, interrupts, and exceptions to uservec in trampoline.S
    uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
    w_stvec(trampoline_uservec); // kernelvec --> uservec

    // set up trapframe values that uservec will need when
    // the process next traps into the kernel.
    p->trapframe->kernel_satp = r_satp();         // kernel page table
    p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
    p->trapframe->kernel_trap = (uint64)usertrap; 
    p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

    // set up the registers that trampoline.S's sret will use
    // to get to user space.

    // set S Previous Privilege mode to User.
    unsigned long x = r_sstatus();
    x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
    x |= SSTATUS_SPIE; // enable interrupts in user mode
    w_sstatus(x);

    // set S Exception Program Counter to the saved user pc.
    w_sepc(p->trapframe->epc);

    // tell trampoline.S the user page table to switch to.
    uint64 satp = MAKE_SATP(p->pagetable);

    // jump to userret in trampoline.S at the top of memory, which
    // switches to the user page table, restores user registers,
    // and switches to user mode with sret.
    uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
    ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void kerneltrap() {
    int which_dev = 0;
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();

    if ((sstatus & SSTATUS_SPP) == 0)
        panic("kerneltrap: not from supervisor mode");
    if (intr_get() != 0)
        panic("kerneltrap: interrupts enabled");

    if ((which_dev = devintr()) == 0) {
        printf("scause %p\n", scause);
        printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
        panic("kerneltrap");
    }

    // give up the CPU if this is a timer interrupt.
    if (which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
        yield();

    // the yield() may have caused some traps to occur,
    // so restore trap registers for use by kernelvec.S's sepc instruction.
    w_sepc(sepc);
    w_sstatus(sstatus);
}

void clockintr() {
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt, 定时器中断
// 1 if other device,
// 0 if not recognized.
int devintr() {
    uint64 scause = r_scause();

    if ((scause & 0x8000000000000000L) && (scause & 0xff) == 9) {
        // this is a supervisor external interrupt, via PLIC.

        // irq indicates which device interrupted.
        int irq = plic_claim();

        if (irq == UART0_IRQ) {
            uartintr();
        } else if (irq == VIRTIO0_IRQ) {
            virtio_disk_intr();
        } else if (irq) {
            printf("unexpected interrupt irq=%d\n", irq);
        }

        // the PLIC allows each device to raise at most one
        // interrupt at a time; tell the PLIC the device is
        // now allowed to interrupt again.
        if (irq)
            plic_complete(irq);

        return 1;
    } else if (scause == 0x8000000000000001L) {
        // software interrupt from a machine-mode timer interrupt,
        // forwarded by timervec in kernelvec.S.
        // 软件中断

        if (cpuid() == 0) {
            clockintr();
        }

        // acknowledge the software interrupt by clearing
        // the SSIP bit in sip.
        w_sip(r_sip() & ~2);

        return 2;
    } else {
        return 0;
    }
}
