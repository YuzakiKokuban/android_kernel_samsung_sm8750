/*
 *
 * $Id: stk6d2x.h
 *
 * Copyright (C) 2012~2018 Bk, sensortek Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#ifndef __STK6D2X_H__
#define __STK6D2X_H__

#if IS_ENABLED(CONFIG_SENSORS_FLICKER_SELF_TEST)
#include "flicker_test.h"
#endif

#include "common_define.h"
#include "stk6d2x_ver.h"

#define STK6D2X_DEV_NAME         "STK6D2X"
//#define ALS_NAME               "lightsensor-level"
#define MODULE_NAME_ALS		 "als_rear"
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG" %s\n", __FUNCTION__)
#define APS_ERR(fmt, args...)    printk(KERN_ERR APS_TAG" %s %d: "fmt"\n", __FUNCTION__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    printk(KERN_INFO APS_TAG" %s %d: "fmt"\n", __FUNCTION__, __LINE__, ##args)
#define APS_DBG(fmt, args...)    printk(KERN_INFO APS_TAG" %s %d: "fmt"\n", __FUNCTION__, __LINE__, ##args)

/* Driver Settings */
#define STK_ALS_ENABLE
// #define STK_ALS_CALI
#define STK_FIFO_ENABLE
#define STK_ALS_AGC
#define STK_CHK_XFLG
#define CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS
#define CONFIG_AMS_ALWAYS_ON_MODE_FOR_AUTO_BRIGHTNESS

#ifdef STK_FIFO_ENABLE
	// #define STK_DATA_SUMMATION
	// #define STK_FIFO_DATA_SUMMATION
	#define STK_FFT_FLICKER
	#ifdef STK_FFT_FLICKER
		#define STK_CHK_CLK_SRC
		#define SEC_FFT_FLICKER_1024
	#endif
#endif

/* Define Register Map */
#define STK6D2X_REG_STATE                   0x00
#define STK6D2X_REG_ALS01_DGAIN             0x01
#define STK6D2X_REG_ALS2_DGAIN              0x02
#define STK6D2X_REG_IT1                     0x03
#define STK6D2X_REG_IT2                     0x04
#define STK6D2X_REG_WAIT1                   0x05
#define STK6D2X_REG_WAIT2                   0x06
#define STK6D2X_REG_ALS_SUM_GAIN1           0x08
#define STK6D2X_REG_ALS_SUM_GAIN2           0x09
#define STK6D2X_REG_THDH1_ALS               0x0A
#define STK6D2X_REG_THDH2_ALS               0x0B
#define STK6D2X_REG_THDL1_ALS               0x0C
#define STK6D2X_REG_THDL2_ALS               0x0D
#define STK6D2X_REG_ALS_IT_EXT              0x0E
#define STK6D2X_REG_FLAG                    0x10
#define STK6D2X_REG_DATA1_ALS0              0x11
#define STK6D2X_REG_DATA2_ALS0              0x12
#define STK6D2X_REG_DATA1_ALS1              0x13
#define STK6D2X_REG_DATA2_ALS1              0x14
#define STK6D2X_REG_DATA1_ALS2              0x15
#define STK6D2X_REG_DATA2_ALS2              0x16
#define STK6D2X_REG_AGC1_DG                 0x17
#define STK6D2X_REG_AGC2_DG                 0x18
#define STK6D2X_REG_AGC_CROS_THD_FLAG       0x19
#define STK6D2X_REG_AGC_AG                  0x1A
#define STK6D2X_REG_AGC_PD                  0x1B
#define STK6D2X_REG_DATA1_ALS0_SUM          0x1C
#define STK6D2X_REG_DATA2_ALS0_SUM          0x1D
#define STK6D2X_REG_DATA1_ALS1_SUM          0x1E
#define STK6D2X_REG_DATA2_ALS1_SUM          0x1F
#define STK6D2X_REG_DATA1_ALS2_SUM          0x20
#define STK6D2X_REG_DATA2_ALS2_SUM          0x21
#define STK6D2X_REG_DATA_AGC_SUM            0x22
#define STK6D2X_REG_RID                     0x3F
#define STK6D2X_REG_ALS_PRST                0x40
#define STK6D2X_REG_FIFO1                   0x60
#define STK6D2X_REG_FIFO1_WM_LV             0x61
#define STK6D2X_REG_FIFO2_WM_LV             0x62
#define STK6D2X_REG_FIFO_FCNT1              0x64
#define STK6D2X_REG_FIFO_FCNT2              0x65
#define STK6D2X_REG_FIFO_OUT                0x66
#define STK6D2X_REG_AGC1                    0x6A
#define STK6D2X_REG_AGC2                    0x6B
#define STK6D2X_REG_ALS_SUM                 0x70
#define STK6D2X_REG_SW_RESET                0x80
#define STK6D2X_REG_PDT_ID                  0x92
#define STK6D2X_REG_INT2                    0xA5
#define STK6D2X_REG_XFLAG                   0xA6
#define STK6D2X_REG_ALS_AGAIN               0xDB
#define STK6D2X_REG_ALS_PD_REDUCE           0xF4

