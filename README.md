# Linux_HashTable_Example

A toy program that read-delete or insert objects in a hash table in kernel. User-kernel communication is implemented via netlink.

To run the program, run these commands to set up:

> make

> sudo insmod netlinkKernel.ko

> gcc netlinkUser.c -o netlinkUser

Run this a few times to play with the toy program:

> ./netlinkUser

Run these to tear down:

> sudo rmmod netlinkKernel

> make clean
