#!/bin/bash
cd /home/utnso/tp-2023-1c-The-Kernel-Crew/consola/Debug
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/ERROR_1 &
(sleep 0.2
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/ERROR_2) &
(sleep 0.4
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/ERROR_3) &
(sleep 0.6
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/ERROR_4)