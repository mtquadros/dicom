#!/bin/bash

echo "Criando a pasta build, onde estará o programa executável"
   mkdir build

echo "Montando o projeto..."
   cmake -G Ninja -S . -B build

echo "Gerando o programa executável..."
   cmake --build build