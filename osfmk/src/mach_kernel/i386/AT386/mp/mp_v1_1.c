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
 */
/*
 * MkLinux
 */

#include <cpus.h>
#include <mach_rt.h>
#include <mach_kdb.h>
#include <mach_ldebug.h>

#include <i386/AT386/mp/mp_v1_1.h>
#include <i386/AT386/mp/mp.h>
#include <i386/AT386/mp/boot.h>
#include <i386/apic.h>
#include <i386/pic.h>
#include <i386/ipl.h>
#include <i386/fpu.h>
#include <i386/pio.h>
#include <i386/cpuid.h>
#include <i386/proc_reg.h>
#include <i386/misc_protos.h>
#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <vm/vm_kern.h>
#include <kern/startup.h>
#include <device/driver_lock.h>

#define MP_DEBUG 1

#if	MP_DEBUG
vm_offset_t	bios_start;
#endif	/* MP_DEBUG */

unsigned int 	lapic_id_initdata = 0;
int 		lapic_id = (int)&lapic_id_initdata;
vm_offset_t 	lapic_start;

#define NIOAPIC 4

vm_offset_t 	ioapic_start[NIOAPIC];
vm_offset_t 	pioapic[NIOAPIC]={0};
unsigned int 	ioapic_index = 1;

#define DISCRETE_APIC		0
#define INTEGRATED_APIC		1

unsigned int	lapic_type = -1;	

extern void set_pic_mask(int);
extern int curr_ipl[NCPUS];

void 		lapic_init(void);
boolean_t 	find_fps(void);
int 		get_ncpus(void);
void 		validate_cpus(int ncpus);
void 		cpu_interrupt(int cpu);
void 		slave_boot(int cpu);
void 		mp_intr(int, int, char *, struct i386_interrupt_state *);
void 		mp_v1_1_error_intr(void);
extern void 	master_up(void);

decl_simple_lock_data(,mp_v1_1_iolock)
volatile int 	mp_v1_1_iocount = 0;
volatile int 	mp_v1_1_iocpu = 0;
boolean_t 	mp_v1_1_initialized = FALSE;

struct MP_FPS_struct 	*MP_FPS;
struct MP_Config_Table 	*MP_CT;

boolean_t 	mp_v1_1_valid_cpus[NCPUS]={FALSE};
boolean_t 	mp_v1_1_PIC_mode = TRUE;

#define INT_VEC_START 	0x50
#define NSPL 		(SPLHI+1)

intr_t 		mp_v1_1_intr[NSPL*NINTR];
int 		mp_v1_1_unit[NSPL*NINTR];
boolean_t 	mp_v1_1_inuse[NSPL*NINTR];
boolean_t 	mp_v1_1_level[NINTR];
int 		ioapic_number = 0;
boolean_t 	ioapic_found = FALSE;
int		cpu_int_word[NCPUS];

#define SPURIOUS_INTERRUPT 0x4F
#define INTERPROCESS_INTERRUPT 0xDE
#define APIC_ERROR_INTERRUPT 0xDF

#if	NCPUS > 1

#define IOLOCK_DEBUG 0
#if	IOLOCK_DEBUG
decl_simple_lock_data(,mp_v1_1_iodebug_lock)
#endif	/* IOLOCK_DEBUG */

int	check_default_configs = 1;

struct mp_config {
	int		id;
	char 		*label;
	int		ncpus;
	vm_offset_t	local_apic;
	vm_offset_t	io_apic;
	unsigned	apic_type;
} mp_configs[] = {

    	MP_ISA_CONF,	
	"Intel MP ISA default configuration",
	2,
	LAPIC_START,
	IOAPIC_START,
	DISCRETE_APIC,

	MP_EISA_1_CONF,	
	"Intel MP EISA default configuration (no IRQ0/DMA_chain)",
	2,
	LAPIC_START,
	IOAPIC_START,
	DISCRETE_APIC,

	MP_EISA_2_CONF,	
	"Intel MP EISA default configuration",
	2,
	LAPIC_START,
	IOAPIC_START,
	DISCRETE_APIC,

	MP_MCA_CONF,	
	"Intel MP MCA default configuration",
	2,
	LAPIC_START,
	IOAPIC_START,
	DISCRETE_APIC,

	MP_ISA_PCI_CONF,	
	"Intel MP ISA+PCI default configuration",
	2,
	LAPIC_START,
	IOAPIC_START,
	INTEGRATED_APIC,
	
	MP_EISA_PCI_CONF,	
	"Intel MP EISA+PCI default configuration",
	2,
	LAPIC_START,
	IOAPIC_START,
	INTEGRATED_APIC,
	
	MP_MCA_PCI_CONF,	
	"Intel MP MCA+PCI default configuration",
	2,
	LAPIC_START,
	IOAPIC_START,
	INTEGRATED_APIC,
	
	MP_PROPRIETARY_CONF,
};

/*
 * We assume that devices have spl raised suffienctly to block themselves
 * from interrupting while holding the io_lock.  We must grab splhigh,
 * though, or else a higher spl device might interrupt a lower holding
 * the io_lock
 */

#if MACH_LDEBUG
int mp_v1_1_iolock_max = 2000000;
#endif	/* MACH_LDEBUG */

extern int get_pc(void);

#if	MACH_RT
DECL_FUNNEL(, mp_v1_1_io_funnel)
boolean_t mp_v1_1_io_funnel_initted = FALSE;
#endif	/* MACH_RT */

