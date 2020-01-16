
//
//wangpengpeng@wind-mobi.com 20180410 start
//
//sunsiyuan@wind-mobi.com add at 20180514 begin
#if BUILD_WIND_FOR_FACTORY_DIAG
//sunsiyuan@wind-mobi.com add at 20180514 end

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/fs_struct.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/kconfig.h>

/* gaozhixiang@wind-mobi.com add at 20180501 begin */
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <uapi/linux/input-event-codes.h>
/* gaozhixiang@wind-mobi.com add at 20180501 end */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/spmi.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/log2.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/qpnp/power-on.h>

//wangpengpeng@wind-mobi.com add at 20180411 begin
#include <linux/err.h>
#include <linux/kthread.h>
//wangpengpeng@wind-mobi.com add at 20180411 end


#define WIND_DIAG_TAG                  "[WIND/DIAG] "
#define WIND_DIAG_FUN(f)               printk(KERN_INFO WIND_DIAG_TAG"%s\n", __FUNCTION__)
#define WIND_DIAG_ERR(fmt, args...)    printk(KERN_ERR  WIND_DIAG_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define WIND_DIAG_LOG(fmt, args...)    printk(KERN_INFO WIND_DIAG_TAG fmt, ##args)
#define WIND_DIAG_DBG(fmt, args...)    printk(KERN_INFO WIND_DIAG_TAG fmt, ##args) 

#define SMT_CMD_FAST_SLEEP  0
#define SMT_CMD_DISABLE_CHG 1
#define SMT_CMD_ENABLE_CHG 2
#define SMT_CMD_DISABLE_VBUS 3
#define SMT_CMD_ENABLE_VBUS 4
/* gaozhixiang@wind-mobi.com add at 20180501 begin */
#define SMT_CMD_WRITE_NV 5
/* gaozhixiang@wind-mobi.com add at 20180501 end */


#define FAST_SLEEP_PATH "/factory/ccode"
#define DISABLE_ENABLE_CHG_PATH "/sys/class/power_supply/battery/battery_charging_enabled"
#define DISABLE_ENABLE_VBUS_PATH "/sys/class/power_supply/battery/charging_enabled"
//wangpengpeng@wind-mobi.com add at 20180411 begin
#define POWER_WAKE_UNLOCK "sys/power/wake_unlock"
#define SYS_POWER_STATE "/sys/power/state"
#define BACKLIGHT_FILE "/sys/class/leds/lcd-backlight/brightness"
struct task_struct *thread_fast_sleep = NULL;
//wangpengpeng@wind-mobi.com add at 20180411 end

/* gaozhixiang@wind-mobi.com add at 20180501 begin */
static struct input_dev *input_dev;
static int err = 0;
/* gaozhixiang@wind-mobi.com add at 20180501 end */
struct mutex mutex_wind;


unsigned char cmd_table [][4] = 
{
	{0x4b,0xfa,0x0d,0x00}, //SMT_CMD_FAST_SLEEP  0
	{0x4b,0xfa,0x02,0x00}, //SMT_CMD_DISABLE_CHG 1
	{0x4b,0xfa,0x03,0x00}, //SMT_CMD_ENABLE_CHG 2
	{0x4b,0xfa,0xf1,0x00}, //SMT_CMD_DISABLE_VBUS 3
	{0x4b,0xfa,0xf2,0x00}, //SMT_CMD_ENABLE_VBUS 4
	/* sunsiyuan@wind-mobi.com modify at 20180514 begin */
	{0x4b,0xfb,0x01,0x00}, //SMT_CMD_WRITE_NV 5
	/* sunisyuan@wind-mobi.com modify at 20180514 end */
};


