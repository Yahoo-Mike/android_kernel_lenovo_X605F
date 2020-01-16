#sunsiyuan@wind-mobi.com add at 20180514 begin
#huyunge@wind-mobi.com modify for user version 20180823 begin
DEFS = -DBUILD_WIND_FOR_FACTORY_DIAG=1
#huyunge@wind-mobi.com modify for user version 20180823 end
ccflags-y += $(DEFS)
#sunsiyuan@wind-mobi.com add at 20180514 end
obj-$(CONFIG_DIAG_CHAR) := diagchar.o
obj-$(CONFIG_DIAGFWD_BRIDGE_CODE) += diagfwd_bridge.o
obj-$(CONFIG_USB_QCOM_DIAG_BRIDGE) += diagfwd_hsic.o
obj-$(CONFIG_USB_QCOM_DIAG_BRIDGE) += diagfwd_smux.o
obj-$(CONFIG_MSM_MHI) += diagfwd_mhi.o
diagchar-objs := diagchar_core.o diagchar_hdlc.o diagfwd.o diagfwd_peripheral.o diagfwd_smd.o diagfwd_socket.o diag_mux.o diag_memorydevice.o diag_usb.o diagmem.o diagfwd_cntl.o diag_dci.o diag_masks.o diag_debugfs.o
#sunsiyuan@wind-mobi.com add at 20180514 begin
#huyunge@wind-mobi.com modify for user version 20180823 begin
#ifeq ($(WIND_DEF_P100_FACTORY),true)
    DEFS = -DBUILD_WIND_FOR_FACTORY_DIAG=1
	ccflags-y += $(DEFS)
	diagchar-objs += diag_wind.o
#endif
#huyunge@wind-mobi.com modify for user version 20180823 end
#sunsiyuan@wind-mobi.com add at 20180514 end

