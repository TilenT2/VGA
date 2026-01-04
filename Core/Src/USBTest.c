/*
 * USBTest.c
 *
 *  Created on: Jan 3, 2026
 *      Author: syn
 */
#include "usb_frame_buffer.h"
#include "image.h"
#include <string.h>

void USBTest_Function(void) {
    uint8_t testChunk[WRITE_CHUNK];

    // Check if we've completed a frame FIRST
    if (frame_manager.processed_bytes >= (HRES * VRES)) {
        uint8_t cmd = CMD_FRAME_END;
        USB_ProcessReceivedData(&cmd, 1);

        // Start next frame
        uint8_t cmd2 = CMD_DATA_CHUNK;
        USB_ProcessReceivedData(&cmd2, 1);
        return; // Exit this iteration
    }

    // Send data if we're receiving and buffer is low
    if (frame_manager.state == FRAME_STATE_RECEIVING) {
        if (ring_buffer.available_bytes <= 160*10) {
            // Calculate how much data we can send without exceeding frame size
            uint16_t remaining = (HRES * VRES) - frame_manager.received_bytes;
            uint16_t to_send = (remaining < WRITE_CHUNK) ? remaining : WRITE_CHUNK;

            // FIXED: Use memcpy to copy actual image data
            memcpy(testChunk, &testData[frame_manager.received_bytes], to_send);
            USB_ProcessReceivedData(testChunk, to_send);
        }
    }
}

