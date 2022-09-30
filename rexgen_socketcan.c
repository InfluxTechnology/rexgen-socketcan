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

MODULE_DEVICE_TABLE (usb, influx_usb_table);

void printkBuffer(void *data, int len, char* prefix)
{
    if (USB_CMD_DEBUG != 1)
        return;

    char buff[255];
    char* ptr = &buff[0];
    int i, c = 0;
    for (i = 0; i < len; i++)
    {
        ptr += sprintf(ptr, "%02X ", *((unsigned char*)(data + i)));
        if (ptr - &buff[0] > 200)
        {
            c++;
            *ptr = 0;
            printk("%s: %s (%i) - %s", DeviceName, prefix, c, buff);
            ptr = &buff[0];
        }
    }
    *ptr = 0;

    if (c > 0)
        printk("%s: %s (%i) - %s", DeviceName, prefix, c+1, buff);
    else
        printk("%s: %s - %s", DeviceName, prefix, buff);
}

void printkrx(struct rexgen_cmd* cmd)
{
    printkBuffer(cmd->rx_data, cmd->rx_len, "RX data");
}

void printkrxusb(struct rexgen_usb *dev)
{
    printkBuffer(dev->usb_rx_data, dev->usb_rx_len, "RX data");
}

void printktx(struct rexgen_cmd * cmd)
{
    printkBuffer(cmd->tx_data, cmd->tx_len, "TX data");
}

void printkLiveData(void *buff, int len)
{
    if (USB_CMD_DEBUG != 1)
        return;

    unsigned short blockSize = 0;
    unsigned short pos = 2;

    if (len < 2)
        return;
    
    blockSize =  ((unsigned short*)buff)[0];
    printk("%s: LiveData block size is %i, received %i bytes total", DeviceName, blockSize, len);

    while (pos < blockSize)
    {
        unsigned short uid = *(unsigned short*)(buff + pos);
        unsigned char rSize = *(unsigned char*)(buff + pos + 2);
        unsigned char dlc = *(unsigned char*)(buff + pos + 3);
        unsigned int timestamp = *(unsigned int*)(buff + pos + 4);

        printk("%s: UID - %i, InfSize - %i, DLC - %i, Timestamp - %i\n", DeviceName, uid, rSize, dlc, timestamp);

        if (uid == 100 || uid == 200)
        {
            printkBuffer(buff + 4 + rSize, dlc, "data");
        }
        pos += rSize + dlc + 4;   //skip bytes from unknown frames
    }
}

// Forward declarations
static void unlink_tx_urbs(struct rexgen_net *net);
static void remove_interfaces(struct rexgen_usb *dev);

static int setup_endpoints(struct rexgen_usb *dev)
{
    const struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    int i;

    iface_desc = dev->intf->cur_altsetting;

    for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) 
    {
        endpoint = &iface_desc->endpoint[i].desc;

        printk("%s: Endpoint 0x%02X found", DeviceName, endpoint->bEndpointAddress);
        if ((endpoint->bEndpointAddress & 0xF) == 2)
        {
            if (endpoint->bEndpointAddress & 0x80)
                dev->bulk_in = endpoint;
            else 
                dev->bulk_out = endpoint;
        }
        else if ((endpoint->bEndpointAddress & 0xF) == 3)
        {
            if (endpoint->bEndpointAddress & 0x80)
                dev->live_in = endpoint;
            else 
                dev->live_out = endpoint;
        }

        if (dev->bulk_in && dev->bulk_out && dev->live_in && dev->live_out)
	       return SUCCESS;
    }

    return -ENODEV;
}

struct sk_buff *alloc_canfd_skb(struct net_device *dev, struct canfd_frame **cfd)
{
   struct sk_buff *skb;

   skb = netdev_alloc_skb(dev, sizeof(struct can_skb_priv) + sizeof(struct canfd_frame));
   if (unlikely(!skb))
       return NULL;

   skb->protocol = htons(ETH_P_CANFD);
   skb->pkt_type = PACKET_BROADCAST;
   skb->ip_summed = CHECKSUM_UNNECESSARY;

   can_skb_reserve(skb);
   can_skb_prv(skb)->ifindex = dev->ifindex;

   *cfd = (struct canfd_frame *)skb_put(skb, sizeof(struct canfd_frame));
   memset(*cfd, 0, sizeof(struct canfd_frame));

