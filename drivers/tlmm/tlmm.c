/*chengyewei@wind-mobi.com 20180418 begin*/

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/kernel.h>  
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <linux/mm.h> 


static ssize_t tlmm_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int gpio_val;
	gpio_val = gpio_get_value(54);
	if(gpio_val == 0){
		return sprintf(buf, "1UI2 = 0\n");
	}else if(gpio_val == 1){
		return sprintf(buf, "1UI2 = 1\n");
	}
	else{
		return sprintf(buf, "gpio_get_value error\n");
	}
}

/*
static ssize_t tlmm_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	sscanf(buf, "%s", tlmm);
	return count;
}
*/

static struct kobj_attribute tlmm_attribute =
	__ATTR(tlmm, 0644, tlmm_show, NULL);


static struct attribute *attrs[] = {
	&tlmm_attribute.attr,
	NULL,
};


static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *tlmm_kobj;

static int __init create_private_init(void)
{
	int retval;

	tlmm_kobj = kobject_create_and_add("private", NULL);
	if (!tlmm_kobj){
		printk("Error:kobject_create_and_add!\n");
		return -ENOMEM;
	}

	retval = sysfs_create_group(tlmm_kobj, &attr_group);
	if (retval){
		kobject_put(tlmm_kobj);
		printk("Error:sysfs_create_group\n");
		return -ENOMEM;
	}
	return retval;
}

static void __exit create_private_exit(void)
{
	kobject_put(tlmm_kobj);
}

module_init(create_private_init);
module_exit(create_private_exit);
MODULE_LICENSE("GPL");
/*chengyewei@wind-mobi.com 20180418 end*/
