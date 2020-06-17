//fengnan@wind-mobi.com 20180227 begin
#ifdef CONFIG_INPUT_SX9310
#include <linux/input/sx9310.h>
#endif

#ifdef CONFIG_INPUT_SX9310
/* IO Used for NIRQ */
#define GPIO_SX9310_NIRQ S3C64XX_GPN(9) //here use GPN9/EINT9 as IRQ

#define SX9310_NIRQ 110//use EINT9 as interrupt pin//gpio_to_irq(GPIO_SX9310_NIRQ)

static int sx9310_get_nirq_state(void)
{
	return !gpio_get_value(GPIO_SX9310_NIRQ);
}

static inline void __init sx9310_init(void)
{
	if ((gpio_request(GPIO_SX9310_NIRQ, "SX9310_NIRQ") == 0) &&
	    (gpio_direction_input(GPIO_SX9310_NIRQ) == 0)) {
		gpio_export(GPIO_SX9310_NIRQ, 0);
		s3c_gpio_setpull(GPIO_SX9310_NIRQ,S3C_GPIO_PULL_UP);
    printk(KERN_ERR "obtained gpio for SX9310_NIRQ\n");
	} else {
		printk(KERN_ERR "could not obtain gpio for SX9310_NIRQ\n");
		return;
	}
}

static struct _totalButtonInformation smtcButtonInformation = {
  .buttons = psmtcButtons,
  .buttonSize = ARRAY_SIZE(psmtcButtons),
};

static sx9310_platform_data_t sx9310_config = {
  /* Function pointer to get the NIRQ state (1->NIRQ-low, 0->NIRQ-high) */
  .get_is_nirq_low = sx9310_get_nirq_state,
  /*  pointer to an initializer function. Here in case needed in the future */
  //.init_platform_hw = sx9310_init_ts,
  .init_platform_hw = NULL,
  /*  pointer to an exit function. Here in case needed in the future */
  //.exit_platform_hw = sx9310_exit_ts,
  .exit_platform_hw = NULL,
	
  .pi2c_reg = sx9310_i2c_reg_setup,
  .i2c_reg_num = ARRAY_SIZE(sx9310_i2c_reg_setup),

  .pbuttonInformation = &smtcButtonInformation,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
	{
	I2C_BOARD_INFO("sx9310",0x28),
	.flags = I2C_CLIENT_WAKE,
	.irq = SX9310_NIRQ,
	.platform_data = &sx9310_config,
	},
};

static void __init smdk6410_machine_init(void)
{
	//.....
	sx9310_init();
	//.....
}
#endif
//fengnan@wind-mobi.com 20180227 end