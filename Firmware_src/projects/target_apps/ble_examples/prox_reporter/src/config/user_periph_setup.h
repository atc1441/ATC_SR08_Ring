#ifndef _USER_PERIPH_SETUP_H_
#define _USER_PERIPH_SETUP_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gpio.h"
#include "uart.h"
#include "spi.h"
#include "spi_flash.h"
#include "i2c.h"
#include "i2c_eeprom.h"

/****************************************************************************************/
/* UART2 configuration to use with arch_console print messages                          */
/****************************************************************************************/
// Define UART2 Tx Pad
#define UART2_TX_PORT           GPIO_PORT_0
#define UART2_TX_PIN            GPIO_PIN_4

// Define UART2 Settings
#define UART2_BAUDRATE              UART_BAUDRATE_115200 //UART_BAUDRATE_1000000
#define UART2_DATABITS              UART_DATABITS_8
#define UART2_PARITY                UART_PARITY_NONE
#define UART2_STOPBITS              UART_STOPBITS_1
#define UART2_AFCE                  UART_AFCE_DIS
#define UART2_FIFO                  UART_FIFO_EN
#define UART2_TX_FIFO_LEVEL         UART_TX_FIFO_LEVEL_0
#define UART2_RX_FIFO_LEVEL         UART_RX_FIFO_LEVEL_0

/****************************************************************************************/
/* SPI configuration                                                                    */
/****************************************************************************************/
// Define SPI Pads
    #define SPI_EN_PORT             GPIO_PORT_0
    #define SPI_EN_PIN              GPIO_PIN_3

    #define SPI_CLK_PORT            GPIO_PORT_0
    #define SPI_CLK_PIN             GPIO_PIN_0

    #define SPI_DO_PORT             GPIO_PORT_0
    #define SPI_DO_PIN              GPIO_PIN_6

    #define SPI_DI_PORT             GPIO_PORT_0
    #define SPI_DI_PIN              GPIO_PIN_5

// Define SPI Configuration
    #define SPI_MS_MODE             SPI_MS_MODE_MASTER
    #define SPI_CP_MODE             SPI_CP_MODE_0
    #define SPI_WSZ                 SPI_MODE_8BIT
    #define SPI_CS                  SPI_CS_0

    #define SPI_SPEED_MODE          SPI_SPEED_MODE_4MHz

/****************************************************************************************/
/* SPI Flash configuration                                                              */
/****************************************************************************************/
#define SPI_FLASH_DEV_SIZE          ((256 * 1024)*4)

/****************************************************************************************/
/* I2C configuration                                                                    */
/****************************************************************************************/
// Define I2C Pads
#define I2C_SCL_PORT                GPIO_PORT_2
#define I2C_SCL_PIN                 GPIO_PIN_3

#define I2C_SDA_PORT                GPIO_PORT_2
#define I2C_SDA_PIN                 GPIO_PIN_4

// Define I2C Configuration
#define I2C_SLAVE_ADDRESS           (0x50)
#define I2C_SPEED_MODE              I2C_SPEED_FAST
#define I2C_ADDRESS_MODE            I2C_ADDRESSING_7B
#define I2C_ADDRESS_SIZE            I2C_2BYTES_ADDR

/****************************************************************************************/
/* Button configuration                                                                 */
/****************************************************************************************/
#define GPIO_LEDC_PORT        GPIO_PORT_1
#define GPIO_LEDC_PIN         GPIO_PIN_0
#define GPIO_BUTTON_PORT        GPIO_PORT_1
#define GPIO_BUTTON_PIN         GPIO_PIN_5

/****************************************************************************************/
/* Proximity Reporter pad configuration                                                 */
/****************************************************************************************/
#define USE_BAT_LEVEL_ALERT         0

#define GPIO_ALERT_LED_PORT     GPIO_PORT_1
#define GPIO_ALERT_LED_PIN      GPIO_PIN_0
#define GPIO_BAT_LED_PORT       GPIO_PORT_1
#define GPIO_BAT_LED_PIN        GPIO_PIN_2

/***************************************************************************************/
/* Production debug output configuration                                               */
/***************************************************************************************/
#if PRODUCTION_DEBUG_OUTPUT
    #define PRODUCTION_DEBUG_PORT   GPIO_PORT_2
    #define PRODUCTION_DEBUG_PIN    GPIO_PIN_5
#endif
/**
 ****************************************************************************************
 * @brief Enable pad and peripheral clocks assuming that peripheral power domain
 *        is down. The UART and SPI clocks are set.
 ****************************************************************************************
 */
void periph_init(void);

/**
 ****************************************************************************************
 * @brief Each application reserves its own GPIOs here.
 ****************************************************************************************
 */
void GPIO_reservations(void);

/**
 ****************************************************************************************
 * @brief Set gpio port function mode
 ****************************************************************************************
 */
void set_pad_functions(void);

#endif // _USER_PERIPH_SETUP_H_
