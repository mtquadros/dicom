#!/bin/bash

echo "Montando o projeto..."
   cmake -G Ninja -S . -B build

echo "Gerando o programa executável..."
   cmake --build build