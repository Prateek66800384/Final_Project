
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>

#define led_ltc_LENGTH	30
#define red_SHFT	2
#define blue_shift	1
#define gren_shift	2
#define CMD_main_led_ltc	6
#define sub_led_ltc	6
#define enable_en	(1 << 2)


struct dev_led_ltc
{
	u8 bright;
	struct led_classdev dev_class;
	struct led_ltc_priv *private;
};


struct led_ltc_priv {
	u32 led_ltc_num;
	u8 cmmnd[3];
	struct gpio_desc *disp_cs;
	struct i2c_client *clnt1;
};


static int led_ltc_wrt(struct i2c_client *clnt1, u8 *cmmnd)
{
	int ret = i2c_master_send(clnt1, cmmnd, 3);
	if (ret >= 0)
		return 0;
	return ret;
}


static ssize_t selct_sub(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	char *buffer;
	struct i2c_client *clnt1;
	struct led_ltc_priv *private;

	buffer = buf;


	*(buffer+(count-1)) = '\0';

	clnt1 = to_i2c_client(dev);
	private = i2c_get_clnt1data(clnt1);

	private->cmmnd[0] |= enable_en;
	led_ltc_wrt(private->clnt1, private->cmmnd);

	if(!strcmp(buffer, "on")) {
		gpiod_set_value(private->disp_cs, 1);
		usleep_range(10, 20);
		gpiod_set_value(private->disp_cs, 0);
	}
	else if (!strcmp(buffer, "off")) {
		gpiod_set_value(private->disp_cs, 0);
		usleep_range(100, 200);
		gpiod_set_value(private->disp_cs, 1);
	}
	else {
		dev_err(&clnt1->dev, "Bad led_ltc value.\n");
		return -EINVAL;
	}

	return count;
}


static int led_ltc_control(struct led_classdev *led_ltc_dev_class,
		       enum led_ltc_bright value)
{
	struct led_classdev *dev_class;
	struct dev_led_ltc *led_ltc;
	led_ltc = container_of(led_ltc_dev_class, struct dev_led_ltc, dev_class);
	dev_class = &led_ltc->dev_class;
	led_ltc->bright = value;






	if (strcmp(dev_class->name,"red") == 0) {
		led_ltc->private->cmmnd[0] &= 0x0F;
		led_ltc->private->cmmnd[0] |= ((led_ltc->bright << red_SHFT) & 0xF0);
	}
	else if (strcmp(dev_class->name,"sub") == 0) {
		led_ltc->private->cmmnd[2] &= 0xF0;
		led_ltc->private->cmmnd[2] |= ((led_ltc->bright << sub_led_ltc) & 0x0F);
	}
	else
		dev_info(dev_class->dev, "No display found\n");

	return led_ltc_wrt(led_ltc->private->clnt1, led_ltc->private->cmmnd);
}

static int _probe(struct i2c_client *clnt1,
				const struct i2c_device_id *id)
{
	int count, ret;
	u8 value[3];
	struct fwnode_handle *child;
	struct device *dev = &clnt1->dev;
	struct led_ltc_priv *private;

	dev_info(dev, "platform_probe enter\n");

	value[0] = 0x00;
	value[1] = 0xF0;
	value[2] = 0x00;

	i2c_master_send(clnt1, value, 3);

	private->clnt1 = clnt1;
	i2c_set_clnt1data(clnt1, private);

	private->disp_cs = devm_gpiod_get(dev, NULL, GPIOD_ASIS);
	if (IS_ERR(private->disp_cs)) {
		ret = PTR_ERR(private->disp_cs);
		dev_err(dev, "Unable to claim gpio\n");
		return ret;
	}

	gpiod_direction_output(private->disp_cs, 1);

	ret = sysfs_create_group(&clnt1->dev.kobj, &disp_cs_group);
	if (ret < 0) {
		dev_err(&clnt1->dev, "couldn't register sysfs group\n");
		return ret;
	}
	fwnode_property_read_string(child, "label", &dev_class->name);

		if (strcmp(dev_class->name,"main") == 0) {
			dev_led_ltc->dev_class.bright_set_blocking = led_ltc_control;
			ret = devm_led_classdev_register(dev, &dev_led_ltc->dev_class);
			if (ret)
				goto err;
			dev_info(dev_class->dev, " subsysis %s and num is %d\n",
				 dev_class->name, private->led_ltc_num);
		}

		else if (strcmp(dev_class->name,"sub") == 0) {
			dev_led_ltc->dev_class.bright_set_blocking = led_ltc_control;
			ret = devm_led_classdev_register(dev, &dev_led_ltc->dev_class);
			if (ret)
				goto err;
			dev_info(dev_class->dev, "the subsystem is %s and num is %d\n",
				 dev_class->name, private->led_ltc_num);
		}

		else {
			dev_err(dev, "Bad device tree value\n");
			return -EINVAL;
		}
		private->led_ltc_num++;



	return ret;
}

static int remv(struct i2c_client *clnt1)
{
	dev_info(&clnt1->dev, "led_ltcs_remove enter\n");
	sysfs_remove_group(&clnt1->dev.kobj, &disp_cs_group);
	dev_info(&clnt1->dev, "led_ltcs_remove exit\n");

	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "arrow,ltc_3206"},
	{},
};
MODULE_DEVICE_TABLE(of, my_of_ids);

static const struct i2c_device_id ltc_3206_id[] = {
	{ "ltc_3206", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ltc_3206_id);

static struct i2c_driver driver_led_ltc = {
	.probe = _probe,
	.remove = remv,
	.id_table	= ltc_3206_id,
	.driver = {
		.name = "ltc_3206",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	}
};

module_i2c_driver(driver_led_ltc);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for led_ltc");
