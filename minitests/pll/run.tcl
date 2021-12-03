import_device eagle_s20.db -package BG256
read_pnl pll.pnl
bitgen -bit "pll.bit" -version 0X00 -g ucode:000000000000000000000000 -info -log_file dummy.log
