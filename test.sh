rm disk.img
build_linux/loopless disk.img create 1000000000
build_linux/loopless disk.img create-gpt
build_linux/loopless disk.img create-part
build_linux/loopless disk.img part-info
build_linux/loopless disk.img:1 mkfs
echo 'SAY Hello World!' | build_linux/loopless disk.img:1 write /syslinux.cfg 600
build_linux/loopless disk.img:1 syslinux
build_linux/loopless disk.img:1 set-bootable true
build_linux/loopless disk.img install-mbr
