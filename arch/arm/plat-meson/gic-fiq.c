/*
 *  arch/arm/plat-meson/core.c
 *
 *  Copyright (C) 2010 AMLOGIC, INC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 #include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <plat/io.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/mach/irq.h>
#include <asm/hardware/gic.h>
#include <asm/fiq.h>

#define MESON_GIC_IRQ_LEVEL 0x3
#define MESON_GIC_FIQ_LEVEL 0x0
#define MAX_FIQ 4

typedef void (*fiq_routine)(void);
//static spinlock_t lock = SPIN_LOCK_UNLOCKED;
static DEFINE_SPINLOCK(lock);

static u8 fiq_stack[4096];
static u8 fiq_index[MAX_FIQ];
static fiq_routine fiq_func[MAX_FIQ];

#ifndef MAX_GIC_NR
#define MAX_GIC_NR	1
#endif

static unsigned int gic_active_irq()
{    
	unsigned int ret;
	uint32_t cpu_base = (uint32_t)(IO_PERIPH_BASE+0x100);    
    ret = aml_read_reg32(cpu_base + GIC_CPU_INTACK) & 0x3FF;
    return ret;
}

static void gic_ack_irq(unsigned int fiq)
{
	uint32_t cpu_base = (uint32_t)(IO_PERIPH_BASE+0x100);    
	aml_write_reg32(cpu_base+GIC_CPU_EOI, fiq);
}

static void __attribute__((naked)) fiq_vector(void)
{
    asm __volatile__("mov pc, r8 ;");
}

static void __attribute__((naked)) fiq_isr(void)
{
    int i;
    unsigned int fiq;
    
    asm __volatile__(
        "mov    ip, sp;\n"
        "stmfd	sp!, {r0-r12, lr};\n"
        "sub    sp, sp, #256;\n"
        "sub    fp, sp, #256;\n");

	fiq=gic_active_irq();
	if((fiq!= 1022) && (fiq!=1023)){
		gic_ack_irq(fiq);
		for(i=0; i<MAX_FIQ; i++){
			if ((fiq_index[i] != 0xff) && (fiq_func[i] != NULL)) {
				if(fiq_index[i] == fiq)
					fiq_func[i]();
			}
		}
	}

    dsb();

    asm __volatile__(
        "add	sp, sp, #256 ;\n"
        "ldmia	sp!, {r0-r12, lr};\n"
        "subs	pc, lr, #4;\n");
}

void fiq_isr_fake(unsigned int fiq)
{
	    int i;
		for(i=0; i<MAX_FIQ; i++){
			if ((fiq_index[i] != 0xff) && (fiq_func[i] != NULL)) {
				if(fiq_index[i] == fiq)
					fiq_func[i]();
			}
		}	
		 dsb();
}

void  init_fiq(void)
{
    struct pt_regs regs;
    int i;
    	unsigned cmd; 
	char c;
	
	cmd = readl(P_AO_RTI_STATUS_REG1);
	c = (char)cmd;
	if(c == 'r')
		return;

    for (i = 0; i < MAX_FIQ; i++) {
        fiq_index[i] = 0xff;
        fiq_func[i] = NULL;
    }

    /* prepare FIQ mode regs */
    memset(&regs, 0, sizeof(regs));
    regs.ARM_r8 = (unsigned long)fiq_isr;
    regs.ARM_sp = (unsigned long)fiq_stack + sizeof(fiq_stack) - 4;
 
    set_fiq_regs(&regs);
    set_fiq_handler(fiq_vector, 8);

  //  return 0;
}


extern void  gic_set_fiq_fake(unsigned fiq);
void request_fiq(unsigned fiq, void (*isr)(void))
{
    int i;
    ulong flags;
    int fiq_share = 0;
    unsigned int mask;
    uint32_t dist_base;

	printk("%s:%d: fiq=%d\n", __FUNCTION__, __LINE__, fiq);
	
   BUG_ON(fiq >= NR_IRQS) ;   
    spin_lock_irqsave(&lock, flags);
		
    for (i = 0; i < MAX_FIQ; i++) {
        if (fiq_index[i] == fiq) {
            fiq_share++;
        }
    }
    for (i = 0; i < MAX_FIQ; i++) {
        if (fiq_index[i] == 0xff) {
            fiq_index[i] = fiq;
            fiq_func[i] = isr;

            if (fiq_share == 0) {
            		gic_set_fiq_fake(fiq);
            		dist_base =  (uint32_t)(IO_PERIPH_BASE+0x1000);    
				mask = aml_read_reg32(dist_base+GIC_DIST_GROUP + (fiq / 32) * 4);
				mask &= (~(1 << (fiq % 32)) & 0xffffffff);
				aml_write_reg32(dist_base + GIC_DIST_GROUP + (fiq / 32) * 4, mask);
				
				aml_set_reg32_bits(dist_base+GIC_DIST_CONFIG + (fiq/16)*4,3,(fiq%16)*2,2);
				aml_set_reg32_bits(dist_base+GIC_DIST_PRI + (fiq  / 4)* 4,0,(fiq%4)*8,8);				
				
				dsb();				
				mask = (1<<(fiq%32));
				aml_write_reg32(dist_base + GIC_DIST_ENABLE_SET+(fiq/32)*4, mask);
			}
            break;
        }
    }

    spin_unlock_irqrestore(&lock, flags);    
    printk("%s:%d: end\n", __FUNCTION__, __LINE__);
}
EXPORT_SYMBOL(request_fiq);

void free_fiq(unsigned fiq, void (*isr)(void))
{
    int i;
    ulong flags;
    int fiq_share = 0;
    unsigned int mask;
    uint32_t dist_base;
    
    spin_lock_irqsave(&lock, flags);

    for (i = 0; i < MAX_FIQ; i++) {
        if (fiq_index[i] == fiq) {
            fiq_share++;
        }
    }

    for (i = 0; i < MAX_FIQ; i++) {
        if ((fiq_index[i] == fiq) && (fiq_func[i] == isr)) {
            if (fiq_share == 1) {
            		dist_base =  (uint32_t)(IO_PERIPH_BASE+0x1000);    
				mask = aml_read_reg32(dist_base+GIC_DIST_GROUP + (fiq / 32) * 4);
				mask |= (1 << (fiq % 32));
				aml_write_reg32(dist_base + GIC_DIST_GROUP + (fiq / 32) * 4, mask);								
				aml_set_reg32_bits(dist_base+GIC_DIST_PRI + (fiq  / 4)* 4,0xff,(fiq%4)*8,3);				
				dsb();
				mask = (1<<(fiq%32));
				aml_write_reg32(dist_base + GIC_DIST_ENABLE_CLEAR + (fiq/32)*4, mask);            
            }
            fiq_index[i] = 0xff;
            fiq_func[i] = NULL;
        }
    }

    spin_unlock_irqrestore(&lock, flags);
}
EXPORT_SYMBOL(free_fiq);

//arch_initcall(init_fiq);