   return skb;
}

static void read_bulk_callback(struct urb *urb)
{
    struct rexgen_usb *dev = urb->context;
    int err;
    unsigned int i, live_size;
    unsigned short pos;
    usb_record rec;
    void* usb_buff;

    switch (urb->status) {
    case 0:
        break;
    case -ENOENT:
    case -EPIPE:
    case -EPROTO:
    case -ESHUTDOWN:
        return;
    default:
        dev_info(&dev->intf->dev, "Rx URB aborted (%d)\n", urb->status);
        goto resubmit_urb;
    }

    usb_buff = urb->transfer_buffer;
    while (usb_buff < urb->transfer_buffer + urb->actual_length)
    {
        live_size = livedata_size(usb_buff, urb->transfer_buffer + urb->actual_length - usb_buff);
        if (live_size > urb->transfer_buffer + urb->actual_length - usb_buff)
            break;

        //printk("ReXgen: LiveData block size is %i, actual length is %i", live_size, urb->actual_length);
        memcpy(&dev->usb_rx_data, usb_buff, live_size);
        dev->usb_rx_len = live_size;

        pos = 2;
        while (pos < live_size)
        {
            pos+= ptr2rec(&rec, usb_buff + pos);
            if (rec.uid >= 100 && rec.uid < 100 + dev->nchannels)
                can2socket(dev, &rec);
        }
        usb_buff += 512;
    }

resubmit_urb:
    usb_fill_bulk_urb(urb, dev->udev,
            usb_rcvbulkpipe(dev->udev, dev->live_in->bEndpointAddress),
            urb->transfer_buffer, USB_RX_BUFFER_SIZE,
            read_bulk_callback, dev);

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err == -ENODEV) {
        for (i = 0; i < dev->nchannels; i++) {
            if (!dev->nets[i])
                continue;

            netif_device_detach(dev->nets[i]->netdev);
        }
    } else if (err) {
        dev_err(&dev->intf->dev,
            "Failed resubmitting read bulk urb: %d\n", err);
    }
}

static int setup_rx_urbs(struct rexgen_usb *dev)
{
    int i, err = 0;

    if (dev->rxinitdone)
	return 0;

    for (i = 0; i < USB_MAX_RX_URBS; i++) {
	   struct urb *urb = NULL;
	   u8 *buf = NULL;
	   dma_addr_t buf_dma;

	   urb = usb_alloc_urb(0, GFP_KERNEL);
	   if (!urb) {
	       err = -ENOMEM;
	       break;
	   }

	   buf = usb_alloc_coherent(dev->udev, USB_RX_BUFFER_SIZE, GFP_KERNEL, &buf_dma);
	   if (!buf) {
	       printk("No memory left for USB buffer");
	       usb_free_urb(urb);
	       err = -ENOMEM;
	       break;
	   }

       printk("%s: Setup live rx urb", DeviceName);
	   usb_fill_bulk_urb(urb, dev->udev, usb_rcvbulkpipe
		    (dev->udev, dev->live_in->bEndpointAddress),
		      buf, USB_RX_BUFFER_SIZE, read_bulk_callback, dev);
	   urb->transfer_dma = buf_dma;
	   urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	   usb_anchor_urb(urb, &dev->rx_submitted);

	   err = usb_submit_urb(urb, GFP_KERNEL);
	   if (err) {
	       usb_unanchor_urb(urb);
	       usb_free_coherent(dev->udev, USB_RX_BUFFER_SIZE, buf, buf_dma);
	       usb_free_urb(urb);
	       break;
	   }

	   dev->rxbuf[i] = buf;
	   dev->rxbuf_dma[i] = buf_dma;

	   usb_free_urb(urb);
    }
    if (i == 0) {
	   printk("Cannot setup read URBs, error %d\n", err);
	   return err;
    } 
    else if (i < USB_MAX_RX_URBS) {
	   printk("RX performances may be slow");
    }

    dev->rxinitdone = true;

    return 0;
}

static void write_bulk_callback(struct urb *urb)
{
    struct usb_tx_context *context = urb->context;
    struct rexgen_net *net;
    struct net_device *netdev;

    if (WARN_ON(!context))
        return;

    net = context->net;
    netdev = net->netdev;

    kfree(urb->transfer_buffer);

    if (!netif_device_present(netdev))
    {
        return;
    }

    if (urb->status)
        netdev_info(netdev, "Tx URB aborted (%d)\n", urb->status);
}

