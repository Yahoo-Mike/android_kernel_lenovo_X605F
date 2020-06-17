/*
 * Copyright (c) 2015, 2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SDHCI_MSM_H__
#define __SDHCI_MSM_H__

#include <linux/mmc/mmc.h>
#include <linux/pm_qos.h>
#include "sdhci-pltfm.h"

/* This structure keeps information per regulator */
struct sdhci_msm_reg_data {
	/* voltage regulator handle */
	struct regulator *reg;
	/* regulator name */
	const char *name;
	/* voltage level to be set */
	u32 low_vol_level;
	u32 high_vol_level;
	/* Load values for low power and high power mode */
	u32 lpm_uA;
	u32 hpm_uA;

	/* is this regulator enabled? */
	bool is_enabled;
	/* is this regulator needs to be always on? */
	bool is_always_on;
	/* is low power mode setting required for this regulator? */
	bool lpm_sup;
	bool set_voltage_sup;
};

/*
 * This structure keeps information for all the
 * regulators required for a SDCC slot.
 */
struct sdhci_msm_slot_reg_data {
	/* keeps VDD/VCC regulator info */
	struct sdhci_msm_reg_data *vdd_data;
	 /* keeps VDD IO regulator info */
	struct sdhci_msm_reg_data *vdd_io_data;
};

struct sdhci_msm_gpio {
	u32 no;
	const char *name;
	bool is_enabled;
};

struct sdhci_msm_gpio_data {
	struct sdhci_msm_gpio *gpio;
	u8 size;
};

struct sdhci_msm_pin_data {
	/*
	 * = 1 if controller pins are using gpios
	 * = 0 if controller has dedicated MSM pads
	 */
	u8 is_gpio;
	struct sdhci_msm_gpio_data *gpio_data;
};

struct sdhci_pinctrl_data {
	struct pinctrl          *pctrl;
	struct pinctrl_state    *pins_active;
	struct pinctrl_state    *pins_sleep;
};

struct sdhci_msm_bus_voting_data {
	struct msm_bus_scale_pdata *bus_pdata;
	unsigned int *bw_vecs;
	unsigned int bw_vecs_size;
};

struct sdhci_msm_cpu_group_map {
	int nr_groups;
	cpumask_t *mask;
};

struct sdhci_msm_pm_qos_latency {
	s32 latency[SDHCI_POWER_POLICY_NUM];
};

struct sdhci_msm_pm_qos_data {
	struct sdhci_msm_cpu_group_map cpu_group_map;
	enum pm_qos_req_type irq_req_type;
	int irq_cpu;
	struct sdhci_msm_pm_qos_latency irq_latency;
	struct sdhci_msm_pm_qos_latency *cmdq_latency;
	struct sdhci_msm_pm_qos_latency *latency;
	bool irq_valid;
	bool cmdq_valid;
	bool legacy_valid;
};

/*
 * PM QoS for group voting management - each cpu group defined is associated
 * with 1 instance of this structure.
 */
struct sdhci_msm_pm_qos_group {
	struct pm_qos_request req;
	struct delayed_work unvote_work;
	atomic_t counter;
	s32 latency;
};

/* PM QoS HW IRQ voting */
struct sdhci_msm_pm_qos_irq {
	struct pm_qos_request req;
	struct delayed_work unvote_work;
	struct device_attribute enable_attr;
	struct device_attribute status_attr;
	atomic_t counter;
	s32 latency;
	bool enabled;
};

struct sdhci_msm_pltfm_data {
	/* Supported UHS-I Modes */
	u32 caps;

	/* More capabilities */
	u32 caps2;

	unsigned long mmc_bus_width;
	struct sdhci_msm_slot_reg_data *vreg_data;
	bool nonremovable;
	bool nonhotplug;
	bool largeaddressbus;
	bool pin_cfg_sts;
	struct sdhci_msm_pin_data *pin_data;
	struct sdhci_pinctrl_data *pctrl_data;
	int status_gpio; /* card detection GPIO that is configured as IRQ */
	struct sdhci_msm_bus_voting_data *voting_data;
	u32 *sup_clk_table;
	unsigned char sup_clk_cnt;
	int sdiowakeup_irq;
	u32 *sup_ice_clk_table;
	unsigned char sup_ice_clk_cnt;
	u32 ice_clk_max;
	u32 ice_clk_min;
	struct sdhci_msm_pm_qos_data pm_qos_data;
	bool sdr104_wa;
};

struct sdhci_msm_bus_vote {
	uint32_t client_handle;
	uint32_t curr_vote;
	int min_bw_vote;
	int max_bw_vote;
	bool is_max_bw_needed;
	struct delayed_work vote_work;
	struct device_attribute max_bus_bw;
};

