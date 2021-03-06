/*
 * Misc driver
 *
 * Copyright (C) 2009 LGE Inc.
 *
 * seokhee.han@lge.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/syscalls.h>

//#include <mach/pmic_tps6586x.h>
#include <linux/mfd/tps6586x.h>
#include <linux/regulator/consumer.h>

#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <mach/hardware.h>
#include <mach/gpio-names.h>
#include <mach/hardware.h>
#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
#include <linux/workqueue.h>
#endif

struct proc_dir_entry *sumproc_root_fp          = NULL;

#define ON	1
#define OFF	0

//#define GPIO_SPK_SWITCH TEGRA_GPIO_PK5
static int GPIO_SPK_SWITCH;

#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
#define GPIO_MDM_RESET_INT_N		TEGRA_GPIO_PS2	// SDIO_LTE_DAT3 for wakeup source
#endif

typedef struct MiscDeviceRec
{
    struct input_dev    *inputDev;
    struct task_struct  *task;
#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
	unsigned int mdm_reset_irq;
#endif
} MiscDevice;

static MiscDevice s_misc;

#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
static struct workqueue_struct *mdm_reset_wq;
static struct delayed_work work;
#endif
 /*
 *                      AT command
 */
extern void battery_set_charge_mode(int value);
extern int battery_get_charge_status(void);
extern void battery_at_fuelrst(void);
#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
extern void cwc_notify_reset(void);
#endif

static ssize_t at_charge_show(struct device *dev,  struct device_attribute *attr,  char *buf)
{
	int value = -1;
#ifdef CONFIG_STARTABLET_BATTERY
	value = battery_get_charge_status();
#endif

	return sprintf(buf, "%d\n", value);
}

static ssize_t at_charge_store(struct device *dev, struct device_attribute *attr,
                               const char *buf, size_t count)
{
	unsigned long state;
	int err;

	err = strict_strtoul(buf, 0, &state);
	if (err)
		return err;

#ifdef CONFIG_STARTABLET_BATTERY
	battery_set_charge_mode(!!state);
#endif

	return err ?: count;
}

static ssize_t at_fuelrst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", 0);
}

static ssize_t at_fuelrst_store(struct device *dev, struct device_attribute *attr,
                                const char *buf, size_t count)
{
	unsigned long state;
	int err;

	err = strict_strtoul(buf, 0, &state);
	if (err)
		return err;

#ifdef CONFIG_STARTABLET_BATTERY
	battery_at_fuelrst();
#endif

	return err ?: count;
}

static ssize_t pmic_test_show(struct device *dev,  struct device_attribute *attr,  char *buf)
{
	return sprintf(buf, "%d\n", 0);
}

static ssize_t pmic_test_store(struct device *dev, struct device_attribute *attr,
                               const char *buf, size_t count)
{
	unsigned long state;
	int err;
	struct regulator* regul;

	err = strict_strtoul(buf, 0, &state);
	if (err)
		return err;

	regul = regulator_get(dev, "vdd_3v3");  /* digital core */
	if (IS_ERR(regul)) {
		printk(KERN_ERR "[MISC] Failed to get regulator\n");
		return 0;
	}
	//regulator_set_voltage(regul, 0, 0);
	regulator_enable(regul);
	mdelay(100);
	regulator_disable(regul);
	regulator_put(regul);

	return err ?: count;
}

static ssize_t spk_switch_show(struct device *dev,  struct device_attribute *attr,  char *buf)
{
	int value = 0;
	value = gpio_get_value(GPIO_SPK_SWITCH);
	return sprintf(buf, "%d\n", value);
}

static ssize_t spk_switch_store(struct device *dev, struct device_attribute *attr,
                                const char *buf, size_t count)
{
	int mode = 0;

	if (sscanf(buf, "%d", &mode) != 1) {
		return count;
	}

	if (mode == 1) {
		gpio_set_value(GPIO_SPK_SWITCH, 1);
	}
	else if (mode == 0) {
		gpio_set_value(GPIO_SPK_SWITCH, 0);
	}

	return count;
}

#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
void disable_mdm_irq(void)
{
	printk(KERN_INFO "MDM is going down..........\n");

	if (s_misc.mdm_reset_irq >= 0) {
		free_irq(s_misc.mdm_reset_irq, (void*)&s_misc);
		printk(KERN_INFO "MDM is going down: Free irq for mdm reset...\n");
	}

	if (mdm_reset_wq) {
		flush_workqueue(mdm_reset_wq);
		destroy_workqueue(mdm_reset_wq);
		mdm_reset_wq = NULL;
		printk(KERN_INFO "MDM is going down: Flush workqueue for mdm reset...\n");
	}
}
EXPORT_SYMBOL(disable_mdm_irq);

static ssize_t mdm_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int pin_val = 0;

	pin_val = gpio_get_value(GPIO_MDM_RESET_INT_N);

	return sprintf(buf, "%s\n", ((pin_val)?"MDM On":"MDM Off"));
}

static ssize_t mdm_status_store(struct device *dev, struct device_attribute *sttr,
								const char *buf, size_t count)
{
	unsigned long state;
	int err;

	err = strict_strtoul(buf, 0, &state);
	if (err)
		return err;

	if (state == 0) {
		disable_mdm_irq();
	}

	return err ?: count;
}
#endif