boolean_t
mp_v1_1_io_lock(int why, struct processor **saved_processor_p)
{
	int i=0;
	spl_t s;
#if	MACH_RT
	DECL_FUNNEL_VARS
#endif	/* MACH_RT */

#if	MACH_RT
	if (!mp_v1_1_io_funnel_initted) {
		FUNNEL_INIT(&mp_v1_1_io_funnel, master_processor);
		mp_v1_1_io_funnel_initted = TRUE;
	}
	FUNNEL_ENTER(&mp_v1_1_io_funnel);
	*saved_processor_p = funnel_saved_processor;
	return TRUE;
#endif	/* MACH_RT */

      restart:
	while(mp_v1_1_iocount && mp_v1_1_iocpu != cpu_number()){
#if MACH_LDEBUG
		if (i++ == mp_v1_1_iolock_max) {
			i = 0;
			printf("Looping acquiring io_lock\n");
#if	MACH_KDB
			Debugger("mp_v1_1_io_lock");
#endif	/* MACH_KDB */
		}
#endif	/* MACH_LDEBUG */
	}
	s = splhigh();
	simple_lock(&mp_v1_1_iolock);
	if (mp_v1_1_iocount && mp_v1_1_iocpu != cpu_number()) {
		simple_unlock(&mp_v1_1_iolock);
		splx(s);
		goto restart;
	}
#if	IOLOCK_DEBUG
	if (mp_v1_1_iocount == 0)
	    simple_lock(&mp_v1_1_iodebug_lock);
#endif	/* IOLOCK_DEBUG */
	mp_v1_1_iocount++;
	mp_v1_1_iocpu = cpu_number();
	simple_unlock(&mp_v1_1_iolock);
	splx(s);
	return TRUE;
}

void
mp_v1_1_io_unlock(processor_t saved_processor)
{
	spl_t s;
#if	MACH_RT
	DECL_FUNNEL_VARS
#endif	/* MACH_RT */

#if	MACH_RT
	funnel_saved_processor = saved_processor;
	FUNNEL_EXIT(&mp_v1_1_io_funnel);
	return;
#endif	/* MACH_RT */

	s = splhigh();
	simple_lock(&mp_v1_1_iolock);
	assert(mp_v1_1_iocount && mp_v1_1_iocpu == cpu_number());
	mp_v1_1_iocount--;
#if	IOLOCK_DEBUG
	if (mp_v1_1_iocount == 0)
	    simple_unlock(&mp_v1_1_iodebug_lock);
#endif	/* IOLOCK_DEBUG */
	simple_unlock(&mp_v1_1_iolock);
	splx(s);
}

#endif	/* NCPUS > 1 */

boolean_t
mp_v1_1_take_irq(int pic, int unit, int spl, intr_t intr)
{
	int ind;
	unsigned int vec;

	unsigned int *select =(unsigned int *)(ioapic_start[0]+IOAPIC_RSELECT);
	unsigned int *window =(unsigned int *)(ioapic_start[0]+IOAPIC_RWINDOW);

	if (!mp_v1_1_initialized) {
#if	MP_DEBUG
		printf("mp_v1_1_take_irq: %d %d %d 0x%x\n",
		       pic, unit, spl, intr);
#endif	/* MP_DEBUG */
		return (FALSE);
	}
	for(ind = spl*NINTR; ind < spl*NINTR + NINTR; ind++)
	    if (mp_v1_1_inuse[ind]==FALSE) {
		    mp_v1_1_inuse[ind]=TRUE;
		    break;
	    }

	mp_v1_1_intr[ind] = intr;
	mp_v1_1_unit[ind] = unit;

	*select = IOA_R_REDIRECTION+(2*pic)+1;
	mp_disable_preemption();
	*window = (1<<(31-cpu_number()));
	mp_enable_preemption();

	*select = IOA_R_REDIRECTION+(2*pic);
	vec = (ind + INT_VEC_START)|IOA_R_R_DEST_LOGICAL;
	vec |= IOA_R_R_DM_FIXED;
	if (mp_v1_1_level[pic])
	    vec |= IOA_R_R_TM_LEVEL;
	*window = vec;

#if	MP_DEBUG
	printf("take_irq: pic %2d unit %2d spl %d intr 0x%x ind 0x%x\n",
	       pic, unit, spl, intr, ind+INT_VEC_START);
#endif	/* MP_DEBUG */

	return (TRUE);
}

/*
 * Clear current pic settings
 * Return old settings
 */

boolean_t
mp_v1_1_reset_irq(
	int 	pic,
	int 	*unit, 
	int 	*spl, 
	intr_t 	*intr)
{
	int ind;

	unsigned int *select =(unsigned int *)(ioapic_start[0]+IOAPIC_RSELECT);
	unsigned int *window =(unsigned int *)(ioapic_start[0]+IOAPIC_RWINDOW);

	if (!mp_v1_1_initialized) {
		return (FALSE);
	}

	*select = IOA_R_REDIRECTION+(2*pic);
	ind = ((*window) & IOA_R_R_VECTOR_MASK) - INT_VEC_START;

	if (ind <= 0 || !mp_v1_1_inuse[ind]) {
		*spl = 0;
		*intr = (intr_t) intnull;
		*unit = 0;
		return (TRUE);
	}

	*spl = ind/NINTR;
	*intr = mp_v1_1_intr[ind];
	*unit = mp_v1_1_unit[ind];

	mp_v1_1_inuse[ind] = FALSE;
	mp_v1_1_intr[ind]  = 0;
	mp_v1_1_unit[ind]  = 0;
	*select = IOA_R_REDIRECTION+(2*pic);
	*window = IOA_R_R_MASKED;		
	*select = IOA_R_REDIRECTION+(2*pic)+1;
	*window = IOA_R_R_MASKED;		

	return (TRUE);
}

