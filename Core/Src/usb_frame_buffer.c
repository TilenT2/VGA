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
    frame_manager.current_display_line = 0;
    frame_manager.line_was_read = false;
    frame_manager.received_bytes = 0;
    frame_manager.frame_counter = 0;
}



/**
 * Send a request to host for more data
 * Uses USB CDC to send single command byte
 */
static void SendDataRequest(void) {
    uint8_t cmd = CMD_REQUEST_DATA;

    // Only send if we haven't already requested
    if (!ring_buffer.request_pending) {
        CDC_Transmit_FS(&cmd, 1);
        ring_buffer.request_pending = true;
    }
}

/**
 * Check if we need to request more data
 */
static void CheckAndRequestData(void) {
    if (frame_manager.state != FRAME_STATE_RECEIVING) {
        return;  // Don't request if not receiving
    }

    uint16_t available_lines = ring_buffer.available_bytes / LINE_WIDTH;

    // Request more data if buffer is running low
    if (available_lines < REQUEST_THRESHOLD && !ring_buffer.request_pending) {
        SendDataRequest();
    }

}




/**
 * Write data into ring buffer (called when USB receives pixel data)
 * Handles wrap-around automatically
 *
 * @param data: pointer to pixel data
 * @param len: number of bytes to write
 * @return: number of bytes actually written (may be less if buffer full)
 */
static uint16_t RingBuffer_Write(const uint8_t* data, uint16_t len) {
    uint16_t written = 0;

    for (uint16_t i = 0; i < len; i++) {
        // Check if buffer is full
        if (ring_buffer.available_bytes >= RING_BUFFER_SIZE) {
            break;  // Buffer full
        }

        // Write byte at write position
        ring_buffer.data[ring_buffer.write_pos] = data[i];

        // Advance write position with wrap-around
        ring_buffer.write_pos++;
        if (ring_buffer.write_pos >= RING_BUFFER_SIZE) {
            ring_buffer.write_pos = 0;  // Wrap to beginning
        }

        // Increment available byte counter
        ring_buffer.available_bytes++;
        written++;
    }

    return written;
}


/**
 * Read a complete line from ring buffer
 * Does NOT advance read pointer - call RingBuffer_ConsumeLine() after reading
 *
 * @param output: buffer to copy line into (must be LINE_WIDTH bytes)
 * @return: true if line was available, false if not enough data
 */
static bool RingBuffer_ReadLine(uint8_t* output) {
    // Check if we have a full line available
    if (ring_buffer.available_bytes < LINE_WIDTH) {
        return false;  // Not enough data
    }

    // Copy line data, handling wrap-around
    for (uint16_t i = 0; i < LINE_WIDTH; i++) {
        uint16_t read_index = (ring_buffer.read_pos + i) % RING_BUFFER_SIZE;
        output[i] = ring_buffer.data[read_index];
    }

    return true;
}

/**
 * Advance read pointer after successfully reading a line
 * Call this only after RingBuffer_ReadLine() returns true
 */
static void RingBuffer_ConsumeLine(void) {
    // Advance read position by one line
    ring_buffer.read_pos += LINE_WIDTH;
    if (ring_buffer.read_pos >= RING_BUFFER_SIZE) {
        ring_buffer.read_pos -= RING_BUFFER_SIZE;  // Wrap around
    }

    // Decrease available bytes
    ring_buffer.available_bytes -= LINE_WIDTH;


    // After consuming, we might need more data
    CheckAndRequestData();

}




/**
 * Process received USB data
 * Call this from CDC_Receive_FS() callback in usbd_cdc_if.c
 *
 * @param buf: received data buffer
 * @param len: number of bytes received
 */
void USB_ProcessReceivedData(uint8_t* buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uint8_t byte = buf[i];

        if (byte == CMD_FRAME_START) {
            // New frame starting
            ring_buffer.write_pos = 0;
            ring_buffer.read_pos = 0;
            ring_buffer.available_bytes = 0;
            ring_buffer.request_pending = false;

            frame_manager.state = FRAME_STATE_RECEIVING;
            // DON'T reset current_display_line - let VGA continue naturally
            frame_manager.line_was_read = false;  // Reset read flag - important!
            frame_manager.received_bytes = 0;

        } else if (byte == CMD_DATA_CHUNK) {
            // Data chunk header
            ring_buffer.request_pending = false;

        } else if (byte == CMD_FRAME_END) {
            // Frame complete
            frame_manager.state = FRAME_STATE_COMPLETE;
            frame_manager.frame_counter++;

        } else {
            // Pixel data
            if (frame_manager.state == FRAME_STATE_RECEIVING) {
                uint16_t written = RingBuffer_Write(&byte, 1);
                if (written > 0) {
                    frame_manager.received_bytes++;
                }
            }
        }
    }

    // Check if we need more data
    CheckAndRequestData();
}


/**
 * Get a line of image data for VGA display
 * Call this from PrepareLineBuffer() in VGA.c
 *
 * @param line_number: which line of the image to retrieve (0-119)
 * @param output_buffer: buffer to copy line into (must be LINE_WIDTH bytes)
 * @return: true if line was retrieved, false if no data (show black)
 */
bool USB_GetLine(uint8_t line_number, uint8_t* output_buffer) {
    if (frame_manager.state == FRAME_STATE_IDLE) {
        memset(output_buffer, 0x00, LINE_WIDTH);
        return false;
    }

    // Check if we've moved to a different line
    if (line_number != frame_manager.current_display_line) {
        // Moving to a new line

        // ONLY consume if we successfully read the previous line
        // This prevents consuming data from frames we never read
        if (frame_manager.line_was_read) {
            RingBuffer_ConsumeLine();
        }

        // Update to new line and reset read flag
        frame_manager.current_display_line = line_number;
        frame_manager.line_was_read = false;
    }

    // Try to read the current line
    if (RingBuffer_ReadLine(output_buffer)) {
        // Successfully read - mark that we read this line
        frame_manager.line_was_read = true;
        return true;
    }

    // No data available
    memset(output_buffer, 0x00, LINE_WIDTH);
    return false;
}

/**
 * Get number of complete lines available in buffer
 * Useful for debugging
 */
uint16_t USB_GetAvailableLines(void) {
    return ring_buffer.available_bytes / LINE_WIDTH;
}



