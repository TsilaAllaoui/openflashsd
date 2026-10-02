## OpenFlashSD - GBA Cart flasher via SDCard and ESP32

### Working GBA frontend/screens side
    - File browser
    - Flashing rom
    - Dump cartridge
    - Backup/restore save

### Working ESP32 side
    - receive PING packet
    - respond PONG to GBA

## Pictures of GBA Side
![main_menu](images/main_menu.bmp)
![file_browser](images/file_browser.bmp)
![process](images/process.bmp)
![restore_save_process](images/restore_save_process.bmp)
![dump_infos](images/dump_infos.bmp)
![backup_save_process](images/backup_save_process.bmp)
![rom_infos](images/rom_infos.bmp)
![save_process](images/save_process.bmp)


# TODO
- Make ESP32 take all the remaining command as only PING works for now
- Wire the two side for the rest of the commands