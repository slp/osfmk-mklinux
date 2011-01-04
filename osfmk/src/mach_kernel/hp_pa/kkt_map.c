/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * MkLinux
 */

#include <mach_kdb.h>
#include <kern/assert.h>
#include <kern/lock.h>

#include <mach/boolean.h>

#include <kern/misc_protos.h>
#include <kern/lock.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <kern/ipc_sched.h>
#include <ddb/tr.h>

#include <machine/kkt_map.h>

char *kkt_virt_status;	/* table of status of reserved virtual addresses */
vm_page_t *kkt_virt_vmp; /* table of vm_page's attached to the virtual addresses */
vm_offset_t kkt_virt_start_vaddr;	/* starting virtual address of the range */
unsigned int kkt_virt_free_index = KKT_VIRT_SIZE; /* number of free virtual addr. */
decl_simple_lock_data(,kkt_virt_lock)

/*
 * Ensure mutual exclusion in kkt_do_mappings().
 */
decl_mutex_data(,kkt_mapping_lock)

void 		kkt_map_init(void);

vm_offset_t 	kkt_find_mapping(vm_page_t , vm_offset_t, boolean_t);

void		kkt_do_mappings(void);

void 		kkt_unmap(vm_offset_t);

/*
 * Initializes the range of reserved virtual addresses used by DIPC
 * to map the vm_page's it allocates for messages.
 */
void kkt_map_init(void)
{
  vm_offset_t     vaddr;
  vm_map_entry_t  entry;
  kern_return_t kr;
  unsigned int ind;

  /*
   *      Find space to map vm_page's allocated by DIPC.
   */

  kr = vm_map_find_space(kernel_map, &vaddr, (KKT_VIRT_SIZE * PAGE_SIZE), 
			 (vm_offset_t) 0, &entry);
  if (kr != KERN_SUCCESS) {
    panic("kkt_map_init failed");
  };
  vm_map_unlock(kernel_map);

  kkt_virt_start_vaddr = vaddr;
  kkt_virt_status = (char *) kalloc(KKT_VIRT_SIZE * sizeof(char));
  if (kkt_virt_status == NULL)
    panic("Unable to allocate kkt_virt_status[]");
  kkt_virt_vmp = (vm_page_t *) kalloc(KKT_VIRT_SIZE * sizeof(vm_page_t));
  if (kkt_virt_vmp == NULL)
    panic("Unable to allocate kkt_virt_vmp[]");


  for (ind = 0; ind < KKT_VIRT_SIZE; ind ++) {
    kkt_virt_status[ind] = FREE_VADDR;
    kkt_virt_vmp[ind] = (vm_page_t) NULL;
  }
  simple_lock_init(&kkt_virt_lock, ETAP_DIPC_PREP_FILL);
  mutex_init(&kkt_mapping_lock, ETAP_DIPC_PREP_FILL);

  return;
}

/*
 * Finds a virtual address in the reserved range to map the vm_page.
 * It may block waiting for free entries in the reserved range only
 * if can_block is TRUE.
 */
vm_offset_t kkt_find_mapping(vm_page_t m, vm_offset_t offset, boolean_t can_block)
{
  unsigned int ind_range;
  vm_offset_t virt_addr;

  if (kkt_virt_free_index == 0) {
    if (can_block) {
      simple_lock(&kkt_virt_lock);
      while (kkt_virt_free_index == 0) {
	simple_unlock(&kkt_virt_lock);
	assert_wait((event_t) kkt_virt_status, FALSE);
	thread_block((void (*)(void)) 0);
	simple_lock(&kkt_virt_lock);
      }
      simple_unlock(&kkt_virt_lock);
    }
    else {
      /* XXX we cannot block because of locks held */
      panic("kkt_find_mapping: virtual range full and cannot block\n");
    }
  }

  virt_addr = (vm_offset_t) 0;

  simple_lock(&kkt_virt_lock);
  for (ind_range = 0; ind_range < KKT_VIRT_SIZE; ind_range++) {
    if (kkt_virt_status[ind_range] == FREE_VADDR) {
      kkt_virt_status[ind_range] = MAP_VADDR;
      kkt_virt_vmp[ind_range] = m;
      kkt_virt_free_index--;
      virt_addr = kkt_virt_start_vaddr + (ind_range * PAGE_SIZE);
      break;
    }
  }
  simple_unlock(&kkt_virt_lock);

  assert(virt_addr);

  return(virt_addr + offset);
}

/*
 * Actually maps the vm_page's to the chosen virtual addresses 
 * in the reserved range.
 * It may block during PMAP_ENTER().
 */
void kkt_do_mappings(void)
{
  unsigned int ind_range;
  vm_offset_t virt_addr;
  vm_page_t m;

  mutex_lock(&kkt_mapping_lock);

  simple_lock(&kkt_virt_lock);
  for (ind_range = 0; ind_range < KKT_VIRT_SIZE; ind_range++) {
    if (kkt_virt_status[ind_range] == MAP_VADDR) {
      virt_addr = kkt_virt_start_vaddr + (ind_range * PAGE_SIZE);
      m = kkt_virt_vmp[ind_range];
      kkt_virt_status[ind_range] = USED_VADDR;
      simple_unlock(&kkt_virt_lock);
      PMAP_ENTER(kernel_pmap, virt_addr, m, VM_PROT_READ | VM_PROT_WRITE, FALSE);
      simple_lock(&kkt_virt_lock);
    }
  }
  simple_unlock(&kkt_virt_lock);

  mutex_unlock(&kkt_mapping_lock);

}

/*
 * Unmaps a a vm_page mapped by kkt_do_mappings().
 */
void kkt_unmap(vm_offset_t vaddr)
{
  unsigned int range_index, free_index;

  simple_lock(&kkt_virt_lock);
  range_index = (vaddr - kkt_virt_start_vaddr) / PAGE_SIZE;
  if (kkt_virt_status[range_index] != USED_VADDR)
    printf("kkt_unmap error: virt_addr= %x, range_index= %d\n",
	   vaddr, range_index);
  kkt_virt_status[range_index] = FREE_VADDR ;
  kkt_virt_vmp[range_index] = (vm_page_t) NULL ;
  kkt_virt_free_index++;
  free_index = kkt_virt_free_index;
  simple_unlock(&kkt_virt_lock);

  pmap_remove(kernel_pmap, vaddr, (vaddr + PAGE_SIZE));

  /* 
   * If we are releasing the only available virtual address of the range,
   * wake up the waiting threads.
   */
  if (free_index == 1)
    thread_wakeup((event_t) kkt_virt_status);
}
