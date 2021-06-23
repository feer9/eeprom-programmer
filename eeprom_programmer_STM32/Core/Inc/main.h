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
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
enum memtype_e { MEMTYPE_24LC16, MEMTYPE_X24645, MEMTYPE_24LC256,
				 MEMTYPE_NONE, MEMTYPE_mAX = MEMTYPE_NONE};
extern uint32_t g_memsize;
extern enum memtype_e g_memtype;

extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart2;

extern uint8_t membuffer[];
extern uint8_t recvbuffer[];

enum commands_e {
	CMD_INIT, CMD_START_XFER, CMD_END_XFER, CMD_PING,
	CMD_READMEM16,  CMD_READMEMX64,  CMD_READMEM256,
	CMD_WRITEMEM16, CMD_WRITEMEMX64, CMD_WRITEMEM256,
	CMD_ERR, CMD_OK,
	CMD_mAX
};

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define PAGE_SZ 32
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

int MEM24LC64_write(const uint8_t *buffer, uint16_t register_base, uint16_t size);
int MEM24LC64_write_page(const uint8_t *pagebuffer, uint16_t register_address);
int MEM24LC64_read(uint8_t *buffer, uint16_t register_base, uint16_t size);
int MEM24LC64_read_page(uint8_t *pagebuffer, uint16_t register_address);

int MEM24LC16_write(const uint8_t *buffer, uint16_t register_base, uint16_t size);
int MEM24LC16_write_page(const uint8_t *pagebuffer, uint16_t register_address);
int MEM24LC16_read(uint8_t *buffer, uint16_t register_base, uint16_t size);

int MEMX24645_write(const uint8_t *buffer, uint16_t register_base, uint16_t size);
int MEMX24645_write_page(const uint8_t *pagebuffer, uint16_t register_address);
int MEMX24645_write_reg(uint8_t reg, uint16_t register_address);
int MEMX24645_read(uint8_t *buffer, uint16_t register_base, uint16_t size);
int MEMX24645_read_page(uint8_t *pagebuffer, uint16_t register_address);
int MEMX24645_read_reg(uint8_t *reg, uint16_t register_address);

int serial_write(const uint8_t *data, uint16_t len);
int serial_read(uint8_t *data, uint16_t len);
int serial_print(const char *s);
int serial_printnum(const char *s, int num);
int serial_println(const char *s);
int serial_printnumln(const char *s, int num);
int serial_clearScreen(void);

int read_test();
int write_test();

int readMemory(uint8_t *);
int saveMemory(const uint8_t *);

int sendErr(uint8_t);
int sendOK(void);
int receiveMemory(void);
int sendMemory(void);
int writeMemory(int memtype);
void i2c_scanner(int startAddress);
void uart_fsm(void);

uint32_t getMemSize(int memtype);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
