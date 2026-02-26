#!/bin/bash

echo "Atualizando sistema..."
sudo apt update

echo "Instalando gcc,g++,make e biblioteca libc6-dev..."
sudo apt install -y build-essential

echo "Instalando CMAKE..."
sudo apt install cmake -y

echo "Instalando o ninja..."
sudo apt install ninja-build -y

echo "Instalando o git..."
sudo apt install git -y

echo "Fim da execução."