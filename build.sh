rm *.o
unzip -o -q xio.zip
xcc -report -O3 -lquadflash xio.xn $1.c xio.c.o xio.xc.o xud_ep_funcs.s.o xud_power_sig.xc.o lib_assert.xc.o xud_ep_functions.xc.o xud_set_crc_table_addr.c.o lib_gpio.xc.o xud_get_done.c.o xud_set_dev_addr.xc.o otp_board_info.xc.o xud_glx.xc.o xud_setup_chan_override.s.o xud_io_loop.s.o xud_test_mode.xc.o xud_io_loop_call.xc.o xud_uifm_pconfig.s.o xud_client.xc.o xud_manager.xc.o xud_uifm_reg_access.s.o xud_crc5_table.s.o xud_phy_reset_user.xc.o xud_user.c.o xud_device_attach.xc.o xud_ports.xc.o -o $1.xe
xflash --boot-partition-size 1044480 --no-compression --factory-version 14.3 --upgrade 1 $1.xe -o $1.bin
rm *.o