/* Define state reg */
#define STK6D2X_STATE_EN_SUMMATION_SHIFT       3
#define STK6D2X_STATE_EN_WAIT_SHIFT            2
#define STK6D2X_STATE_EN_ALS_SHIFT             1
#define STK6D2X_STATE_EN_FSM_RESTART_MASK      0x10
#define STK6D2X_STATE_EN_SUMMATION_MASK        0x08
#define STK6D2X_STATE_EN_WAIT_MASK             0x04
#define STK6D2X_STATE_EN_ALS_MASK              0x02

/* Define ALS DGAIN reg */
#define STK6D2X_ALS2_DGAIN_SHIFT        4
#define STK6D2X_ALS1_DGAIN_SHIFT        0
#define STK6D2X_ALS0_DGAIN_SHIFT        4
#define STK6D2X_ALS2_DGAIN_MASK         0x70
#define STK6D2X_ALS1_DGAIN_MASK         0x07
#define STK6D2X_ALS0_DGAIN_MASK         0x70

/* Define interrupt reg */
#define STK6D2X_INT_ALS_SHIFT           3
#define STK6D2X_INT_ALS_MASK            0x08
#define STK6D2X_INT_ALS                 0x08

/* Define flag reg */
#define STK6D2X_FLG_ALSDR_SHIFT            7
#define STK6D2X_FLG_ALSINT_SHIFT           5
#define STK6D2X_FLG_ALSSAT_SHIFT           2

#define STK6D2X_FLG_ALSDR_MASK          0x80
#define STK6D2X_FLG_ALSINT_MASK         0x20
#define STK6D2X_FLG_ALSSAT_MASK         0x04

/* Define ALS parameters */
#define STK6D2X_ALS_PRS1               0x00
#define STK6D2X_ALS_PRS2               0x40
#define STK6D2X_ALS_PRS4               0x80
#define STK6D2X_ALS_PRS8               0xC0

#define STK6D2X_ALS_DGAIN1             0x00
#define STK6D2X_ALS_DGAIN4             0x01
#define STK6D2X_ALS_DGAIN16            0x02
#define STK6D2X_ALS_DGAIN64            0x03
#define STK6D2X_ALS_DGAIN128           0x04
#define STK6D2X_ALS_DGAIN256           0x05
#define STK6D2X_ALS_DGAIN512           0x06
#define STK6D2X_ALS_DGAIN1024          0x07
#define STK6D2X_ALS_DGAIN_MASK         0x07

#define STK6D2X_ALS_SUM_GAIN_DIV1      0x00
#define STK6D2X_ALS_SUM_GAIN_DIV2      0x01
#define STK6D2X_ALS_SUM_GAIN_DIV4      0x02
#define STK6D2X_ALS_SUM_GAIN_DIV8      0x03
#define STK6D2X_ALS_SUM_GAIN_DIV6      0x04
#define STK6D2X_ALS_SUM_GAIN_DIV32     0x05
#define STK6D2X_ALS_SUM_GAIN_DIV64     0x06
#define STK6D2X_ALS_SUM_GAIN_DIV128    0x07
#define STK6D2X_ALS_SUM_GAIN_DIV256    0x08
#define STK6D2X_ALS_SUM_GAIN_MASK      0x0F

