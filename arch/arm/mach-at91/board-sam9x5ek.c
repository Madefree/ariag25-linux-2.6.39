/*
 *  Board-specific setup code for the AT91SAM9x5 Evaluation Kit family
 *
 *  Copyright (C) 2010 Atmel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/flash.h>
#include <linux/spi/spi.h>
#include <linux/fb.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/leds.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <mach/cpu.h>

#include <video/atmel_lcdfb.h>
#include <media/soc_camera.h>
#include <media/atmel-isi.h>

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/irq.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/atmel_hlcdc.h>
#include <mach/at91sam9_smc.h>
#include <mach/at91_shdwc.h>

#include "sam9_smc.h"
#include "generic.h"
#include <mach/board-sam9x5.h>

#include <linux/w1-gpio.h>


static void __init ek_map_io(void)
{
	/* Initialize processor and DBGU */
	cm_map_io();

	/* USART0 on /dev/ttyS1. (RX,TX,RTS,CTS) */
	at91_register_uart(AT91SAM9X5_ID_USART0,1,ATMEL_UART_RTS|ATMEL_UART_CTS);

	/* Pull-up on RXD0 */
	at91_set_A_periph(AT91_PIN_PA1,1);  
	
	/* USART1 on /dev/ttyS2. (RX,TX,RTS,CTS) */
	at91_register_uart(AT91SAM9X5_ID_USART1,2,ATMEL_UART_RTS|ATMEL_UART_CTS);
	/* Pull-up on RXD1 */
	at91_set_A_periph(AT91_PIN_PA6,1);  

	/* USART2 on /dev/ttyS3. (RX,TX) */
	at91_register_uart(AT91SAM9X5_ID_USART2,3,0);
	/* Pull-up on RXD2 */
	at91_set_A_periph(AT91_PIN_PA8,1);
	
	/* USART3 on /dev/ttyS4. (RX,TX,RTS,CTS) */
	at91_register_uart(AT91SAM9X5_ID_USART3,4,ATMEL_UART_RTS|ATMEL_UART_CTS);
	/* Pull-up on RXD3 */
	at91_set_B_periph(AT91_PIN_PC23,1);

	/* UART0 on /dev/ttyS5. (RX,TX) */
	/*at91_register_uart(AT91SAM9X5_ID_UART0,5,0); */
	/* Pull-up on URXD0 */
	/*at91_set_C_periph(AT91_PIN_PC9,1); */

	/* UART1 on /dev/ttyS6. (RX,TX) */
	/*at91_register_uart(AT91SAM9X5_ID_UART1,6,0); */
	/* Pull-up on URXD1 */
	/*at91_set_C_periph(AT91_PIN_PC17,1); */
}

/*
 * USB Host port (OHCI)
 */
/* Port A is shared with gadget port & Port C is full-speed only */
static struct at91_usbh_data __initdata ek_usbh_fs_data = {
	.ports		= 3,

};

/*
 * USB HS Host port (EHCI)
 */
/* Port A is shared with gadget port */
static struct at91_usbh_data __initdata ek_usbh_hs_data = {
	.ports		= 2,
};


/*
 * USB HS Device port
 */
static struct usba_platform_data __initdata ek_usba_udc_data;


/*
 * MACB Ethernet devices
 */
static struct at91_eth_data __initdata ek_macb0_data = {
	.is_rmii	= 1,
};

/*
static struct at91_eth_data __initdata ek_macb1_data = {
	.phy_irq_pin	= AT91_PIN_PC26,
	.is_rmii	= 1,
};
*/


/*
 * MCI (SD/MMC)
 */
/* mci0 detect_pin is revision dependent */
static struct mci_platform_data __initdata mci0_data = {
	.slot[0] = {
		.bus_width	= 4,
		.wp_pin		= -1,
	},
};

/*
static struct mci_platform_data __initdata mci1_data = {
	.slot[0] = {
		.bus_width	= 4,
		.detect_pin	= AT91_PIN_PD14,
		.wp_pin		= -1,
	},
};
*/

/*
 * I2C Devices
 */
static struct i2c_board_info __initdata ek_i2c_devices[] = {
	{
		I2C_BOARD_INFO("wm8731", 0x1b)
	},
};

static struct spi_board_info ariag25_spi_devices[] = {
    {
        .modalias    = "spidev",
        .chip_select    = 0,
        .max_speed_hz    = 10 * 1000 * 1000,
        .bus_num    = 0,
        .mode = SPI_MODE_3,
    },
};

static void __init ek_board_configure_pins(void)
{
	  ek_macb0_data.phy_irq_pin = 0;
}

#if defined(CONFIG_W1_MASTER_GPIO) || defined(CONFIG_W1_MASTER_GPIO_MODULE)
static struct w1_gpio_platform_data w1_gpio_pdata = {
	.pin		= AT91_PIN_PA21,
	.is_open_drain  = 1,
};

static struct platform_device w1_device = {
	.name			= "w1-gpio",
	.id			= -1,
	.dev.platform_data	= &w1_gpio_pdata,
};

static void __init at91_add_device_w1(void)
{
	at91_set_GPIO_periph(w1_gpio_pdata.pin, 1);
	at91_set_multi_drive(w1_gpio_pdata.pin, 1);
	platform_device_register(&w1_device);
}
#else
void __init at91_add_device_w1(void) {}
#endif


static void __init ek_board_init(void)
{
	u32 cm_config;

	cm_board_init(&cm_config);
	ek_board_configure_pins();

	/* Serial */
	at91_add_device_serial();

	/* USB HS Host */
	at91_add_device_usbh_ohci(&ek_usbh_fs_data);
	at91_add_device_usbh_ehci(&ek_usbh_hs_data);

	/* USB HS Device */
	at91_add_device_usba(&ek_usba_udc_data);

	/* Ethernet */
	at91_add_device_eth(0, &ek_macb0_data);
	/* at91_add_device_eth(1, &ek_macb1_data); */

	/* SPI */
	at91_add_device_spi(ariag25_spi_devices, ARRAY_SIZE(ariag25_spi_devices));

	/* MMC */
	at91_add_device_mci(0, &mci0_data);

	/* Add 1-wire bus */
	at91_add_device_w1();
	
	/* I2C */
	if (cm_config & CM_CONFIG_I2C0_ENABLE)
		i2c_register_board_info(0, ek_i2c_devices, ARRAY_SIZE(ek_i2c_devices));
	else
		at91_add_device_i2c(0, ek_i2c_devices, ARRAY_SIZE(ek_i2c_devices));

	/* SSC (for WM8731) */
	at91_add_device_ssc(AT91SAM9X5_ID_SSC, ATMEL_SSC_TX | ATMEL_SSC_RX);
}

MACHINE_START(AT91SAM9X5EK, "Acme Systems Aria G25")
	/* Maintainer: Atmel */
/* XXX/ukl: can we drop .boot_params? */
	.boot_params	= AT91_SDRAM_BASE + 0x100,
	.timer		= &at91sam926x_timer,
	.map_io		= ek_map_io,
	.init_irq	= cm_init_irq,
	.init_machine	= ek_board_init,
MACHINE_END
