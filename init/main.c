/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  GK 2/5/95  -  Changed to support mounting root fs via NFS
 *  Added initrd & change_root: Werner Almesberger & Hans Lermen, Feb '96
 *  Moan early if gcc is old, avoiding bogus kernels - Paul Gortmaker, May '96
 *  Simplified starting of init:  Michael A. Griffith <grif@acm.org>
 */

#define DEBUG		/* Enable initcall_debug */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/stackprotector.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/bootmem.h>
#include <linux/acpi.h>
#include <linux/tty.h>
#include <linux/percpu.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/kernel_stat.h>
#include <linux/start_kernel.h>
#include <linux/security.h>
#include <linux/smp.h>
#include <linux/profile.h>
#include <linux/rcupdate.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>
#include <linux/writeback.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/cgroup.h>
#include <linux/efi.h>
#include <linux/tick.h>
#include <linux/interrupt.h>
#include <linux/taskstats_kern.h>
#include <linux/delayacct.h>
#include <linux/unistd.h>
#include <linux/rmap.h>
#include <linux/mempolicy.h>
#include <linux/key.h>
#include <linux/buffer_head.h>
#include <linux/page_cgroup.h>
#include <linux/debug_locks.h>
#include <linux/debugobjects.h>
#include <linux/lockdep.h>
#include <linux/kmemleak.h>
#include <linux/pid_namespace.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/idr.h>
#include <linux/kgdb.h>
#include <linux/ftrace.h>
#include <linux/async.h>
#include <linux/kmemcheck.h>
#include <linux/sfi.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/perf_event.h>
#include <linux/file.h>
#include <linux/ptrace.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/sched_clock.h>
#include <linux/random.h>

#include <asm/io.h>
#include <asm/bugs.h>
#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/cacheflush.h>

#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/smp.h>
#endif

#include <linux/asus_global.h>

static int kernel_init(void *);

extern void init_IRQ(void);
extern void fork_init(unsigned long);
extern void mca_init(void);
extern void sbus_init(void);
extern void radix_tree_init(void);
#ifndef CONFIG_DEBUG_RODATA
static inline void mark_rodata_ro(void) { }
#endif

#ifdef CONFIG_TC
extern void tc_init(void);
#endif

/*
 * Debug helper: via this flag we know that we are in 'early bootup code'
 * where only the boot processor is running with IRQ disabled.  This means
 * two things - IRQ must not be enabled before the flag is cleared and some
 * operations which are not allowed with IRQ disabled are allowed while the
 * flag is set.
 */
bool early_boot_irqs_disabled __read_mostly;

enum system_states system_state __read_mostly;
EXPORT_SYMBOL(system_state);

/*
 * Boot command-line arguments
 */
#define MAX_INIT_ARGS CONFIG_INIT_ENV_ARG_LIMIT
#define MAX_INIT_ENVS CONFIG_INIT_ENV_ARG_LIMIT

extern void time_init(void);
/* Default late time init is NULL. archs can override this later. */
void (*__initdata late_time_init)(void);
extern void softirq_init(void);

/* Untouched command line saved by arch-specific code. */
char __initdata boot_command_line[COMMAND_LINE_SIZE];
/* Untouched saved command line (eg. for /proc) */
char *saved_command_line;
/* Command line for parameter parsing */
static char *static_command_line;

static char *execute_command;
static char *ramdisk_execute_command;