#define STK6D2X_ALS_CI_2_0             0x00
#define STK6D2X_ALS_CI_1_0             0x01
#define STK6D2X_ALS_CI_0_5             0x02

#define STK6D2X_ALS_PD_REDUCE_DIS      0x00
#define STK6D2X_ALS_PD_REDUCE_LV1      0x01
#define STK6D2X_ALS_PD_REDUCE_LV2      0x02

#define STK6D2X_ALS_IT25               0x3A9
#define STK6D2X_ALS_IT40               0x5DB
#define STK6D2X_ALS_IT50               0x753
#define STK6D2X_ALS_IT100              0xEA6
#define STK6D2X_ALS_IT200              0x1D4D
#define STK6D2X_ALS_IT1_MASK           0x3F

#define STK6D2X_ALS_IT_EXT_MASK        0x3F

#define STK6D2X_WAIT20                 0x359
#define STK6D2X_WAIT50                 0x85F
#define STK6D2X_WAIT100                0x10BE

#define STK6D2X_PRST1                   0x00
#define STK6D2X_PRST2                   0x01
#define STK6D2X_PRST4                   0x02
#define STK6D2X_PRST8                   0x03

#define STK6D2X_FLG_ALS2_DG_MASK        0x40
#define STK6D2X_FLG_ALS1_DG_MASK        0x20
#define STK6D2X_FLG_ALS0_DG_MASK        0x10
#define STK6D2X_ALS2_AGC_DG_MASK        0x07
#define STK6D2X_ALS1_AGC_DG_MASK        0x70
#define STK6D2X_ALS0_AGC_DG_MASK        0x07

#define STK6D2X_ALS2_AGC_AG_MASK        0x30
#define STK6D2X_ALS1_AGC_AG_MASK        0x0C
#define STK6D2X_ALS0_AGC_AG_MASK        0x03
#define STK6D2X_ALS2_AGC_PD_MASK        0x30
#define STK6D2X_ALS1_AGC_PD_MASK        0x0C
#define STK6D2X_ALS0_AGC_PD_MASK        0x01
#define STK6D2X_ALS0_AGC_PDMODE_MASK    0xC0
#define STK6D2X_ALS0_AGC_PDMODE0        0x00
#define STK6D2X_ALS0_AGC_PDMODE1        0x01
#define STK6D2X_ALS0_AGC_PDMODE_SHIFT   6

#define STK6D2X_FLG_ALS2_SUM_AGC_MASK   0x40
#define STK6D2X_FLG_ALS1_SUM_AGC_MASK   0x20
#define STK6D2X_FLG_ALS0_SUM_AGC_MASK   0x10

#define STK6D2X_TYPE_ALS0               0
#define STK6D2X_TYPE_ALS1               1
#define STK6D2X_TYPE_ALS2               2

/** sw reset value */
#define STK_STK6D2X_SWRESET             0x00

/** Off to idle time */
#define STK6D2X_OFF_TO_IDLE_MS          10  //ms

/** ALS threshold */
#define STK6D2X_ALS_THD_ADJ             0.05
#define STK6D2X_NUM_AXES                3
#define STK6D2X_MAX_MIN_DIFF            200
#define STK6D2X_LT_N_CT                 1700
#define STK6D2X_HT_N_CT                 2200

#define STK6D2X_ALS_DATA_READY_TIME     60
#define STK6D2X_ALS_THRESHOLD           30

#define STK6D2X_ALS_SUMMATION_CNT       51

#ifdef STK_ALS_CALI
	#define STK6D2X_ALS_CALI_DATA_READY_US  55000000
	#define STK6D2X_ALS_CALI_TARGET_LUX     500
#endif

#ifdef STK_UV_CALI
	#define STK6D2X_UV_CALI_TARGET          10
#endif

#define STK_FLICKER_IT                      0x0C
#define STK_FLICKER_EXT_IT                  0x0D

#ifdef STK_FIFO_ENABLE
	#define STK_FIFO_DATA_BUFFER_LEN            1024
	#define STK_FIFO_I2C_READ_FRAME             30//get fifo data one time
	#define STK_FIFO_I2C_READ_FRAME_BUF_SIZE	STK_FIFO_I2C_READ_FRAME * 10
	#define STK_FIFO_I2C_READ_FRAME_TARGET      102