struct sdhci_msm_ice_data {
	struct qcom_ice_variant_ops *vops;
	struct platform_device *pdev;
	int state;
};

//BH201LN driver--ernest.zhang@bayhubtech.com modify at 20190620 begin
#ifndef TUNING_PHASE_SIZE
#define TUNING_PHASE_SIZE 11
#endif

#define	SD_FNC_AM_SDR12		0x0
#define	SD_FNC_AM_SDR25		0x1
#define	SD_FNC_AM_HS		0x1
#define	SD_FNC_AM_DS		0x0

#define	SD_FNC_AM_SDR50		0x2
#define	SD_FNC_AM_SDR104	0x3
#define	SD_FNC_AM_DDR50		0x4

#define BIT_PASS_MASK  (0x7ff )
#define SDR104_MANUAL_INJECT 0x3ff
#define SDR50_MANUAL_INJECT  0x77f

#define  TRUNING_RING_IDX(x)  ((x)% TUNING_PHASE_SIZE)
#define  GET_IDX_VALUE(tb, x)  (tb &(1 << (x)))
#define  GENERATE_IDX_VALUE( x)  (1 << (x))
#define  GET_TRUNING_RING_IDX_VALUE(tb, x)  (tb &(1 << TRUNING_RING_IDX(x)))
#define  GENERATE_TRUNING_RING_IDX_VALUE( x)  (1 << TRUNING_RING_IDX(x))

// tb list
u32 all_selb_failed_tb_get(struct sdhci_host *host, int sela);
void all_selb_failed_tb_update(struct sdhci_host *host, int sela, u32 val);

u32 tx_selb_failed_tb_get(struct sdhci_host *host, int sela);
void tx_selb_failed_tb_update(struct sdhci_host *host, int sela, u32 val);

u32 tx_selb_failed_history_get(struct sdhci_host *host);
void tx_selb_failed_history_update(struct sdhci_host *host , u32 val);

void _ggc_reset_tuning_result_for_dll(struct sdhci_host * host);
void ggc_reset_selx_failed_tb(struct sdhci_host *host);
//dbg dump
void vct_2_string(u8 * tb, u32 n);
void dump_selx_vct_with_comment(char * info, u32 vct);
//func
int get_best_window_phase(u32 vct, int * max_pass_win)  ;
int get_best_window_phase_shift_left(u32 vct, int * pmax_pass_win);
bool  get_refine_sel(struct sdhci_host *host, u32 tga, u32 tgb, u32 * rfa, u32 * rfb);
bool selx_failure_point_exist(u32 val);
int get_selx_weight(u32  val);
void tx_selb_calculate_valid_phase_range(u32 val, int* start, int *pass_cnt);
int get_selb_failure_point(int start, u64 raw_tx_selb, int tuning_cnt);
void tx_selb_inject_policy(struct sdhci_host *host, int tx_selb);

typedef struct
{

    u32 tx_selb_tb[TUNING_PHASE_SIZE];
    u32 all_selb_tb[TUNING_PHASE_SIZE];
    u32 tx_selb_failed_history;
    int bus_mode;
//
    int default_sela;
    int default_selb;
    u32 default_delaycode;
    u32 * dll_voltage_unlock_cnt;

    u32 max_delaycode;
    u32 min_delaycode;
    u32  delaycode_narrowdown_index;
    u32 fail_phase;
} ggc_bus_mode_cfg_t, * pggc_bus_mode_cfg_t;

typedef enum
{
    NO_TUNING = 0,
    OUTPUT_PASS_TYPE = 1,
    SET_PHASE_FAIL_TYPE = 2,
    TUNING_FAIL_TYPE = 3,
    READ_STATUS_FAIL_TYPE = 4,
    TUNING_CMD7_TIMEOUT = 5,
    RETUNING_CASE = 6,
} tuning_stat_et;

typedef struct ggc_platform_data
{
//
    ggc_bus_mode_cfg_t sdr50;
    ggc_bus_mode_cfg_t sdr104;
    pggc_bus_mode_cfg_t  cur_bus_mode;
//
    u8 _cur_cfg_data[512];//for write data & cache it for merge
    u8 _cur_read_buf[512];//only for read
//
    u32 ggc_cur_ssc_level;

//
    int ssc_crc_recovery_retry_cnt;

    u32 crc_retry_cnt;
//


    u32 cclk_ds_18v;
    u32 cdata_ds_18v;
    u32 ds_inc_cnt;
    bool ggc_400k_update_ds_flg;
    bool ggc_400k_update_ssc_arg_flg;

//
    bool ggc_ocb_in_check;
    bool dll_unlock_reinit_flg;
    u8 driver_strength_reinit_flg;
    bool tuning_cmd7_timeout_reinit_flg;
    u32 tuning_cmd7_timeout_reinit_cnt;
    u32  ggc_400k_setting_change_flg: 1;
    u32 ggc_cur_sela;
    u32 ggc_target_selb;
    u32 target_tuning_pass_win;
    bool selx_tuning_done_flag;
    u32 ggc_cmd_tx_selb_failed_range;
    int ggc_sw_selb_tuning_first_selb;
    tuning_stat_et ggc_sela_tuning_result[11];
    int dll_voltage_scan_map[4];
    int cur_dll_voltage_idx;
    //bool ggc_read_delay_code_flg;
} ggc_platform_t;
//BH201LN driver--ernest.zhang@bayhubtech.com modify at 20190620 end


