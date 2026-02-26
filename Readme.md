### Solução
   A solução foi escrita em c++ em um sistema operacional Ubuntu. Portanto não há garantias
   que funcione em outros sistemas sem ajustes no projeto.

   Após instalar as ferramentas de geração, deve ser gerado o executável dentro da pasta "build". Como descrito abaixo. 

### Bibliotecas utilizadas:
- libgdcm-dev
- libgdcm3.0t64
- qtbase5-dev
- qtbase5-dev-tools
- libc6-dev

### Instalação das ferramentas de geração, e geração do programa executável
#### Etapa 1: Instalação das ferramentas
1. Digite na linha de comando para instalar:
>   ./ferramentas.sh
#### Etapa 2: Clonar o repositorio
1. Escolha uma pasta (de preferencia exclusiva para este fim)
2. Clone o repositório digitando:
>   `git clone https://github.com/mtquadros/dicom.git`
#### Etapa 3: Geração do programa executável
1. Digite a linha de comando para compilar:
>   ./compilar.sh
#### Etapa 4: Executar o programa
1. Digite a linha de comando 
>   ./read_dicom
2. Abra o arquivo "anonymized_mamo.dcm" na pasta dicom (na pasta pai da pasta do programa)

### Exemplo de execução
