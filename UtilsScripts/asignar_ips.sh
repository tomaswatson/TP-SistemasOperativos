#!/bin/bash

ipmemoria="127.0.0.1"
ipcpu="127.0.0.1"
ipfs="127.0.0.1"
ipkernel="127.0.0.1"

#IPs en CPU
cd ..
cd cpu/cfg
sed -i "s/IP_MEMORIA=127.0.0.1/IP_MEMORIA=$ipmemoria/g" cpu.config
cd ..
cd ..

#IPs en FS
cd fileSystem/cfg
sed -i "s/IP_MEMORIA=127.0.0.1/IP_MEMORIA=$ipmemoria/g" fileSystem.config
cd ..
cd ..

#IPs en Kernel
cd kernel/cfg
sed -i "s/IP_MEMORIA=127.0.0.1/IP_MEMORIA=$ipmemoria/g" *.config
sed -i "s/IP_FILESYSTEM=127.0.0.1/IP_FILESYSTEM=$ipfs/g" *.config
sed -i "s/IP_CPU=127.0.0.1/IP_CPU=$ipcpu/g" *.config
cd ..
cd ..

#IPs en Consola
cd consola/cfg
sed -i "s/IP_KERNEL=127.0.0.1/IP_KERNEL=$ipkernel/g" consola.config