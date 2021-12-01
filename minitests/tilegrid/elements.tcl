import_device eagle_s20.db -package BG256
read_verilog eagle_s20.v
optimize_rtl
optimize_gate
legalize_phy_inst
place
route
write_pnl eagle_s20.pnl
bitgen -bit "eagle_s20.bit" -version 0X00 -g ucode:00000000000000000000000000000000 -info -log_file eagle_s20.log
