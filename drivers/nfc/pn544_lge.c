/*
 * Copyright (C) 2010 NXP Semiconductors
 */
#include <linux/nfc/pn544_lge.h>
//                                                                                                           

/*                                                               */
#include <linux/nfc/pn544_lge_hwadapter.h>
#include <linux/of_gpio.h>
/*                                                             */
//                                                                                      
#include <linux/wakelock.h>
#define NFC_POWER_OFF false
#define NFC_POWER_ON  true
#define NFC_PACKET_HEADER false
#define NFC_PACKET_DATA   true
//                                                                                    

#ifdef CONFIG_LGE_NFC_MULTICORE_FASTBOOT
#include <linux/kthread.h>
#endif
/*              */

#define MAX_BUFFER_SIZE	512
#define PN544_RESET_CMD 	0
#define PN544_DOWNLOAD_CMD	1

//                                                                            
#ifdef CONFIG_LGE_NFC_SET_IRQ_WAKEUP
static bool sIrqState = false;
#endif
//                                                                          

//                                                                                      
static bool sReadSequence = NFC_PACKET_HEADER;
static bool sPowerState = NFC_POWER_OFF;
//                                                                                    

#ifdef LGE_NFC_READ_IRQ_MODIFY
bool do_reading = false;//DY_TEST
static bool cancle_read = false;//DY_TEST
#endif

static int	stReadIntFlag;
static struct i2c_client *pn544_client;

//                                                                                      
struct wake_lock nfc_wake_lock;
//                                                                                    

static void pn544_parse_dt(struct device *dev, struct pn544_dev *pn544_dev)
{
	struct device_node *np = dev->of_node;

	/* irq gpio info */
	//pn544_dev->ven_gpio = of_get_named_gpio_flags(np, "NXP,gpio_ven", 0, NULL);
	//pn544_dev->firm_gpio = of_get_named_gpio_flags(np, "NXP,gpio_mode", 0, NULL);
	//pn544_dev->irq_gpio = of_get_named_gpio_flags(np, "NXP,gpio_irq", 0, NULL);
	pn544_dev->ven_gpio = of_get_named_gpio_flags(np, "nxp,gpio_ven", 0, NULL);
	pn544_dev->firm_gpio = of_get_named_gpio_flags(np, "nxp,gpio_mode", 0, NULL);
	pn544_dev->irq_gpio = of_get_named_gpio_flags(np, "nxp,gpio_irq", 0, NULL);
}

#ifdef CONFIG_LGE_NFC_PRESTANDBY
static struct mutex        mode_mutex;
#endif

