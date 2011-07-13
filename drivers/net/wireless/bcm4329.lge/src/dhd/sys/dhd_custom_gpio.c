/*
* Customer code to add GPIO control during WLAN start/stop
* Copyright (C) 1999-2010, Broadcom Corporation
*
*      Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2 (the "GPL"),
* available at http://www.broadcom.com/licenses/GPLv2.php, with the
* following added to such license:
*
*      As a special exception, the copyright holders of this software give you
* permission to link this software with independent modules, and to copy and
* distribute the resulting executable under terms of your choice, provided that
* you also meet, for each linked independent module, the terms and conditions of
* the license of that module.  An independent module is a module which is not
* derived from this software.  The special exception does not apply to any
* modifications of the software.
*
*      Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*
* $Id: dhd_custom_gpio.c,v 1.1.4.8.4.3 2011/01/20 02:30:58 Exp $
*/


#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#include <bcmutils.h>

#include <dngl_stats.h>
#include <dhd.h>

#include <wlioctl.h>
#include <wl_iw.h>

/* LGE_CHANGE_S [yoohoo@lge.com] 2009-05-14, support start/stop */
#if defined(CONFIG_LGE_BCM432X_PATCH)
#include <asm/gpio.h>
#include <linux/interrupt.h>
#endif /* CONFIG_LGE_BCM432X_PATCH */
/* LGE_CHANGE_E [yoohoo@lge.com] 2009-05-14, support start/stop */

#define WL_ERROR(x) printf x
#define WL_TRACE(x)

#include <linux/wlan_plat.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mmc/host.h>
#include <mach/hardware.h>
#include <mach/gpio-names.h>

/*bill.park@lge.com, comment out because of 32k clock always on */
//static struct clk *wifi_32k_clk;

#ifdef CUSTOMER_HW
extern  void bcm_wlan_power_off(int);
extern  void bcm_wlan_power_on(int);
#endif /* CUSTOMER_HW */
#ifdef CUSTOMER_HW2
int wifi_set_carddetect(int on);
int wifi_set_power(int on, unsigned long msec);
int wifi_get_irq_number(unsigned long *irq_flags_ptr);
#endif

#if defined(OOB_INTR_ONLY)

#if defined(BCMLXSDMMC)
extern int sdioh_mmc_irq(int irq);
#endif /* (BCMLXSDMMC)  */

#ifdef CUSTOMER_HW3
#include <mach/gpio.h>
#endif

/* Customer specific Host GPIO defintion  */
static int dhd_oob_gpio_num = -1; /* GG 19 */

module_param(dhd_oob_gpio_num, int, 0644);
MODULE_PARM_DESC(dhd_oob_gpio_num, "DHD oob gpio number");

int dhd_customer_oob_irq_map(unsigned long *irq_flags_ptr)
{
	int  host_oob_irq = 0;

#ifdef CUSTOMER_HW2
	host_oob_irq = wifi_get_irq_number(irq_flags_ptr);

#else /* for NOT  CUSTOMER_HW2 */
#if defined(CUSTOM_OOB_GPIO_NUM)
	if (dhd_oob_gpio_num < 0) {
		dhd_oob_gpio_num = CUSTOM_OOB_GPIO_NUM;
	}
#endif

	if (dhd_oob_gpio_num < 0) {
		WL_ERROR(("%s: ERROR customer specific Host GPIO is NOT defined \n",
			__FUNCTION__));
		return (dhd_oob_gpio_num);
	}

	WL_ERROR(("%s: customer specific Host GPIO number is (%d)\n",
	         __FUNCTION__, dhd_oob_gpio_num));

#if defined CUSTOMER_HW
	host_oob_irq = MSM_GPIO_TO_INT(dhd_oob_gpio_num);
#elif defined CUSTOMER_HW3
	gpio_request(dhd_oob_gpio_num, "oob irq");
	host_oob_irq = gpio_to_irq(dhd_oob_gpio_num);
	gpio_direction_input(dhd_oob_gpio_num);
#endif /* CUSTOMER_HW */
#endif /* CUSTOMER_HW2 */

	return (host_oob_irq);
}
#endif /* defined(OOB_INTR_ONLY) */

#if defined(CONFIG_LGE_BCM432X_PATCH)
#include "mach/io.h"
#endif

static int interrupt_en_flag = 0;		//by sjpark 11-03-10