static netdev_tx_t on_xmit(struct sk_buff *skb, struct net_device *netdev)
{
    struct rexgen_net *net = netdev_priv(netdev);
    struct rexgen_usb *dev = net->dev;
    struct net_device_stats *stats = &netdev->stats;
    struct usb_tx_context *context = NULL;
    struct urb *urb;
    void *buf;
    int cmd_len = 0;
    int err, ret = NETDEV_TX_OK;
    unsigned int i;
    unsigned long flags;
    struct can_frame *cf = (struct can_frame *)skb->data;
    unsigned char *canlen;

    if (can_dropped_invalid_skb(netdev, skb))
        return NETDEV_TX_OK;

    urb = usb_alloc_urb(0, GFP_ATOMIC);
    if (!urb) {
        stats->tx_dropped++;
        dev_kfree_skb(skb);
        return NETDEV_TX_OK;
    }

    spin_lock_irqsave(&net->tx_contexts_lock, flags);
    for (i = 0; i < USB_MAX_TX_URBS; i++) {
        if (net->tx_contexts[i].echo_index == USB_MAX_TX_URBS) {
            context = &net->tx_contexts[i];

            context->echo_index = i;
            ++net->active_tx_contexts;
            if (net->active_tx_contexts >= (int)USB_MAX_TX_URBS)
                netif_stop_queue(netdev);

            break;
        }
    }
    spin_unlock_irqrestore(&net->tx_contexts_lock, flags);

    // * This should never happen; it implies a flow control bug * /
    if (!context) {
        netdev_warn(netdev, "cannot find free context\n");

        ret = NETDEV_TX_BUSY;
        goto freeurb;
    }

    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
        canlen = &cf->len;
    #else
        canlen = &cf->can_dlc;
    #endif

    cmd_len = 4 + 9 + *canlen;
    buf = kmalloc(512, GFP_ATOMIC);
    if (buf) {
        // Header
        *((unsigned short*)(buf + 0)) = 1200 + net->channel;
        *((unsigned char*)(buf + 2)) = 9;
        *((unsigned char*)(buf + 3)) = *canlen;
        // Inf data
        *((unsigned long*)(buf + 4)) = 0;
        *((unsigned long*)(buf + 8)) = cf->can_id;
        *((unsigned char*)(buf + 12)) = 0;
        // Can data
        memcpy(buf + 13, cf->data, *canlen);
    }
    if (!buf) {
        stats->tx_dropped++;
        dev_kfree_skb(skb);
        spin_lock_irqsave(&net->tx_contexts_lock, flags);

        context->echo_index = USB_MAX_TX_URBS;
        --net->active_tx_contexts;
        netif_wake_queue(netdev);

        spin_unlock_irqrestore(&net->tx_contexts_lock, flags);
        goto freeurb;
    }

    context->net = net;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0))
    can_put_echo_skb(skb, netdev, context->echo_index, 0);
#else
    can_put_echo_skb(skb, netdev, context->echo_index);
#endif
    //printkBuffer(buf, cmd_len, "Socket TX");

    usb_fill_bulk_urb(urb, dev->udev,
              usb_sndbulkpipe(dev->udev, dev->live_out->bEndpointAddress),
              buf, cmd_len, write_bulk_callback, context);
    usb_anchor_urb(urb, &net->tx_submitted);

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (unlikely(err)) 
    {
        spin_lock_irqsave(&net->tx_contexts_lock, flags);

        #if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
            can_free_echo_skb(netdev, context->echo_index, NULL);
        #else
            can_free_echo_skb(netdev, context->echo_index);
        #endif
        context->echo_index = USB_MAX_TX_URBS;
        --net->active_tx_contexts;
        netif_wake_queue(netdev);

        spin_unlock_irqrestore(&net->tx_contexts_lock, flags);

        usb_unanchor_urb(urb);
        kfree(buf);

        stats->tx_dropped++;

        if (err == -ENODEV)
            netif_device_detach(netdev);
        else
            netdev_warn(netdev, "Failed tx_urb %d\n", err);

        goto freeurb;
    }

    ret = NETDEV_TX_OK;