int wind_diag_backup_file_write(char * path, char *w_buf, int w_len)
{
	struct file *fp;
	int ret = 0;

	mutex_lock(&mutex_wind);
	
	fp =filp_open(path, O_RDWR | O_SYNC, 0);
	
	if (IS_ERR(fp )) {
		
	 WIND_DIAG_ERR("error occured while opening file %s\n", path);
	 ret = -1;
	 
	}else{

	 WIND_DIAG_DBG("file %s is exist", path);
	 ret = fp->f_op->write(fp, w_buf, w_len, &fp->f_pos);
	 filp_close(fp, NULL);
	
	}
	
	mutex_unlock(&mutex_wind);

	return ret;

}


int wind_diag_file_write(char * path, char *w_buf, int w_len)
{
	struct file *fp;
	int ret = 0;

	mutex_lock(&mutex_wind);
	
	fp =filp_open(path, O_RDWR | O_CREAT | O_SYNC, 0660);
	
	if (IS_ERR(fp )) {
		
	 WIND_DIAG_ERR("error occured while opening file %s\n", path);
	 ret = -1;
	 
	}else{

	 WIND_DIAG_DBG("file %s is exist", path);
	 ret = fp->f_op->write(fp, w_buf, w_len, &fp->f_pos);
	 filp_close(fp, NULL);
	
	}
	mutex_unlock(&mutex_wind);

	return ret;

}

int wind_diag_file_read(char * path, char *r_buf, int r_len)
{
	struct file *fp;
	int ret = 0;

	mutex_lock(&mutex_wind);

	fp =filp_open(path, O_RDWR | O_CREAT, 0660);
	
	if (IS_ERR(fp )) {
		
	 WIND_DIAG_ERR("error occured while opening file %s\n", path);
	 ret = -1;
	 
	}else{

	 WIND_DIAG_DBG("file %s is exist\n", path);
	 ret = fp->f_op->read(fp, r_buf, r_len, &fp->f_pos);
	 filp_close(fp, NULL);
	
	}
	
	mutex_unlock(&mutex_wind);

	return ret;

}

int turn_off_backlight(char *buf, int count)
{
    int ret = 0;
    char path[] = "/sys/class/leds/lcd-backlight/brightness";
    //set kernel domain begin
    mm_segment_t old_fs;
    struct file *fp;

    printk(KERN_WARNING "turn_off_backlight()...\n");
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    fp=filp_open(path, O_RDWR | O_CREAT | O_SYNC, 0660);
    if (!IS_ERR_OR_NULL(fp)){
    printk(KERN_WARNING "turn_off_backlight() fp != null\n");
        if (fp->f_op){
    printk(KERN_WARNING "turn_off_backlight() f_op not null\n");
            fp->f_pos = 0;
            if(fp->f_op->write){//O_RDWR|O_SYNC
                ret = fp->f_op->write(fp, buf, count, &fp->f_pos);
    printk(KERN_WARNING "turn_off_backlight() op_write ret: %d\n", ret);
            }else{
                ret = (int)vfs_write(fp, buf, count, &fp->f_pos);
    printk(KERN_WARNING "turn_off_backlight() vfs_write ret: %d\n", ret);
            }
        }
        filp_close(fp,NULL);
    }
    //set user domain again
    set_fs(old_fs);
    printk(KERN_WARNING "turn_off_backlight() ret: %d\n", ret);
    return ret > 0;
}

//wangpengpeng@wind-mobi.com add at 20180411 begin
int write_backlight(char *value){
    struct file *fp;
	int ret = 0;

	fp =filp_open(BACKLIGHT_FILE, O_RDWR | O_CREAT | O_SYNC, 0660);

	if (IS_ERR(fp )) {
	 WIND_DIAG_ERR("file %s is not exist\n", BACKLIGHT_FILE);
	 printk("sunsiyuan:write_backlight --fail\n");
	 ret = 0;
	}else{
	 WIND_DIAG_DBG("file %s is exist\n", BACKLIGHT_FILE);
	 ret = fp->f_op->write(fp, value, sizeof(value), &fp->f_pos);
	 filp_close(fp, NULL);
	 printk("sunsiyuan:write_backlight --success\n");
	}

	return ret;
}

