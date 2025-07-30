# Datalogger de Movimento com Interface Local via RP2040

Um sistema embarcado para registro contínuo de movimentos, utilizando o sensor **MPU6050** com o **Raspberry Pi Pico W**, capaz de armazenar dados de aceleração, giroscópio e temperatura em formato `.csv`, além de fornecer uma interface interativa local com feedback visual e sonoro para controle completo do processo de aquisição.

---

## Funcionalidades Principais

-   **Leitura Contínua de Movimento e Temperatura**
    -   Captura dados brutos do sensor MPU6050 (aceleração, rotação e temperatura).
    -   Conversão dos dados para unidades físicas:  
        -   **Aceleração (g)**  
        -   **Velocidade angular (°/s)**  
        -   **Temperatura (°C)**

-   **Armazenamento em Cartão microSD**
    -   Dados são salvos em arquivos `.csv` com cabeçalho estruturado.
    -   Novo nome de arquivo é gerado automaticamente (`datalogX.csv`).
    -   Suporte à leitura e troca de arquivos diretamente no dispositivo.

-   **Interface Local com OLED e Joystick**
    -   Menu interativo exibido em display OLED (navegável por joystick e botões).
    -   Opções: montar cartão SD, iniciar/parar gravação, visualizar dados, selecionar arquivos e ativar BOOTSEL.

-   **Feedback Visual e Auditivo**
    -   **LED RGB** indica estado do sistema com cores distintas.
    -   **Buzzer** fornece feedback sonoro para ações como erro, sucesso, início/fim de gravação.

-   **Controle Total com Botões e Joystick**
    -   Navegação por menus sem necessidade de conexão externa.
    -   Sistema totalmente operável localmente.

---

## Componentes Utilizados

| Componente           | GPIO/Pino             | Função                                                                 |
| :------------------- | :-------------------- | :--------------------------------------------------------------------- |
| MPU6050 (I2C)        | 0 (SDA), 1 (SCL)      | Sensor IMU: Aceleração, Giroscópio, Temperatura                        |
| Display OLED SSD1306 | 14 (SDA), 15 (SCL)    | Exibe menus, dados de gravação e mensagens do sistema                  |
| Cartão microSD       | SPI padrão   | Armazenamento dos dados em `.csv`                                     |
| Joystick Analógico   | 26 (VRY), 27 (VRX)    | Controle de navegação nos menus                                        |
| Botões               | 5 (A), 6 (B), 22 (SW) | Botão A (Selecionar), B (Voltar), SW (Alternar opções)                |
| LED RGB              | 11 (R), 12 (G), 13 (B)| Indicação visual de status do sistema                                 |
| Buzzer               | 21                    | Alertas sonoros conforme ações executadas                             |

---

## Estrutura do Código

O software do MoveLogger foi escrito em C/C++ com suporte ao SDK do Raspberry Pi Pico. Ele adota uma abordagem orientada a estados com um menu local exibido no OLED.

### Arquivos principais:

-   **`main.c`**
    -   Inicializa periféricos, configura os menus, define os callbacks de botões e joystick.
    -   Gera os nomes de arquivos `.csv`, realiza leitura dos sensores e gravação no cartão SD.
    -   Os dados gerados são salvos no formato `.csv` com o seguinte cabeçalho:

```csv
amostra,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,temp
```

### Pontos Relevantes do Código:

-   **Conversão de Dados IMU:**
    -   Os dados brutos do sensor MPU6050 são convertidos em unidades físicas:
        ```c
        accel_x = accel_x_raw / 16384.0;  // g (gravidade)
        gyro_x = gyro_x_raw / 131.0;      // graus por segundo
        temp_c = temp_raw / 340.0 + 36.53; // temperatura em °C
        ```
    -   Essa conversão garante que os dados salvos no `.csv` sejam compreensíveis e prontos para análise.

-   **Gerador de Nomes de Arquivo:**
    -   O código verifica os arquivos existentes no SD e cria um novo automaticamente com nome incremental:
        ```c
        datalog0.csv, datalog1.csv, datalog2.csv, ...
        ```

-   **Interface Interativa via Menu:**
    -   Menu renderizado no OLED, com controle via joystick e botões.
    -   O estado do sistema é mantido com uma variável `enum` que representa o modo atual.

-   **Feedback Multissensorial:**
    -   LED RGB com código de cores:
        -   Verde: Pronto
        -   Vermelho: Gravando
        -   Azul piscando: Acesso ao SD
        -   Roxo piscando: Erro
    -   Buzzer:
        -   1 beep: início
        -   2 beeps: erro
        -   3 beeps: sucesso

---

## ⚙️ Instalação e Uso

1. **Pré-requisitos**
   - Clonar o repositório:
     ```bash
     git clone https://github.com/JotaPablo/datalogger.git
     cd datalogger
     ```
   - Instalar no VS Code:
     - **C/C++**
     - **Pico SDK**
     - **CMake Tools**
     - **Compilador ARM GCC**

2. **Compilação**
   - Crie a pasta de build e compile:
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```
   - Ou use o botão “Build” do VS Code com a extensão Raspberry Pi Pico.

3. **Execução**
   - Conecte o Pico segurando o botão BOOTSEL.
   - Copie o `.uf2` gerado na pasta `build` para o drive `RPI-RP2`.

---

