obj-$(CONFIG_DIAG_CHAR) := diagchar.o
obj-$(CONFIG_DIAG_SDIO_PIPE) += diagfwd_sdio.o
obj-$(CONFIG_DIAGFWD_BRIDGE_CODE) += diagfwd_bridge.o
obj-$(CONFIG_DIAGFWD_BRIDGE_CODE) += diagfwd_hsic.o
obj-$(CONFIG_DIAGFWD_BRIDGE_CODE) += diagfwd_smux.o
diagchar-objs := diagchar_core.o diagchar_hdlc.o diagfwd.o diagmem.o diagfwd_cntl.o diag_dci.o diag_masks.o diag_debugfs.o mts_tty.o

# LGE_CHANGE_S : CONFIG_LGE_DM_APP
obj-$(CONFIG_LGE_DM_APP) += lg_dm_tty.o
# LGE_CHANGE_E : CONFIG_LGE_DM_APP
obj-$(CONFIG_LGE_DIAG_BYPASS) += lg_diag_bypass.o