#!/bin/bash
#Me voy a la carpeta principal
cd ..
cd shared
cd Debug
echo "COMPILANDO SHARED"
echo "$PWD"
make all

#Me voy a la carpeta principal
cd ..
cd ..
cd consola
cd Debug
echo "COMPILANDO CONSOLA"
echo "$PWD"
make all

#Me voy a la carpeta principal
cd ..
cd ..
cd kernel
cd Debug
echo "COMPILANDO KERNEL"
echo "$PWD"
make all

#Me voy a la carpeta principal
cd ..
cd ..
cd cpu
cd Debug
echo "COMPILANDO CPU"
echo "$PWD"
make all

#Me voy a la carpeta principal
cd ..
cd ..
cd memoria
cd Debug
echo "COMPILANDO MEMORIA"
echo "$PWD"
make all

#Me voy a la carpeta principal
cd ..
cd ..
cd fileSystem
cd Debug
echo "COMPILANDO FILESYSTEM"
echo "$PWD"
make all
#Creo la carpeta fcb
cd ..
cd cfg
mkdir fcb