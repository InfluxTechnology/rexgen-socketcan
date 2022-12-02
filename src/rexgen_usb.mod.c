#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xabab44ce, "module_layout" },
	{ 0xacc6e2c2, "netdev_info" },
	{ 0xd0295e98, "kmalloc_caches" },
	{ 0xdd12a5e3, "register_candev" },
	{ 0x5341cb20, "skb_clone" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xc3dfb0f9, "__dev_kfree_skb_any" },
	{ 0x1fae6f75, "usb_unanchor_urb" },
	{ 0x20dbee58, "__netdev_alloc_skb" },
	{ 0x1523a22, "netif_rx" },
	{ 0x5bbe49f4, "__init_waitqueue_head" },
	{ 0xf5b58ea5, "sock_efree" },
	{ 0x5f754e5a, "memset" },
	{ 0x416dc623, "close_candev" },
	{ 0x628ee85e, "netif_tx_wake_queue" },
	{ 0xf3d0b495, "_raw_spin_unlock_irqrestore" },
	{ 0xfcdd9238, "usb_deregister" },
	{ 0x707c7497, "kfree_skb_reason" },
	{ 0x60905276, "alloc_candev_mqs" },
	{ 0x27512469, "free_candev" },
	{ 0xe30cd7d9, "usb_free_coherent" },
	{ 0x5b434143, "_dev_err" },
	{ 0x9fcfe10b, "can_change_mtu" },
	{ 0x77304b46, "_dev_info" },
	{ 0xed15a9d6, "usb_submit_urb" },
	{ 0x7d009a98, "unregister_candev" },
	{ 0xab399396, "netif_device_detach" },
	{ 0xf47de486, "usb_kill_anchored_urbs" },
	{ 0x532bf1c2, "alloc_can_skb" },
	{ 0x296695f, "refcount_warn_saturate" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x7d5e92f5, "usb_bulk_msg" },
	{ 0x77f6f183, "kmalloc_order_trace" },
	{ 0x92997ed8, "_printk" },
	{ 0x4c859384, "open_candev" },
	{ 0x39624ead, "kmem_cache_alloc_trace" },
	{ 0xde55e795, "_raw_spin_lock_irqsave" },
	{ 0x71d64480, "netdev_warn" },
	{ 0x2cfde9a2, "warn_slowpath_fmt" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x8d7b6590, "usb_register_driver" },
	{ 0x870d5a1c, "__init_swait_queue_head" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x6dcdc2ae, "consume_skb" },
	{ 0xe64df41f, "usb_alloc_coherent" },
	{ 0xb154b2f7, "skb_put" },
	{ 0x860ea50a, "devm_kmalloc" },
	{ 0x6f9a1cb1, "can_free_echo_skb" },
	{ 0x67f7a04c, "usb_free_urb" },
	{ 0xb780f79c, "usb_anchor_urb" },
	{ 0xdb96fefb, "usb_alloc_urb" },
};

MODULE_INFO(depends, "can-dev");

MODULE_ALIAS("usb:v16D0p0F14d*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "7D0A9C1340B5BB4B0368AE0");
