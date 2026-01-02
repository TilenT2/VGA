#include "VGA.h"
//#include "PupkinExport.h"
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	//tim 1 pixel clock (6mhz)
	//tim 2 horizontal sync
	//tim 3 vertical sync
	if (htim != &htim2)
		return;
	current_line++;
	// During visible vertical area
	if (current_line > VBPORCH && current_line < (VBPORCH + VVISIBLE - VSYNC)) {
		// Send pixel data for current row
		DMA1_Channel5->CCR &= ~DMA_CCR_EN;  		// Disable
		DMA1_Channel5->CMAR = (uint32_t) lineBuffer;
		DMA1_Channel5->CNDTR = HRESFULL;        	// Reset counter
		DMA1_Channel5->CCR |= DMA_CCR_EN;   		// Re-enable

		PrepareLineBuffer(Syn_cyan_bg, IMAGE_WIDTH, IMAGE_HEIGHT);
	}
	// Reset at end of frame
	if (current_line >= WHOLEFRAME) {
		current_line = 0;
	}
}

void VGA_Init(void) {
	PrepareLineBuffer(Syn_cyan_bg, IMAGE_WIDTH, IMAGE_HEIGHT);
	HAL_DMA_Start(&hdma_tim1_up, (uint32_t) lineBuffer, (uint32_t) &GPIOB->ODR,
	HRESFULL);
	__HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_UPDATE);
	GPIOB->ODR = 0x0000; // Clear all pixels

}

void PrepareLineBuffer(const uint8_t *imageData, uint8_t imageWidth,
		uint8_t imageHeight) {
	uint16_t displayLine = current_line - VBPORCH - 1;
	if ((displayLine) % UPSCALE == 0) {
		uint16_t SourceRow = ((displayLine) / UPSCALE);
		if (SourceRow < IMAGE_HEIGHT) {
			fastCopy160(lineBuffer + OFFSET,
					Syn_cyan_bg + (SourceRow * IMAGE_WIDTH));
		}
	}

}

void fastCopy160(uint8_t *dst, const uint8_t *src) {
	uint32_t *src32 = (uint32_t*) src;
	uint32_t *dst32 = (uint32_t*) dst;
	for (int i = 0; i < (IMAGE_WIDTH / 4); i++) {
		*dst32++ = *src32++;
	}
}



/*
 *
 * vova is a derg
 *
 *
 *
 */