static DEVICE_ATTR(at_charge, 0600, at_charge_show, at_charge_store);
static DEVICE_ATTR(at_fuelrst, 0600, at_fuelrst_show, at_fuelrst_store);
static DEVICE_ATTR(pmic_test, 0600, pmic_test_show, pmic_test_store);
static DEVICE_ATTR(spk_switch, 0666, spk_switch_show, spk_switch_store);
#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
static DEVICE_ATTR(mdm_stat, 0644, mdm_status_show, mdm_status_store);
#endif

static struct attribute *misc_attributes[] = {
	&dev_attr_at_charge.attr,
	&dev_attr_at_fuelrst.attr,
	&dev_attr_pmic_test.attr,
	&dev_attr_spk_switch.attr,
#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
	&dev_attr_mdm_stat.attr,
#endif
	NULL
};

static const struct attribute_group misc_group = {
	.attrs = misc_attributes,
};


#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
extern void ts0710_cp_emergency_state(void);

static void mdm_reset_delayed_work(struct work_struct *work)
{
	if (gpio_get_value(GPIO_MDM_RESET_INT_N))
		printk(KERN_ERR ">>>>MDM status pin makes interrupt, but still HIGH.....\n");
	else {
		printk(KERN_ERR ">>>>MDM6200 was reset!\n");
//    cwc_notify_reset();
	}
  ts0710_cp_emergency_state();  // filtering of the false alarm, instead of the cwc_notify_reset()
}

static irqreturn_t mdm_reset_interrupt_handler(int irq, void *dev_id)
{
	printk(KERN_INFO ">>>>MDM rst int\n");
	queue_delayed_work(mdm_reset_wq, &work, msecs_to_jiffies(100));
    return IRQ_HANDLED;
}
#endif

static int __init misc_probe(struct platform_device *pdev)
{
	int err;
	struct device *dev = &pdev->dev;

	printk(KERN_INFO "%s: start.....\n",__func__);

	memset(&s_misc, 0x00, sizeof(s_misc));

	// set speak change gpio
	if (get_hw_rev() >= REV_F)
		GPIO_SPK_SWITCH = TEGRA_GPIO_PK5;
	else
		GPIO_SPK_SWITCH = TEGRA_GPIO_PV7;

	gpio_request(GPIO_SPK_SWITCH, "spk_switch");
	tegra_gpio_enable(GPIO_SPK_SWITCH);
	gpio_direction_output(GPIO_SPK_SWITCH, 0);

	if (is_modem_connected()){
		sumproc_root_fp   = proc_mkdir( "modem", 0 );
		//create_proc_entry( "exist", S_IFREG | S_IRWXU, sumproc_root_fp );
		create_proc_entry( "exist", S_IFREG | S_IRWXO, sumproc_root_fp );
		printk(KERN_INFO ">>>Modem exist\n");
	}
	else printk(KERN_ERR ">>>Modem is not exist\n");
	//hary.cho

#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
	gpio_request(GPIO_MDM_RESET_INT_N, "mdm_reset_int_n");
	tegra_gpio_enable(GPIO_MDM_RESET_INT_N);
	gpio_direction_input(GPIO_MDM_RESET_INT_N);
	s_misc.mdm_reset_irq = gpio_to_irq(GPIO_MDM_RESET_INT_N);

	err = request_irq(s_misc.mdm_reset_irq, mdm_reset_interrupt_handler,
						IRQF_TRIGGER_FALLING, "mdm_reset_int_n", (void*)&s_misc);
	if(err)
	{
		printk(KERN_ERR "%s: Failed: request_irq for mdm_reset_irq!!! (err:%d)\n", __func__, err);
	}

	err = set_irq_wake(s_misc.mdm_reset_irq, !!is_modem_connected());
	if(err)
	{
		printk(KERN_ERR "%s: Failed: set_irq_wake for mdm_reset_irq!!! (err:%d)\n", __func__, err);
	}

	mdm_reset_wq = create_singlethread_workqueue("mdm_reset");
	if (!mdm_reset_wq) {
		printk(KERN_ERR "%s: Failed: allocate workqueue for mdm reset", __func__);
	}

    INIT_DELAYED_WORK(&work, mdm_reset_delayed_work);
#endif

	if ((err = sysfs_create_group(&dev->kobj, &misc_group)))
	{
		printk(KERN_ERR "%s: Failed: sysfs_create_group \n", __func__);
		goto err_sysfs_create;
	}

	return 0;

err_sysfs_create:
	printk("%s: misc_device_register_failed\n", __func__);

	return err;
}

static int misc_remove(struct platform_device *pdev)
{
#if 0 //#ifndef CONFIG_STARTABLET_XMM6160
    if (s_misc.mdm_reset_irq >= 0)
		free_irq(s_misc.mdm_reset_irq, (void*)&s_misc);
	if (mdm_reset_wq) {
		flush_workqueue(mdm_reset_wq);
		destroy_workqueue(mdm_reset_wq);
		mdm_reset_wq = NULL;
	}
#endif
	return 0;
}

#define misc_suspend  NULL
#define misc_resume   NULL

static struct platform_driver misc_driver = {
	.probe    = misc_probe,
	.remove   = misc_remove,
	//	.suspend  =  misc_suspend,
	//	.resume   =  misc_resume,
	.driver =  {
		.name = "tegra_misc",
		.owner = THIS_MODULE,
	},
};

static int __init misc_init(void)
{
	return platform_driver_register(&misc_driver);
}

static void __exit misc_exit(void)
{
	platform_driver_unregister(&misc_driver);
}

module_init(misc_init);
module_exit(misc_exit);

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("MISC Driver for StarTablet");
MODULE_LICENSE("GPL");

