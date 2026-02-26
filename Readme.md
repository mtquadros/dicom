
#### Instalação das ferramentas para geração do programa executável no sistema ubuntu
1. Instalar compilador GNU gcc
   sudo apt install build-essential -y
2. Instalar CMAKE
   sudo apt install cmake -y
3. Instalar Ninja build
   sudo apt install ninja-build -y
##### Digite na linha de comando
1. cmake -G Ninja -S . -B build
2. cmake --build build
3. /read_dicom
Abra o arquivo "anonymized_mamo.dcm" na pasta dicom