static void pn544_disable_irq(struct pn544_dev *pn544_dev)
{
	unsigned long flags;

	spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	if (pn544_dev->irq_enabled) {
		disable_irq_nosync(pn544_get_irq_pin(pn544_dev));
//                                                             
#if !defined(CONFIG_LGE_NFC_HW_QCT_MSM8660)&&!defined(CONFIG_LGE_NFC_HW_QCT_MSM8255)
		disable_irq_wake(pn544_get_irq_pin(pn544_dev));
#endif
		pn544_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
}

//                                                                                      
static void pn544_enable_irq(struct pn544_dev *pn544_dev)
{
    unsigned long flags;
    spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
    if (!pn544_dev->irq_enabled) {
        pn544_dev->irq_enabled = true;
        enable_irq(pn544_dev->client->irq);
        enable_irq_wake(pn544_dev->client->irq);
    }
    spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
}
//                                                                                    

static irqreturn_t pn544_dev_irq_handler(int irq, void *dev_id)
{
	struct pn544_dev *pn544_dev = dev_id;
    //                                                                                      
    unsigned long flags;
    unsigned int irq_gpio_val;

    irq_gpio_val = gpio_get_value(pn544_dev->irq_gpio);

    if (irq_gpio_val == 0) {
        pr_err("%s called, IRQ GPIO == %d\n", __func__, irq_gpio_val);
        return 0;
    }

    if (sPowerState == NFC_POWER_ON) {
        spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
        pn544_dev->count_irq++;
        spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
        /* Wake up waiting readers */
        wake_up(&pn544_dev->read_wq);
        wake_lock(&nfc_wake_lock);

        //pr_err("%s: wake_lock (%d)\n", __func__, pn544_dev->count_irq); // for debug
    }
    else {
         pr_err("%s, NFC IRQ Triggered during NFC OFF\n", __func__);
    }
    //                                                                                    

	return IRQ_HANDLED;
}

#ifdef CONFIG_LGE_NFC_PRESTANDBY
#ifdef CONFIG_LGE_NFC_PN544_C2
void pn544_factory_standby_set(void)
{
    int ret;
    struct pn544_dev *pn544_dev;
    struct pn544_i2c_platform_data *platform_data;
    uint8_t EEDATA_WRITE[9] = {0x08, 0x00, 0x06, 0x00, 0x9E, 0xAA, 0x00, 0x01, 0x01};

    platform_data = pn544_client->dev.platform_data;

    pn544_dev = i2c_get_clientdata(pn544_client);
    // 1. Go To Dnld mode 2

    gpio_set_value(pn544_pData.ven_gpio, 1);
    gpio_set_value(pn544_pData.firm_gpio, 1);
    msleep(10);
    gpio_set_value(pn544_pData.ven_gpio, 0);
    msleep(10);
    gpio_set_value(pn544_pData.ven_gpio, 1);
    msleep(10);

    // 2. I2c write
    dprintk("%s Go To I2c write\n", __func__);
    ret = 0;
    ret = i2c_master_send(pn544_client, EEDATA_WRITE, 9);
    if (ret != 9) {
         dprintk(PN544_DRV_NAME ":%s : i2c_master_send returned %d\n", __func__, ret);
    }
    msleep(10);

    // 3. HW reset 1,0,1
    dprintk("%s Go To PN544 reset\n", __func__);

    //--> # reset 1
    gpio_set_value(pn544_pData.firm_gpio, 0);
    gpio_set_value(pn544_pData.ven_gpio, 1);
    msleep(10);

    //--> # reset 0
    gpio_set_value(pn544_pData.firm_gpio, 0);
    gpio_set_value(pn544_pData.ven_gpio, 0);
    msleep(10);

    //--> # reset 1
    gpio_set_value(pn544_pData.firm_gpio, 0);
    gpio_set_value(pn544_pData.ven_gpio, 1);
    msleep(10);


    // 4. power off
    dprintk(PN544_DRV_NAME ":%s power off\n", __func__);
    gpio_set_value(pn544_pData.firm_gpio, 0);
    gpio_set_value(pn544_pData.ven_gpio, 0);
    msleep(10);
}
#elif defined(CONFIG_LGE_NFC_PN544_C3)
static char pn544_standby_set_val1[6]={0x05, 0xF9, 0x04, 0x00, 0xC3, 0xE5};
static char pn544_standby_set_val2[6]={0x05, 0x80, 0x83, 0x03, 0x5A, 0x0A};
static char pn544_standby_set_val3[10]={0x09, 0x89, 0x83, 0x3F, 0x00, 0x9E, 0xAA, 0x01, 0xC2, 0x85};
#define NFC_I2C_WRITE_RETRY_NUM 3
static int __pn544_kwrite(void *dev, void* data, int size)
{
    struct pn544_dev *pn544_dev;
    int ret = 0;
           unsigned int retry = 0;

    if(dev != NULL)
        pn544_dev = (struct pn544_dev*)dev;

    ret = i2c_master_send(pn544_client, data, size);

    while(retry != NFC_I2C_WRITE_RETRY_NUM)
    {
          msleep(10);
        if(ret == size)
                                break;
                     ret = i2c_master_send(pn544_client, data, size);
                     retry++;
                     printk("%s i2c_master_send retry[%d]\n",__func__, retry);
    }

    if(ret != size){
        pr_err("%s i2c_master_send failed[%d]\n", __func__, ret);
        return -1;
    }

    return 0;
}

static int __pn544_kread(void *dev, unsigned int length)
{
    struct pn544_dev *pn544_dev = NULL;
    char tmp[MAX_BUFFER_SIZE];
    int ret = 0;
    int irq_gpio_val = 0;

    if(dev != NULL)
        pn544_dev = (struct pn544_dev*)dev;

    mutex_lock(&mode_mutex);

    irq_gpio_val = gpio_get_value(pn544_dev->irq_gpio);
    dprintk(PN544_DRV_NAME ":IRQ GPIO = %d\n", irq_gpio_val);
    if (irq_gpio_val == 0) {
        pn544_dev->irq_enabled = true;
#ifdef READ_IRQ_MODIFY
        do_reading=0;//DY_TEST
#endif
        enable_irq_wake(pn544_get_irq_pin(pn544_dev));
        enable_irq(pn544_get_irq_pin(pn544_dev));
#ifdef READ_IRQ_MODIFY
        ret = wait_event_interruptible(pn544_dev->read_wq, do_reading);
#else
        ret = wait_event_interruptible(pn544_dev->read_wq, gpio_get_value(pn544_dev->irq_gpio));
#endif
        pn544_disable_irq(pn544_dev);
        if(ret){
               	mutex_unlock(&mode_mutex);
            		goto fail;
            	}
    }
    memset(tmp, 0x00, MAX_BUFFER_SIZE);
    ret = i2c_master_recv(pn544_dev->client, tmp, length);
    while(tmp[0]==0x51&&tmp[1]==0xFF&&tmp[2]==0xFF){
        ret = i2c_master_recv(pn544_dev->client, tmp, length);
        printk("%s read retry!\n", __func__);
    }
    mutex_unlock(&mode_mutex);
    printk("%s: read data : 0x%X 0x%X 0x%X 0x%X\n", __func__, tmp[0], tmp[1], tmp[2], tmp[3]);
    if (ret < 0) {
        dprintk("%s: i2c_master_recv returned %d\n", __func__, ret);
        return ret;
    }
    if (ret > length) {
        pr_err("%s: received too many bytes from i2c (%d)\n", __func__, ret);
        return -EIO;
    }

fail:
        return ret;
}


void pn544_factory_standby_set(void)
{
    int ret = 0;
    struct pn544_dev *pn544_dev;
    struct pn544_i2c_platform_data *platform_data;

    platform_data = pn544_client->dev.platform_data;
    pn544_dev = i2c_get_clientdata(pn544_client);
    // 1. Go To Dnld mode 2
    dprintk("%s Go To Dnld mode 2\n", __func__);
    gpio_set_value(pn544_pData.ven_gpio, 0);
    gpio_set_value(pn544_pData.firm_gpio, 0);
    msleep(10);
    gpio_set_value(pn544_pData.ven_gpio, 1);
    msleep(10);

    // 2. I2c write
    dprintk("%s Go To I2c write\n", __func__);

    ret = __pn544_kwrite(pn544_dev, pn544_standby_set_val1, 6);
    if (ret == 0) {
    	printk("%s: standby write val1 success\n", __func__);
    	ret = __pn544_kread(pn544_dev, 4);
    } else {
    	printk("%s: standby write val1 fail\n", __func__);
    	return;
    }

    ret = __pn544_kwrite(pn544_dev, pn544_standby_set_val2, 6);
    if (ret == 0) {
    	printk("%s: standby write val2 success\n", __func__);
    	ret = __pn544_kread(pn544_dev, 6);
    } else {
    	printk("%s: standby write val2 fail\n", __func__);
    	return;
    }

    ret = __pn544_kwrite(pn544_dev, pn544_standby_set_val3, 10);
    if (ret == 0) {
    	printk("%s: standby write val3 success\n", __func__);
        ret = __pn544_kread(pn544_dev, 7);
    } else {
    	printk("%s: standby write val3 fail\n", __func__);
    	return;
    }

    // 4. power off
    dprintk(PN544_DRV_NAME ":%s power off\n", __func__);
    gpio_set_value(pn544_dev->firm_gpio, 0);
    gpio_set_value(pn544_dev->ven_gpio, 0);
    msleep(10);

    return;
}
#endif /*                                                   */
#endif /*                           */

/*             
  
                                  
                                    
                                   
  
                                  
 */
#if defined(CONFIG_LGE_NFC_MULTICORE_FASTBOOT)&&defined(CONFIG_LGE_NFC_PRESTANDBY)
static int pn544_factory_standby_set_thread(void *arg)
{
	pn544_factory_standby_set();
	dprintk("%s end\n", __func__);
	return 0;
}
#endif /*                                                                                */
/*              */

static ssize_t pn544_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offset)
{
	struct pn544_dev *pn544_dev = filp->private_data;
	static char tmp[MAX_BUFFER_SIZE];
	int ret;
	int irq_gpio_val = 0;
    //                                                                                      
    unsigned long flags;

    if (count > MAX_BUFFER_SIZE)
        count = MAX_BUFFER_SIZE;

    //pr_err("%s : reading %zu bytes.\n", __func__, count); // for debug
    irq_gpio_val = gpio_get_value(pn544_dev->irq_gpio);

    if (sReadSequence == NFC_PACKET_HEADER) {
        wait_event_interruptible(pn544_dev->read_wq, pn544_dev->count_irq > 0);
        //pr_err("%s: pass wait_event_interruptible (%d)\n", __func__, pn544_dev->count_irq); // for debug
        if (pn544_dev->count_irq <= 0) {
            pr_err("%s: pass wait_event_interruptible without irq triggering. Skip i2c_master_recv!\n", __func__);
            return -107;
        }
        //pr_err("%s: Read Header\n", __func__); // for debug
    }
    else {
        //pr_err("%s: Read data\n", __func__); // for debug
    }

    /* Read data */
    mutex_lock(&pn544_dev->read_mutex);
    //                                                                                    
	memset(tmp, 0x00, MAX_BUFFER_SIZE);
	ret = i2c_master_recv(pn544_dev->client, tmp, count);
	mutex_unlock(&pn544_dev->read_mutex);

	if (ret < 0) {
		pr_err("%s: i2c_master_recv returned %d\n", __func__, ret);
		return ret;
	}
	if (ret > count) {
		pr_err("%s: received too many bytes from i2c (%d)\n",
			__func__, ret);
		return -EIO;
	}
	if (copy_to_user(buf, tmp, ret)) {
		pr_warning("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}

    //                                                                                      
    //pr_err("%s: i2c_master_recv success (%d)\n", __func__, ret); // for debug

    if (sReadSequence == NFC_PACKET_HEADER) {
        if (ret == count) {
            sReadSequence = NFC_PACKET_DATA;
        }
        else {
            pr_err("%s: received lesser bytes(%d) than desired(%d).\n", __func__, ret, count);
        }
    }
    else {
        sReadSequence = NFC_PACKET_HEADER;
        spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
        if (pn544_dev->count_irq > 0) {
            pn544_dev->count_irq--;
            //pr_err("%s: current count_irq = %d\n", __func__, pn544_dev->count_irq); // for debug
        }
        if (pn544_dev->count_irq == 0) {
            wake_unlock(&nfc_wake_lock);
            //pr_err("%s: wake_unlock\n", __func__); // for debug
        }
        spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
    }
    //                                                                                    
	return ret;
}

static ssize_t pn544_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *offset)
{
	struct pn544_dev  *pn544_dev;
	static char tmp[MAX_BUFFER_SIZE];
	int ret;

	pn544_dev = filp->private_data;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	memset(tmp, 0x00, MAX_BUFFER_SIZE);
	if (copy_from_user(tmp, buf, count)) {
		pr_err(PN544_DRV_NAME ":%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

	pr_debug("%s : writing %zu bytes.\n", __func__, count);
	/* Write data */
	dprintk(PN544_DRV_NAME ":write: pn544_write len=:%d\n", count);

    //                                                                                      
    mutex_lock(&pn544_dev->read_mutex);
	ret = i2c_master_send(pn544_dev->client, tmp, count);
	if (ret != count) {
		pr_err("%s : i2c_master_send returned %d\n", __func__, ret);
		ret = -EIO;
	}
    mutex_unlock(&pn544_dev->read_mutex);
    //                                                                                    

	return ret;
}

static int pn544_dev_open(struct inode *inode, struct file *filp)
{
    //                                                                                      
    struct pn544_dev *pn544_dev = i2c_get_clientdata(pn544_client);
    filp->private_data = pn544_dev;
    pn544_dev->count_irq = 0;
    pn544_enable_irq(pn544_dev);
    //                                                                                    
	pr_debug("%s : %d,%d\n", __func__, imajor(inode), iminor(inode));

	return 0;
}

static long pn544_dev_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct pn544_dev *pn544_dev = filp->private_data;
        //                                                                                      
        unsigned long flags;
        //                                                                                    
	switch (cmd) {
	case PN544_SET_PWR:
		if (arg == 2) {
			/*
			power on with firmware download (requires hw reset)
			*/
			dprintk(PN544_DRV_NAME ":%s power on with firmware\n", __func__);

			gpio_set_value(pn544_dev->ven_gpio, 1);
			gpio_set_value(pn544_dev->firm_gpio, 1);
			msleep(10);
			gpio_set_value(pn544_dev->ven_gpio, 0);
			msleep(10);
			gpio_set_value(pn544_dev->ven_gpio, 1);
			msleep(10);
		} else if (arg == 1) {
			/* power on */
			dprintk(PN544_DRV_NAME ":%s power on\n", __func__);
	            //                                                                                      
	            if (sPowerState == NFC_POWER_OFF) {
	                gpio_set_value(pn544_dev->firm_gpio, 0);
	                gpio_set_value(pn544_dev->ven_gpio, 1);
	                msleep(10);
	
	                spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	#ifdef CONFIG_LGE_NFC_SET_IRQ_WAKEUP
	                if (sIrqState == false) {
	                    irq_set_irq_wake(pn544_dev->client->irq, 1);
	                    sIrqState = true;
	                    // dprintk(PN544_DRV_NAME ":%s enable IRQ\n", __func__);
	                }
	                else {
	                    pr_err("%s IRQ is already enabled!\n", __func__);
	                }
	#endif
	                //pr_err("%s NFC_POWER_ON\n", __func__); // for debug
	                sPowerState = NFC_POWER_ON;
	                sReadSequence = NFC_PACKET_HEADER;
	                spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
	            }
	            else {
	                pr_err("%s NFC is alread On!\n", __func__);
	            }
	            //                                                                                    
		} else  if (arg == 0) {
			/* power off */
			dprintk(PN544_DRV_NAME ":%s power off\n", __func__);
			//                                                                                      
                        if (sPowerState == NFC_POWER_ON) {
				gpio_set_value(pn544_dev->firm_gpio, 0);
				gpio_set_value(pn544_dev->ven_gpio, 0);
				msleep(10);
				
				spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
#ifdef CONFIG_LGE_NFC_SET_IRQ_WAKEUP
				if (sIrqState == true) {
					irq_set_irq_wake(pn544_dev->client->irq,0);
					sIrqState = false;
					dprintk(PN544_DRV_NAME ":%s disable IRQ\n", __func__);
				}
				else {
					pr_err("%s IRQ is already disabled!\n", __func__);
				}
#endif			

				if (pn544_dev->count_irq > 0) {
					pr_err("%s: Clear IRQ(%d)\n", __func__, pn544_dev->count_irq);
					pn544_dev->count_irq = 0;
					wake_unlock(&nfc_wake_lock);
				}
				//pr_err("%s NFC_POWER_OFF\n", __func__); // for debug
				sPowerState = NFC_POWER_OFF;
				sReadSequence = NFC_PACKET_HEADER;
				spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);

			}
			else {
				pr_err("%s NFC is alread Off!\n", __func__);
			}
			//                                                                                    
#ifdef LGE_NFC_READ_IRQ_MODIFY
		} else if (arg == 3) {//DY_TEST
			dprintk("%s Read Cancle\n", __func__);
            cancle_read = true;
            do_reading = 1;
        	wake_up(&pn544_dev->read_wq);
#endif
		} else {
				pr_err("%s bad arg %ld\n", __func__, arg);
			return -EINVAL;
		}
		break;
	case PN544_INTERRUPT_CMD:
		{
			/*
			pn544_disable_irq = level;
			*/
			dprintk(PN544_DRV_NAME ":ioctl: pn544_interrupt enable level:%ld\n", arg);
			break;
		}
	case PN544_READ_POLLING_CMD:
		{
			stReadIntFlag = arg;
			dprintk(PN544_DRV_NAME ":ioctl: pn544_polling flag set:%ld\n", arg);
			break;
		}
	case PN544_HW_REVISION:
		{
			return pn544_get_hw_revision();
		}
	default:
		pr_err("%s bad ioctl %d\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations pn544_dev_fops = {
	.owner	= THIS_MODULE,
	.llseek	= no_llseek,
	.read	= pn544_dev_read,
	.write	= pn544_dev_write,
	.open	= pn544_dev_open,
	.unlocked_ioctl = pn544_dev_unlocked_ioctl,
};

static int pn544_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;
	struct pn544_dev *pn544_dev = NULL;
    pn544_client = client;

	dprintk(PN544_DRV_NAME ": pn544_probe() start\n");

	pn544_dev = kzalloc(sizeof(*pn544_dev), GFP_KERNEL);
	if (pn544_dev == NULL) {
		dev_err(&client->dev,
				"failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	pn544_parse_dt(&client->dev, pn544_dev);

	pn544_dev->client   = client;
	dprintk(PN544_DRV_NAME ":IRQ : %d\nVEN : %d\nFIRM : %d\n",
			pn544_dev->irq_gpio, pn544_dev->ven_gpio, pn544_dev->firm_gpio);

	ret = gpio_request(pn544_dev->irq_gpio, "nfc_int");
	if (ret) {
		dprintk(PN544_DRV_NAME ":pn544_probe() : nfc_int request failed!\n");
		goto err_int;
	}
	ret = gpio_request(pn544_dev->ven_gpio, "nfc_ven");
	if (ret) {
		dprintk(PN544_DRV_NAME ":pn544_probe() : nfc_ven request failed!\n");
		goto err_ven;
	}
	ret = gpio_request(pn544_dev->firm_gpio, "nfc_firm");
	if (ret) {
		dprintk(PN544_DRV_NAME ":pn544_probe() : nfc_firm request failed!\n");
		goto err_firm;
	}

	pn544_gpio_enable(pn544_dev);

	ret = gpio_direction_output(pn544_dev->ven_gpio,0);
	ret = gpio_direction_output(pn544_dev->firm_gpio,0);
	ret = gpio_direction_input(pn544_dev->irq_gpio);

	/* init mutex and queues */
	init_waitqueue_head(&pn544_dev->read_wq);
	mutex_init(&pn544_dev->read_mutex);
#ifdef CONFIG_LGE_NFC_PRESTANDBY
	mutex_init(&mode_mutex);
#endif
	spin_lock_init(&pn544_dev->irq_enabled_lock);

	pn544_dev->pn544_device.minor = MISC_DYNAMIC_MINOR;
	pn544_dev->pn544_device.name = PN544_DRV_NAME;
	pn544_dev->pn544_device.fops = &pn544_dev_fops;

	ret = misc_register(&pn544_dev->pn544_device);
	if (ret) {
		pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

    //                                                                                      
    wake_lock_init(&nfc_wake_lock, WAKE_LOCK_SUSPEND, "NFCWAKE");
    //                                                                                    

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	pr_info("%s : requesting IRQ %d\n", __func__, client->irq);
	pn544_dev->irq_enabled = true;
    //                                                                                      
    ret = request_irq(pn544_gpio_to_irq(pn544_dev), pn544_dev_irq_handler,
              IRQF_TRIGGER_RISING|IRQF_NO_SUSPEND, client->name, pn544_dev);
    //                                                                                    
	if (ret) {
		dev_err(&client->dev, "request_irq failed\n");
		goto err_request_irq_failed;
	}
#if !defined(LGE_NFC_HW_QCT_MSM8660)&&!defined(CONFIG_LGE_NFC_HW_QCT_MSM8255)
	enable_irq_wake(pn544_get_irq_pin(pn544_dev));
#endif
	pn544_disable_irq(pn544_dev);
	i2c_set_clientdata(client, pn544_dev);
	dprintk(PN544_DRV_NAME ": pn544_probe() end\n");
/*             
  
                                  
                                    
                                   
  
                                  
 */
#ifdef CONFIG_LGE_NFC_PRESTANDBY
    if (pn544_validate_boot_mode()) {
	    dprintk("%s : get in the standbyset\n", __func__);
#ifdef CONFIG_LGE_NFC_MULTICORE_FASTBOOT
    	{
    		struct task_struct *th;
    		th = kthread_create(pn544_factory_standby_set_thread, NULL, "pn544_factory_standby");
    		if (IS_ERR(th)) {
    			ret = PTR_ERR(th);
    			goto err_request_irq_failed;
    		}
    		wake_up_process(th);
    	}
#else
    	pn544_factory_standby_set();
#endif
/*              */
    }
#endif
	return 0;

err_request_irq_failed:
	misc_deregister(&pn544_dev->pn544_device);

err_misc_register:
	mutex_destroy(&pn544_dev->read_mutex);
#ifdef CONFIG_LGE_NFC_PRESTANDBY
	mutex_destroy(&mode_mutex);
#endif
	gpio_free(pn544_dev->firm_gpio);

err_firm:
	gpio_free(pn544_dev->ven_gpio);

err_ven:
	gpio_free(pn544_dev->irq_gpio);

err_int:
	kfree(pn544_dev);

err_exit:
	pr_err(PN544_DRV_NAME ": pn544_dev is null\n");

	pr_err(PN544_DRV_NAME ": pn544_probe() end with error!\n");

	return ret;
}

static __devexit int pn544_remove(struct i2c_client *client)
{
	struct pn544_dev *pn544_dev;

	pn544_dev = i2c_get_clientdata(client);
	free_irq(pn544_gpio_to_irq(pn544_dev), pn544_dev);
	misc_deregister(&pn544_dev->pn544_device);
	mutex_destroy(&pn544_dev->read_mutex);
#ifdef CONFIG_LGE_NFC_PRESTANDBY
	mutex_destroy(&mode_mutex);
#endif
    gpio_free(pn544_dev->firm_gpio);
	gpio_free(pn544_dev->ven_gpio);
    gpio_free(pn544_dev->irq_gpio);
	kfree(pn544_dev);

	return 0;
}

static void pn544_shutdown(struct i2c_client *client)
{
	struct pn544_dev *pn544_dev;
	// Get PN544 Device Structure data
	pn544_dev = i2c_get_clientdata(client);

	pn544_shutdown_cb(pn544_dev);
	return;
}

static const struct i2c_device_id pn544_id[] = {
	{ PN544_DRV_NAME, 0 },
	{ }
};

#ifdef CONFIG_LGE_NFC_PN547
static struct of_device_id pn547_match_table[] = {
	{ .compatible = "NXP,pn547",},
	{ },
};

static struct i2c_driver pn544_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = PN544_DRV_NAME,
		.of_match_table = pn547_match_table,
	},
	.probe = pn544_probe,
	.remove = __devexit_p(pn544_remove),
	.shutdown	= pn544_shutdown,
	.id_table = pn544_id,
};
#else
static struct of_device_id pn544_match_table[] = {
	{ .compatible = "NXP,pn544",},
	{ },
};

static struct i2c_driver pn544_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = PN544_DRV_NAME,
		.of_match_table = pn544_match_table,
	},
	.probe = pn544_probe,
	.remove = __devexit_p(pn544_remove),
	.shutdown	= pn544_shutdown,
	.id_table = pn544_id,
};
#endif

/*
 * module load/unload record keeping
 */

static int __init pn544_dev_init(void)
{
	int ret = 0;

	pr_info("Loading pn544 driver\n");

	ret = i2c_add_driver(&pn544_driver);
	if (ret < 0) {
		printk("[NFC]failed to i2c_add_driver\n");
	}
	pr_info("Loading pn544 or pn547 driver Success! \n");
	return ret;

}
module_init(pn544_dev_init);

static void __exit pn544_dev_exit(void)
{
	pr_info("Unloading pn544 driver\n");
	i2c_del_driver(&pn544_driver);
}
module_exit(pn544_dev_exit);

MODULE_DEVICE_TABLE(i2c, pn544_id);

MODULE_AUTHOR("Sylvain Fonteneau");
MODULE_DESCRIPTION("NFC PN544 driver");
MODULE_LICENSE("GPL");
