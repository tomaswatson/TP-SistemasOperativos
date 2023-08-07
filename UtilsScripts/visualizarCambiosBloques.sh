#!/bin/bash

if [$# -ne 2]; then
        echo "Please provide a byte count and file name as arguments."
          exit 1

fi

byte_count=64
filename="/home/utnso/tp-2023-1c-The-Kernel-Crew/fileSystem/cfg/bloques.dat"

watch -n 0.2 -x bash -c "xxd -b -c$byte_count $filename | cut -d' ' -f2-"