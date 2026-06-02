#ifndef __BITBAND_H
#define __BITBAND_H

#include "stm32f10x_conf.h"

#define BIT_BAND(ADDR, N) ((ADDR&0xF0000000)+0x2000000+((ADDR&0xFFFFF)<<5)+(N<<2))
#define MEM_ADDR(ADDR) *(volatile unsigned int *)(ADDR)
#define BITBAND(ADDR,N) MEM_ADDR(BIT_BAND(ADDR, N))

#define GPIOA_IDR (GPIOA_BASE + 0x08)
#define GPIOA_ODR (GPIOA_BASE + 0x0C)

#define GPIOB_IDR (GPIOB_BASE + 0x08)
#define GPIOB_ODR (GPIOB_BASE + 0x0C)

#define GPIOC_IDR (GPIOC_BASE + 0x08)
#define GPIOC_ODR (GPIOC_BASE + 0x0C)

#define GPIOD_IDR (GPIOD_BASE + 0x08)
#define GPIOD_ODR (GPIOD_BASE + 0x0C)

#define PAIn(n) BITBAND(GPIOA_IDR, n)
#define PAOut(n) BITBAND(GPIOA_ODR, n)

#define PBIn(n) BITBAND(GPIOB_IDR, n)
#define PBOut(n) BITBAND(GPIOB_ODR, n)

#define PCIn(n) BITBAND(GPIOC_IDR, n)
#define PCOut(n) BITBAND(GPIOC_ODR, n)

#define PDIn(n) BITBAND(GPIOD_IDR, n)
#define PDOut(n) BITBAND(GPIOD_ODR, n)

#endif








