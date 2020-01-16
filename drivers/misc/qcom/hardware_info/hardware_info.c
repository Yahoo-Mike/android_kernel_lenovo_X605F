//sunsiyuan@wind-mobi.com add at 20171201 begin
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/proc_fs.h>
#include <linux/of_fdt.h>

#define qcom_debug 0

char *tp_name;
static char  lcm_name[50] = {0};
static char  lcm_real_name[50] = {0};  //sunjingtao@wind-mobi.com modify add 20180524
char *main_camera;
char *sub_camera;
u8 ctp_fw_version;
u16 ctp_fw_version_2;

extern unsigned int g_fg_battery_id;

#define LCM_UNKOWN  0
#define LCM_KOWN    1
static int lcm_name_id = LCM_UNKOWN;

static ssize_t show_lcm(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    if (lcm_name_id == LCM_KOWN)
        ret_value = sprintf(buf, "lcd name    :%s\n", lcm_real_name);  //sunjingtao@wind-mobi.com modify add 20180524
    else
        ret_value = sprintf(buf, "lcd  not found\n");
    return ret_value;
}

static ssize_t show_ctp(struct device *dev,struct device_attribute *attr, char *buf)
{
    	int ret_value = 1;
	
	if(tp_name){
	   
		if(!strcmp(tp_name,"FOCALTECH"))
			ret_value = sprintf(buf, "ctp name:%s fw_ver:0x%02x\n",tp_name, ctp_fw_version);
	}
    	else
        	ret_value = sprintf(buf, "ctp name:Not found\n");

    	return ret_value;
}

static ssize_t show_main_camera(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;

    if(main_camera)
        ret_value = sprintf(buf , "main camera :%s\n", main_camera);
    else
        ret_value = sprintf(buf , "main camera :can not  get camera id\n");

    return ret_value;
}

static ssize_t show_sub_camera(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;

    if(sub_camera)
        ret_value = sprintf(buf , "sub camera  :%s\n", sub_camera);
    else
        ret_value = sprintf(buf , "sub camera  :can not  get camera id\n");

    return ret_value;
}

#if qcom_debug
static ssize_t show_wifi(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf, "wifi name   :WCN3615\n");
    return ret_value;
}
static ssize_t show_bt(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf, "bt name     :WCN3615\n");
    return ret_value;
}
static ssize_t show_gps(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf, "GPS name    :WCN3615\n");
    return ret_value;
}
static ssize_t show_fm(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf, "FM name     :WCN3615\n");
    return ret_value;
}
#endif


static DEVICE_ATTR(00_lcm, 0444, show_lcm, NULL);
static DEVICE_ATTR(01_ctp, 0444, show_ctp, NULL);
static DEVICE_ATTR(02_main_camera, 0444, show_main_camera, NULL);
static DEVICE_ATTR(04_sub_camera, 0444, show_sub_camera, NULL);
#if qcom_debug
static DEVICE_ATTR(09_wifi, 0444, show_wifi, NULL);
static DEVICE_ATTR(10_bt, 0444, show_bt, NULL);
static DEVICE_ATTR(11_gps, 0444, show_gps, NULL);
static DEVICE_ATTR(12_fm, 0444, show_fm, NULL);
#endif


static int HardwareInfo_driver_probe(struct platform_device *pdev)
{
    int ret_device_file = 0;
    int ret = 0;
    struct device * device;
    struct device_node *dnode;

    device=&pdev->dev;

    printk("** HardwareInfo_driver_probe!! **\n" );
    if ((ret_device_file = device_create_file(&(pdev->dev), &dev_attr_00_lcm)) != 0) goto exit_error;
    if ((ret_device_file = device_create_file(&(pdev->dev), &dev_attr_01_ctp)) != 0) goto exit_error;
    if ((ret_device_file = device_create_file(&(pdev->dev), &dev_attr_02_main_camera)) != 0) goto exit_error;
    if ((ret_device_file = device_create_file(&(pdev->dev), &dev_attr_04_sub_camera)) != 0) goto exit_error;
#if qcom_debug
    if ((ret_device_file = device_create_file(&(pdev->dev), &dev_attr_09_wifi)) != 0) goto exit_error;
    if ((ret_device_file = device_create_file(&(pdev->dev), &dev_attr_10_bt)) != 0) goto exit_error;
    if ((ret_device_file = device_create_file(&(pdev->dev), &dev_attr_11_gps)) != 0) goto exit_error;
    if ((ret_device_file = device_create_file(&(pdev->dev), &dev_attr_12_fm)) != 0) goto exit_error;
#endif

    dnode = device->of_node;

    printk("hardware_info probe done\n");
    return ret;

exit_error:
    return ret_device_file;
}

