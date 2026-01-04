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

//from VGA.h
#define VRES 120                       // Vertical resolution
#define HRES 160                       // Horizontal resolution

// Protocol commands
#define CMD_IDLE         0xF0    // Host signals: idle/no action
#define CMD_DATA_CHUNK   0xF1  // Host signals: data chunk follows
#define CMD_FRAME_END    0xF2  // STM32 signals: frame complete
#define CMD_REQUEST_DATA 0xA0  // STM32 requests: send more data


#define ITEM_SIZE HRES                // Horizontal resolution
#define WRITE_CHUNK 480               // USB write chunk size
#define RING_BUFFER_SIZE ((VRES*HRES)/4)  // Total buffer size 4,800


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
} RingBuffer_t;


typedef struct {
    FrameState_t state;               // Current frame reception state
    uint16_t received_bytes;          // Total bytes received in current frame
    uint32_t frame_counter;           // Total frames received (for debugging)
    uint16_t processed_bytes;	      // Total bytes processed by VGA
} FrameManager_t;



// Global instances
extern RingBuffer_t ring_buffer;
extern FrameManager_t frame_manager;

extern void fastCopy160(uint8_t *dst, const uint8_t *src);

void USB_FrameBuffer_Init(void);
void USB_ProcessReceivedData(uint8_t* buf, uint32_t len);
void SendCommands(uint8_t cmd);
void RingBuffer_Write( uint8_t* data, uint16_t len);
void RingBuffer_Read(uint8_t* output);



#endif /* INC_USB_FRAME_BUFFER_H_ */
