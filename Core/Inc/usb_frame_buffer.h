/*
 * usb_frame_buffer.h
 *
 *  Created on: Dec 31, 2025
 *      Author: syn
 */

#ifndef INC_USB_FRAME_BUFFER_H_
#define INC_USB_FRAME_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>

// Protocol commands
#define CMD_FRAME_START  0xF0  // Host signals: new frame starting
#define CMD_DATA_CHUNK   0xF1  // Host signals: data chunk follows
#define CMD_FRAME_END    0xF2  // Host signals: frame complete
#define CMD_REQUEST_DATA 0xA0  // STM32 requests: send more data


#define RING_BUFFER_LINES 20          // Number of lines to buffer (20 × 160 = 3.2KB)
#define LINE_WIDTH 160                // Horizontal resolution
#define RING_BUFFER_SIZE (RING_BUFFER_LINES * LINE_WIDTH)  // Total buffer size

#define REQUEST_THRESHOLD 5           // Request when < 5 lines remain


// Frame state machine states
typedef enum {
    FRAME_STATE_IDLE,                 // No frame active, waiting for FRAME_START
    FRAME_STATE_RECEIVING,            // Currently receiving frame data
    FRAME_STATE_COMPLETE,             // Frame fully received, displaying
} FrameState_t;


typedef struct {
    uint8_t data[RING_BUFFER_SIZE];   // Actual pixel data storage
    uint16_t write_pos;               // Where USB writes next byte (0 to RING_BUFFER_SIZE-1)
    uint16_t read_pos;                // Where VGA reads next byte (0 to RING_BUFFER_SIZE-1)
    uint16_t available_bytes;         // How many bytes are ready to read
    bool request_pending;             // True if we've already requested more data
} RingBuffer_t;


typedef struct {
    FrameState_t state;               // Current frame reception state
    uint16_t current_display_line;    // Which line of the frame VGA is currently displaying (0-119)
    bool line_was_read; 			  // True if we successfully read the current line
    uint16_t received_bytes;          // Total bytes received in current frame
    uint32_t frame_counter;           // Total frames received (for debugging)
} FrameManager_t;



// Global instances //not sure i understand why
extern RingBuffer_t ring_buffer;
extern FrameManager_t frame_manager;

// Initialization
void USB_FrameBuffer_Init(void);

// USB receive handling (call from CDC_Receive_FS callback)
void USB_ProcessReceivedData(uint8_t* buf, uint32_t len);

// VGA read interface (call from PrepareLineBuffer)
bool USB_GetLine(uint8_t line_number, uint8_t* output_buffer);

// Buffer status (for debugging)
uint16_t USB_GetAvailableLines(void);

#endif /* INC_USB_FRAME_BUFFER_H_ */