static int HardwareInfo_driver_remove(struct platform_device *dev)
{
    printk("** HardwareInfo_drvier_remove!! **");

    device_remove_file(&(dev->dev), &dev_attr_00_lcm);
    device_remove_file(&(dev->dev), &dev_attr_01_ctp);
    device_remove_file(&(dev->dev), &dev_attr_02_main_camera);
    device_remove_file(&(dev->dev), &dev_attr_04_sub_camera);
#if qcom_debug
    device_remove_file(&(dev->dev), &dev_attr_09_wifi);
    device_remove_file(&(dev->dev), &dev_attr_10_bt);
    device_remove_file(&(dev->dev), &dev_attr_11_gps);
    device_remove_file(&(dev->dev), &dev_attr_12_fm);
#endif

    return 0;
}

static void init_proinfo_value_from_cmdline(void){
    char *token;
    char delim[] = "= ";
    char *temp_ptr;
    int size = strlen(saved_command_line)+1;
    char *strs = (char *)kmalloc(size, GFP_KERNEL);
    if(strs == NULL){
        return;
    }
    memcpy(strs, saved_command_line, size);
    for(token = strsep(&strs, delim); token != NULL; token = strsep(&strs, delim)){
        if (NULL != strstr(token, "qcom,")) {
            temp_ptr = token;
            token = strsep(&temp_ptr, ",");
            strcpy(lcm_name, strsep(&temp_ptr, ":"));
//sunjingtao@wind-mobi.com modify add 20180524 begin
	     if(!(strcmp(lcm_name, "mdss_dsi_truly_1080p_video") ))
	         strcpy(lcm_real_name, "BOE_TDDI_FT8201");
		 else
	         strcpy(lcm_real_name, "INX_TDDI_FT8201");
//sunjingtao@wind-mobi.com modify add 20180524 end
            lcm_name_id = LCM_KOWN;
        }
    }
    kfree(strs);
}

static const struct of_device_id exynos_board_id_gpio_match[] = {
    {
        .compatible = "qcom,hardware_info",
    },
    {},
};
MODULE_DEVICE_TABLE(of, exynos_board_id_gpio_match);

static struct platform_driver HardwareInfo_driver = {
    .probe  = HardwareInfo_driver_probe,
    .remove     = HardwareInfo_driver_remove,
    .driver     = {
        .name = "HardwareInfo",
    .owner  = THIS_MODULE,
    .of_match_table = exynos_board_id_gpio_match,
    },
};

static int __init HardwareInfo_mod_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&HardwareInfo_driver);
    if (ret) {
        printk("**HardwareInfo_mod_init  Unable to driver register(%d)\n", ret);
        goto  fail_1;
    }
	init_proinfo_value_from_cmdline();
    goto ok_result;

fail_1:
    platform_driver_unregister(&HardwareInfo_driver);
ok_result:

    return ret;
}

static void __exit HardwareInfo_mod_exit(void)
{
    platform_driver_unregister(&HardwareInfo_driver);
}

module_init(HardwareInfo_mod_init);
module_exit(HardwareInfo_mod_exit);
MODULE_AUTHOR(" <sunsiyuan@wind-mobi.com>");
MODULE_DESCRIPTION("qcom Hardware Info driver");
MODULE_LICENSE("GPL");
//sunsiyuan@wind-mobi.com add at 20171201 end
