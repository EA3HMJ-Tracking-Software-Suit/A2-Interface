@if "%1"=="" goto bad
esptool.exe --chip esp32s2 --port %1 --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 EncoderA2.ino.bootloader.bin 0x8000 EncoderA2.ino.partitions.bin 0xe000 boot_app0.bin 0x10000 EncoderA2.ino.bin 
@goto end
:bad
rem Usage: update <COM port ESP32 connected> like: update COM5 , (uppercase)
:end

