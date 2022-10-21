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

#include <linux/version.h>
#include "rexgen_def.h"

unsigned char Seq;

static int usb_send_cmd(const struct rexgen_usb *dev, void *cmd, int len)
{
    int actual_len;

    if (!usb_bulk_msg(dev->udev,
            usb_sndbulkpipe(dev->udev, dev->bulk_out->bEndpointAddress),
            cmd, len, &actual_len, USB_TIMEOUT))
        return SUCCESS;

    printk("%s: TX error ", DeviceName);
    return USB_COMMUNICATION_ERROR;
}

static int usb_recv_cmd(const struct rexgen_usb *dev, void *cmd, int len, int *actual_len)
{
    int res;

    res = usb_bulk_msg(dev->udev,
            usb_rcvbulkpipe(dev->udev, dev->bulk_in->bEndpointAddress),
            cmd, len, actual_len, USB_TIMEOUT);

    switch(res)
    {
        case SUCCESS:
            return SUCCESS;
        case -ETIMEDOUT:
            printk("%s: RX Timeout, %i", DeviceName, res);
            break;
        default:
            printk("%s: RX error %i", DeviceName, res);
            break;
    }
    return USB_COMMUNICATION_ERROR;
}

static void build_check_sum(unsigned char* data, unsigned short count)
{
    int i;
    unsigned char* crc = data + count - 1;

    *crc = 0;
    for (i = 0; i < count - 1; i++)
        *crc += data[i];
}

static void build_cmd(struct rexgen_cmd *cmd, const cmd_struct *cmdstruct)
{
    unsigned short len = cmdstruct->tx_len + 4;
    cmd->tx_data[0] = Seq++;
    memcpy(&cmd->tx_data[1], &len, sizeof(len));
    memcpy(&cmd->tx_data[3], cmdstruct->cmd_data, cmdstruct->tx_len);
    build_check_sum((unsigned char*)cmd->tx_data, len);
    cmd->tx_len = len;
    cmd->rx_len = cmdstruct->rx_len;
}

static int send_cmd_usb(struct rexgen_usb *dev, const cmd_struct *cmdstruct)
{
    struct rexgen_cmd *cmd;
    int res, actual_len;
    
    cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
    if (!cmd)
        return -ENOMEM;

    build_cmd(cmd, cmdstruct);
    memset(&dev->usb_rx_data, 0, sizeof(dev->usb_rx_data));

    printktx(cmd);
    res = usb_send_cmd(dev, cmd->tx_data, cmd->tx_len);
    if (res)
        goto end;

    actual_len = 0;
    res = usb_recv_cmd(dev, cmd->rx_data, cmd->rx_len, &actual_len);
    if (res)
        goto end;

    printkrx(cmd);
    memcpy(&dev->usb_rx_data, &cmd->rx_data, cmd->rx_len);
    dev->usb_rx_len = cmd->rx_len;
end:
    kfree(cmd);
    return res;
}

int usb_get_firmware(struct rexgen_usb *dev)
{
    int res;
    
    res = send_cmd_usb(dev, &cmdGetFwVersion);
    if (!res)
    {
        dev->fw_ver[0] = (dev->usb_rx_data[6] << 8) + dev->usb_rx_data[5];
        dev->fw_ver[1] = dev->usb_rx_data[7];
        dev->fw_ver[2] = dev->usb_rx_data[8];
        dev->fw_ver[3] = dev->usb_rx_data[9];

        // checking firmware version, supports the socket CAN
        if (dev->fw_ver[0] < SUPP_GET_NUM_CHANNELS_MAJOR)
            return NOT_SUPPORTED;
        else if (dev->fw_ver[0] == SUPP_GET_NUM_CHANNELS_MAJOR)
        {	
    	    if (dev->fw_ver[1] < SUPP_GET_NUM_CHANNELS_MINOR)
                return NOT_SUPPORTED;
    	    else if (dev->fw_ver[1] == SUPP_GET_NUM_CHANNELS_MINOR)
            {
                if (dev->fw_ver[2] < SUPP_GET_NUM_CHANNELS_PATCH)
                    return NOT_SUPPORTED;
    	    }
        }
    }
    return res;
}

