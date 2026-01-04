/*
 * usb_frame_buffer.c
 *
 *  Created on: Dec 31, 2025
 *      Author: syn
 */



#include "usb_frame_buffer.h"
#include "usbd_cdc_if.h"
#include <string.h>

// Global instances
RingBuffer_t ring_buffer;
FrameManager_t frame_manager;


void USB_FrameBuffer_Init(void) {
    // Clear ring buffer
    memset(&ring_buffer, 0, sizeof(RingBuffer_t));

    // Initialize frame manager
    frame_manager.state = FRAME_STATE_IDLE;
    frame_manager.received_bytes = 0;
    frame_manager.frame_counter = 0;
}



/**
 * Send a request to host for more data
 * Uses USB CDC to send single command byte
 */
void SendCommands(uint8_t cmd) {
    // Only send if we haven't already requested
        CDC_Transmit_FS(&cmd, 1);
    }





/**
 * Write data into ring buffer (called when USB receives pixel data)
 * Data should only be requested when enough space is available
 *
 * @param data: pointer to pixel data
 * @param len: number of bytes to write
 *
 *
 */
void RingBuffer_Write( uint8_t* data, uint16_t len) {

	ring_buffer.available_bytes += len;
	if (len > RING_BUFFER_SIZE - ring_buffer.write_pos) {
		uint16_t space_left = RING_BUFFER_SIZE - ring_buffer.write_pos;
		memcpy(&ring_buffer.data[ring_buffer.write_pos], data, space_left); //copy to end
	    ring_buffer.write_pos = 0; //wrap around
	    uint16_t remaining = len - space_left; //remaining after wrap
	    memcpy(&ring_buffer.data[ring_buffer.write_pos], &data[space_left], remaining);
		//copy remaining after wrap
		ring_buffer.write_pos += remaining;

	}
	else{
		memcpy(&ring_buffer.data[ring_buffer.write_pos], data, len);
		ring_buffer.write_pos += len;
		frame_manager.received_bytes += len;
	}

}


/**
 * Read a complete line from ring buffer
 * Does NOT advance read pointer - call RingBuffer_ConsumeLine() after reading
 *
 * @param output: buffer to copy line into (must be LINE_WIDTH bytes)
 *
 */
void RingBuffer_Read(uint8_t *output) {
	if (ring_buffer.available_bytes < ITEM_SIZE) {
		return; // Not enough data
	}

	fastCopy160(output, &ring_buffer.data[ring_buffer.read_pos]);
	ring_buffer.available_bytes -= ITEM_SIZE;
	if (ring_buffer.read_pos + ITEM_SIZE >= RING_BUFFER_SIZE) {
		ring_buffer.read_pos = 0; //wrap around
	}
	else{
		ring_buffer.read_pos += ITEM_SIZE;
	}
	frame_manager.processed_bytes += ITEM_SIZE;
}




/**
 * Process received USB data
 * Call this from CDC_Receive_FS() callback in usbd_cdc_if.c
 *
 * @param buf: received data buffer
 * @param len: number of bytes received
 */
void USB_ProcessReceivedData(uint8_t *buf, uint32_t len) {
    uint8_t byte = buf[0];

	if (len == 1) { // Command byte

		if (byte == CMD_DATA_CHUNK) {
			// Data chunk header - next bytes are pixel data
			frame_manager.state = FRAME_STATE_RECEIVING;
		} else if (byte == CMD_FRAME_END) {
			// Frame complete
			frame_manager.state = FRAME_STATE_COMPLETE;
			frame_manager.frame_counter++;
			frame_manager.processed_bytes = 0;
			frame_manager.received_bytes = 0;
			ring_buffer.available_bytes = 0;
			ring_buffer.write_pos = 0;
			ring_buffer.read_pos = 0;


		}
	} else if (frame_manager.state == FRAME_STATE_RECEIVING) {
		// Pixel data
		RingBuffer_Write(buf, len);

	}
}


void RequestChunk(){
	if (ring_buffer.available_bytes <= (160*15)){
		uint8_t cmd = CMD_REQUEST_DATA;
		SendCommands(cmd);
	}
}


