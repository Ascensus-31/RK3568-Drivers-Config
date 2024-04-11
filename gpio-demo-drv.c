/*
 * GPIO驱动程序模板
 * FUNCTIONS:获取设备树代码，注册字符设备、设备类、设备、平台驱动。
 * 为应用程序提供读写接口，可配置的GPIO数量。
 * ATTENTION:使用时注意根据总共的GPIO数量修改GPIO_SUB_COUNT
 * 同时注意根据设备树顺序添加write/read函数中的switch内容。
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include </home/asc3nt/100ask_imx6ull-sdk/Linux-4.9.88/arch/arm64/boot/dts/include/dt-bindings/gpio/gpio.h>
#include </home/asc3nt/100ask_imx6ull-sdk/Uboot-2018.03/arch/arm/dts/include/dt-bindings/pinctrl/rockchip.h>

#define GPIO_DEVICE_NAME "mygpio"
#define GPIO_DEVICE_MINOR 0
#define GPIO_SUB_COUNT 2

static int major = 0;
static int major_exit[GPIO_SUB_COUNT];
static int gpio_exit[GPIO_SUB_COUNT];
static int probe_count = 0;
static struct class *gpio_class;
static struct device *my_devices[GPIO_SUB_COUNT];

struct my_gpio_info {
    int gpio_pin;
    int gpio_enable_value;
    int gpio_input_value;
};

static struct my_gpio_info *gpio_info;  // 全局变量

static DEFINE_MUTEX(gpio_mutex);  // 互斥锁

static int mygpio_open(struct inode *inode, struct file *file) {
    printk("Open File:%s\n",file->f_path.dentry->d_iname);
    return 0;
}

static ssize_t mygpio_read(struct file *file, char __user *buffer, size_t length, loff_t *offset) {
    int rValue;
    int gpio_pin;
    char cValue;
    int idLen = strlen(file->f_path.dentry->d_iname);
    char gpio_id = file->f_path.dentry->d_iname[idLen-1] - '0';
    switch (gpio_id){
        case 0:gpio_pin = 14;break;
        case 1:gpio_pin = 6;break;
    }
    rValue = gpio_get_value(gpio_pin);
    if (rValue < 0) {
        return -EFAULT;
    }
    cValue = rValue ? '1' : '0';
    if (copy_to_user(buffer, &cValue, 1) != 0) {
        return -EFAULT;
    }
    return 1;
}

static ssize_t mygpio_write(struct file *file, const char *buffer, size_t length, loff_t *offset) {
    int wValue;
    int gpio_pin;
    int idLen = strlen(file->f_path.dentry->d_iname);
    char gpio_id = file->f_path.dentry->d_iname[idLen-1] - '0';

    if (copy_from_user(&wValue, buffer, 1)) {
    return -EFAULT;
    }

    switch (gpio_id){
        case 0:gpio_pin = 14;break;
        case 1:gpio_pin = 6;break;
    }
    gpio_info->gpio_input_value = wValue;
    gpio_info->gpio_pin = gpio_pin;
    mutex_lock(&gpio_mutex);
    
    gpio_direction_output(gpio_info->gpio_pin, gpio_info->gpio_input_value);
    
    mutex_unlock(&gpio_mutex);


    return length;

}

/* 字符设备结构体 */
static struct file_operations mygpio_fops = {
    .owner = THIS_MODULE,
    .open = mygpio_open,
    .write = mygpio_write,
    .read = mygpio_read,
};

static int my_gpio_probe(struct platform_device *pdev)
{
    int gpio;
    enum of_gpio_flags flag;
    struct device_node *my_gpio_node = pdev->dev.of_node;

    gpio_info = devm_kzalloc(&pdev->dev, sizeof(struct my_gpio_info), GFP_KERNEL);
    if (!gpio_info) {
        dev_err(&pdev->dev, "devm_kzalloc failed!\n");
        return -ENOMEM;
    }
    gpio = of_get_named_gpio_flags(my_gpio_node, "my-gpio", 0, &flag);/*此处获取的是设备树的gpio硬件编号*/

    if (!gpio_is_valid(gpio)) {
        dev_err(&pdev->dev, "my-gpio: %d is invalid\n", gpio);
        return -ENODEV;
    }
    if (gpio_request(gpio, "my-gpio")) {
        dev_err(&pdev->dev, "my-gpio: %d request failed!\n", gpio);
        gpio_free(gpio);
        return -ENODEV;
    }
    gpio_info->gpio_pin = gpio;
    gpio_exit[probe_count] = gpio;
    probe_count ++;
    gpio_info->gpio_enable_value = (flag == GPIO_ACTIVE_LOW) ? 0:1;
    printk("GPIO %d status %d",gpio_info->gpio_pin ,gpio_info->gpio_enable_value);
    gpio_direction_output(gpio_info->gpio_pin, gpio_info->gpio_enable_value);

    return 0;
}

static struct of_device_id my_match_table[] = {
        { .compatible = "asc3nt,rk356x-gpio",},
        {},
};

/* 平台驱动结构体 */
static struct platform_driver my_gpio_driver = {
        .driver = {
                .name = "my-gpio",
                .owner = THIS_MODULE,
                .of_match_table = my_match_table,
        },
        .probe = my_gpio_probe,
};
/* 初始化函数
 * 1. 注册字符设备
 * 2. 注册设备类
 * 3. 注册设备
 * 4. 注册平台驱动 */
static int my_gpio_init(void)
{
    int i;
    int err;
    char device_name[20];

    for (i = 0; i < GPIO_SUB_COUNT; i++){
        snprintf(device_name, sizeof(device_name), "mygpio%d", i);
        major = register_chrdev(GPIO_DEVICE_MINOR, device_name, &mygpio_fops);
        if (major < 0) {
            printk(KERN_ERR "Failed to register character device\n");
            return major;
        }
        major_exit[i] = major;
    }
    gpio_class = class_create(THIS_MODULE, "my_gpio_class");
    err = PTR_ERR(gpio_class);
	if (IS_ERR(gpio_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, GPIO_DEVICE_NAME);
		return -1;
	}

    for (i = 0; i < GPIO_SUB_COUNT; i++){
		my_devices[i] = device_create(gpio_class, NULL, MKDEV(major, i), NULL, "mygpio%d", i); /* /dev/mygpio0,1,... */
        printk("Device name: %s registered\n", my_devices[i]->kobj.name);}

    return platform_driver_register(&my_gpio_driver);
}
module_init(my_gpio_init);

/* 退出函数
 * 1. 释放GPIO
 * 2. 注销平台驱动
 * 3. 注销设备
 * 4. 注销设备类
 * 5. 注销字符设备 */ 
static void my_gpio_exit(void)
{
    int i;
    for (i = 0; i < GPIO_SUB_COUNT; i++){
    gpio_free(gpio_exit[i]);}

    platform_driver_unregister(&my_gpio_driver);

    for (i = 0; i < GPIO_SUB_COUNT; i++){
	device_destroy(gpio_class, MKDEV(major, i));} /* /dev/mygpio0,1,... */

    class_destroy(gpio_class);

    for (i = 0; i < GPIO_SUB_COUNT; i++){
    unregister_chrdev(major_exit[i], GPIO_DEVICE_NAME);}

    printk(KERN_INFO "Device unregistered\n");
}
module_exit(my_gpio_exit);

MODULE_AUTHOR("asc3nt");
MODULE_DESCRIPTION("GPIO driver");
MODULE_ALIAS("platform:firefly-gpio");
MODULE_LICENSE("GPL");