void
mp_v1_1_init(void)
{
	struct mp_conf	*mp;
	kern_return_t 	result;
	vm_map_entry_t 	entry;
	int 		i,j;
	int		config;

#if	MP_DEBUG
	char *IT[4]={
		"Conforming",
		"Edge      ",
		"Bogus     ",
		"Level     "};
	char *IP[4]={
		"Conforming",
		"High      ",
		"Bogus     ",
		"Low       "};
#endif	/* MP_DEBUG */
						 


	printf("MP_V1_1 initialization...\n");

	simple_lock_init(&mp_v1_1_iolock, ETAP_MISC_MP_IO);
#if	IOLOCK_DEBUG
	simple_lock_init(&mp_v1_1_iodebug_lock, ETAP_MISC_MP_IO);
#endif	/* IOLOCK_DEBUG */

	if (!find_fps()) {
		printf("Unable to locate Floating Pointer Structure\n");
		return;
	}

#if	MP_DEBUG
    {
#define B_START 0xFFFE0000
#define B_END   0xFFFFFFFF

	    vm_size_t vsize = round_page(B_END-B_START);
	    vm_offset_t paddr = B_START;
	    vm_offset_t vaddr;

	    bios_start = vm_map_min(kernel_map);
	    result = vm_map_find_space(kernel_map, &bios_start,
				       vsize, 0, &entry);

	    if (result != KERN_SUCCESS) {
		    printf(" vm_map_find_entry failed(%d)\n", result);
		    panic("vm_map_find_entry");
	    };
	    vm_map_unlock(kernel_map);

	    vaddr = bios_start;

	    while (vsize >= I386_PGBYTES) {
		    pmap_enter(pmap_kernel(), vaddr, paddr,
			       VM_PROT_READ|VM_PROT_WRITE, TRUE);
		vaddr += I386_PGBYTES;
		paddr += I386_PGBYTES;
		vsize -= I386_PGBYTES;
	    };
    }
#endif	/* MP_DEBUG */

	assert(round_page(LAPIC_SIZE) == I386_PGBYTES);

	lapic_start  = vm_map_min(kernel_map);

	result = vm_map_find_space(kernel_map, &lapic_start,
				   round_page(LAPIC_SIZE), 0, &entry);

	if (result != KERN_SUCCESS) {
		printf(" vm_map_find_entry failed(%d)\n", result);
		panic("vm_map_find_entry");
	};

	vm_map_unlock(kernel_map);

	assert(NINTR == 16);

	for(i=0;i<NINTR;i++)
	    mp_v1_1_level[i]=FALSE;

	assert(MP_FPS->Length == 1);
	assert(MP_FPS->Spec_Rev == 1);
	if (check_default_configs  && 
	    (config = MP_FPS->Feature[0])) {
		struct mp_config *mpc;

		for (mpc = mp_configs; mpc->id != MP_PROPRIETARY_CONF; mpc++) {
			if (config != mpc->id)
				continue;
			printf("%s\n", mpc->label);
			for (i=0; i < mpc->ncpus; i++)
				mp_v1_1_valid_cpus[i] = TRUE;
			ioapic_found = TRUE;
			lapic_type = mpc->apic_type;
			ioapic_number = mpc->ncpus;
			pioapic[0]= mpc->io_apic;
			pmap_enter(pmap_kernel(), lapic_start, LAPIC_START,
			   VM_PROT_READ|VM_PROT_WRITE, TRUE);
			lapic_id = (int)(lapic_start + LAPIC_ID);
			break;
		}
	}
	if (!ioapic_found) {
		struct MP_Config_EntryP *MP_CEP;
#if	MP_DEBUG
		char *IntType[]={
			"INT   ",
			"NMI   ",
			"SMI   ",
			"ExtINT"};
		char buf[22];
#endif	/* MP_DEBUG */

		if (MP_FPS->Feature[1]&(1<<7)) {
			printf(" IMCRP present:  PIC Mode\n");
		} else {
			printf(" IMCRP not present:  Virtual Wire Mode\n");
			mp_v1_1_PIC_mode = FALSE;
		}
		MP_CT = (struct MP_Config_Table *)phystokv(MP_FPS->Config_Ptr);
		assert(MP_CT->Signature == 0x504d4350);
		assert(MP_CT->Spec_Rev == 1);
#if	MP_DEBUG
		bcopy(MP_CT->OEM, buf, 8);
		bcopy(MP_CT->PROD, &buf[9], 12);
		buf[8]=' ';
		buf[21]=0;
		printf(" MP_FPS 0x%x MP_CT 0x%x %s\n",
		       kvtophys((vm_offset_t)MP_FPS),
		       kvtophys((vm_offset_t)MP_CT), buf);
#endif	/* MP_DEBUG */
		pmap_enter(pmap_kernel(), lapic_start, MP_CT->Local_Apic,
			   VM_PROT_READ|VM_PROT_WRITE, TRUE);

		lapic_id = (int)(lapic_start + LAPIC_ID);
#if 0
		assert(MP_CT->Local_Apic == LAPIC_START);
#endif
		MP_CEP = (struct MP_Config_EntryP *)(MP_CT + 1);

		for(i=0;i<MP_CT->Entries;i++)
		    switch(MP_CEP->Entry_Type) {
		        case MP_CPU_ENTRY: {
				int family = (MP_CEP->CPU_Signature>>8)&0xf;
				int model = (MP_CEP->CPU_Signature>>4)&0xf;
				printf(" CPU%d", MP_CEP->Local_Apic_Id);
				if (MP_CEP->CPU_Flags & 2)
				    printf("[Boot  Processor]");
				else
				    printf("[Slave Processor]");
				if (MP_CEP->CPU_Flags & 1) {
					printf("[Enabled ]");
					assert(!mp_v1_1_valid_cpus[MP_CEP->
							   Local_Apic_Id]);
					mp_v1_1_valid_cpus[MP_CEP->
							   Local_Apic_Id]=TRUE;
				} else {
					printf("[Disabled]");
				}
				switch(family) {
				      case 4: switch (model) {
					    case 0:
					    case 1:printf("486DX\n");break;
					    case 2:printf("486SX\n");break;
					    case 3:printf("486DX2\n");break;
					    case 4:printf("486SL\n");break;
					    case 5:printf("486SX2\n");break;
					    case 8:printf("486DX4\n");break;
					    default:printf("Unknown 486\n");
				      }
					break;
				      case 5: switch (model) {
					    case 1:printf("Pentium 60/66\n");
					      break;
					    case 2:printf("Pentium 90/100\n");
					      break;
					    default:printf("Unknown Pentium\n");
				      }
					break;
				      default:
					printf(" Unknown CPU 0x%x\n",
					       MP_CEP->CPU_Signature);
				}
				if (MP_CEP->Local_Apic_Version & 0x10) {
					assert(lapic_type != DISCRETE_APIC);
					lapic_type = INTEGRATED_APIC;
				} else {
					assert(lapic_type != INTEGRATED_APIC);
					lapic_type = DISCRETE_APIC;
				}
				MP_CEP++;
				break;
			}
			case MP_BUS_ENTRY: {
				struct MP_Config_EntryB *MP_CEB =
				    (struct MP_Config_EntryB *)MP_CEP;
#if	MP_DEBUG
				char buf[7];
				bcopy(MP_CEB->Ident, buf, 6);
				buf[6]=0;
				printf(" Bus%03d %s\n", MP_CEB->Bus_Id, buf);
#endif	/* MP_DEBUG */
				MP_CEP = (struct MP_Config_EntryP *)
				    (MP_CEB + 1);
				break;
			}
			case MP_IO_APIC_ENTRY: {
				struct MP_Config_EntryA *MP_CEA =
				    (struct MP_Config_EntryA *)MP_CEP;
#if	MP_DEBUG
				printf(" I/O Apic%d at 0x%x",
				       MP_CEA->IO_Apic_Id,
				       MP_CEA->IO_Apic_Address);
#endif	/* MP_DEBUG */
				if (MP_CEA->IO_Apic_Address == IOAPIC_START) {
					ioapic_found = TRUE;
					ioapic_number = MP_CEA->IO_Apic_Id;
					pioapic[0]= IOAPIC_START;
#if	MP_DEBUG
					printf("(0)\n");
#endif	/* MP_DEBUG */
				} else {
#if	MP_DEBUG
					printf("(%d)\n",ioapic_index);
#endif	/* MP_DEBUG */
					pioapic[ioapic_index++] = 
					    MP_CEA->IO_Apic_Address;
				}
				MP_CEP = (struct MP_Config_EntryP *)
				    (MP_CEA + 1);
				break;
			}
			case MP_IO_INT_ENTRY: {
				struct MP_Config_EntryI *MP_CEI =
				    (struct MP_Config_EntryI *)MP_CEP;
#if	MP_DEBUG
				printf(" Bus%03d IRQ%02d %s %s %s maps to I/O   Apic%03d Intin%02d\n",
				       MP_CEI->Source_Bus,
				       MP_CEI->Source_IRQ,
				       IntType[MP_CEI->Int_Type],
				       IT[(MP_CEI->Int_Flag>>2)&3],
				       IP[MP_CEI->Int_Flag&3],
				       MP_CEI->Dest_IO_Apic,
				       MP_CEI->Dest_INTIN);
#endif	/* MP_DEBUG */
				if ((MP_CEI->Dest_IO_Apic == ioapic_number) &&
				    (((MP_CEI->Int_Flag>>2)&3) == 3))
				    mp_v1_1_level[MP_CEI->Dest_INTIN]=TRUE;

				MP_CEP = (struct MP_Config_EntryP *)
				    (MP_CEI + 1);
				break;
			}
			case MP_LOC_INT_ENTRY: {
				struct MP_Config_EntryL *MP_CEL =
				    (struct MP_Config_EntryL *)MP_CEP;
#if	MP_DEBUG
				printf(" Bus%03d IRQ%02d %s %s %s maps to Local Apic%03d Intin%02d\n",
				       MP_CEL->Source_Bus,
				       MP_CEL->Source_IRQ,
				       IntType[MP_CEL->Int_Type],
				       IT[(MP_CEL->Int_Flag>>2)&3],
				       IP[MP_CEL->Int_Flag&3],
				       MP_CEL->Dest_Local_Apic,
				       MP_CEL->Dest_INTIN);
#endif	/* MP_DEBUG */
				MP_CEP = (struct MP_Config_EntryP *)
				    (MP_CEL + 1);
				break;
			}
			default:
			  printf("Invalid Configuration Information\n");
			  goto done;
		  }
	      done:;
	}
	if (!ioapic_found)
	    panic("I/O Apic not found at expected address");

	assert(round_page(IOAPIC_SIZE) == I386_PGBYTES);

	for(i=0;i<ioapic_index;i++) {
		ioapic_start[i]  = vm_map_min(kernel_map);

		result = vm_map_find_space(kernel_map, &ioapic_start[i],
					   round_page(IOAPIC_SIZE), 0, &entry);

		if (result != KERN_SUCCESS) {
			printf(" vm_map_find_entry failed(%d)\n", result);
			panic("vm_map_find_entry");
		};

		vm_map_unlock(kernel_map);

		pmap_enter(pmap_kernel(), ioapic_start[i], pioapic[i],
			   VM_PROT_READ|VM_PROT_WRITE, TRUE);
		ioapic_start[i] += (pioapic[i] - trunc_page(pioapic[i]));
	}

	for(i=0;i<NINTR;i++) {
		for(j=0;j<NSPL;j++) {
			mp_v1_1_intr[j*NINTR+i]=(intr_t)intnull;
			mp_v1_1_unit[j*NINTR+i]=0;
			mp_v1_1_inuse[j*NINTR+i]=FALSE;
		}
	}

	/*
	 * APIC_ERROR_INTERRUPT for apic error
	 * INTERPROCESS_INTERRUPT for interprocess interrupts
	 */

	mp_v1_1_inuse[APIC_ERROR_INTERRUPT - INT_VEC_START] = TRUE;
	mp_v1_1_intr[APIC_ERROR_INTERRUPT - INT_VEC_START] = 
	    (intr_t)mp_v1_1_error_intr;

#if	NCPUS > 1
	mp_v1_1_inuse[INTERPROCESS_INTERRUPT - INT_VEC_START] = TRUE;
	mp_v1_1_intr[INTERPROCESS_INTERRUPT - INT_VEC_START] = (intr_t)mp_intr;
#endif	/* NCPUS > 1 */

	/*
	 * Disable all interrupts from PIC for ever
	 */

	set_pic_mask(-1);

	lapic_init();

	mp_v1_1_initialized = TRUE;

	/*
	 * We must setup kdintr first for printfs.
	 * Hardclock is on irq2 on the ioapic instead of 0 on the pic
	 * XXX We should get hardclock mapping from the MP Config Table
	 */

	mp_v1_1_take_irq(1, iunit[1], intpri[1], ivect[1]);

	for(i=3;i<NINTR;i++)
	    if (ivect[i] != (intr_t)intnull)
		mp_v1_1_take_irq(i, iunit[i], intpri[i], ivect[i]);

	validate_cpus(wncpu);

	printf("MP_V1_1 initialization done\n");
	return;
}