int usb_get_num_channels(struct rexgen_usb *dev)
{
    int res;
    dev->nchannels = CAN_CHANNELS; 
    
    res = send_cmd_usb(dev, &cmdCANBusCount);
    if (!res)
        dev->nchannels = dev->usb_rx_data[5];

    return res;
}

int usb_start_live_data(struct rexgen_usb *dev)
{
    return send_cmd_usb(dev, &cmmdUSBStartLiveData);
}

int usb_stop_live_data(struct rexgen_usb *dev)
{
    return send_cmd_usb(dev, &cmmdUSBStopLiveData);
}

int usb_can_intf_enable(struct rexgen_usb *dev)
{
    int res, i, j;

    res = send_cmd_usb(dev, &cmdCANIntfEnable);
    if (res)
    	return res;

    // load channels UID
    for (i = 0; i < dev->nchannels; i++)
    {
	   for (j = 0; j < 3; j++)
	   {
            cmdCANBlockUIDGet.cmd_data[2] = i;
            cmdCANBlockUIDGet.cmd_data[3] = i >> 8;
            cmdCANBlockUIDGet.cmd_data[4] = j;

            res = send_cmd_usb(dev, &cmdCANBlockUIDGet);
            if (res)
            {
                printk("%s: Cannot read CAN block UID", DeviceName);
                return res;
            }

            // returned errors
            if ((dev->usb_rx_data[5] == 0 && dev->usb_rx_data[6] == 0 && 
                dev->usb_rx_data[7] == 0 && dev->usb_rx_data[8] == 0) ||
                (dev->usb_rx_data[6] == 255 && dev->usb_rx_data[7] == 255 && dev->usb_rx_data[8] == 255))
            {
                printk("%s: Error reading block UID for channel %i and type %i", DeviceName, i, j);
                return USB_COMMUNICATION_ERROR;
    	    }

    	    dev->nets[i]->usb_block_uid[j] = (dev->usb_rx_data[6] << 8) + dev->usb_rx_data[5];
        }
    }

    return res;
}

int usb_can_intf_disable(struct rexgen_usb *dev)
{
    return send_cmd_usb(dev, &cmdCANIntfDisable);
}

int usb_set_bittiming(struct net_device *netdev)
{
    struct rexgen_net *net = netdev_priv(netdev);
    struct can_bittiming *bt = &net->can.bittiming;
    struct rexgen_usb *dev = net->dev;

    if (USB_CMD_DEBUG)
    {
        printk("%s: CAN bittiming", DeviceName);
        printk("%s:      bitrate - %i", DeviceName, bt->bitrate);
        printk("%s: sample_point - %i", DeviceName, bt->sample_point);
        printk("%s:           tq - %i", DeviceName, bt->tq);
        printk("%s:     prop_seg - %i", DeviceName, bt->prop_seg);
        printk("%s:   phase_seg1 - %i", DeviceName, bt->phase_seg1);
        printk("%s:   phase_seg2 - %i", DeviceName, bt->phase_seg2);
        printk("%s:          sjw - %i", DeviceName, bt->sjw);
        printk("%s:          brp - %i", DeviceName, bt->brp);
    }

    cmdCANParamSet.cmd_data[2] = net->channel; 
    cmdCANParamSet.cmd_data[3] = net->channel >> 8;
    cmdCANParamSet.cmd_data[4] = bt->bitrate; 
    cmdCANParamSet.cmd_data[5] = bt->bitrate >> 8; 
    cmdCANParamSet.cmd_data[6] = bt->bitrate >> 16;
    cmdCANParamSet.cmd_data[7] = bt->bitrate >> 24;
    cmdCANParamSet.cmd_data[8] = bt->prop_seg + bt->phase_seg1;   
    cmdCANParamSet.cmd_data[9] = bt->phase_seg2;
    cmdCANParamSet.cmd_data[10] = bt->sjw;      
    cmdCANParamSet.cmd_data[11] = bt->brp; 
  
    return send_cmd_usb(dev, &cmdCANParamSet);
}

