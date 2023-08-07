#!/bin/bash
cd /home/utnso/tp-2023-1c-The-Kernel-Crew/consola/Debug
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/BASE_1 &
(sleep 3
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/BASE_2) &
(sleep 3
LD_LIBRARY_PATH=/home/utnso/tp-2023-1c-The-Kernel-Crew/shared/Debug ./consola ../cfg/consola.config ../cfg/BASE_2)