#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
struct pinfo {
  int ppid;
  int syscall_count;
  int page_usage;
};
extern uint total_syscalls;
extern struct proc proc[NPROC];
uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
uint64
sys_procinfo(void)
{
  uint64 addr;
  struct pinfo info;
  struct proc *p = myproc();
  struct proc *parent;
  
  argaddr(0, &addr);
  
  // Validate addr is not null
  if(addr == 0)
    return -1;
  
  // Fill the pinfo structure
  parent = p->parent;
  
  info.ppid = parent ? parent->pid : 0;
  info.syscall_count = p->syscall_count;
  
  // Calculate memory usage in pages
  info.page_usage = (p->sz + PGSIZE - 1) / PGSIZE;
  
  // Copy info struct to user space
  if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
    return -1;
  
  return 0;
}
struct run {
  struct run *next;
};
int
count_active_procs(void)
{
struct proc *p;
int count = 0;

for(p = proc; p < &proc[NPROC]; p++) {
  acquire(&p->lock);
  if(p->state != UNUSED)
    count++;
  release(&p->lock);
}
return count;
}

uint64
sys_sysinfo(void)
{
int param;

argint(0, &param);

switch(param) {
  case 0:
    return count_active_procs();
  case 1:
    return total_syscalls;
  case 2:
    return count_free_pages();
  default:
    return -1;
}
}