int usb_set_data_bittiming(struct net_device *netdev)
{
    struct rexgen_net *net = netdev_priv(netdev);
    struct can_bittiming *bt = &net->can.data_bittiming;
    struct rexgen_usb *dev = net->dev;

    if (USB_CMD_DEBUG)
    {
        printk("%s: CANFD bittiming", DeviceName);
        printk("%s:      bitrate - %i", DeviceName, bt->bitrate);
        printk("%s: sample_point - %i", DeviceName, bt->sample_point);
        printk("%s:           tq - %i", DeviceName, bt->tq);
        printk("%s:     prop_seg - %i", DeviceName, bt->prop_seg);
        printk("%s:   phase_seg1 - %i", DeviceName, bt->phase_seg1);
        printk("%s:   phase_seg2 - %i", DeviceName, bt->phase_seg2);
        printk("%s:          sjw - %i", DeviceName, bt->sjw);
        printk("%s:          brp - %i", DeviceName, bt->brp);
    }

    cmdCANDataParamSet.cmd_data[2] = net->channel; 
    cmdCANDataParamSet.cmd_data[3] = net->channel >> 8;
    cmdCANDataParamSet.cmd_data[4] = bt->bitrate; 
    cmdCANDataParamSet.cmd_data[5] = bt->bitrate >> 8; 
    cmdCANDataParamSet.cmd_data[6] = bt->bitrate >> 16;
    cmdCANDataParamSet.cmd_data[7] = bt->bitrate >> 24;
    cmdCANDataParamSet.cmd_data[8] = bt->prop_seg + bt->phase_seg1;   
    cmdCANDataParamSet.cmd_data[9] = bt->phase_seg2;
    cmdCANDataParamSet.cmd_data[10] = bt->sjw;      
    cmdCANDataParamSet.cmd_data[11] = bt->brp; 
  
    return send_cmd_usb(dev, &cmdCANDataParamSet);
}

int usb_can_bus_open(struct rexgen_usb *dev, unsigned short channel, unsigned char flags)
{
    int res;

    cmdCANBusOpen.cmd_data[2] = channel;
    cmdCANBusOpen.cmd_data[3] = channel >> 8;
    cmdCANBusOpen.cmd_data[4] = flags;
    
    res = send_cmd_usb(dev, &cmdCANBusOpen);
    if (res)
    {
       printk("%s: Can not open channel %i", DeviceName, channel);
       return res;
    }
    else
    {
        printk("%s: Channel %i is opened", DeviceName, channel);
        return SUCCESS;
    }
}

int usb_can_bus_close(struct rexgen_usb *dev, unsigned short channel)
{
    int res;

    cmdCANBusClose.cmd_data[2] = channel;
    cmdCANBusClose.cmd_data[3] = channel >> 8;
    
    res = send_cmd_usb(dev, &cmdCANBusClose);
    if (res)
    {
       printk("%s: Can not close channel %i", DeviceName, channel);
       return res;
    }
    else
    {
        printk("%s: Channel %i is closed", DeviceName, channel);
        return SUCCESS;
    }
}

int usb_can_bus_on(struct rexgen_usb *dev, unsigned short channel)
{
    int res;

    memcpy(&(cmdCANBusOn.cmd_data[2]), &channel, 2);
    cmdCANBusOn.cmd_data[2] = channel;
    cmdCANBusOn.cmd_data[3] = channel >> 8;

    res = send_cmd_usb(dev, &cmdCANBusOn);
    if (res)
    {       
       printk("%s: Can not start channel %i", DeviceName, channel);
       return res;
    }
    else
    {
        printk("%s: Channel %i is turned on", DeviceName, channel);
        return SUCCESS;
    }
}

