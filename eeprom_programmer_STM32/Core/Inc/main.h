/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define PACKED __attribute__((packed))

enum memtype_e {
	MEMTYPE_NONE,
	MEMTYPE_24LC16,
	MEMTYPE_24LC64,
	MEMTYPE_X24645,
	MEMTYPE_24LC256,
	MEMTYPE_mAX
};
// 24LC256 NOT SUPPORTED!!!
typedef enum memtype_e memtype_t;

extern uint32_t g_memsize;
extern enum memtype_e g_memtype;
//extern uint8_t membuffer[];
extern uint8_t g_buffer[];

extern I2C_HandleTypeDef hi2c2;
//extern UART_HandleTypeDef huart2;

enum PACKED commands_e {
	CMD_NONE			= 0x00,

	CMD_INIT			= 0x01,
	CMD_PING			= 0x02,
	CMD_MEMID			= 0x03,
	CMD_STARTXFER		= 0xA5, /* not really a command */
	CMD_ENDXFER			= 0x5A, /* not really a command */
	CMD_DISCONNECT		= 0x0F,

	CMD_OK				= 0x10,
	CMD_TXRX_ACK		= 0x11,
	CMD_TXRX_DONE		= 0x12,
	CMD_ERR				= 0xF0, /* general error message followed by an error code    */
	CMD_TXRX_ERR		= 0xF1, /* mid transfer error, meant to resend content        */

	/* read eeprom and send to PC */
	CMD_READMEM			= 0x60, /* Specifies which memory is required, and start TX process */
	CMD_READNEXT		= 0x61, /* Request to send next block */

	CMD_MEMDATA			= 0x70, /* Data is being sent over */
	CMD_DATA			= 0x71, /* Simple 1byte data command */
	CMD_INFO			= 0x72, /* PKG_DATA_MAX bytes of text */

	/* get data from pc and write to eeprom */
	CMD_WRITEMEM		= 0x80
};
typedef enum commands_e command_t;

/*
 * PACKAGE STRUCTURE:
 * <STX><COMMAND>[<DATA><DATA>...]<CHECKSUM[1]><CHECKSUM[0]><ETX>
 */

typedef struct {
	command_t cmd;
	uint16_t crc;
	uint16_t datalen;
	uint8_t *data;
} package_t;


struct memory_info {
	uint16_t address7;
	uint16_t size;
	uint16_t pageSz;
	uint16_t addrSz;
};

enum PACKED errorcode_e {
	ERROR_NONE,
	ERROR_UNKNOWN,   /* Implies CMD_DISCONNECT */
	ERROR_MEMID,
	ERROR_READMEM,
	ERROR_WRITEMEM,
	ERROR_COMM,
	ERROR_MAX_RETRY, /* Implies CMD_DISCONNECT */
	ERROR_TIMEOUT,   /* Implies CMD_DISCONNECT */
	ERROR_MEMIDX
};
typedef enum errorcode_e errorcode_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* Maximum data length in a single package */
#define PKG_DATA_MAX 256

/* Maximum number of times we will resend a message before giving up */
#define RETRIES_MAX 10

/* Timeout for receiving a package */
#define TIMEOUT_MS 5000
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

int EEPROM_write(memtype_t device, const uint8_t *buffer, uint16_t register_base, uint16_t size);
int EEPROM_write_page(memtype_t device, const uint8_t *pagebuffer, uint16_t register_address);
int EEPROM_write_reg(memtype_t device, uint8_t reg, uint16_t register_address);
int EEPROM_read(memtype_t device, uint8_t *buffer, uint16_t register_base, uint16_t size);
int EEPROM_read_page(memtype_t device, uint8_t *pagebuffer, uint16_t register_address);
int EEPROM_read_reg(memtype_t device, uint8_t *reg, uint16_t register_address);

int MEMX24645_write_page(const uint8_t *page, uint16_t register_address);
int MEMX24645_write(const uint8_t *buffer, uint16_t register_base, uint16_t size);
int MEMX24645_write_reg(uint8_t reg, uint16_t register_address);
int MEMX24645_read_reg(uint8_t *reg, uint16_t register_address);
int MEMX24645_read_page(uint8_t *page, uint16_t register_address);
int MEMX24645_read(uint8_t *buf, uint16_t register_base, uint16_t size);

int serial_write(const uint8_t *data, uint16_t len);
int serial_writebyte(uint8_t byte);
int serial_read(uint8_t *data, uint16_t len);
int serial_readbyte(uint8_t *byte);
int serial_print(const char *s);
int serial_printnum(const char *s, int num);
int serial_println(const char *s);
int serial_printnumln(const char *s, int num);
int serial_clearScreen(void);
bool serial_available(void);

int read_test();
int write_test();

int readMemory(uint8_t *);
int readMemoryBlock(uint8_t *membuffer, uint16_t offset);
int saveMemory(const uint8_t *);
int saveMemoryBlock(const uint8_t *membuffer, uint16_t offset);

HAL_StatusTypeDef sendErr(uint8_t);
HAL_StatusTypeDef sendOK(void);
HAL_StatusTypeDef sendRxACK(void);
HAL_StatusTypeDef receiveMemory(void);
HAL_StatusTypeDef sendMemory();
int writeMemory(enum memtype_e memtype);
void i2c_scanner(int);
void uart_fsm(void);

HAL_StatusTypeDef sendCommand(uint8_t cmd);
HAL_StatusTypeDef sendPackage(uint8_t cmd, uint8_t *data, uint16_t len);
HAL_StatusTypeDef receivePackage(package_t *pkg);
HAL_StatusTypeDef try_receive(package_t *pkg);

uint32_t getMemSize(enum memtype_e memtype);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_12
#define LED_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

#define LED_ON  GPIO_PIN_RESET
#define LED_OFF GPIO_PIN_SET

#define led_on()  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_ON)
#define led_off() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, LED_OFF)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