#ifdef STK_FFT_FLICKER
	#define FFT_BLOCK_SIZE 128
	#define FFT_BUF_SIZE 1024
#endif

	#define  STK6D2X_FIFO_SEL_ALS0                      0x00
	#define  STK6D2X_FIFO_SEL_ALS1                      0x01
	#define  STK6D2X_FIFO_SEL_ALS2                      0x02
	#define  STK6D2X_FIFO_SEL_ALS0_ALS1                 0x03
	#define  STK6D2X_FIFO_SEL_ALS0_ALS2                 0x04
	#define  STK6D2X_FIFO_SEL_ALS1_ALS2                 0x05
	#define  STK6D2X_FIFO_SEL_ALS0_ALS1_ALS2            0x06
	#define  STK6D2X_FIFO_SEL_STA0_ALS0                 0x08
	#define  STK6D2X_FIFO_SEL_STA1_ALS1                 0x09
	#define  STK6D2X_FIFO_SEL_STA2_ALS2                 0x0A
	#define  STK6D2X_FIFO_SEL_STA01_ALS0_ALS1           0x0B
	#define  STK6D2X_FIFO_SEL_STA02_ALS0_ALS2           0x0C
	#define  STK6D2X_FIFO_SEL_STA12_ALS1_ALS2           0x0D
	#define  STK6D2X_FIFO_SEL_STA01_STA2_ALS0_ALS1_ALS2 0x0E
	#define  STK6D2X_FIFO_SEL_MASK                      0x0F

	#define  STK6D2X_FIFO_MODE_OFF              0x00
	#define  STK6D2X_FIFO_MODE_BYPASS           0x10
	#define  STK6D2X_FIFO_MODE_NORMAL           0x20
	#define  STK6D2X_FIFO_MODE_STREAM           0x30

	#define  STK6D2X_FOVR_EN_MASK               0x04
	#define  STK6D2X_FWM_EN_MASK                0x02
	#define  STK6D2X_FFULL_EN_MASK              0x01

	#define  STK6D2X_FLG_FIFO_OVR_MASK          0x04
	#define  STK6D2X_FLG_FIFO_WM_MASK           0x02
	#define  STK6D2X_FLG_FIFO_FULL_MASK         0x01
#endif

#define STK_ALSPS_TIMER_MS                      90

#define STK6D2X_CALI_FILE               "/persist/sensors/stkalpscali.conf"
#define STK_CHANNEL_NUMBER              3
#define STK_CHANNEL_OFFSET              3
#define STK_CHANNEL_GOLDEN              10

#define STK6D2X_REG_READ(stk_data, reg, val)                    ((stk_data)->bops->read((stk_data)->bus_idx, reg, val))
#define STK6D2X_REG_WRITE(stk_data, reg, val)                   ((stk_data)->bops->write((stk_data)->bus_idx, reg, val))
#define STK6D2X_REG_WRITE_BLOCK(stk_data, reg, val, len)        ((stk_data)->bops->write_block((stk_data)->bus_idx, reg, val, len))
#define STK6D2X_REG_READ_MODIFY_WRITE(stk_data, reg, val, mask) ((stk_data)->bops->read_modify_write((stk_data)->bus_idx, reg, val, mask))
#define STK6D2X_REG_BLOCK_READ(stk_data, reg, count, buf)       ((stk_data)->bops->read_block((stk_data)->bus_idx, reg, count, buf))

#define STK6D2X_TIMER_REGISTER(stk_data, t_info)                ((stk_data)->tops->register_timer(t_info))
#define STK6D2X_TIMER_START(stk_data, t_info)                   ((stk_data)->tops->start_timer(t_info))
#define STK6D2X_TIMER_STOP(stk_data, t_info)                    ((stk_data)->tops->stop_timer(t_info))
#define STK6D2X_TIMER_REMOVE(stk_data, t_info)                  ((stk_data)->tops->remove(t_info))
#define STK6D2X_TIMER_BUSY_WAIT(stk_data, min, max, mode)       ((stk_data)->tops->busy_wait(min, max, mode))

