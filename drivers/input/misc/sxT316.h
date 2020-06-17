/*
 *  fengnan@wind-mobi.com add file at 20180411
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
#ifndef SXT316_H
#define SXT316_H

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
//#include <sensors_io.h>
/*
 *  I2C Registers
 */
#define SXT316_IRQSTAT_REG    	0x00
#define ABOV_VERSION_REG		0x01
#define ABOV_MODELNO_REG		0x02
#define SXT316_CH_REG           0x08
//+bug 271433 ,zwk.wt ,20170620,add sar sensor register map
#define SXT316_CTRL_MODE        0x07
#define SXT316_CALIBRATION      0xFB
#define SXT316_SLEEP_THRELOD    0x28
//+bug 273000 ,zwk.wt ,20170629,add sar sensor register 
#define SXT316_VENDOR_ID        0x03
#define SXT316_DIFF_MSB_CH0     0x1C
#define SXT316_DIFF_LSB_CH0     0x1D
#define SXT316_DIFF_MSB_CH1     0x1E
#define SXT316_DIFF_LSB_CH1     0x1F
#define SXT316_GAIN             0x09
#define SAR_SXT316_DEBUG 1

//ioctl
#define SARSENSOR                         0x90
#define SARSENSOR_SET_MODE               _IO(SARSENSOR, 0x01)
#define SARSENSOR_GET_DIFF_CH0           _IOR(SARSENSOR, 0x02, int)
#define SARSENSOR_GET_DIFF_CH1           _IOR(SARSENSOR, 0x03, int)
#define SARSENSOR_GET_INT_STATUS         _IOR(SARSENSOR, 0x04, int)
#define SARSENSOR_GET_VENDOR_ID          _IOR(SARSENSOR, 0x05, int)

//-bug 273000 ,zwk.wt ,20170629,add sar sensor register 
/**************************************
*			define platform data
*
**************************************/

struct smtc_reg_data {
  unsigned char reg;
  unsigned char val;
};

typedef struct smtc_reg_data smtc_reg_data_t;
typedef struct smtc_reg_data *psmtc_reg_data_t;


struct _buttonInfo {
  /*! The Key to send to the input */
  int keycode;
  int  keycode_release;
  /*! Mask to look for on Touch Status */
  int mask;
  /*! Current state of button. */
  int state;
};

struct _totalButtonInformation {
  struct _buttonInfo *buttons;
  int buttonSize;
  struct input_dev *input;
};

typedef struct _totalButtonInformation buttonInformation_t;
typedef struct _totalButtonInformation *pbuttonInformation_t;

#define KEY_SXSNER_TOUCH     253
#define KEY_SXSNER_TOUCH_WIFI     254
#define KEY_SXSNER_TOUCH_RELEASE	199
#define KEY_SXSNER_TOUCH_WIFI_RELEASE	198

static struct _buttonInfo psmtcButtons[] = {
  {
    .keycode = KEY_SAR_WIFI_CS,    //fengnan@wind-mobi.com add at  20180425
    .keycode_release = KEY_SXSNER_TOUCH_RELEASE,
    //.mask = SXT316_TCHCMPSTAT_TCHSTAT0_FLAG,
  },
  {
    .keycode = KEY_SAR_WIFI_CS,    /*fengnan@wind-mobi.com modify at 20180425*/
    .keycode_release = KEY_SXSNER_TOUCH_WIFI_RELEASE,
    //.mask = SXT316_TCHCMPSTAT_TCHSTAT1_FLAG,
  },
};

struct sxT316_platform_data {
  int i2c_reg_num;
  struct smtc_reg_data *pi2c_reg;
  
  pbuttonInformation_t pbuttonInformation;

 // int (*get_is_nirq_low)(void);
  
//  int     (*init_platform_hw)(void);
 // void    (*exit_platform_hw)(void);
};
typedef struct sxT316_platform_data sxT316_platform_data_t;
typedef struct sxT316_platform_data *psxT316_platform_data_t;

/***************************************
*		define data struct/interrupt
*
***************************************/
//#define USE_THREADED_IRQ
	
#define MAX_NUM_STATUS_BITS (2)

typedef struct sxT316XX sxT316XX_t, *psxT316XX_t;
struct sxT316XX 
{
  void * bus; /* either i2c_client or spi_client */
  
  struct device *pdev; /* common device struction for linux */

  void *pDevice; /* device specific struct pointer */

  /* Function Pointers */
  int (*init)(psxT316XX_t this); /* (re)initialize device */

  /* since we are trying to avoid knowing registers, create a pointer to a
   * common read register which would be to read what the interrupt source
   * is from 
   */
  int (*refreshStatus)(psxT316XX_t this); /* read register status */

  int (*get_nirq_low)(void); /* get whether nirq is low (platform data) */
  
  /* array of functions to call for corresponding status bit */
  void (*statusFunc[MAX_NUM_STATUS_BITS])(psxT316XX_t this); 

#if defined(USE_THREADED_IRQ)
  struct mutex mutex;
#else  
  spinlock_t	      lock; /* Spin Lock used for nirq worker function */
#endif 
  int irq; /* irq number used */

  /* whether irq should be ignored.. cases if enable/disable irq is not used
   * or does not work properly */
  char irq_disabled;

  u8 useIrqTimer; /* older models need irq timer for pen up cases */

  int irqTimeout; /* msecs only set if useIrqTimer is true */

  /* struct workqueue_struct	*ts_workq;  */  /* if want to use non default */
	struct delayed_work dworker; /* work struct for worker function */
 
    bool loading_fw;
    const char *fw_name;
	struct work_struct fw_update_work;

#if 0//def CONFIG_HAS_WAKELOCK
  struct early_suspend early_suspend;  /* early suspend data  */
#endif  
};

static struct _totalButtonInformation smtcButtonInformation = {
  .buttons = psmtcButtons,
  .buttonSize = ARRAY_SIZE(psmtcButtons),
};



void sxT316XX_suspend(psxT316XX_t this);
void sxT316XX_resume(psxT316XX_t this);
int sxT316XX_init(psxT316XX_t this);
int sxT316XX_remove(psxT316XX_t this);

#endif
