import_device {part}.db -package {package}
read_pnl wire.pnl
bitgen -bit "wire.bit" -version 0X00 -g ucode:00000000000000000000000000000000 -info -log_file wire.log