#if 0
enum DEVICE_HWID g_ASUS_hwID=ZE500KL_UNKNOWN;
char hwid_info[32]={0};
EXPORT_SYMBOL(g_ASUS_hwID);
 static int set_hardware_id(char *str)
 {
	strcpy(hwid_info,"HW ID : ");
	if ( strcmp("ZE500KL_EVB", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_EVB;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_EVB\n");
	}
	else if ( strcmp("ZE500KL_SR1", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_SR1;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_SR1\n");
	}
	else if ( strcmp("ZE500KL_SR2", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_SR2;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_SR2\n");
	}
	else if ( strcmp("ZE500KL_ER1", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_ER1;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_ER1\n");
	}
	else if ( strcmp("ZE500KL_ER2", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_ER2;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_ER2\n");
	}
	else if ( strcmp("ZE500KL_PR", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_PR;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_PR\n");
	}
	else if ( strcmp("ZE500KL_PREMP", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_PREMP;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_PREMP\n");
	}
	else if ( strcmp("ZE500KL_MP", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_MP;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_MP\n");
	}
	// ZE500KG
	else if ( strcmp("ZE500KG_EVB", str) == 0 )
	{
		g_ASUS_hwID = ZE500KG_EVB;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KG_EVB\n");
	}
	else if ( strcmp("ZE500KG_SR1", str) == 0 )
	{
		g_ASUS_hwID = ZE500KG_SR1;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KG_SR1\n");
	}
	else if ( strcmp("ZE500KG_SR2", str) == 0 )
	{
		g_ASUS_hwID = ZE500KG_SR2;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KG_SR2\n");
	}
	else if ( strcmp("ZE500KG_ER1", str) == 0 )
	{
		g_ASUS_hwID = ZE500KG_ER1;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KG_ER1\n");
	}
	else if ( strcmp("ZE500KG_ER2", str) == 0 )
	{
		g_ASUS_hwID = ZE500KG_ER2;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KG_ER2\n");
	}
	else if ( strcmp("ZE500KG_PR", str) == 0 )
	{
		g_ASUS_hwID = ZE500KG_PR;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KG_PR\n");
	}
	else if ( strcmp("ZE500KG_PREMP", str) == 0 )
	{
		g_ASUS_hwID = ZE500KG_PREMP;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KG_PREMP\n");
	}
	else if ( strcmp("ZE500KG_MP", str) == 0 )
	{
		g_ASUS_hwID = ZE500KG_MP;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KG_MP\n");
	}
	// ZE500KL_CMCC
	else if ( strcmp("ZE500KL_CMCC_EVB", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_CMCC_EVB;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_CMCC_EVB\n");
	}
	else if ( strcmp("ZE500KL_CMCC_SR1", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_CMCC_SR1;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_CMCC_SR1\n");
	}
	else if ( strcmp("ZE500KL_CMCC_SR2", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_CMCC_SR2;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_CMCC_SR2\n");
	}
	else if ( strcmp("ZE500KL_CMCC_ER1", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_CMCC_ER1;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_CMCC_ER1\n");
	}
	else if ( strcmp("ZE500KL_CMCC_ER2", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_CMCC_ER2;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_CMCC_ER2\n");
	}
	else if ( strcmp("ZE500KL_CMCC_PR", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_CMCC_PR;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_CMCC_PR\n");
	}
	else if ( strcmp("ZE500KL_CMCC_PREMP", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_CMCC_PREMP;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_CMCC_PREMP\n");
	}
	else if ( strcmp("ZE500KL_CMCC_MP", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_CMCC_MP;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_CMCC_MP\n");
	}
	// ZE500KL_MSM8939
	else if ( strcmp("ZE500KL_MSM8939_EVB", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_MSM8939_EVB;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_MSM8939_EVB\n");
	}
	else if ( strcmp("ZE500KL_MSM8939_SR1", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_MSM8939_SR1;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_MSM8939_SR1\n");
	}
	else if ( strcmp("ZE500KL_MSM8939_SR2", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_MSM8939_SR2;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_MSM8939_SR2\n");
	}
	else if ( strcmp("ZE500KL_MSM8939_ER1", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_MSM8939_ER1;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_MSM8939_ER1\n");
	}
	else if ( strcmp("ZE500KL_MSM8939_ER2", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_MSM8939_ER2;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_MSM8939_ER2\n");
	}
	else if ( strcmp("ZE500KL_MSM8939_PR", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_MSM8939_PR;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_MSM8939_PR\n");
	}
	else if ( strcmp("ZE500KL_MSM8939_MP", str) == 0 )
	{
		g_ASUS_hwID = ZE500KL_MSM8939_MP;
		strcat(hwid_info,str);
		printk("Kernel HW ID = ZE500KL_MSM8939_MP\n");
	}
	else
	{
		g_ASUS_hwID = ZE500KL_SR1;
		strcat(hwid_info,"Unknown");
		printk("Kernel HW ID = UNKNOWN HW_ID (FORCE to ZE500KL_SR1)\n");
	}

	printk("g_Asus_hwID = %d\n", g_ASUS_hwID);
	return 0;
}
 __setup("HW_ID=", set_hardware_id);
#endif
//--- ASUS_BSP : miniporting

//+++ ASUS_BSP : Add for FM ID
int g_ASUS_fmID = 1;

static int set_fm_id(char *str)
{
    if ( strcmp("1", str) == 0 )
    {
        g_ASUS_fmID = 1;
    }
    else
    {
        g_ASUS_fmID = 0;
    }
    printk("Kernel FM ID = %d\n", g_ASUS_fmID);
    return 0;
}
__setup("FM_ID=", set_fm_id);
EXPORT_SYMBOL(g_ASUS_fmID);
//--- ASUS_BSP : Add for FM ID

//+++ ASUS_BSP : Add for LASER ID
int g_ASUS_laserID = 1;

static int set_laser_id(char *str)
{
    if ( strcmp("1", str) == 0 )
    {
        g_ASUS_laserID = 1;
    }
    else
    {
        g_ASUS_laserID = 0;
    }
    printk("Kernel LASER ID = %d\n", g_ASUS_laserID);
    return 0;
}
__setup("LASER_ID=", set_laser_id);
EXPORT_SYMBOL(g_ASUS_laserID);
//--- ASUS_BSP : Add for LASER ID

//#ifdef CONFIG_ASUS_ZC550KL_PROJECT
uint32_t zc550kl_pcb_rev_val = ZC550KL_8916_MP;
static int set_zc550kl_pcb_rev(char *str)
{
	if(NULL == str || strlen(str) == 0)
	{
		zc550kl_pcb_rev_val = ZC550KL_8916_MP;
		return 0;
	}

	if(0 == strcmp(str, "0"))
	{
		zc550kl_pcb_rev_val = ZC550KL_8916_MP;
		//printk("PCB_VER: ZC550KL_8916_MP\n");
	}
	else if(0 == strcmp(str, "1"))
	{
		zc550kl_pcb_rev_val = ZC550KL_8939_PR;
		//printk("PCB_VER: ZC550KL_8939_PR\n");
	}
	else if(0 == strcmp(str, "2"))
	{
		zc550kl_pcb_rev_val = ZC550KL_8939_ER;
		//printk("PCB_VER: ZC550KL_8939_ER\n");
	}

	//printk("PCB_VER val = %u\n", zc550kl_pcb_rev_val);

	return 0;
}
early_param("pcb_rev", set_zc550kl_pcb_rev);

uint32_t get_zc550kl_pcb_rev(void)
{
	return zc550kl_pcb_rev_val;
}
EXPORT_SYMBOL(get_zc550kl_pcb_rev);

//+++ ASUS_BSP : miniporting : Add for uart / kernel log
int g_user_klog_mode = 0;
EXPORT_SYMBOL(g_user_klog_mode);

static int set_user_klog_mode(char *str)
{
    if ( strcmp("y", str) == 0 )
    {
        g_user_klog_mode = 1;
    }
    else
    {
        g_user_klog_mode = 0;
    }

    //printk("Kernel log mode = %d\n", g_user_klog_mode);
    return 0;
}
early_param("klog", set_user_klog_mode);
int g_uart_dbg_mode = 0;
EXPORT_SYMBOL(g_uart_dbg_mode);

//static int set_uart_dbg_mode(char *str)
//{
//    if ( strcmp("y", str) == 0 )
//    {
//        g_uart_dbg_mode = 1;
//    }
//    else
//    {
//        g_uart_dbg_mode = 0;
//    }
//
    //printk("Kernel uart dbg mode = %d\n", g_uart_dbg_mode);
//    return 0;
//}
//early_param("dbg", set_uart_dbg_mode);
//--- ASUS_BSP : miniporting : Add for uart / kernel log
//#else
//+++ ASUS_BSP : miniporting : Add for audio dbg mode
int g_user_dbg_mode = 1;
EXPORT_SYMBOL(g_user_dbg_mode);

static int set_user_dbg_mode(char *str)
{
   if ( strcmp("y", str) == 0 )
    {
        g_user_dbg_mode = 1;
    }
    else
    {
       g_user_dbg_mode = 0;
    }
    g_user_dbg_mode = 1;
    printk("Kernel dbg mode = %d\n", g_user_dbg_mode);
    return 0;
}
__setup("dbg=", set_user_dbg_mode);
//--- ASUS_BSP : miniporting : Add for audio dbg mode
//#endif

//ASUS_BSP Austin_T : add for kernel charger mode. +++
bool g_Charger_mode = false;
int g_CHG_mode = 0;
static int set_charger_mode(char *str)
{
    if ( strncmp("charger", str, 7) == 0 )
    {
        g_Charger_mode = true;
        g_CHG_mode = 1;
	}
    else
    {
        g_Charger_mode = false;
        if( strcmp("bank", str) == 0 )
            g_CHG_mode = 2;
         else
           g_CHG_mode = 0;
	}

    printk("g_Charger_mode = %d\n", g_Charger_mode);
    return 0;
}
__setup("androidboot.mode=", set_charger_mode);
EXPORT_SYMBOL(g_Charger_mode);
//ASUS_BSP Austin_T : add for kernel charger mode. ---

//+++ ASUS_BSP: Louis, parsing lcd id from aboot
int g_asus_lcdID = -1;
EXPORT_SYMBOL(g_asus_lcdID);

static int set_lcd_id(char *str)
{
    if (strcmp("AUO", str) == 0 ) {
        g_asus_lcdID = ZE500KL_LCD_AUO;
		printk("LCD ID = AUO\n");
	} else if (strcmp("TM", str) == 0 ){
        g_asus_lcdID = ZE500KL_LCD_TIANMA;
		printk("LCD ID = TM\n");
	} else if (strcmp("BOE", str) == 0 ){
        g_asus_lcdID = ZE500KL_LCD_BOE;
		printk("LCD ID = BOE\n");
	} else if (strcmp("HSD", str) == 0 ){
        g_asus_lcdID = A500_HSD;
		printk("LCD ID = HSD\n");
	}

	printk("g_asus_lcdID = %d\n" , g_asus_lcdID);
    return 0;
}
__setup("PANEL=", set_lcd_id);
//--- ASUS_BSP: Louis

//+++ ASUS_BSP: Enter_Zhang
static uint32_t g_rf_sku_id = (uint32_t)-1;
static int set_rf_sku_id(char *str)
{
	g_rf_sku_id = simple_strtoul(str, NULL, 16);
	printk("rf_sku_id = 0x%X\n" , g_rf_sku_id);
    return 0;
}
__setup("androidboot.rf.sku.id=", set_rf_sku_id);

uint32_t get_rf_sku_id(void)
{
	return g_rf_sku_id;
}

static char g_oem_ssn_str[64];
static int set_oem_ssn(char *str)
{
	strncpy(g_oem_ssn_str, str, sizeof(g_oem_ssn_str) - 1);
	printk("SSN = %s\n" , g_oem_ssn_str);
    return 0;
}
__setup("androidboot.serialno=", set_oem_ssn);

const char* get_oem_ssn(void)
{
	return g_oem_ssn_str;
}
//--- ASUS_BSP: Enter_Zhang

//+++ ASUS_BSP: Yan_Sun
enum CPU_FEATUREID cpu_feature_id = FEATUREID_UNKNOWN;
EXPORT_SYMBOL(cpu_feature_id);
static int set_oem_feature_ID(char *str)
{
	if (!strncmp(str,"05",2) )
	{
		cpu_feature_id = FEATUREID_8939;
		printk("cpu_feature_id = FEATUREID_8939\n");
	}
	else if(!strncmp(str,"0d",2))
	{
		cpu_feature_id = FEATUREID_8916;
		printk("cpu_feature_id = FEATUREID_8916\n");
	}
	else if(!strncmp(str,"11",2))
	{
		cpu_feature_id = FEATUREID_COS;
		printk("cpu_feature_id = FEATUREID_COS\n");
	}
	else
	{
		cpu_feature_id = FEATUREID_UNKNOWN;
		printk("cpu_feature_id = FEATUREID_UNKNOWN\n");
	}
    return 0;
}
__setup("CPU_FEATURE_ID=", set_oem_feature_ID);

//--- ASUS_BSP: Yan_Sun

//+++ ASUS_BSP : Younger
enum BOOT_MODE g_ASUS_bootmode=BOOTMODE_UNKNOWN;

EXPORT_SYMBOL(g_ASUS_bootmode);

 static int set_boot_mode(char *str)
 {

	if (!strncmp(str,"user",4) )
	{
		g_ASUS_bootmode = USER_MODE;
		printk("androidboot.mode=user\n");
	}
	else if(!strncmp(str,"ffbm-",5))
	{
		g_ASUS_bootmode = FFBM_MODE;
		printk("androidboot.mode=ffbm-01\n");
	}
	else if(!strncmp(str,"shipping",8))
	{
		g_ASUS_bootmode = SHIPPING_MODE;
		printk("androidboot.mode=shipping\n");
	}
	else if(!strncmp(str,"charger_factory",15))
	{
		g_ASUS_bootmode = CHARGER_FACTORY_MODE;
		printk("androidboot.mode=charger_factory\n");
	}
	else if(!strncmp(str,"charger_shipping",16))
	{
		g_ASUS_bootmode = CHARGER_SHIPPING_MODE;
		printk("androidboot.mode=charger_shipping\n");
	}
	else
	{
		g_ASUS_bootmode = SHIPPING_MODE;
		printk("androidboot.mode=shipping\n");
	}
	printk("g_ASUS_bootmode = %d\n", g_ASUS_bootmode);
	return 0;
}
__setup("androidboot.mode=", set_boot_mode);
//--- ASUS_BSP : Younger

//+++ ASUS_BSP: Louis, parsing lcd unique id from aboot
char lcd_unique_id[64] = {0};
EXPORT_SYMBOL(lcd_unique_id);

static int get_lcd_uniqueId(char *str)
{
	strncpy(lcd_unique_id, str, sizeof(lcd_unique_id));
	printk("lcd_unique_id = %s\n ", lcd_unique_id);

    return 0;
}
__setup("LCD_UNIQUE_ID=", get_lcd_uniqueId);
//--- ASUS_BSP: Louis

//+++ ASUS_BSP: Deeo add for MCP_ID
enum DEVICE_MCPID g_mcp_id = MCP_ID_UNKNOW;
EXPORT_SYMBOL(g_mcp_id);

static int get_mcp_id(char *str)
{
	if (strcmp("SAMSUNG_16G_8G", str) == 0 ) {
		g_mcp_id = SAMSUNG_16G_8G;
	} else if (strcmp("HYNIX_16G_8G", str) == 0 ) {
		g_mcp_id = HYNIX_16G_8G;
	} else if (strcmp("HYNIX_32G_16G", str) == 0 ) {
		g_mcp_id = HYNIX_32G_16G;
	} else if (strcmp("SAMSUNG_16G_16G", str) == 0 ) {
		g_mcp_id = SAMSUNG_16G_16G;
	} else if (strcmp("SAMSUNG_8G_16G", str) == 0 ) {
		g_mcp_id = SAMSUNG_8G_16G;
	} else if (strcmp("SAMSUNG_8G_8G", str) == 0 ) {
		g_mcp_id = SAMSUNG_8G_8G;
	} else if (strcmp("HYNIX_16G_16G", str) == 0 ) {
		g_mcp_id = HYNIX_16G_16G;
	} else if (strcmp("HYNIX_8G_16G", str) == 0 ) {
		g_mcp_id = HYNIX_8G_16G;
	} else if (strcmp("HYNIX_8G_8G", str) == 0 ) {
		g_mcp_id = HYNIX_8G_8G;
	} else {
		g_mcp_id = MCP_ID_UNKNOW;
	}

	printk("MCP_ID = %d\n ", g_mcp_id);
    return 0;
}
__setup("MCP_ID=", get_mcp_id);
//--- ASUS_BSP: Deeo add for MCP_ID
/*
 * If set, this is an indication to the drivers that reset the underlying
 * device before going ahead with the initialization otherwise driver might
 * rely on the BIOS and skip the reset operation.
 *
 * This is useful if kernel is booting in an unreliable environment.
 * For ex. kdump situaiton where previous kernel has crashed, BIOS has been
 * skipped and devices will be in unknown state.
 */
unsigned int reset_devices;
EXPORT_SYMBOL(reset_devices);

static int __init set_reset_devices(char *str)
{
	reset_devices = 1;
	return 1;
}

__setup("reset_devices", set_reset_devices);

static const char * argv_init[MAX_INIT_ARGS+2] = { "init", NULL, };
const char * envp_init[MAX_INIT_ENVS+2] = { "HOME=/", "TERM=linux", NULL, };
static const char *panic_later, *panic_param;

extern const struct obs_kernel_param __setup_start[], __setup_end[];

static int __init obsolete_checksetup(char *line)
{
	const struct obs_kernel_param *p;
	int had_early_param = 0;

	p = __setup_start;
	do {
		int n = strlen(p->str);
		if (parameqn(line, p->str, n)) {
			if (p->early) {
				/* Already done in parse_early_param?
				 * (Needs exact match on param part).
				 * Keep iterating, as we can have early
				 * params and __setups of same names 8( */
				if (line[n] == '\0' || line[n] == '=')
					had_early_param = 1;
			} else if (!p->setup_func) {
				pr_warn("Parameter %s is obsolete, ignored\n",
					p->str);
				return 1;
			} else if (p->setup_func(line + n))
				return 1;
		}
		p++;
	} while (p < __setup_end);

	return had_early_param;
}

/*
 * This should be approx 2 Bo*oMips to start (note initial shift), and will
 * still work even if initially too large, it will just take slightly longer
 */
unsigned long loops_per_jiffy = (1<<12);

EXPORT_SYMBOL(loops_per_jiffy);

static int __init debug_kernel(char *str)
{
	console_loglevel = 10;
	return 0;
}

static int __init quiet_kernel(char *str)
{
	console_loglevel = 4;
	return 0;
}

early_param("debug", debug_kernel);
early_param("quiet", quiet_kernel);

static int __init loglevel(char *str)
{
	int newlevel;

	/*
	 * Only update loglevel value when a correct setting was passed,
	 * to prevent blind crashes (when loglevel being set to 0) that
	 * are quite hard to debug
	 */
	if (get_option(&str, &newlevel)) {
		console_loglevel = newlevel;
		return 0;
	}

	return -EINVAL;
}

early_param("loglevel", loglevel);

/* Change NUL term back to "=", to make "param" the whole string. */
static int __init repair_env_string(char *param, char *val, const char *unused)
{
	if (val) {
		/* param=val or param="val"? */
		if (val == param+strlen(param)+1)
			val[-1] = '=';
		else if (val == param+strlen(param)+2) {
			val[-2] = '=';
			memmove(val-1, val, strlen(val)+1);
			val--;
		} else
			BUG();
	}
	return 0;
}

/*
 * Unknown boot options get handed to init, unless they look like
 * unused parameters (modprobe will find them in /proc/cmdline).
 */
static int __init unknown_bootoption(char *param, char *val, const char *unused)
{
	repair_env_string(param, val, unused);

	/* Handle obsolete-style parameters */
	if (obsolete_checksetup(param))
		return 0;

	/* Unused module parameter. */
	if (strchr(param, '.') && (!val || strchr(param, '.') < val))
		return 0;

	if (panic_later)
		return 0;

	if (val) {
		/* Environment option */
		unsigned int i;
		for (i = 0; envp_init[i]; i++) {
			if (i == MAX_INIT_ENVS) {
				panic_later = "Too many boot env vars at `%s'";
				panic_param = param;
			}
			if (!strncmp(param, envp_init[i], val - param))
				break;
		}
		envp_init[i] = param;
	} else {
		/* Command line option */
		unsigned int i;
		for (i = 0; argv_init[i]; i++) {
			if (i == MAX_INIT_ARGS) {
				panic_later = "Too many boot init vars at `%s'";
				panic_param = param;
			}
		}
		argv_init[i] = param;
	}
	return 0;
}

static int __init init_setup(char *str)
{
	unsigned int i;

	execute_command = str;
	/*
	 * In case LILO is going to boot us with default command line,
	 * it prepends "auto" before the whole cmdline which makes
	 * the shell think it should execute a script with such name.
	 * So we ignore all arguments entered _before_ init=... [MJ]
	 */
	for (i = 1; i < MAX_INIT_ARGS; i++)
		argv_init[i] = NULL;
	return 1;
}
__setup("init=", init_setup);

static int __init rdinit_setup(char *str)
{
	unsigned int i;

	ramdisk_execute_command = str;
	/* See "auto" comment in init_setup */
	for (i = 1; i < MAX_INIT_ARGS; i++)
		argv_init[i] = NULL;
	return 1;
}
__setup("rdinit=", rdinit_setup);

#ifndef CONFIG_SMP
static const unsigned int setup_max_cpus = NR_CPUS;
#ifdef CONFIG_X86_LOCAL_APIC
static void __init smp_init(void)
{
	APIC_init_uniprocessor();
}
#else
#define smp_init()	do { } while (0)
#endif

static inline void setup_nr_cpu_ids(void) { }
static inline void smp_prepare_cpus(unsigned int maxcpus) { }
#endif

/*
 * We need to store the untouched command line for future reference.
 * We also need to store the touched command line since the parameter
 * parsing is performed in place, and we should allow a component to
 * store reference of name/value for future reference.
 */
static void __init setup_command_line(char *command_line)
{
	saved_command_line = alloc_bootmem(strlen (boot_command_line)+1);
	static_command_line = alloc_bootmem(strlen (command_line)+1);
	strcpy (saved_command_line, boot_command_line);
	strcpy (static_command_line, command_line);
}

/*
 * We need to finalize in a non-__init function or else race conditions
 * between the root thread and the init thread may cause start_kernel to
 * be reaped by free_initmem before the root thread has proceeded to
 * cpu_idle.
 *
 * gcc-3.4 accidentally inlines this function, so use noinline.
 */

static __initdata DECLARE_COMPLETION(kthreadd_done);

static noinline void __init_refok rest_init(void)
{
	int pid;
	const struct sched_param param = { .sched_priority = 1 };

	rcu_scheduler_starting();
	/*
	 * We need to spawn init first so that it obtains pid 1, however
	 * the init task will end up wanting to create kthreads, which, if
	 * we schedule it before we create kthreadd, will OOPS.
	 */
	kernel_thread(kernel_init, NULL, CLONE_FS | CLONE_SIGHAND);
	numa_default_policy();
	pid = kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES);
	rcu_read_lock();
	kthreadd_task = find_task_by_pid_ns(pid, &init_pid_ns);
	rcu_read_unlock();
	sched_setscheduler_nocheck(kthreadd_task, SCHED_FIFO, &param);
	complete(&kthreadd_done);

	/*
	 * The boot idle thread must execute schedule()
	 * at least once to get things moving:
	 */
	init_idle_bootup_task(current);
	schedule_preempt_disabled();
	/* Call into cpu_idle with preempt disabled */
	cpu_startup_entry(CPUHP_ONLINE);
}

/* Check for early params. */
static int __init do_early_param(char *param, char *val, const char *unused)
{
	const struct obs_kernel_param *p;

	for (p = __setup_start; p < __setup_end; p++) {
		if ((p->early && parameq(param, p->str)) ||
		    (strcmp(param, "console") == 0 &&
		     strcmp(p->str, "earlycon") == 0)
		) {
			if (p->setup_func(val) != 0)
				pr_warn("Malformed early option '%s'\n", param);
		}
	}
	/* We accept everything at this stage. */
	return 0;
}

void __init parse_early_options(char *cmdline)
{
	parse_args("early options", cmdline, NULL, 0, 0, 0, do_early_param);
}

/* Arch code calls this early on, or if not, just before other parsing. */
void __init parse_early_param(void)
{
	static __initdata int done = 0;
	static __initdata char tmp_cmdline[COMMAND_LINE_SIZE];

	if (done)
		return;

	/* All fall through to do_early_param. */
	strlcpy(tmp_cmdline, boot_command_line, COMMAND_LINE_SIZE);
	parse_early_options(tmp_cmdline);
	done = 1;
}

/*
 *	Activate the first processor.
 */

static void __init boot_cpu_init(void)
{
	int cpu = smp_processor_id();
	/* Mark the boot cpu "present", "online" etc for SMP and UP case */
	set_cpu_online(cpu, true);
	set_cpu_active(cpu, true);
	set_cpu_present(cpu, true);
	set_cpu_possible(cpu, true);
}

void __init __weak smp_setup_processor_id(void)
{
}

# if THREAD_SIZE >= PAGE_SIZE
void __init __weak thread_info_cache_init(void)
{
}
#endif

/*
 * Set up kernel memory allocators
 */
static void __init mm_init(void)
{
	/*
	 * page_cgroup requires contiguous pages,
	 * bigger than MAX_ORDER unless SPARSEMEM.
	 */
	page_cgroup_init_flatmem();
	mem_init();
	kmem_cache_init();
	percpu_init_late();
	pgtable_cache_init();
	vmalloc_init();
}

asmlinkage void __init start_kernel(void)
{
	char * command_line;
	extern const struct kernel_param __start___param[], __stop___param[];

	/*
	 * Need to run as early as possible, to initialize the
	 * lockdep hash:
	 */
	lockdep_init();
	smp_setup_processor_id();
	debug_objects_early_init();

	cgroup_init_early();

	local_irq_disable();
	early_boot_irqs_disabled = true;

/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */
	boot_cpu_init();
	page_address_init();
	pr_notice("%s", linux_banner);
	setup_arch(&command_line);
	/*
	 * Set up the the initial canary ASAP:
	 */
	boot_init_stack_canary();
	mm_init_owner(&init_mm, &init_task);
	mm_init_cpumask(&init_mm);
	setup_command_line(command_line);
	setup_nr_cpu_ids();
	setup_per_cpu_areas();
	smp_prepare_boot_cpu();	/* arch-specific boot-cpu hooks */

	build_all_zonelists(NULL, NULL);
	page_alloc_init();

	pr_notice("Kernel command line: %s\n", boot_command_line);
	parse_early_param();
	parse_args("Booting kernel", static_command_line, __start___param,
		   __stop___param - __start___param,
		   -1, -1, &unknown_bootoption);

	jump_label_init();

	/*
	 * These use large bootmem allocations and must precede
	 * kmem_cache_init()
	 */
	setup_log_buf(0);
	pidhash_init();
	vfs_caches_init_early();
	sort_main_extable();
	trap_init();
	mm_init();

	/*
	 * Set up the scheduler prior starting any interrupts (such as the
	 * timer interrupt). Full topology setup happens at smp_init()
	 * time - but meanwhile we still have a functioning scheduler.
	 */
	sched_init();
	/*
	 * Disable preemption - early bootup scheduling is extremely
	 * fragile until we cpu_idle() for the first time.
	 */
	preempt_disable();
	if (WARN(!irqs_disabled(), "Interrupts were enabled *very* early, fixing it\n"))
		local_irq_disable();
	idr_init_cache();
	perf_event_init();
	rcu_init();
	tick_nohz_init();
	radix_tree_init();
	/* init some links before init_ISA_irqs() */
	early_irq_init();
	init_IRQ();
	tick_init();
	init_timers();
	hrtimers_init();
	softirq_init();
	timekeeping_init();
	time_init();
	sched_clock_postinit();
	profile_init();
	call_function_init();
	WARN(!irqs_disabled(), "Interrupts were enabled early\n");
	early_boot_irqs_disabled = false;
	local_irq_enable();

	kmem_cache_init_late();

	/*
	 * HACK ALERT! This is early. We're enabling the console before
	 * we've done PCI setups etc, and console_init() must be aware of
	 * this. But we do want output early, in case something goes wrong.
	 */
	console_init();
	if (panic_later)
		panic(panic_later, panic_param);

	lockdep_info();

	/*
	 * Need to run this when irqs are enabled, because it wants
	 * to self-test [hard/soft]-irqs on/off lock inversion bugs
	 * too:
	 */
	locking_selftest();

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start && !initrd_below_start_ok &&
	    page_to_pfn(virt_to_page((void *)initrd_start)) < min_low_pfn) {
		pr_crit("initrd overwritten (0x%08lx < 0x%08lx) - disabling it.\n",
		    page_to_pfn(virt_to_page((void *)initrd_start)),
		    min_low_pfn);
		initrd_start = 0;
	}
#endif
	page_cgroup_init();
	debug_objects_mem_init();
	kmemleak_init();
	setup_per_cpu_pageset();
	numa_policy_init();
	if (late_time_init)
		late_time_init();
	sched_clock_init();
	calibrate_delay();
	pidmap_init();
	anon_vma_init();
#ifdef CONFIG_X86
	if (efi_enabled(EFI_RUNTIME_SERVICES))
		efi_enter_virtual_mode();
#endif
	thread_info_cache_init();
	cred_init();
	fork_init(totalram_pages);
	proc_caches_init();
	buffer_init();
	key_init();
	security_init();
	dbg_late_init();
	vfs_caches_init(totalram_pages);
	signals_init();
	/* rootfs populating might need page-writeback */
	page_writeback_init();
#ifdef CONFIG_PROC_FS
	proc_root_init();
#endif
	cgroup_init();
	cpuset_init();
	taskstats_init_early();
	delayacct_init();

	check_bugs();

	acpi_early_init(); /* before LAPIC and SMP init */
	sfi_init_late();

	if (efi_enabled(EFI_RUNTIME_SERVICES)) {
		efi_late_init();
		efi_free_boot_services();
	}

	ftrace_init();

	/* Do the rest non-__init'ed, we're now alive */
	rest_init();
}

/* Call all constructor functions linked into the kernel. */
static void __init do_ctors(void)
{
#ifdef CONFIG_CONSTRUCTORS
	ctor_fn_t *fn = (ctor_fn_t *) __ctors_start;

	for (; fn < (ctor_fn_t *) __ctors_end; fn++)
		(*fn)();
#endif
}

bool initcall_debug;
core_param(initcall_debug, initcall_debug, bool, 0644);

static char msgbuf[64];

static int __init_or_module do_one_initcall_debug(initcall_t fn)
{
	ktime_t calltime, delta, rettime;
	unsigned long long duration;
	int ret;

	if (initcall_debug)
		pr_debug("calling  %pF @ %i\n", fn, task_pid_nr(current));
	calltime = ktime_get();
	ret = fn();
	rettime = ktime_get();
	delta = ktime_sub(rettime, calltime);
	duration = (unsigned long long) ktime_to_ns(delta) >> 10;
	if (initcall_debug)
		pr_debug("initcall %pF returned %d after %lld usecs\n",
			 fn, ret, duration);

	if (initcall_debug == 0) {
		if (duration > 100000)
			printk(KERN_WARNING "[debuginit] initcall %pF returned %d after %lld usecs\n", fn,
				ret, duration);
	}

	return ret;
}

int __init_or_module do_one_initcall(initcall_t fn)
{
	int count = preempt_count();
	int ret;

	ret = do_one_initcall_debug(fn);

	msgbuf[0] = 0;

	if (preempt_count() != count) {
		sprintf(msgbuf, "preemption imbalance ");
		preempt_count() = count;
	}
	if (irqs_disabled()) {
		strlcat(msgbuf, "disabled interrupts ", sizeof(msgbuf));
		local_irq_enable();
	}
	WARN(msgbuf[0], "initcall %pF returned with %s\n", fn, msgbuf);

	return ret;
}


extern initcall_t __initcall_start[];
extern initcall_t __initcall0_start[];
extern initcall_t __initcall1_start[];
extern initcall_t __initcall2_start[];
extern initcall_t __initcall3_start[];
extern initcall_t __initcall4_start[];
extern initcall_t __initcall5_start[];
extern initcall_t __initcall6_start[];
extern initcall_t __initcall7_start[];
extern initcall_t __initcall_end[];

static initcall_t *initcall_levels[] __initdata = {
	__initcall0_start,
	__initcall1_start,
	__initcall2_start,
	__initcall3_start,
	__initcall4_start,
	__initcall5_start,
	__initcall6_start,
	__initcall7_start,
	__initcall_end,
};

/* Keep these in sync with initcalls in include/linux/init.h */
static char *initcall_level_names[] __initdata = {
	"early",
	"core",
	"postcore",
	"arch",
	"subsys",
	"fs",
	"device",
	"late",
};

static void __init do_initcall_level(int level)
{
	extern const struct kernel_param __start___param[], __stop___param[];
	initcall_t *fn;

	strcpy(static_command_line, saved_command_line);
	parse_args(initcall_level_names[level],
		   static_command_line, __start___param,
		   __stop___param - __start___param,
		   level, level,
		   &repair_env_string);

	for (fn = initcall_levels[level]; fn < initcall_levels[level+1]; fn++)
		do_one_initcall(*fn);
}

static void __init do_initcalls(void)
{
	int level;

	for (level = 0; level < ARRAY_SIZE(initcall_levels) - 1; level++)
		do_initcall_level(level);
}

/*
 * Ok, the machine is now initialized. None of the devices
 * have been touched yet, but the CPU subsystem is up and
 * running, and memory and process management works.
 *
 * Now we can finally start doing some real work..
 */
static void __init do_basic_setup(void)
{
	cpuset_init_smp();
	usermodehelper_init();
	shmem_init();
	driver_init();
	init_irq_proc();
	do_ctors();
	usermodehelper_enable();
	do_initcalls();
	random_int_secret_init();
}

static void __init do_pre_smp_initcalls(void)
{
	initcall_t *fn;

	for (fn = __initcall_start; fn < __initcall0_start; fn++)
		do_one_initcall(*fn);
}

/*
 * This function requests modules which should be loaded by default and is
 * called twice right after initrd is mounted and right before init is
 * exec'd.  If such modules are on either initrd or rootfs, they will be
 * loaded before control is passed to userland.
 */
void __init load_default_modules(void)
{
	load_default_elevator_module();
}

static int run_init_process(const char *init_filename)
{
	argv_init[0] = init_filename;
	return do_execve(init_filename,
		(const char __user *const __user *)argv_init,
		(const char __user *const __user *)envp_init);
}

static noinline void __init kernel_init_freeable(void);

static int __ref kernel_init(void *unused)
{
	kernel_init_freeable();
	/* need to finish all async __init code before freeing the memory */
	async_synchronize_full();
	free_initmem();
	mark_rodata_ro();
	system_state = SYSTEM_RUNNING;
	numa_default_policy();

	flush_delayed_fput();

	if (ramdisk_execute_command) {
		if (!run_init_process(ramdisk_execute_command))
			return 0;
		pr_err("Failed to execute %s\n", ramdisk_execute_command);
	}

	/*
	 * We try each of these until one succeeds.
	 *
	 * The Bourne shell can be used instead of init if we are
	 * trying to recover a really broken machine.
	 */
	if (execute_command) {
		if (!run_init_process(execute_command))
			return 0;
		pr_err("Failed to execute %s.  Attempting defaults...\n",
			execute_command);
	}
	if (!run_init_process("/sbin/init") ||
	    !run_init_process("/etc/init") ||
	    !run_init_process("/bin/init") ||
	    !run_init_process("/bin/sh"))
		return 0;

	panic("No init found.  Try passing init= option to kernel. "
	      "See Linux Documentation/init.txt for guidance.");
}

static noinline void __init kernel_init_freeable(void)
{
	/*
	 * Wait until kthreadd is all set-up.
	 */
	wait_for_completion(&kthreadd_done);

	/* Now the scheduler is fully set up and can do blocking allocations */
	gfp_allowed_mask = __GFP_BITS_MASK;

	/*
	 * init can allocate pages on any node
	 */
	set_mems_allowed(node_states[N_MEMORY]);
	/*
	 * init can run on any cpu.
	 */
	set_cpus_allowed_ptr(current, cpu_all_mask);

	cad_pid = task_pid(current);

	smp_prepare_cpus(setup_max_cpus);

	do_pre_smp_initcalls();
	lockup_detector_init();

	smp_init();
	sched_init_smp();

	do_basic_setup();

	/* Open the /dev/console on the rootfs, this should never fail */
	if (sys_open((const char __user *) "/dev/console", O_RDWR, 0) < 0)
		pr_err("Warning: unable to open an initial console.\n");

	(void) sys_dup(0);
	(void) sys_dup(0);
	/*
	 * check if there is an early userspace init.  If yes, let it do all
	 * the work
	 */

	if (!ramdisk_execute_command)
		ramdisk_execute_command = "/init";

	if (sys_access((const char __user *) ramdisk_execute_command, 0) != 0) {
		ramdisk_execute_command = NULL;
		prepare_namespace();
	}

	/*
	 * Ok, we have completed the initial bootup, and
	 * we're essentially up and running. Get rid of the
	 * initmem segments and start the user-mode stuff..
	 */

	/* rootfs is available now, try loading default modules */
	load_default_modules();
}