freeurb:
    usb_free_urb(urb);
    return ret;
}

static int on_open(struct net_device *netdev)
{
    struct rexgen_net *net = netdev_priv(netdev);
    struct rexgen_usb *dev = net->dev;
    int err;
    unsigned short channel = net->channel;

    err = open_candev(netdev);
    if (err)
    {
        printk("%s: open_candev error %i", DeviceName, err);
        goto error;
    }

    err = usb_get_firmware(dev);
    if (err)
    {
       if (err == NOT_SUPPORTED)
           printk("%s: Firmware not supports socket CAN", DeviceName);
       else
           printk("%s: Cannot get firmware", DeviceName);
       return err;
    }
    else
    {
        printk("%s: Firmware %i.%i.%i.%i", DeviceName, dev->fw_ver[0], dev->fw_ver[1], dev->fw_ver[2], dev->fw_ver[3]);
    }

    err = usb_can_bus_open(dev, channel);
    if (err)
    {
        printk("%s: Cannot open channel %i, error %i", DeviceName, net->channel, err);
        goto error;
    }

    err = usb_can_bus_on(dev, channel);
    if (err)
    {
        printk("%s: Cannot turn on channel %i, error %i", DeviceName, net->channel, err);
        goto error;
    }

    err = setup_rx_urbs(dev);
    if (err)
    {
        printk("%s: Cannot setup rx urbs. Error %i", DeviceName, err);
        goto error;
    }

    net->can.state = CAN_STATE_ERROR_ACTIVE;

    return 0;

error:
    close_candev(netdev);
    return err;
}

static int on_close(struct net_device *netdev)
{
    printk("%s: Usb closing...", DeviceName);
    struct rexgen_net *net = netdev_priv(netdev);
 
    netif_stop_queue(netdev);
    unlink_tx_urbs(net);
    net->can.state = CAN_STATE_STOPPED;
    close_candev(net->netdev);
    printk("%s: Usb closed!", DeviceName);
    return 0;
}


static void reset_tx_urb_contexts(struct rexgen_net *net)
{
    int i;

    net->active_tx_contexts = 0;
    for (i = 0; i < USB_MAX_TX_URBS; i++)
        net->tx_contexts[i].echo_index = USB_MAX_TX_URBS;
}

static const struct net_device_ops rex_ops = {
    .ndo_open = on_open,
    .ndo_stop = on_close,
    .ndo_start_xmit = on_xmit,
    .ndo_change_mtu = can_change_mtu,
};

static int get_berr_counter(const struct net_device *netdev, struct can_berr_counter *bec)
{
    struct rexgen_net *net = netdev_priv(netdev);
    *bec = net->bec;
    return 0;
}

static int init_interface(struct rexgen_usb *dev, const struct usb_device_id *id, int channel)
{
    struct net_device *netdev;
    struct rexgen_net *net;
    int err;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0))
    netdev = alloc_candev(struct_size(net, tx_contexts, USB_MAX_TX_URBS), USB_MAX_TX_URBS);
#else
    netdev = alloc_candev(sizeof(*net) + USB_MAX_TX_URBS * sizeof(*net->tx_contexts), USB_MAX_TX_URBS);
#endif

    if (!netdev) {
	   printk("%s: Cannot alloc candev", DeviceName);
	   return -ENOMEM;
    }

    net = netdev_priv(netdev);

    init_usb_anchor(&net->tx_submitted);
    init_completion(&net->start_comp);
    init_completion(&net->stop_comp);
    net->can.ctrlmode_supported = 0;

    net->dev = dev;
    net->netdev = netdev;
    net->channel = channel;

    spin_lock_init(&net->tx_contexts_lock);
    reset_tx_urb_contexts(net);
    
    net->can.state = CAN_STATE_STOPPED;
    net->can.clock.freq = rex_usb_cfg.clock.freq;
    net->can.bittiming_const = rex_usb_cfg.bittiming_const;
    net->can.do_set_bittiming = usb_set_bittiming;
    net->can.do_get_berr_counter = get_berr_counter;

    netdev->flags |= IFF_ECHO;	
    netdev->netdev_ops = &rex_ops;

    SET_NETDEV_DEV(netdev, &dev->intf->dev);
    netdev->dev_id = channel;

    dev->nets[channel] = net;
    memset(&net->usb_block_uid, 0, sizeof(net->usb_block_uid));

    err = register_candev(netdev);
    if (err) {
	   printk("%s: Failed to register CAN device", DeviceName);
	   free_candev(netdev);
	   dev->nets[channel] = NULL;
	   return err;
    }
    else
    {
        netdev_dbg(netdev, "device registered\n");
        return 0;
    }
}