#define STK6D2X_GPIO_IRQ_REGISTER(stk_data, g_info)             ((stk_data)->gops->register_gpio_irq(g_info))
#define STK6D2X_GPIO_IRQ_START(stk_data, g_info)                ((stk_data)->gops->start_gpio_irq(g_info))
#define STK6D2X_GPIO_IRQ_STOP(stk_data, g_info)                 ((stk_data)->gops->stop_gpio_irq(g_info))
#define STK6D2X_GPIO_IRQ_REMOVE(stk_data, g_info)               ((stk_data)->gops->remove(g_info))

#define STK6D2X_ALS_REPORT(stk_data, als_data, num)         if ((stk_data)->als_report_cb)      ((stk_data)->als_report_cb(stk_data, als_data, num))

#define STK_ABS(x)              ((x < 0)? (-x):(x))

enum fft_size {
	FFT_128 = 128,
	FFT_256 = 256,
	FFT_512 = 512,
	FFT_1024 = 1024,
	//FFT_2048 = 2048,  // not ready
};

// sec_fft
#define SEC_FFT_SIZE FFT_1024
#define SAMPLING_TIME                   (488319)  /* nsec (2048Hz) */
//#define FLICKER_AVG_SHIFT               7
#define FLICKER_AVG_SHIFT               14
#define SEC_LOCAL_AVG_SIZE	            128  //  1024/n  recommended,  no 1024

// average/max thd
#define FLICKER_AVGMAX_THD      65LL
#define FLICKER_AVGMAX_THD2     29LL

// sec_ calc_thd
#define FLICKER_THD_CLEAR       1800LL    /* 1.8 * 1000 */
#define FLICKER_THD_RATIO       3LL       /* 0.003 * 1000 */
#define FLICKER_GAIN_MAX        256LL    /* 256 */
#define FLICKER_THD_RATIO_AUTO  1000LL    /* 1 * 1000 */

typedef struct stk6d2x_data stk6d2x_data;
typedef void (*STK_REPORT_CB)(stk6d2x_data *, uint32_t*, uint32_t);

/* platform data */
struct stk6d2x_platform_data
{
	uint32_t    als_scale;
	uint32_t    int_flags;
};
typedef enum
{
	STK6D2X_NONE        = 0x0,
	STK6D2X_ALS         = 0x1,
	STK6D2X_ALL         = 0x2,
} stk6d2x_sensor_type;

typedef enum
{
	STK6D2X_DATA_TIMER_ALPS,
#ifdef STK_FIFO_ENABLE
	//STK6D2X_FIFO_RELEASE_TIMER,
#endif
} stk6d2x_timer_type;

typedef struct stk6d2x_register_table
{
	uint8_t address;
	uint8_t value;
	uint8_t mask_bit;
} stk6d2x_register_table;

#ifdef STK_FIFO_ENABLE
typedef enum
{
	STK6D2X_FIFO_ALS0 = 0x0,
	STK6D2X_FIFO_ALS1 = 0x1,
	STK6D2X_FIFO_ALS2 = 0x2,
	STK6D2X_FIFO_ALS0_ALS1 = 0x3,
	STK6D2X_FIFO_ALS0_ALS2 = 0x4,
	STK6D2X_FIFO_ALS1_ALS2 = 0x5,
	STK6D2X_FIFO_ALS0_ALS1_ALS2 = 0x6,
	STK6D2X_FIFO_STA0_ALS0 = 0x8,
	STK6D2X_FIFO_STA1_ALS1 = 0x9,
	STK6D2X_FIFO_STA2_ALS2 = 0xA,
	STK6D2X_FIFO_STA01_ALS0_ALS1 = 0xB,
	STK6D2X_FIFO_STA02_ALS0_ALS2 = 0xC,
	STK6D2X_FIFO_STA12_ALS1_ALS2 = 0xD,
	STK6D2X_FIFO_STA01_STA2_ALS0_ALS1_ALS2 = 0xE,
	STK6D2X_FIFO_UNAVAILABLE = 0xF,
} stk6d2x_frame_type;
#endif

