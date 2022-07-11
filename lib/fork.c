// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
    // extern pte_t uvpt[];
    pte_t pte;
    pte = uvpt[PGNUM(addr)];
    // cprintf("pid = %08x\n", sys_getenvid());
    if ( (err & FEC_WR) == 0 || (pte & PTE_COW) == 0) {
        panic("pgfault: the faulting access is not write "
              "or the page is not copy-on-write");
    }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	// panic("pgfault not implemented");
    if ((r = sys_page_alloc(0, (void*)PFTEMP, PTE_U | PTE_W | PTE_P)) < 0) 
    {
        panic("pgfault: %e\n", r);
    }
    memcpy((void*)PFTEMP, (void*)ROUNDDOWN(addr, PGSIZE), PGSIZE);
    if ((r = sys_page_map(0, (void*)PFTEMP, 0,(void*)ROUNDDOWN(addr, PGSIZE),
                      PTE_U | PTE_W | PTE_P)) < 0)
    {
        panic("pgfault: %e\n", r);
    }
    if ((r = sys_page_unmap(0, (void*)PFTEMP)) < 0)
    {
        panic("pgfault: %e\n", r);
    }
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	// panic("duppage not implemented");
    // extern pte_t uvpt[];
    struct Env *e;
    void *va = (void*)(pn * PGSIZE);

    if (uvpt[pn] & PTE_W || uvpt[pn] & PTE_COW)
    {
        sys_page_map(0, va, envid, va, PTE_U | PTE_P | PTE_COW);
        sys_page_map(0, va, 0, va, PTE_U | PTE_P | PTE_COW);
    
    }
    else
    {
        sys_page_map(0, va, envid, va, PTE_U | PTE_P);
    }
    
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// panic("fork not implemented");
	extern unsigned char end[];
    extern void _pgfault_upcall(void);

    envid_t envid;
	uintptr_t addr;
    set_pgfault_handler(pgfault);
    int err;

    envid = sys_exofork();

    if (envid == 0)
    {
		thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }
    else if (envid < 0)
    {
        panic("fork: for a new child faild\n");
    }

    // map code and data
    for (addr = UTEXT; addr < (uintptr_t)end; addr += PGSIZE)
        duppage(envid, PGNUM(addr));

    // map normal stack
    duppage(envid, PGNUM((uintptr_t)&addr));

    // alloc page for the new process's exception stack and set _pgfault_upcall.
    if ((err = sys_page_alloc(envid, (void*)(UXSTACKTOP - PGSIZE),
                              PTE_W | PTE_U | PTE_P)) < 0)
    {
        panic("fork: %e\n", err);
    }
    
    // eanbel process envid.
    if ((err = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
    {
        panic("fork: %e\n", err);
    }

    return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