int power_suspend(void){
    struct file *fp;
	int ret = 0;

	fp =filp_open(SYS_POWER_STATE, O_RDWR | O_CREAT | O_SYNC, 0660);

	if (IS_ERR(fp )) {
	 WIND_DIAG_ERR("file %s is not exist\n",SYS_POWER_STATE);
	 printk("sunsiyuan:power_suspend --fail\n");
	 ret = 0;
	}else{
	 WIND_DIAG_DBG("file %s is exist\n", SYS_POWER_STATE);
	 ret = fp->f_op->write(fp, "mem", 3, &fp->f_pos);
	 filp_close(fp, NULL);
	 printk("sunsiyuan:power_suspend --success\n");
	}
	return ret;
}

int release_wakelock(void){
    struct file *fp;
	int ret = 0;
	
	fp =filp_open("sys/power/wake_unlock", O_RDWR | O_CREAT | O_SYNC, 0660);

	if (IS_ERR(fp )) {
	 WIND_DIAG_ERR("file %s is not exist\n",POWER_WAKE_UNLOCK);
	 printk("sunsiyuan:release_wakelock --fail\n");
	 ret = 0;
	}else{
	 WIND_DIAG_DBG("file %s is exist\n", POWER_WAKE_UNLOCK);
	 ret = fp->f_op->write(fp, "mmi", 3, &fp->f_pos);
	 filp_close(fp, NULL);
	 printk("sunsiyuan:release_wakelock --success\n");
	}
	return ret;
}

int wind_fast_sleep_func(void *unused)
{
	int ret = 1,i = 0;
	printk("sunsiyuan:wind_fast_sleep_func\n");
	write_backlight("0");
	release_wakelock();
	while(i < 20){//panben@wind-mobi.com modify at 20180801
		msleep(2000);//panben@wind-mobi.com modify at 20180801
		printk("wangpengpeng:wind_fast_sleep_func %d\n",i);//wangpengpeng@wind-mobi.com add at 20180704
		write_backlight("0");//wangpengpeng@wind-mobi.com add at 20180704
		power_suspend();
		i++;
	}
	kthread_stop(thread_fast_sleep);	
	return ret;
}
//wangpengpeng@wind-mobi.com add at 20180411 end