#define LF(field) (*((volatile int *)(lapic_start + LAPIC_##field)))
#define LFO(field,off) (*((volatile int *)(lapic_start+LAPIC_##field + (off))))

#define LS(field,value) \
    do { \
	int *r; \
	r = (int *)(lapic_start + LAPIC_##field); \
	*r = (value); \
    } while (0)

void
lapic_init(void)
{
	int pri;
	int error;

	mp_disable_preemption();
	LS(DFR, LAPIC_DFR_FLAT);
	LS(LDR, (1<<(7-cpu_number()))<<LAPIC_LDR_SHIFT);
	pri = (curr_ipl[cpu_number()]<<4) + INT_VEC_START;
	if (pri == INT_VEC_START) pri--;
	LS(TPR, pri);
	LS(SVR,(SPURIOUS_INTERRUPT | LAPIC_SVR_ENABLE));
	LS(LVT_LINT0, LF(LVT_LINT0)|LAPIC_LVT_MASKED);
	LS(ERROR_STATUS,0);		/* Spec says write before read */
	error = LF(ERROR_STATUS);	/* clear it */
	LS(LVT_ERROR, APIC_ERROR_INTERRUPT);
	LS(ICR, LAPIC_ICR_DM_INIT|LAPIC_ICR_TRIGGER_LEVEL|LAPIC_ICR_DSS_ALL);
	mp_enable_preemption();
}

void
mp_v1_1_error_intr(void)
{
	mp_disable_preemption();
	LS(ERROR_STATUS,0);	/* Spec says write before read */
	printf("[%d]APIC Error 0x%x",cpu_number(),LF(ERROR_STATUS));
	mp_enable_preemption();
}

/*
 * The MPFPS is in either the first 1K of the EBDA,  the last KB of base
 * memory, or in the BIOS ROM address space from (0xf0000-0xfffff).
 */

boolean_t
find_fps(void)
{
	vm_offset_t EBDA;
	extern vm_offset_t cnvmem;
	vm_offset_t top_cnv_mem = phystokv(1024*(cnvmem-1));
	vm_offset_t top_ROM = phystokv(0xf0000);

	EBDA = phystokv(16*(vm_offset_t)*((unsigned short *)phystokv(0x40e)));

	for(MP_FPS = (struct MP_FPS_struct *)EBDA;
	    MP_FPS < (struct MP_FPS_struct *)(EBDA+1024);
	    MP_FPS++)
	    if (MP_FPS->Signature == 0x5f504d5f)
		return TRUE;

	for(MP_FPS = (struct MP_FPS_struct *)top_cnv_mem;
	    MP_FPS < (struct MP_FPS_struct *)(top_cnv_mem+1024);
	    MP_FPS++)
	    if (MP_FPS->Signature == 0x5f504d5f)
		return TRUE;

	for(MP_FPS = (struct MP_FPS_struct *)top_ROM;
	    MP_FPS < (struct MP_FPS_struct *)(top_ROM+0xffff);
	    MP_FPS++)
	    if (MP_FPS->Signature == 0x5f504d5f)
		return TRUE;

	return FALSE;
}

void
cpu_interrupt(
	int	cpu)
{

	if (mp_v1_1_initialized) {
		LS(ICRD, cpu<<LAPIC_ICRD_DEST_SHIFT);
		LS(ICR, INTERPROCESS_INTERRUPT|LAPIC_ICR_DM_FIXED);
	}
}

#if	NCPUS > 1

void
mp_intr(
	int				vec,
	int				old_ipl,
	char				*ret_addr, /* ret addr in  handler */
	struct i386_interrupt_state	*regs)
{
	register mycpu;
	volatile int	*my_word;
#if	MACH_KDB && MACH_ASSERT
	int i=100;
#endif	/* MACH_KDB && MACH_ASSERT */

	mp_disable_preemption();

	mycpu = cpu_number();
	my_word = &cpu_int_word[mycpu];

	do {
#if	MACH_KDB && MACH_ASSERT
		if (i-- <= 0)
		    Debugger("mp_intr");
#endif	/* MACH_KDB && MACH_ASSERT */
		if (i_bit(MP_CLOCK, my_word)) {
			i_bit_clear(MP_CLOCK, my_word);
			hardclock(vec, old_ipl, ret_addr, regs);
		} else if (i_bit(MP_TLB_FLUSH, my_word)) {
			i_bit_clear(MP_TLB_FLUSH, my_word);
			pmap_update_interrupt();
		} else if (i_bit(MP_AST, my_word)) {
			i_bit_clear(MP_AST, my_word);
			ast_check();
#if	MACH_KDB
		} else if (i_bit(MP_KDB, my_word)) {
			extern kdb_is_slave[];

			i_bit_clear(MP_KDB, my_word);
			kdb_is_slave[mycpu]++;
			kdb_kintr();
#endif	/* MACH_KDB */
		}
	} while (*my_word);

	mp_enable_preemption();
}

void
slave_boot(
	int	cpu)
{
	int *addr;
	int i=100;

	mp_disable_preemption();

	assert(cpu != cpu_number());

#if	MP_DEBUG
	printf("[%02d]Starting CPU %02d\n",cpu_number(),cpu);
#endif	/* MP_DEBUG */

	if (lapic_type == DISCRETE_APIC) {
		/*
	 	* Set CMOS 0x0f to 0x0a.  This causes jmp through location
	 	* 0x467 on INIT
	 	*/
		__asm__("cli");
		outb(0x70, 0x0f);
		outb(0x71, 0x0a);
		__asm__("sti");
	
		addr = (int *)phystokv(0x467);
		*addr = MP_BOOT;
	}

	__asm__("wbinvd");

	LS(ICRD, cpu<<LAPIC_ICRD_DEST_SHIFT);
	LS(ICR, LAPIC_ICR_DM_INIT);
	delay(10000);

	if (lapic_type != DISCRETE_APIC) {
		LS(ICRD, cpu<<LAPIC_ICRD_DEST_SHIFT);
		LS(ICR, LAPIC_ICR_DM_STARTUP|(MP_BOOT>>12));
		delay(200);
		LS(ICRD, cpu<<LAPIC_ICRD_DEST_SHIFT);
		LS(ICR, LAPIC_ICR_DM_STARTUP|(MP_BOOT>>12));
		delay(200);
	}

	while(i-- > 0) {
		delay(10000);
		if (machine_slot[cpu].running)
		    break;
	}
	if (!machine_slot[cpu].running)
	    printf("Failed to start CPU %02d\n",cpu);

	mp_enable_preemption();
}

void
start_other_cpus(void)
{
	int i;

	extern char slave_boot_base[], slave_boot_end[];
	extern pstart(void);

	master_up();

	bcopy(slave_boot_base,
	      (char *)phystokv(MP_BOOT),
	      slave_boot_end-slave_boot_base);

	bzero((char *)(phystokv(MP_BOOTSTACK+MP_BOOT)-0x400), 0x400);
	*(vm_offset_t *)phystokv(MP_MACH_START+MP_BOOT) = 
						kvtophys((vm_offset_t) pstart);

	__asm__("wbinvd");

	mp_disable_preemption();
	if (!cpu_number())
	    for (i=1; i<NCPUS; i++)
		if (machine_slot[i].is_cpu)
		    slave_boot(i);
	mp_enable_preemption();
}

void
validate_cpus(int ncpus)
{
	int i;

	if (mp_v1_1_initialized) {
		for(i=0;ncpus && i<NCPUS;i++)
		    if (mp_v1_1_valid_cpus[i]) {
			    machine_slot[i].is_cpu = TRUE;
			    ncpus--;
		    } else
			   machine_slot[i].is_cpu = FALSE;
		for(;i<NCPUS;i++)
		    machine_slot[i].is_cpu = FALSE;
 
	} else {
		/*
		 * Too early.  Just set them all to be cpus
		 */
		for(i=0;i<NCPUS;i++)
		    machine_slot[i].is_cpu = TRUE;
	}
}

int
get_ncpus(void)
{
	if (mp_v1_1_initialized) {
		int i,cpus=0;
		for(i=0;i<NCPUS;i++)
		    if (mp_v1_1_valid_cpus[i])
			cpus++;
		return (cpus);
	} else {
		/*
		 * We have no idea how many we have yet.  Its still
		 * too early to tell
		 */
		return NCPUS;
	}
}

void
slave_machine_init(void)
{
	register	my_cpu;
	extern	int	curr_ipl[];

	mp_disable_preemption();
	my_cpu = cpu_number();
	curr_ipl[my_cpu] = SPLHI;
	if (cpuid_family > CPUID_FAMILY_386) /* set cache on */
		set_cr0(get_cr0() & ~(CR0_CD | CR0_NW));
	lapic_init();
	machine_slot[my_cpu].is_cpu = TRUE;
	machine_slot[my_cpu].running = TRUE;
	machine_slot[my_cpu].cpu_type = cpuid_cputype(my_cpu);
	machine_slot[master_cpu].cpu_subtype =
		machine_slot[my_cpu].cpu_subtype = CPU_SUBTYPE_MPS;
	init_fpu();
	printf("cpu %d active\n", my_cpu);
	mp_enable_preemption();
}

#endif	/* NCPUS > 1 */

#if	MACH_KDB
#include <ddb/db_output.h>

#define TRAP_DEBUG 0 /* Must match interrupt.s and spl.s */


#if	TRAP_DEBUG
#define MTRAPS 100
struct mp_trap_hist_struct {
	unsigned char type;
	unsigned char data[5];
} trap_hist[MTRAPS], *cur_trap_hist = trap_hist,
    *max_trap_hist = &trap_hist[MTRAPS];

void db_trap_hist(void);

/*
 * SPL:
 *	1: new spl
 *	2: old spl
 *	3: new tpr
 *	4: old tpr
 * INT:
 * 	1: int vec
 *	2: old spl
 *	3: new spl
 *	4: post eoi tpr
 *	5: exit tpr
 */

void
db_trap_hist(void)
{
	int i,j;
	for(i=0;i<MTRAPS;i++)
	    if (trap_hist[i].type == 1 || trap_hist[i].type == 2) {
		    db_printf("%s%s",
			      (&trap_hist[i]>=cur_trap_hist)?"*":" ",
			      (trap_hist[i].type == 1)?"SPL":"INT");
		    for(j=0;j<5;j++)
			db_printf(" %02x", trap_hist[i].data[j]);
		    db_printf("\n");
	    }
		
}
#endif	/* TRAP_DEBUG */

void db_lapic(int cpu);
unsigned int db_remote_read(int cpu, int reg);
void db_ioapic(unsigned int);
void kdb_console(void);

void
kdb_console(void)
{
}

#define BOOLP(a) ((a)?' ':'!')

static char *DM[8] = {
	"Fixed",
	"Lowest Priority",
	"Invalid",
	"Invalid",
	"NMI",
	"Reset",
	"Invalid",
	"ExtINT"};

unsigned int
db_remote_read(int cpu, int reg)
{
	int i=10000;
	unsigned int ret;

	mp_disable_preemption();
	if (cpu == cpu_number()) {
		mp_enable_preemption();
		return (*((int *)(lapic_start + reg)));
	}

	if ((LF(ICR)&LAPIC_ICR_RR_MASK) == LAPIC_ICR_RR_INPROGRESS) {
		printf("(RR Wedged)");
		mp_enable_preemption();
		return 0xffffffff;
	}
	LS(ICRD, cpu<<LAPIC_ICRD_DEST_SHIFT);
	LS(ICR, (reg>>4)|LAPIC_ICR_DM_REMOTE);
	while(((LF(ICR)&LAPIC_ICR_RR_MASK) == LAPIC_ICR_RR_INPROGRESS) && i)
	    i--;
	if ((LF(ICR)&LAPIC_ICR_RR_MASK) == LAPIC_ICR_RR_INVALID) {
		printf("(RR Failure)");
		mp_enable_preemption();
		return 0xffffffff;
	}
	if ((LF(ICR)&LAPIC_ICR_RR_MASK) == LAPIC_ICR_RR_INPROGRESS) {
		printf("(RR Timeout)");
		mp_enable_preemption();
		return 0xffffffff;
	}
	ret = LF(REMOTE_READ);
	mp_enable_preemption();
	return ret;
}

void
db_lapic(int cpu)
{
	int i;

#define RLF(field) db_remote_read(cpu, LAPIC_##field)
#define RLFO(field, off) db_remote_read(cpu, LAPIC_##field + (off))

	iprintf("LAPIC %d at 0x%x version 0x%x\n", 
		(RLF(ID)>>LAPIC_ID_SHIFT)&LAPIC_ID_MASK,
		lapic_start,
		RLF(VERSION)&LAPIC_VERSION_MASK);
	indent += 2;
	iprintf("Priorities: Task 0x%x  Arbitration 0x%x  Processor 0x%x\n",
		RLF(TPR)&LAPIC_TPR_MASK,
		RLF(APR)&LAPIC_APR_MASK,
		RLF(PPR)&LAPIC_PPR_MASK);
	iprintf("Destination Format 0x%x Logical Destination 0x%x\n",
		RLF(DFR)>>LAPIC_DFR_SHIFT,
		RLF(LDR)>>LAPIC_LDR_SHIFT);
	iprintf("%cEnabled %cFocusChecking SV 0x%x\n",
		BOOLP(RLF(SVR)&LAPIC_SVR_ENABLE),
		BOOLP(!(RLF(SVR)&LAPIC_SVR_FOCUS_OFF)),
		RLF(SVR) & LAPIC_SVR_MASK);
	iprintf("LVT_TIMER: Vector 0x%02x %s %cmasked %s\n",
		RLF(LVT_TIMER)&LAPIC_LVT_VECTOR_MASK,
		(RLF(LVT_TIMER)&LAPIC_LVT_DS_PENDING)?"SendPending":"Idle",
		BOOLP(RLF(LVT_TIMER)&LAPIC_LVT_MASKED),
		(RLF(LVT_TIMER)&LAPIC_LVT_PERIODIC)?"Periodic":"OneShot");
	iprintf("LVT_LINT0: Vector 0x%02x [%s][%s][%s] %s %cmasked\n",
		RLF(LVT_LINT0)&LAPIC_LVT_VECTOR_MASK,
		DM[(RLF(LVT_LINT0)>>LAPIC_LVT_DM_SHIFT)&LAPIC_LVT_DM_MASK],
		(RLF(LVT_TIMER)&LAPIC_LVT_TM_LEVEL)?"Level":"Edge ",
		(RLF(LVT_TIMER)&LAPIC_LVT_IP_PLRITY_LOW)?"Low ":"High",
		(RLF(LVT_LINT0)&LAPIC_LVT_DS_PENDING)?"SendPending":"Idle",
		BOOLP(RLF(LVT_LINT0)&LAPIC_LVT_MASKED));
	iprintf("LVT_LINT1: Vector 0x%02x [%s][%s][%s] %s %cmasked\n",
		RLF(LVT_LINT1)&LAPIC_LVT_VECTOR_MASK,
		DM[(RLF(LVT_LINT1)>>LAPIC_LVT_DM_SHIFT)&LAPIC_LVT_DM_MASK],
		(RLF(LVT_TIMER)&LAPIC_LVT_TM_LEVEL)?"Level":"Edge ",
		(RLF(LVT_TIMER)&LAPIC_LVT_IP_PLRITY_LOW)?"Low ":"High",
		(RLF(LVT_LINT1)&LAPIC_LVT_DS_PENDING)?"SendPending":"Idle",
		BOOLP(RLF(LVT_LINT1)&LAPIC_LVT_MASKED));
	iprintf("LVT_ERROR: Vector 0x%02x %s %cmasked\n",
		RLF(LVT_ERROR)&LAPIC_LVT_VECTOR_MASK,
		(RLF(LVT_ERROR)&LAPIC_LVT_DS_PENDING)?"SendPending":"Idle",
		BOOLP(RLF(LVT_ERROR)&LAPIC_LVT_MASKED));
	mp_disable_preemption();
	if (cpu == cpu_number())
	    LS(ERROR_STATUS,0);		/* Spec says write before read */
	mp_enable_preemption();
	iprintf("ESR: 0x%x\n",RLF(ERROR_STATUS));
	iprintf("REG: 0x");
	for(i=0xf;i>=0;i--)
	    db_printf("%x%x%x%x",i,i,i,i);
	db_printf("\n");
	iprintf("TMR: 0x");
	for(i=7;i>=0;i--)
	    db_printf("%08x",RLFO(TMR_BASE,i*0x10));
	db_printf("\n");
	iprintf("IRR: 0x");
	for(i=7;i>=0;i--)
	    db_printf("%08x",RLFO(IRR_BASE,i*0x10));
	db_printf("\n");
	iprintf("ISR: 0x");
	for(i=7;i>=0;i--)
	    db_printf("%08x",RLFO(ISR_BASE,i*0x10));
	db_printf("\n");
	indent -= 2;
}

void
db_ioapic(unsigned int ind)
{
	unsigned int *select;
	unsigned int *window;
	unsigned int id, v;
	unsigned int max_ent;
	int i;
	int start, p_start;

	if (ind > NIOAPIC)
	    ind = 0;

	start = ioapic_start[ind];
	p_start = pioapic[ind];

	select = (unsigned int *)(start + IOAPIC_RSELECT);
	window = (unsigned int *)(start + IOAPIC_RWINDOW);

	*select = IOA_R_ID;
	id = (*window)>>IOA_R_ID_SHIFT;
	*select = IOA_R_VERSION;
	v = (*window)&IOA_R_VERSION_MASK;
	max_ent = ((*window)>>IOA_R_VERSION_ME_SHIFT)&IOA_R_VERSION_ME_MASK;
	iprintf("IOAPIC %d [%08x] at 0x%x version 0x%x\n",
		id, p_start, start, v);

	for(i=0;i<= max_ent;i++) {
		unsigned int rd,d;

		*select = IOA_R_REDIRECTION+(2*i);
		rd = *window;
		*select = IOA_R_REDIRECTION+(2*i)+1;
		d = *window;

		iprintf("Int %2d: Vec 0x%02x [%s][%s][%s] %s %s %cMasked Dest 0x%08x\n",
			i, rd&IOA_R_R_VECTOR_MASK,
			DM[rd&IOA_R_R_DM_MASK],
			(rd&IOA_R_R_TM_LEVEL)?"Level":"Edge ",
			(rd&IOA_R_R_IP_PLRITY_LOW)?"Low ":"High",
			(rd&IOA_R_R_DEST_LOGICAL)?"Logical ":"Physical",
			(rd&IOA_R_R_DS_PENDING)?"Pend":"Idle",
			BOOLP(rd&IOA_R_R_MASKED),
			d);
	}
}

#endif	/* MACH_KDB */