int usb_can_bus_off(struct rexgen_usb *dev, unsigned short channel)
{
    int res;

    memcpy(&(cmdCANBusOff.cmd_data[2]), &channel, 2);
    cmdCANBusOff.cmd_data[2] = channel;
    cmdCANBusOff.cmd_data[3] = channel >> 8;

    res = send_cmd_usb(dev, &cmdCANBusOff);
    if (res)
    {       
       printk("%s: Can not stop channel %i", DeviceName, channel);
       return res;
    }
    else
    {
        printk("%s: Channel %i is turned off", DeviceName, channel);
        return SUCCESS;
    }
}

void can2socket(struct rexgen_usb *dev, usb_record *rec)
{
    int channel;
    unsigned int timestamp;
    unsigned int canid;
    unsigned char flags;

    struct rexgen_net *net;
    struct can_frame *cf;
    struct canfd_frame *cfdf;
    struct sk_buff *skb;
    struct net_device_stats *stats;
    void *dataptr;
    unsigned char *canlen;

    channel = rec->uid - 100;
    timestamp = *(unsigned int*)(rec->inf);
    canid = *(unsigned int*)(rec->inf + 4);
    flags = *(unsigned char*)(rec->inf + 8);

    net = dev->nets[channel];
    stats = &net->netdev->stats;
    struct can_priv *priv = netdev_priv(net->netdev);

    if (flags & DataFrame_DIR)
    {
        skb = priv->echo_skb[0];
        if (flags & DataFrame_EDL)
            cfdf = (struct canfd_frame*)skb->data;
        else
            cf = (struct can_frame*)skb->data;
    }
    else
    {
        if (flags & DataFrame_EDL)
            skb = alloc_canfd_skb(net->netdev, &cfdf);
        else
            skb = alloc_can_skb(net->netdev, &cf);
    }

    if (!(flags & DataFrame_DIR))
    {
        if (!skb) {
            stats->rx_dropped++;
            return;
        }
    }

    if (flags & DataFrame_IDE)
        canid |= CAN_EFF_FLAG;
    if (flags & DataFrame_SRR)
        canid |= CAN_RTR_FLAG;

    if (flags & DataFrame_EDL)
    {
        cfdf->can_id = canid;
        dataptr = cfdf->data;
        canlen = &cfdf->len;
        if (flags & DataFrame_BRS)
            cfdf->flags |= 0x01;
    }
    else
    {
        cf->can_id = canid;
        dataptr = cf->data;
        #if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
            canlen = &cf->len;
        #else
            canlen = &cf->can_dlc;
        #endif
    }

    *canlen = rec->dlc;
    memcpy(dataptr, &(rec->data[0]), rec->dlc);

    if (flags & DataFrame_DIR)
    {
        can_get_echo_skb(net->netdev, 0);
        can_free_echo_skb(net->netdev, 0);
    }
    else
    {
        stats->rx_bytes += rec->dlc;
        stats->rx_packets++;

        netif_rx(skb);
    }
}

unsigned short livedata_size(void *buff, int len)
{
    if (len < 2)
        return 0;

    return *(unsigned short*)buff + 2;
}

int ptr2rec(usb_record *rec, void *buff)
{
    rec->uid = *(unsigned short*)(buff);
    rec->infsize = MIN(*(unsigned char*)(buff + 2), RexRecordMaxInfLength);
    rec->dlc = MIN(*(unsigned char*)(buff + 3), RexRecordMaxCanLength);
    memcpy(rec->inf, buff + 4, rec->infsize);
    memcpy(rec->data, buff + 4 + rec->infsize, rec->dlc);

    return 4 + rec->infsize + rec->dlc;
}