int wind_diag_cmd_handler(unsigned char *rx_buf, int rx_len, unsigned char *tx_buf, int *tx_len){

	int index = -1;
	char buff = '0';
	for(index = 0; index < sizeof(cmd_table)/sizeof(cmd_table[0]); index++){
		if(memcmp(rx_buf, cmd_table[index], sizeof(cmd_table[0])) == 0){
			memcpy(tx_buf, rx_buf, 4);
			*tx_len +=4;
			WIND_DIAG_DBG(" chusuxia find cmd = %d\n", index);
			break;
		}
	}

	if(index >= sizeof(cmd_table)/sizeof(cmd_table[0])){
		WIND_DIAG_DBG("cmd not in table index = %d\n", index);
		return -1;
	}else{
			WIND_DIAG_DBG("chusuxia ,wind_diag_cmd_handler receive index = %d\n",index);
			
			switch (index){
				
				case SMT_CMD_FAST_SLEEP:
					//wangpengpeng@wind-mobi.com add at 20180411 begin
					thread_fast_sleep = kthread_run(wind_fast_sleep_func, 0, "wind_fast_sleep_thread");
					if (IS_ERR(thread_fast_sleep)) {
						printk(" failed to create kernel wind_fast_sleep_thread\n");
						tx_buf[4] = 0x31;
						*tx_len +=1;
					}else {
						tx_buf[4] = 0x30;
						*tx_len +=1;
					}
					//wangpengpeng@wind-mobi.com add at 20180411 end
					break;
					
				case SMT_CMD_DISABLE_CHG:
					buff = '0';
					if(-1 != wind_diag_file_write(DISABLE_ENABLE_CHG_PATH, &buff, 1)){
							tx_buf[4] = 0x30;
							*tx_len +=1;
						}else {
							tx_buf[4] = 0x31;
							*tx_len +=1;
						}	
						WIND_DIAG_DBG("wangpengpeng ,SMT_CMD_DISABLE_CHG  tx_len = %d\n",*tx_len);	
						break;
				case SMT_CMD_ENABLE_CHG:
					buff = '1';
					if(-1 != wind_diag_file_write(DISABLE_ENABLE_CHG_PATH, &buff, 1)){
							tx_buf[4] = 0x30;
							*tx_len +=1;
						}else {
							tx_buf[4] = 0x31;
							*tx_len +=1;
						}	
						WIND_DIAG_DBG("wangpengpeng ,SMT_CMD_ENABLE_CHG  tx_len = %d\n",*tx_len);	
						break;
				case SMT_CMD_DISABLE_VBUS:
					buff = '0';
					if(-1 != wind_diag_file_write(DISABLE_ENABLE_VBUS_PATH, &buff, 1)){
							tx_buf[4] = 0x30;
							*tx_len +=1;
						}else {
							tx_buf[4] = 0x31;
							*tx_len +=1;
						}	
						WIND_DIAG_DBG("wangpengpeng ,SMT_CMD_DISABLE_VBUS  tx_len = %d\n",*tx_len);	
						break;
				case SMT_CMD_ENABLE_VBUS:
					buff = '1';
					if(-1 != wind_diag_file_write(DISABLE_ENABLE_VBUS_PATH, &buff, 1)){
							tx_buf[4] = 0x30;
							*tx_len +=1;
						}else {
							tx_buf[4] = 0x31;
							*tx_len +=1;
						}	
						WIND_DIAG_DBG("wangpengpeng ,SMT_CMD_ENABLE_VBUS  tx_len = %d\n",*tx_len);	
						break;
				/* gaozhixiang@wind-mobi.com add at 20180509 begin */
				case SMT_CMD_WRITE_NV:
						printk("gaozhixiang: enter SMT_CMD_WRITE_NV success");
						input_report_key(input_dev, KEY_HP, 1);
						input_report_key(input_dev, KEY_HP, 0);
						input_sync(input_dev);
						/*now we here return 0x30 to tx_buf[4], which means SMT_CMD_WRITE_NV has been called successfully*/
						tx_buf[4] = 0x30;
						*tx_len +=1;
						/*we do not provide 0x31 to tx_buf[4] here, cause KEY_HP will always be reported successfully*/
						WIND_DIAG_DBG("gaozhixiang: SMT_CMD_WRITE_NV tx_len = %d\n",*tx_len);
						break;
				/* gaozhixiang@wind-mobi.com add at 20180501 end */
				default:
						break;
		}

	  return index;
			
	}
	
}



int winddiag_proc_init(void)
{

	WIND_DIAG_FUN(">>>\n");

	/* gaozhixiang@wind-mobi.com add at 20180501 begin */
	input_dev = input_allocate_device();
	if (!input_dev){
		err = -ENOMEM;
		printk("failed to allocate input device\n");
	}

	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(KEY_HP, input_dev->keybit);

	input_dev -> name = "wind_diag";
	err = input_register_device(input_dev);
	if (err) {
		printk("failed to register input device: %s\n",__func__);
		input_free_device(input_dev);
	}
	/* gaozhixiang@wind-mobi.com add at 20180501 end */

	mutex_init(&mutex_wind);

	return 0;

}


module_init(winddiag_proc_init);


//wangpengpeng@wind-mobi.com 20180410 end
//sunsiyuan@wind-mobi.com add at 20180514 begin
#endif //BUILD_WIND_FOR_FACTORY_DIAG endif
//sunsiyuan@wind-mobi.com add at 20180514 end