static void unlink_tx_urbs(struct rexgen_net *net)
{
    usb_kill_anchored_urbs(&net->tx_submitted);
    reset_tx_urb_contexts(net);
}

static void unlink_all_urbs(struct rexgen_usb *dev)
{
    int i;

    usb_kill_anchored_urbs(&dev->rx_submitted);

    for (i = 0; i < USB_MAX_RX_URBS; i++)
	   usb_free_coherent(dev->udev, USB_RX_BUFFER_SIZE, dev->rxbuf[i], dev->rxbuf_dma[i]);

    for (i = 0; i < dev->nchannels; i++) {
	   struct rexgen_net *net = dev->nets[i];
	   if (net)
	       unlink_tx_urbs(net);
    }
}

static void remove_interfaces(struct rexgen_usb *dev)
{
    int i;

    for (i = 0; i < dev->nchannels; i++) {
	   if (!dev->nets[i])
	       continue;

	   unregister_candev(dev->nets[i]->netdev);
    }

    unlink_all_urbs(dev);

    for (i = 0; i < dev->nchannels; i++) {
	   if (!dev->nets[i])
	       continue;

	   free_candev(dev->nets[i]->netdev);
    }
}

static int probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct rexgen_usb *dev;
    int err, i;

    dev = devm_kzalloc(&intf->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->intf = intf;
    err = setup_endpoints(dev);
    if (err)
    {
    	printk("%s: Cannot get usb endpoints", DeviceName);
    	return err;
    }
    else 
        printk("%s: Endpoints readed", DeviceName);
    
    dev->udev = interface_to_usbdev(intf);
    init_usb_anchor(&dev->rx_submitted);
    usb_set_intfdata(intf, dev);

    err = usb_get_firmware(dev);
    if (err)
    {
	   if (err == NOT_SUPPORTED)
	       printk("%s: Firmware not supports socket CAN", DeviceName);
	   else
	       printk("%s: Cannot get firmware", DeviceName);
	   return err;
    }
    else
    {
        printk("%s: Firmware %i.%i.%i.%i", DeviceName, dev->fw_ver[0], dev->fw_ver[1], dev->fw_ver[2], dev->fw_ver[3]);
    }

    err = usb_get_num_channels(dev);
    if (err) 
    {
	   printk("%s: Cannot get number of channels", DeviceName);
	   return err;
    }
    else
    {
        printk("%s: Detected %u channels", DeviceName, dev->nchannels);
    }

    for (i = 0; i < dev->nchannels; i++) 
    {
	   err = init_interface(dev, id, i);
	   if (err) 
	   {
           printk("%s: Cannot init interface", DeviceName);
	       remove_interfaces(dev);
	       return err;
	   }
    }

    err = usb_can_intf_enable(dev);
    if (err)
    {
       printk("%s: Cannot enable interface", DeviceName);
       return err;
    }
    else
    {
        printk("%s: Can interface enabled", DeviceName);
    }

    err = usb_start_live_data(dev);
    if (err)
    {
       printk("%s: Cannot start live data", DeviceName);
       return err;
    }
    else
    {
        printk("%s: Live data started", DeviceName);
    }

    return SUCCESS;
}

static void disconnect(struct usb_interface *intf)
{
    struct rexgen_usb *dev = usb_get_intfdata(intf);

    usb_set_intfdata(intf, NULL);

    if (!dev)
    	return;

    remove_interfaces(dev);    
    printk("%s: Disconnected", DeviceName);
}


static struct usb_driver rexgen_usb_driver = {
    .name = "rexgen_usb",
    .probe = probe,
    .disconnect = disconnect,
    .id_table = influx_usb_table,
};

module_usb_driver(rexgen_usb_driver);

MODULE_AUTHOR("Influx Technology LTD <support@influxtechnology.com>");
MODULE_DESCRIPTION("CAN driver for RexGen CAN/USB devices");
MODULE_LICENSE("GPL v2");