/* Customer function to control hw specific wlan gpios */
void
dhd_customer_gpio_wlan_ctrl(int onoff)
{
	switch (onoff) {
		case WLAN_RESET_OFF:
			WL_TRACE(("%s: call customer specific GPIO to insert WLAN RESET\n",
				__FUNCTION__));
#if defined(CONFIG_LGE_BCM432X_PATCH)
			if (get_hw_rev() <= REV_1_2)
			{
				disable_irq(gpio_to_irq(TEGRA_GPIO_PQ5));	//by sjpark 11-03-10
				gpio_set_value(TEGRA_GPIO_PQ5, 0);
				interrupt_en_flag = 1;		//by sjpark 11-03-11
			}
			else
			{
				disable_irq(gpio_to_irq(TEGRA_GPIO_PU2));       //by sjpark 11-03-10
				gpio_set_value(TEGRA_GPIO_PU2, 0);
				interrupt_en_flag = 1;		//by sjpark 11-03-11
			}
#endif

#ifdef CUSTOMER_HW
			bcm_wlan_power_off(2);
#endif /* CUSTOMER_HW */
#ifdef CUSTOMER_HW2
			wifi_set_power(0, 0);
#endif
			WL_ERROR(("=========== WLAN placed in RESET ========\n"));
		break;

		case WLAN_RESET_ON:
			WL_TRACE(("%s: callc customer specific GPIO to remove WLAN RESET\n",
				__FUNCTION__));
#if defined(CONFIG_LGE_BCM432X_PATCH)
			if (get_hw_rev() <= REV_1_2)
				gpio_set_value(TEGRA_GPIO_PQ5, 1);
			else
				gpio_set_value(TEGRA_GPIO_PU2, 1);
			mdelay(150);
#endif
#ifdef CUSTOMER_HW
			bcm_wlan_power_on(2);
#endif /* CUSTOMER_HW */
#ifdef CUSTOMER_HW2
			wifi_set_power(1, 0);
#endif
			WL_ERROR(("=========== WLAN going back to live  ========\n"));
		break;

		case WLAN_POWER_OFF:
			WL_TRACE(("%s: call customer specific GPIO to turn off WL_REG_ON\n",
				__FUNCTION__));
#if defined(CONFIG_LGE_BCM432X_PATCH)
			if (get_hw_rev() <= REV_1_2)
			{
				gpio_set_value(TEGRA_GPIO_PQ5, 0);
				if(interrupt_en_flag == 1){
					printk("[sj-debug] POWER OFF : enable irq.\n");
					enable_irq(gpio_to_irq(TEGRA_GPIO_PQ5));	//by sjpark 11-03-10
					interrupt_en_flag = 0;		//by sjpark 11-03-11
				}
			}
			else
			{
				gpio_set_value(TEGRA_GPIO_PU2, 0);
				if(interrupt_en_flag == 1){
					printk("[sj-debug] POWER OFF : enable irq.\n");
					enable_irq(gpio_to_irq(TEGRA_GPIO_PU2));	//by sjpark 11-03-10
					interrupt_en_flag = 0;		//by sjpark 11-03-11
				}
			}

			mdelay(150);
/* always turn on 32k clock */
//			clk_disable(wifi_32k_clk);

#endif
#ifdef CUSTOMER_HW
			bcm_wlan_power_off(1);
#endif /* CUSTOMER_HW */
		break;

		case WLAN_POWER_ON:
			WL_TRACE(("%s: call customer specific GPIO to turn on WL_REG_ON\n",
				__FUNCTION__));
#if defined(CONFIG_LGE_BCM432X_PATCH)
/* Always turn on 32k clock
			wifi_32k_clk = clk_get_sys(NULL, "blink");
			if (IS_ERR(wifi_32k_clk)) {
				pr_err("%s: unable to get blink clock\n", __func__);
				//return PTR_ERR(wifi_32k_clk);
			}

			clk_enable(wifi_32k_clk);
			printk("[Wi-Fi] wifi_32k_clk is enabled\n");
*/
			if (get_hw_rev() <= REV_1_2) {
				gpio_set_value(TEGRA_GPIO_PQ5, 1);
			} else {
				gpio_set_value(TEGRA_GPIO_PU2, 1);
			}
			mdelay(150);

#endif
#ifdef CUSTOMER_HW
			bcm_wlan_power_on(1);
#endif /* CUSTOMER_HW */
			/* Lets customer power to get stable */
			OSL_DELAY(200);
		break;
	}
}

#ifdef GET_CUSTOM_MAC_ENABLE
/* Function to get custom MAC address */
int
dhd_custom_get_mac_address(unsigned char *buf)
{
	WL_TRACE(("%s Enter\n", __FUNCTION__));
	if (!buf)
		return -EINVAL;

	/* Customer access to MAC address stored outside of DHD driver */

#ifdef EXAMPLE_GET_MAC
	/* EXAMPLE code */
	{
		struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
	}
#endif /* EXAMPLE_GET_MAC */

	return 0;
}
#endif /* GET_CUSTOM_MAC_ENABLE */

/* Customized Locale table : OPTIONAL feature */
const struct cntry_locales_custom translate_custom_table[] = {
/* Table should be filled out based on custom platform regulatory requirement */
#ifdef EXMAPLE_TABLE
	{"US", "US", 69}, /* input ISO "US" to : US regrev 69 */
	{"CA", "US", 69}, /* input ISO "CA" to : US regrev 69 */
	{"FR", "EU", 05},
	{"DE", "EU", 05},
	{"IR", "EU", 05},
	{"UK", "EU", 05}, /* input ISO "UK" to : EU regrev 05 */
	{"KR", "XY", 03},
	{"AU", "XY", 03},
	{"CH", "XY", 03}, /* input ISO "CH" to : XY regrev 03 */
	{"TW", "XY", 03},
	{"AR", "XY", 03}
#endif /* EXMAPLE_TABLE */
	{"KR", "KR", 03}
};


/* Customized Locale convertor
*  input : ISO 3166-1 country abbreviation
*  output: customized cspec
*/
void get_customized_country_code(char *country_iso_code, wl_country_t *cspec)
{
	int size, i;

	size = ARRAYSIZE(translate_custom_table);

	if (cspec == 0)
		 return;

	if (size == 0)
		 return;

	for (i = 0; i < size; i++) {
	if (strcmp(country_iso_code, translate_custom_table[i].iso_abbrev) == 0) {
		memcpy(cspec->ccode,  translate_custom_table[i].custom_locale, WLC_CNTRY_BUF_SZ);
		cspec->rev = translate_custom_table[i].custom_locale_rev;
		}
	}
	return;
}