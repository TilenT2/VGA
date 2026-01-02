/*
 * VGA.h
 *
 *  Created on: Dec 26, 2025
 *      Author: syn
 */

#ifndef INC_VGA_H_
#define INC_VGA_H_

#include <string.h>
#include "main.h"
#include "image.h"
#include "usb_frame_buffer.h"

extern DMA_HandleTypeDef hdma_tim1_up;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim1;

#define HVISIBLE 640
#define HFPORCH 16
#define HSYNC 96
#define HBPORCH 48
#define WHOLELINE 800

#define VVISIBLE 480
#define VFPORCH 10
#define VSYNC 2
#define VBPORCH 34 //should be 33, but 34 works better
#define WHOLEFRAME 525

#define UPSCALE 4

#define VRES (VVISIBLE / UPSCALE) //120
#define HRES (HVISIBLE / UPSCALE) //160
#define HRESFULL (WHOLELINE / UPSCALE)

#define OFFSET 9 //horizontal offset for image
#define RINGBUFFER_LINES 16

uint16_t current_line;
uint8_t lineBuffer[HRESFULL];

void VGA_Init(void);
void PrepareLineBuffer(const uint8_t *imageData, uint8_t imageWidth, uint8_t imageHeight);
void fastCopy160(uint8_t *dst, const uint8_t *src);

#endif /* INC_VGA_H_ */