#ifdef STK_ALS_CALI
typedef enum
{
	STK6D2X_CALI_IDLE,
	STK6D2X_CALI_RUNNING,
	STK6D2X_CALI_FAILED,
	STK6D2X_CALI_DONE
} stk6d2x_calibration_status;
#endif

#ifdef STK_ALS_AGC
typedef enum
{
	STK6D2X_ALS_DGAIN_MULTI1     = 1,
	STK6D2X_ALS_DGAIN_MULTI4     = 4,
	STK6D2X_ALS_DGAIN_MULTI16    = 16,
	STK6D2X_ALS_DGAIN_MULTI64    = 64,
	STK6D2X_ALS_DGAIN_MULTI128   = 128,
	STK6D2X_ALS_DGAIN_MULTI256   = 256,
	STK6D2X_ALS_DGAIN_MULTI512   = 512,
	STK6D2X_ALS_DGAIN_MULTI1024  = 1024,
} stk6d2x_als_dgain_multi;

typedef enum
{
	STK6D2X_ALS_AGAIN_MULTI1   = 1,
	STK6D2X_ALS_AGAIN_MULTI2   = 2,
	STK6D2X_ALS_AGAIN_MULTI4   = 4,
} stk6d2x_als_again_multi;

typedef enum
{
	STK6D2X_ALS_PD_REDUCE_MULTI1   = 1,
	STK6D2X_ALS_PD_REDUCE_MULTI2   = 2,
	STK6D2X_ALS_PD_REDUCE_MULTI3   = 3,
	STK6D2X_ALS_PD_REDUCE_MULTI4   = 4,
} stk6d2x_als_pd_reduce_multi;
#endif

typedef struct stk6d2x_cali_table
{
	uint32_t als_version;
	uint32_t als_scale;
	uint32_t als_bias;
} stk6d2x_cali_table;

typedef struct stk6d2x_cali_info
{
#ifdef STK_ALS_CALI
	stk6d2x_calibration_status  cali_status;
#endif
	stk6d2x_cali_table          cali_para;
} stk6d2x_cali_info;

typedef struct stk6d2x_als_info
{
	bool                        first_init;
	bool                        cali_enable;
	bool                        enable;
	uint32_t                    scale;
	bool                        is_data_ready;
	uint16_t                    als_it;
	uint8_t                     cali_failed_count;
	uint64_t                    last_raw_data[3];
	uint16_t                    als_cali_data;
	uint16_t                    last_lux;
#ifdef STK_ALS_AGC
	uint16_t                    als_agc_sum;
	uint16_t                    als_cur_dgain[3];
	uint16_t                    als_cur_again[3];
	uint16_t                    als_cur_pd_reduce[3];
	uint8_t                     als_cur_pd_mode;
	uint32_t                    als_cur_ratio[3];
	uint8_t                     als_agc_sum_flag;
	uint32_t                    als_sum_gain_div[3];
#endif
} stk6d2x_als_info;

#ifdef STK_FIFO_ENABLE
typedef struct stk6d2x_fifo_info
{
	bool                        fifo_enable;
	bool                        is_fifobuf_alloc;
	uint32_t                    fifo_data0[FFT_BUF_SIZE];
	uint32_t                    fifo_data1[FFT_BUF_SIZE];
	uint32_t                    fifo_data2[FFT_BUF_SIZE];
#ifdef SEC_FFT_FLICKER_1024
	uint32_t                    fifo_data_clear[FFT_BUF_SIZE];
	uint32_t                    fifo_data_uv[FFT_BUF_SIZE];
	uint32_t                    fifo_data_ir[FFT_BUF_SIZE];
	uint32_t                    fifo_gain_clear[FFT_BUF_SIZE];
	uint32_t                    fifo_gain_uv[FFT_BUF_SIZE];
	uint32_t                    fifo_gain_ir[FFT_BUF_SIZE];
	uint8_t                     fifo_xflag[FFT_BUF_SIZE];
#endif
	uint64_t                    fifo_sum_als0;
	uint64_t                    fifo_sum_als1;
	uint64_t                    fifo_sum_als2;
	uint32_t                    fifo_sum_cnt;
	uint16_t                    target_frame_count;
	uint16_t                    read_frame;
	uint16_t                    read_max_data_byte;
	uint8_t                     latency_status;
	stk6d2x_frame_type          data_type;
	bool                        sel_mode;
	uint8_t                     frame_byte;
	uint8_t                     frame_data;
	uint16_t                    last_frame_count;
	bool                        fifo_reading;
	bool                        ext_clk_chk;
	bool                        pre_ext_clk_chk;
	uint16_t                    block_size;
#ifdef STK_FFT_FLICKER
	uint32_t                    fft_buf[FFT_BUF_SIZE];
	uint32_t                    fft_uv_buf[FFT_BUF_SIZE];
	uint32_t                    fft_ir_buf[FFT_BUF_SIZE];
	uint32_t                    fft_gain_clear[FFT_BUF_SIZE];
	uint32_t                    fft_gain_uv[FFT_BUF_SIZE];
	uint32_t                    fft_gain_ir[FFT_BUF_SIZE];
	uint8_t                     fft_xflag[FFT_BUF_SIZE];
	uint32_t                    fft_buf_idx;
#endif
} stk6d2x_fifo_info;
#endif

