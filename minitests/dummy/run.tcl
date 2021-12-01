import_device eagle_s20.db -package BG256
read_verilog dummy.v
optimize_rtl
optimize_gate
legalize_phy_inst
place
route
write_pnl dummy.pnl
bitgen -bit "dummy.bit" -version 0X00 -g ucode:000000000000000000000000 -info -log_file dummy.log
