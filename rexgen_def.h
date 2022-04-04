// SPDX-License-Identifier: GPL-2.0
/* 
    USB to SocketCAN driver for ReXgen
    Copyright (C) 1999-2021 Influx Technology LTD, UK. All rights reserved.
    Contacts: https://www.influxtechnology.com/contact

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef rexgen_usb_H_
#define rexgen_usb_H_

#include <linux/usb.h>
#include <linux/can/dev.h>

#define USB_CMD_DEBUG               0 // 1- Debug TX/RX commands; 0 - Silence
#define DeviceName                  "ReXgen"

#define DEVICE_VENDOR_ID            0x16d0
#define DEVICE_PRODUCT_ID           0x0f14

static const struct usb_device_id influx_usb_table [] = {
    { 
        USB_DEVICE(DEVICE_VENDOR_ID, DEVICE_PRODUCT_ID) 
    }, {},
};

#define USB_TIMEOUT                 2000 // msec
#define USB_MAX_TX_URBS				128
#define USB_MAX_RX_URBS				4
#define USB_TRANSFER_BLOCK_SIZE 	0x4000
#define CAN_CHANNELS				2
#define USB_MAX_NET_DEVICES			5
#define USB_RX_BUFFER_SIZE			512

// bittiming parameters 
#define USB_TSEG1_MIN				1
#define USB_TSEG1_MAX				16
#define USB_TSEG2_MIN				1
#define USB_TSEG2_MAX				8
#define USB_SJW_MAX                 4
#define USB_BRP_MIN                 1
#define USB_BRP_MAX                 64
#define USB_BRP_INC                 1

// firmware supporting socket CAN
#define SUPP_GET_NUM_CHANNELS_MAJOR		2
#define SUPP_GET_NUM_CHANNELS_MINOR		18
#define SUPP_GET_NUM_CHANNELS_PATCH		0

// errors
#define SUCCESS                                 0x00
#define FIRMWARE_UPDATE_IS_IN_PROGRESS          0x01
#define INVALID_FIRMWARE_NAME                   0x02
#define FIRMWARE_OPEN_FILE_ERROR                0x03
#define LOG_FILE_OPEN_ERROR                     0x04 
#define CALLBACK_FUNCTION_NOT_INITIALIZED       0x05
#define USB_COMMUNICATION_ERROR                 0x06
#define DEVICE_IS_NOT_CONNECTED                 0x07
#define NOT_SUPPORTED                           0x08
#define WRONG_CRC                               0x09
#define FILE_SAVE_OPERATION_FAILED              0x0A

// usb commans
#define USB_CMD_GET_FW_VERSION              0x02
#define USB_CMD_START_LIVE_DATA             0x19
#define USB_CMD_STOP_LIVE_DATA              0x1a
#define USB_CMD_GET_LIVE_DATA               0x83

#define USB_CMD_CAN_INTERFACE_ENABLE		0x31
#define USB_CMD_CAN_INTERFACE_DISABLE		0x32
#define USB_CMD_CAN_BUS_COUNT               0x33
#define USB_CMD_CAN_BUS_OPEN			    0x34
//#define USB_CMD_CAN_BUS_CLOSE             0x35
#define USB_CMD_CAN_BUS_ON                  0x36      
//#define USB_CMD_CAN_BUS_OFF			    0x37
#define USB_CMD_CAN_PARAM_SET			    0x38
//#define USB_CMD_CAN_PARAM_GET             0x39
//#define USB_CMD_CAN_DATA_PARAM_SET		0x3a
//#define USB_CMD_CAN_DATA_PARAM_GET		0x3b
#define USB_CMD_CAN_BLOCK_UID_GET           0x3c

// CAN blocks UID indexes
#define IDX_CAN_BLOCK_UID_RX			0
#define IDX_CAN_BLOCK_UID_TX			1
#define IDX_CAN_BLOCK_UID_ERR			2

// CAN DataFrame Flags
#define DataFrame_IDE  1  // Identifier extension bit
#define DataFrame_SRR  2  // Substitute remote request
#define DataFrame_EDL  4  // Extended data length
#define DataFrame_BRS  8  // Bit rate switch
#define DataFrame_DIR 16  // Frame direction - 0: Rx, 1:Tx

struct usb_tx_context {
    struct rexgen_net *net;
    u32 echo_index;
    int dlc;
};

typedef struct 
{
    uint32_t tx_len;
    uint32_t rx_len;
    unsigned char cmd_data[];
} cmd_struct;

static const cmd_struct cmdGetFwVersion = {2, 15, {USB_CMD_GET_FW_VERSION, 0x00}};
static const cmd_struct cmdCANBusCount = {2, 7, {USB_CMD_CAN_BUS_COUNT, 0x00}};
static const cmd_struct cmdCANIntfEnable = {2, 10, {USB_CMD_CAN_INTERFACE_ENABLE, 0x00}};
static const cmd_struct cmdCANIntfDisable = {2, 10, {USB_CMD_CAN_INTERFACE_DISABLE, 0x00}};
static cmd_struct cmdCANBlockUIDGet = {5, 10, {USB_CMD_CAN_BLOCK_UID_GET, 0x00, 0, 0, 0}};
static cmd_struct cmdCANParamSet = {12, 10, {USB_CMD_CAN_PARAM_SET, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
static cmd_struct cmdCANBusOpen = {5, 10, {USB_CMD_CAN_BUS_OPEN, 0x00, 0, 0, 0}};
static cmd_struct cmdCANBusOn = {4, 10, {USB_CMD_CAN_BUS_ON, 0x00, 0, 0}};
static const cmd_struct cmmdUSBStartLiveData = {3, 7, {USB_CMD_START_LIVE_DATA, 0x00, 0x00}}; //Third Param is Channel 
static const cmd_struct cmmdUSBStopLiveData = {3, 7, {USB_CMD_STOP_LIVE_DATA, 0x00, 0}};  //Third Param is Channel
static const cmd_struct cmmdUSBGetLiveData = {3, 16, {USB_CMD_GET_LIVE_DATA, 0x00, 0}};  //Third Param is Channel

struct usb_config {
    const struct can_clock clock;
    const unsigned int timestamp_freq;
    const struct can_bittiming_const * const bittiming_const;
    const struct can_bittiming_const * const data_bittiming_const;
};

static const struct can_bittiming_const bittiming_def = 
{
    .name = "rexgen_usb",
    .tseg1_min = USB_TSEG1_MIN,
    .tseg1_max = USB_TSEG1_MAX,
    .tseg2_min = USB_TSEG2_MIN,
    .tseg2_max = USB_TSEG2_MAX,
    .sjw_max = USB_SJW_MAX,
    .brp_min = USB_BRP_MIN,
    .brp_max = USB_BRP_MAX,
    .brp_inc = USB_BRP_INC,
};

static const struct usb_config rex_usb_cfg = 
{
    .clock = {
        .freq = 40000000,
    },
    .timestamp_freq = 1,
    .bittiming_const = &bittiming_def,
};

struct rexgen_cmd {
    unsigned long Timeout;
    unsigned long tx_len;
    unsigned long rx_len;
    unsigned char tx_data[2 * USB_TRANSFER_BLOCK_SIZE];
    unsigned char rx_data[2 * USB_TRANSFER_BLOCK_SIZE];
};

struct rexgen_usb {
    struct usb_device *udev;
    struct usb_interface *intf;
    struct rexgen_net *nets[USB_MAX_NET_DEVICES];

    struct usb_endpoint_descriptor *bulk_in, *bulk_out; 
    struct usb_endpoint_descriptor *live_in, *live_out; 
    struct usb_anchor rx_submitted;

    unsigned char fw_ver[4];
    unsigned char nchannels;    

    unsigned char usb_rx_data[2 * USB_TRANSFER_BLOCK_SIZE];
    unsigned char usb_rx_len;

    bool rxinitdone;
    void *rxbuf[USB_MAX_RX_URBS];
    dma_addr_t rxbuf_dma[USB_MAX_RX_URBS];
};

struct rexgen_net {
    struct can_priv can;
    struct can_berr_counter bec;

    struct rexgen_usb *dev;
    struct net_device *netdev;
    int channel;
    unsigned int usb_block_uid[3]; 
    

    struct completion start_comp, stop_comp, flush_comp;
    struct usb_anchor tx_submitted;
    
    spinlock_t tx_contexts_lock;
    int active_tx_contexts;
    struct usb_tx_context tx_contexts[];
};

#define RexRecordMaxInfLength  28
#define RexRecordMaxCanLength  64
typedef struct {
    unsigned short uid;
    unsigned char infsize;
    unsigned char dlc;

    unsigned char inf[RexRecordMaxInfLength];
    unsigned char data[RexRecordMaxCanLength];
} usb_record;


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void printkBuffer(void *data, int len, char* prefix);
void printkrx(struct rexgen_cmd* cmd);
void printkrxusb(struct rexgen_usb *dev);
void printktx(struct rexgen_cmd * cmd);
void printkLiveData(void *buff, int len);

int usb_get_firmware(struct rexgen_usb *dev);
int usb_get_num_channels(struct rexgen_usb *dev);
int usb_can_intf_enable(struct rexgen_usb *dev);
int usb_can_intf_disable(struct rexgen_usb *dev);
int usb_set_bittiming(struct net_device *netdev);
int usb_bus_start(struct rexgen_usb_net_priv *priv);
int usb_start_live_data(struct rexgen_usb *dev);
int usb_stop_live_data(struct rexgen_usb *dev);
int usb_can_bus_open(struct rexgen_usb *dev, unsigned short channel);
int usb_can_bus_on(struct rexgen_usb *dev, unsigned short channel);

unsigned short livedata_size(void *buff, int len);
int ptr2rec(usb_record *rec, void *buff);
void can2socket(struct rexgen_usb *dev, usb_record *rec);

#endif //rexgen_usb_H_