#define PWR_ON		1
#define PWR_OFF		0
#define PM_RESUME	1
#define PM_SUSPEND	0

struct stk6d2x_data
{
	struct stk6d2x_platform_data *pdata;
	const struct stk_bus_ops        *bops;
	struct mutex            config_lock;
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	struct mutex            enable_lock;
#endif
	const struct stk_timer_ops      *tops;
	const struct stk_gpio_ops       *gops;
	int                     bus_idx;
	stk_gpio_info           gpio_info;
#ifdef STK_ALS_CALI
	stk_timer_info          cali_timer_info;
#endif
	stk6d2x_cali_info       cali_info;
	stk6d2x_als_info        als_info;
	stk_timer_info          alps_timer_info;
	bool                    saturation;
#ifdef STK_FIFO_ENABLE
	stk6d2x_fifo_info       fifo_info;
	// fifo control timer
	stk_timer_info          fifo_release_timer_info;
	uint64_t                clear_local_average;
	uint64_t                uv_local_average;
	uint64_t                ir_local_average;
	bool                    is_clear_local_sat;
	bool                    is_uv_local_sat;
	bool                    is_ir_local_sat;
	int                     index_last;
	bool                    is_first;
	bool                    is_local_avg_update;
	uint32_t				flicker;
	uint32_t				uv_gain;
	uint32_t				clear_gain;
	uint32_t				ir_gain;
#endif
	uint8_t                 rid;
	uint8_t                 xflag;
	STK_REPORT_CB           als_report_cb;
	bool                    first_init;
	bool                    is_long_it;
	bool pm_state;
	u8 regulator_state;
	struct regulator*       regulator_vdd_1p8;
	struct regulator*       regulator_vbus_1p8;
	bool                    vdd_1p8_enable;
	bool                    vbus_1p8_enable;
	struct pinctrl          *als_pinctrl;
	struct pinctrl_state    *pins_sleep;
	struct pinctrl_state    *pins_active;
#if IS_ENABLED(CONFIG_SENSORS_FLICKER_SELF_TEST)
	bool                    eol_enabled;
	bool                    recover_state;
#endif
	int                     isTrimmed;
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
	bool als_flag;
	bool flicker_flag;
#endif
	int         			ext_clk_gpio;
	struct clk				*pclk;
	struct rcg_clk			*camcc_mclk5_rcg_clk;
	struct regulator		*reg;
	bool 					reg_enable;
	bool					use_ext_clk;
} ;

extern int als_debug;
extern int als_info;

#define ALS_DBG
//#define ALS_INFO

