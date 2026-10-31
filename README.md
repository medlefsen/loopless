loopless
========

A small CLI utility and Ruby library for creating and editing bootable disk images without loopback mounts or root.

Install
-------

Download the binary for the latest release:

    curl -fsSLO https://github.com/medlefsen/loopless/releases/latest/download/loopless-x86_64-linux
    chmod +x loopless-x86_64-linux
    sudo install -m 0755 loopless-x86_64-linux /usr/local/bin/loopless

Releases are listed at https://github.com/medlefsen/loopless/releases, and each
one also carries a tarball with this README and the license text.

Builds are produced in a `debian:bookworm` container, so the binary needs glibc
2.36 or newer (Debian 12, Ubuntu 22.04, Fedora 37 and later).

Building from source
--------------------

The build fetches and compiles pinned copies of lwext4, Syslinux, gptfdisk and
e2fsprogs' libuuid, so it is done inside a container to keep it reproducible:

    ./build.sh      # builds build_linux/loopless
    ./test.sh       # smoke test against a scratch disk image
    ./package.sh    # produces release artifacts in dist/

Set `CONTAINER_ENGINE=docker` if you would rather not use podman. To build
against the host toolchain instead, install the dependencies listed in
`install-build-deps.sh` and run `./do-build.sh build_linux` directly.

Ruby library
------------

The `ruby/` directory holds a Ruby binding that shells out to the binary. The
`loopless` gem on rubygems.org is no longer updated; released versions are
published as binaries on GitHub instead.

Synopsis
--------

Create a 1GB bootable disk image for use with VirtualBox. This will produce a 1GB disk.img image file and a small disk.vmdk image description file.

On the command line

    loopless disk.img create 1000000000
    loopless disk.img create-gpt
    loopless disk.img create-part
    loopless disk.img part-info
    loopless disk.img:1 mkfs 
    echo 'SAY Hello World!' | loopless disk.img:1 write /syslinux.cfg 600
    loopless disk.img:1 syslinux
    loopless disk.img:1 ls
    loopless disk.img:1 set-bootable true
    loopless disk.img install-mbr
    loopless disk.img create-vmdk disk.vmdk

Or in Ruby

    require 'loopless'
    Loopless.create("disk.img", 1000000000)
    Loopless.create_gpt("disk.img")
    Loopless.create_part("disk.img")
    p Loopless.part_info("disk.img")
    Loopless.mkfs("disk.img", 1)
    Loopless.write("disk.img", 1, '/syslinux.cfg', '600', "SAY Hello World!" )
    Loopless.syslinux("disk.img", 1)
    p Loopless.ls("disk.img",1)
    Loopless.set_bootable("disk.img",1, true)
    Loopless.install_mbr("disk.img")
    Loopless.create_vmdk("disk.img", 'disk.vmdk')

Usage
-----

loopless is comprised of a number of subcommands:

### Partitioning

* `loopless FILE create-gpt`

 Writes a blank GPT partition table to the disk image.

* `loopless FILE create-part [ PARTNUM  [ START  [ SIZE [ LABEL ] ] ] ]`

 Creates a partition on the image. The partition number will default to the next
 available, start will default to the next free sector after the last partition, and size
 will default to the remaining size on the disk. Start and size must be provided in units
 of 512 byte sectors.

* `loopless FILE part-info`

 Lists the partitions on the image according to the format: "PARTNUM\tSTART\tSIZE\tBOOTABLE\tLABEL\n"

* `loopless FILE:PARTNUM set-bootable true/false`

 Sets or unsets the legacy_boot GPT flag on the specified partition. This flag is used by syslinux
 to determine the boot partition.

### Ext4

PART may either be a single number indicating GPT part number or a comma separated byte range

PATH must be the absolute path to file, including the initial '/'

* `loopless FILE[:PART] mkfs [ LABEL ]`

 Creates a new ext4 filesystem on the specified partition (or whole image if no partition specified).

* `loopless FILE[:PART] ls`

 List all files in filesystem

* `loopless FILE[:PART] rm PATH`

 Delete file and recursively remove empty directories.

* `loopless FILE[:PART] read PATH`

 Print file to stdout

* `loopless FILE[:PART] write PATH MODE`

 Overwrites file with content from stdin. Will also chmod the file to the given MODE and
 recursively chmod parent directories with MODE + the executable bit set where the read bit is
 (e.g. 640 would become 750 for parent directories ).

### Syslinux

* `loopless FILE:PART syslinux`

 Sets up partition to boot with syslinux. Partition must be ext2/3/4 and should have
 a syslinux.cfg file to read from on boot.

* `loopless FILE install-mbr`

 Installs the syslinux gptmbr to the beginning of the file. This is necessary to boot.

### Utility

* `loopless FILE create SIZE`

 Creates or overwrites file and truncates it to SIZE bytes.

* `loopless FILE create-vmdk VMDK_FILE`

 Creates a vmdk file for the disk image. This can be used to use the disk
 with VM hypervisors that don't support raw images like VirtualBox. The created file
 is only a few hundred bytes and just contains a reference to the actual disk image.
 It is generated with a random image id number that is used to identify the image so if
 regenerated it must be removed and re-added to VirtualBox.


Acknowledgements
-----------------

This software is licensed under the GPL version 2 (See COPYING for full license)
and includes or depends upon the copyrighted work of the following projects and authors:

loopless - http://github.com/medlefsen/loopless :

* Matt Edlefsen <matt.edlefsen@gmail.com>

gptfdisk - https://sourceforge.net/projects/gptfdisk/:

* Roderick W. Smith <rodsmith@rodsbooks.com>

e2fsprogs libuuid - http://e2fsprogs.sourceforge.net/:

* Theodore Ts'o <tytso@mit.edu>

lwext4 - https://github.com/gkostka/lwext4:

* HelenOS Authors - helenos.org
* Grzegorz Kostka <kostka.grzegorz@gmail.com>
* KaHo Ng - https://github.com/ngkaho1234

Syslinux - http://www.syslinux.org/:

* H. Pater Anvin <hpa@zytor.com>
