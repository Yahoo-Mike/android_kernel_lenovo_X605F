//wuminglei@wind-mobi.com add to 20180512 begin
#include <linux/init.h>  
#include <linux/kernel.h>  
#include <linux/module.h>  
#include <linux/cdev.h>  
#include <linux/fs.h>  
#include <asm/uaccess.h>  
#include <asm/io.h>  
#include <linux/device.h>  
#include <linux/platform_device.h>  
#include <linux/kobject.h>
#include <linux/bug.h>
//#include <linux/sysfs.h> 
#include "sysfs.h"
   
static struct kobject *sys_kobj;  
 
static ssize_t sys_efuse_show(struct kobject *kobj, struct kobj_attribute *attr,  
        char *buf)  
{  
	extern char *saved_command_line;
	char *str="HWSEC SBC DAA";
	if(strnstr(saved_command_line, str,strlen(saved_command_line)))
		return snprintf(buf, 20, "%s\n", str);
	else 
		return 0;
}  
  

static ssize_t sys_efuse_store(struct kobject *kobj, struct kobj_attribute *attr, 
		const char *buf, size_t count)  
{  
    return 0;  
}  
  
  
static struct kobj_attribute sys_file_attribute =  
    __ATTR(hwsec_info, 0775, sys_efuse_show, sys_efuse_store);  
  
static struct attribute *sys_hwsec_attr[] = {  
    &sys_file_attribute.attr,  
    NULL,  
};  
  
  
static struct attribute_group sys_file_bootinfo_group = {  
    // .name = "hwsec_info",  
     .attrs = sys_hwsec_attr,  
};
  
  

static int __init create_sys_file_init(void)  
{  
    int ret;  
    sys_kobj = kobject_create_and_add("bootinfo", NULL); 
	if (!sys_kobj) {
			printk(KERN_ERR "bootinfo registration failed.\n");
			return -ENOMEM;
		}	
    ret = sysfs_create_group(sys_kobj, &sys_file_bootinfo_group);  
    if(ret) {  
        printk(KERN_ERR "sysfs_create_group error\n");  
	sysfs_remove_group(sys_kobj, &sys_file_bootinfo_group); 
        return -1;  
    }   
    return 0;  
}  
  
static void  __exit create_sys_file_exit(void)  
{  
    sysfs_remove_group(sys_kobj, &sys_file_bootinfo_group);  
    printk(KERN_ERR "sysfs_remove_group\n");  
}  
  
module_init(create_sys_file_init);  
module_exit(create_sys_file_exit);  
MODULE_AUTHOR("wuminglei@wind-mobi.com");
//wuminglei@wind-mobi.com add to 20180512 end