#ifndef ALS_dbg
#if defined(ALS_DBG)
#define ALS_dbg(format, arg...)		\
			printk(KERN_DEBUG "ALS_dbg : %s: %d " format, __func__, __LINE__, ##arg)
#define ALS_err(format, arg...)		\
			printk(KERN_DEBUG "ALS_err : %s: %d " format, __func__, __LINE__, ##arg)
#else
#define ALS_dbg(format, arg...)		{if (als_debug)\
			printk(KERN_DEBUG "ALS_dbg : %s: %d " format, __func__, __LINE__, ##arg);\
			}
#define ALS_err(format, arg...)		{if (als_debug)\
			printk(KERN_DEBUG "ALS_err : %s: %d " format, __func__, __LINE__, ##arg);\
			}
#endif
#endif

#ifndef ALS_info
#if defined(ALS_INFO)
#define ALS_info(format, arg...)	\
			printk(KERN_INFO "ALS_info : %s: %d " format, __func__, __LINE__, ##arg);
#else
#define ALS_info(format, arg...)	{if (als_info)\
			printk(KERN_INFO "ALS_info : %s: %d " format, __func__, __LINE__, ##arg);\
			}
#endif
#endif

int32_t stk6d2x_cali_als(struct stk6d2x_data *alps_data);
void stk6d2x_get_reg_default_setting(uint8_t reg, uint16_t* value);
int32_t stk6d2x_request_registry(struct stk6d2x_data *alps_data);
int32_t stk6d2x_update_registry(struct stk6d2x_data *alps_data);
int32_t stk6d2x_alps_set_config(stk6d2x_data *alps_data, bool en);
int32_t stk6d2x_init_all_setting(stk6d2x_data *alps_data);
int32_t stk6d2x_als_get_data(stk6d2x_data *alps_data, bool is_skip);
void stk6d2x_dump_reg(struct stk6d2x_data *alps_data);
void stk6d2x_force_stop(stk6d2x_data *alps_data);

extern int sensors_create_symlink(struct kobject *target, const char *name);
extern void sensors_remove_symlink(struct kobject *target, const char *name);
extern int sensors_register(struct device **dev, void *drvdata,
		struct device_attribute *attributes[], char *name);
extern void sensors_unregister(struct device *dev,
		struct device_attribute *attributes[]);

int32_t stk6d2x_enable_als(stk6d2x_data *alps_data, bool en);

#ifdef STK_ALS_AGC
	int32_t stk6d2x_cal_curDGain(uint8_t gain_val);
	uint8_t stk6d2x_als_get_again_multiple(uint8_t gain);
	uint8_t stk6d2x_als_get_pd_multiple(uint8_t gain, uint8_t pd_mode, uint8_t data_type);
	int32_t stk6d2x_get_curGain(struct stk6d2x_data *alps_data);
	void stk6d2x_get_als_ratio(struct stk6d2x_data *alps_data);
#ifdef SEC_FFT_FLICKER_1024
	uint8_t stk6d2x_sec_dgain(uint8_t gain_val);
	uint8_t stk6d2x_sec_again(uint8_t gain);
	uint8_t stk6d2x_sec_pd_multiple(uint8_t gain, uint8_t pd_mode, uint8_t data_type);
#endif
#endif
#ifdef STK_FIFO_ENABLE
	void stk6d2x_free_fifo_data(struct stk6d2x_data *alps_data);
	void stk6d2x_alloc_fifo_data(struct stk6d2x_data *alps_data, uint32_t size);
	void stk6d2x_fifo_init(struct stk6d2x_data *alps_data);
	int32_t stk6d2x_enable_fifo(struct stk6d2x_data *alps_data, bool en);
	void stk6d2x_fifo_get_data(struct stk6d2x_data *alps_data, uint16_t frame_num);
	void stk6d2x_get_fifo_data_polling(struct stk6d2x_data *alps_data);
	void stk6d2x_fifo_stop_control(stk_timer_info *t_info);
#endif
#if defined(CONFIG_AMS_ALS_COMPENSATION_FOR_AUTO_BRIGHTNESS)
void stk_als_init(struct stk6d2x_data *alps_data);
void stk_als_start(struct stk6d2x_data *alps_data);
void stk_als_stop(struct stk6d2x_data *alps_data);
#endif
int32_t stk_power_ctrl(struct stk6d2x_data *alps_data, bool en);
void stk6d2x_pin_control(struct stk6d2x_data *alps_data, bool pin_set);
void stk_sec_report(struct stk6d2x_data *alps_data);
#endif // __STK6D2X_H__
