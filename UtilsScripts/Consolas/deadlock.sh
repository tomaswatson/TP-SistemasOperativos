#!/bin/bash
cd /home/utnso/tp-2023-1c-The-Kernel-Crew/consola/Debug

LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/DEADLOCK_1 &
(sleep 0.4
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/DEADLOCK_2) &
(sleep 0.8
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/DEADLOCK_3) &
(sleep 20
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/DEADLOCK_4)