import_device {part}.db -package {package}
read_verilog {part}.v
optimize_rtl
optimize_gate
place
route
bitgen -bit "{part}.bit" -version 0X00 -g ucode:00000000000000000000000000000000 -info -log_file {part}.log