struct sdhci_msm_host {
	struct platform_device	*pdev;
	void __iomem *core_mem;    /* MSM SDCC mapped address */
	int	pwr_irq;	/* power irq */
	struct clk	 *clk;     /* main SD/MMC bus clock */
	struct clk	 *pclk;    /* SDHC peripheral bus clock */
	struct clk	 *bus_clk; /* SDHC bus voter clock */
	struct clk	 *ff_clk; /* CDC calibration fixed feedback clock */
	struct clk	 *sleep_clk; /* CDC calibration sleep clock */
	struct clk	 *ice_clk; /* SDHC peripheral ICE clock */
	atomic_t clks_on; /* Set if clocks are enabled */
	struct sdhci_msm_pltfm_data *pdata;
	struct mmc_host  *mmc;
	struct sdhci_pltfm_data sdhci_msm_pdata;
	u32 curr_pwr_state;
	u32 curr_io_level;
	struct completion pwr_irq_completion;
	struct sdhci_msm_bus_vote msm_bus_vote;
	struct device_attribute	polling;
	u32 clk_rate; /* Keeps track of current clock rate that is set */
	bool tuning_done;
	bool calibration_done;
	u8 saved_tuning_phase;
	bool en_auto_cmd21;
	struct device_attribute auto_cmd21_attr;
	bool is_sdiowakeup_enabled;
	bool sdio_pending_processing;
	atomic_t controller_clock;
	bool use_cdclp533;
	bool use_updated_dll_reset;
	bool use_14lpp_dll;
	bool enhanced_strobe;
	bool rclk_delay_fix;
	u32 caps_0;
	struct sdhci_msm_ice_data ice;
	u32 ice_clk_rate;
	struct sdhci_msm_pm_qos_group *pm_qos;
	int pm_qos_prev_cpu;
	struct device_attribute pm_qos_group_enable_attr;
	struct device_attribute pm_qos_group_status_attr;
	bool pm_qos_group_enable;
	struct sdhci_msm_pm_qos_irq pm_qos_irq;
	bool tuning_in_progress;
	bool core_3_0v_support;
	bool pltfm_init_done;
	//BH201LN driver--ernest.zhang@bayhubtech.com modify at 20190620 start
	ggc_platform_t  ggc;
	//BH201LN driver--ernest.zhang@bayhubtech.com modify at 20190620 end	
	//BH201LN driver--sunsiyuan@wind-mobi.com modify at 20180326 begin
	int sdr50_notuning_sela_inject_flag;
	int sdr50_notuning_crc_error_flag;
	u32 sdr50_notuning_sela_rx_inject;
	//BH201LN driver--sunsiyuan@wind-mobi.com modify at 20180326 end
};

extern char *saved_command_line;


void sdhci_msm_pm_qos_irq_init(struct sdhci_host *host);
void sdhci_msm_pm_qos_irq_vote(struct sdhci_host *host);
void sdhci_msm_pm_qos_irq_unvote(struct sdhci_host *host, bool async);

void sdhci_msm_pm_qos_cpu_init(struct sdhci_host *host,
		struct sdhci_msm_pm_qos_latency *latency);
void sdhci_msm_pm_qos_cpu_vote(struct sdhci_host *host,
		struct sdhci_msm_pm_qos_latency *latency, int cpu);
bool sdhci_msm_pm_qos_cpu_unvote(struct sdhci_host *host, int cpu, bool async);

//add for BH201LN driver modify at 2019/7/21 end
#define IC_ADD_FUNCTION  1
#if IC_ADD_FUNCTION
void ggc_tuning_result_reset(struct sdhci_host *host);
void set_gg_reg_cur_val(u8 * data);
void get_gg_reg_cur_val(u8 * data);
bool ggc_hw_inject_may_recursive(struct sdhci_host * host, u32 sel200, u32 sel100, bool writetobh201);

#endif
//add for BH201LN driver modify at 2019/7/21 end

#endif /* __SDHCI_MSM_H__ */
