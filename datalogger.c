#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

#include "lib/ssd1306/ssd1306.h"
#include "lib/ssd1306/display.h"
#include "lib/led/led.h"
#include "lib/button/button.h"
#include "lib/matrix_leds/neopixel.h"
#include "lib/buzzer/buzzer.h"
#include "lib/sensors/mpu6050/mpu6050.h" // Biblioteca do MPU6050
#include "lib/sd/sd_utils.h" // Biblioteca de utilidades do SD

#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

/*================== DEFINIÇÕES DE HARDWARE ==================*/
// Configuração I2C para o MPU6050
#define I2C_PORT_MPU i2c0
#define I2C_SDA_MPU_PIN 0
#define I2C_SCL_MPU_PIN 1

// Configuração do joystick analógico
#define VRX_PIN 27  // Pino do eixo X
#define VRY_PIN 26  // Pino do eixo Y
#define ADC_MAX 4095 // Valor máximo do ADC
#define CENTRO 2047  // Valor central (repouso)
#define DEADZONE 250 // Zona morta ao redor do centro

// Pino do buzzer
#define BUZZER_PIN BUZZER_A_PIN

/*================== DEFINIÇÕES DO SISTEMA ==================*/
// Tempo de debounce para os botões (em ms)
const uint32_t delay_debounce = 200;

/*================== VARIÁVEIS GLOBAIS ==================*/
// Estrutura para controle do display OLED
ssd1306_t ssd;

// Variáveis para controle dos botões
volatile uint32_t last_time_debounce_button_a = 0;
volatile uint32_t last_time_debounce_button_b = 0;
volatile bool selecionar = false; // Flag de seleção no menu
volatile bool BUTTON_B_PRESSED = false; // Flag para indicar que o botão B foi pressionado

// Variáveis para controle de arquivos
static char filename[20] = "datalogX.csv"; // Nome do arquivo de dados
static FIL data_file;                     // Objeto do arquivo
static bool sd_card_is_mounted = false;   // Status do cartão SD

// Variáveis para captura de dados
static volatile bool is_capturing = false; // Flag de captura
static uint32_t amostra_count = 0;        // Contador de amostras

// Estados do menu principal
typedef enum {
    MODO_MONTAR_DESMONTAR,
    MODO_GRAVAR,
    MODO_LER,
    MODO_ALTERAR_ARQUIVO,
    MODO_BOOTSEL
} menu_state_t;

static menu_state_t current_state = MODO_MONTAR_DESMONTAR; // Estado inicial

//Prototipos de funções
// Funções de inicialização
void setup();

// Funções de manipulação de arquivos
void read_file(const char *filename);
void print_data_file();
void init_stop_capture();
void list_csv_files();
void selecionar_arquivo_csv();

// Funções de interface
void update_menu_from_joystick();
void update_capture_display();
void draw_menu();
void show_menu();
void beep(uint frequency, uint repeticoes, uint duration);

// Funções de sistema
static void gpio_button_handler(uint gpio, uint32_t events);

// Variáveis para controle de arquivos CSV
#define MAX_FILES 100
#define MAX_FILENAME_LEN 50

char csv_files[MAX_FILES][MAX_FILENAME_LEN];
int csv_file_count = 0;

int main() {
    // Inicializa todos os componentes do sistema
    setup();
    set_led_yellow(); // Inicializando (amarelo)
    ssd1306_fill(&ssd, false);
    draw_centered_text(&ssd, "Iniciando...", 25);
    ssd1306_send_data(&ssd);
    sleep_ms(2000); // Espera 2 segundos para mostrar a mensagem de início
    
    set_led_green();  // Sistema pronto (verde)
    beep(3000, 1, 100); // Beep de inicialização
    
    // Variáveis para dados do sensor
    int16_t aceleracao[3], gyro[3], temp;

    // Loop principal do sistema
    while (true) {
        // Verifica se está em modo de captura
        if (!is_capturing) {
            // Atualiza menu e interface
            update_menu_from_joystick();
            draw_menu();
        } else {
            // Modo de captura ativo - lê dados do sensor
            
            // 1. Ler dados brutos do MPU6050
            mpu6050_read_raw(I2C_PORT_MPU, aceleracao, gyro, &temp);
            
            // 2. Converter valores para unidades físicas
            float accel_g[3] = {
                aceleracao[0] / 16384.0f, // Conversão para g (±2g)
                aceleracao[1] / 16384.0f,
                aceleracao[2] / 16384.0f
            };
            
            float gyro_dps[3] = {
                gyro[0] / 131.0f, // Conversão para °/s (±250°/s)
                gyro[1] / 131.0f,
                gyro[2] / 131.0f
            };
            
            // Converter temperatura para Celsius
            float temp_c = (temp / 340.0f) + 36.53f;
            
            // 3. Formatar dados como linha CSV
            char buffer[100];
            int len = snprintf(buffer, sizeof(buffer),
                "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                amostra_count + 1,       // Número da amostra
                accel_g[0], accel_g[1], accel_g[2],  // Dados de aceleração
                gyro_dps[0], gyro_dps[1], gyro_dps[2], // Dados do giroscópio
                temp_c);                 // Temperatura
            
            // 4. Escrever no arquivo
            UINT bw;
            f_write(&data_file, buffer, len, &bw);
            amostra_count++;
            
            // 5. Atualizar display periodicamente
            char status[30];
            ssd1306_fill(&ssd, false);
            draw_centered_text(&ssd, "GRAVANDO...", 10);
            ssd1306_draw_string(&ssd, filename, 5, 20);
            ssd1306_send_data(&ssd);
            snprintf(status, sizeof(status), "Amostras: %lu", amostra_count);
            ssd1306_draw_string(&ssd, status, 5, 45);
            ssd1306_send_data(&ssd);
        }

        // Verifica se houve seleção no menu
        if (selecionar) {
            selecionar = false; // Reseta o flag de seleção

            switch (current_state) {
                case MODO_MONTAR_DESMONTAR:
                    // Montar/desmontar cartão SD
                    set_led_blue(); // Em processo (azul)
                    beep(3000, 1, 100); // Beep de início

                    if (sd_card_is_mounted) {
                        ssd1306_fill(&ssd, false);
                        draw_centered_text(&ssd, "DESMONTANDO", 20);
                        draw_centered_text(&ssd, "SD CARD", 30);
                        ssd1306_send_data(&ssd);
                        sleep_ms(2000); // Espera 2 segundos para mostrar a mensagem

                        if(sd_unmount() == SD_OK) {
                            ssd1306_fill(&ssd, false);
                            draw_centered_text(&ssd, "SD DESMONTADO", 30);
                            ssd1306_send_data(&ssd);
                            set_led_green(); // Pronto (verde)
                            beep(3000, 3, 100); // Beep de sucesso
                            sd_card_is_mounted = false;

                        } else {
                            ssd1306_fill(&ssd, false);
                            draw_centered_text(&ssd, "ERRO", 20);
                            draw_centered_text(&ssd, "DESMONTAR SD", 30);
                            ssd1306_send_data(&ssd);
                            set_led_magenta(); // Erro (magenta)
                            beep(2000, 2, 100); // Beep de erro
                        }
                    } else {
                        ssd1306_fill(&ssd, false);
                        draw_centered_text(&ssd, "MONTANDO", 20);
                        draw_centered_text(&ssd, "SD CARD", 30);
                        ssd1306_send_data(&ssd);
                        sleep_ms(2000); // Espera 2 segundos para mostrar a mensagem

                        if(sd_mount() == SD_OK) {
                            ssd1306_fill(&ssd, false);
                            draw_centered_text(&ssd, "SD MONTADO", 30);
                            ssd1306_send_data(&ssd);
                            set_led_green(); // Pronto (verde)
                            beep(3000, 3, 100); // Beep de sucesso
                            list_csv_files(); // Gera novo nome de arquivo
                            sd_card_is_mounted = true;

                        } else {
                            ssd1306_fill(&ssd, false);
                            draw_centered_text(&ssd, "ERRO", 20);
                            draw_centered_text(&ssd, "MONTAR SD", 30);
                            ssd1306_send_data(&ssd);
                            set_led_magenta(); // Erro (magenta)
                            beep(2000, 2, 100); // Beep de erro
                        }
                    }
                    sleep_ms(2000); // Espera 2 segundos para mostrar a mensagem
                    break;

                case MODO_GRAVAR:
                    // Iniciar/parar captura de dados
                    init_stop_capture();
                    break;
                    
                case MODO_LER:
                    // Ler arquivo de dados
                    read_file(filename);
                    break;
                
                case MODO_ALTERAR_ARQUIVO:
                    // Verifica se o cartão SD está montado
                    if (!sd_card_is_mounted) {
                        ssd1306_fill(&ssd, false);
                        draw_centered_text(&ssd, "ERRO", 20);
                        draw_centered_text(&ssd, "SD CARD", 30);
                        draw_centered_text(&ssd, "NAO MONTADO", 40);
                        ssd1306_send_data(&ssd);
                        set_led_magenta(); // Erro (magenta)
                        beep(2000, 2, 100); // Beep de erro
                        sleep_ms(2000);
                    }

                    list_csv_files(); // Lista arquivos CSV disponíveis 
                    selecionar_arquivo_csv(); // Seleciona um arquivo CSV
                    break;
                
                case MODO_BOOTSEL:
                    // Habilitar modo BOOTSEL
                    printf("\nHABILITANDO O MODO BOOTSEL\n");

                    // Se o cartão SD não estiver montado
                    if(sd_card_is_mounted){
                        sd_unmount(); // Desmonta o SD Card
                        ssd1306_fill(&ssd, false);
                        draw_centered_text(&ssd, "DESMONTANDO SSD", 30);
                        ssd1306_send_data(&ssd);
                        sleep_ms(2000);

                        ssd1306_fill(&ssd, false);
                        draw_centered_text(&ssd, "SSD DESMONTADO", 30);
                        ssd1306_send_data(&ssd);
                        sleep_ms(2000);
                    }

                    ssd1306_fill(&ssd, false);
                    draw_centered_text(&ssd, "MODO BOOTSEL", 25);
                    draw_centered_text(&ssd, "HABILITADO", 38);
                    ssd1306_send_data(&ssd);
                    reset_usb_boot(0, 0); // Reinicia em modo BOOTSEL
                    break;
            }
            selecionar = false; // Reseta o flag de seleção
        }

        // Pequena pausa entre iterações
        sleep_ms(250);
    }
}

// Função de inicialização os periféricos
void setup() {
    // Inicializa comunicação serial
    stdio_init_all();

    // Inicializa LEDs e buzzer
    init_leds();
    init_buzzer(BUZZER_PIN, 4.0f);

    // Configura ADC para joystick
    adc_init();
    adc_gpio_init(VRY_PIN);
    adc_gpio_init(VRX_PIN);

    // Inicializa display OLED
    init_display(&ssd);

    // Configura I2C para o MPU6050
    i2c_init(I2C_PORT_MPU, 400 * 1000);
    gpio_set_function(I2C_SDA_MPU_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_MPU_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_MPU_PIN);
    gpio_pull_up(I2C_SCL_MPU_PIN);

    // Inicializa MPU6050
    mpu6050_init(I2C_PORT_MPU);

    // Configura botões com interrupções
    button_init_predefined(true, true, true);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_SW, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
}

// Função para iniciar/parar a captura de dados
void init_stop_capture() {
    if (!is_capturing) {

        set_led_red(); // Gravando (vermelho)
        beep(3000, 1, 100); // Beep de início de gravação

        // Verifica se o cartão SD está montado
        if (!sd_card_is_mounted) {
            ssd1306_fill(&ssd, false);
            draw_centered_text(&ssd, "ERRO", 20);
            draw_centered_text(&ssd, "SD CARD", 30);
            draw_centered_text(&ssd, "NAO MONTADO", 40);
            ssd1306_send_data(&ssd);
            set_led_magenta(); // Erro (magenta)
            beep(2000, 2, 100); // Beep de erro
            sleep_ms(2000);
            return;
        }

        // Tenta abrir o arquivo para escrita
        FRESULT res = f_open(&data_file, filename, FA_WRITE | FA_CREATE_ALWAYS);
        if (res != FR_OK) {
            ssd1306_fill(&ssd, false);
            draw_centered_text(&ssd, "ERRO", 20);
            draw_centered_text(&ssd, "ABRIR ARQUIVO", 30);
            ssd1306_send_data(&ssd);
            set_led_magenta(); // Erro (magenta)
            beep(2000, 2, 100); // Beep de erro
            sleep_ms(2000);
            return;
        }

        // Escreve cabeçalho no arquivo CSV
        const char *header = "amostra,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,temp\n";
        UINT bw;
        f_write(&data_file, header, strlen(header), &bw);
        f_sync(&data_file);

        // Inicia captura
        is_capturing = true;
        amostra_count = 0;
        
        // Feedback visual
        ssd1306_fill(&ssd, false);
        draw_centered_text(&ssd, "GRAVANDO...", 10);
        draw_centered_text(&ssd, filename, 20);
        ssd1306_send_data(&ssd);
    } else {
        // Para a captura e fecha o arquivo
        is_capturing = false;
        f_close(&data_file);
        
        // Feedback visual
        ssd1306_fill(&ssd, false);
        draw_centered_text(&ssd, "DADOS SALVOS!", 10);
        char msg[30];
        snprintf(msg, sizeof(msg), "Amostras: %lu", amostra_count);
        ssd1306_draw_string(&ssd, msg, 5, 40);
        ssd1306_send_data(&ssd);
        set_led_green(); // Volta para pronto (verde)
        beep(3000, 3, 100); // Beep de fim de gravação

        list_csv_files(); // Atualiza lista de arquivos

        sleep_ms(2000);
    }
}

// Função para ler os arquivos csv existentes
void list_csv_files() {
    DIR dir;
    FILINFO fno;
    FRESULT fr;
    char cwdbuf[FF_LFN_BUF] = {0};
    int max_index = 0;

    csv_file_count = 0;

    // Obtém diretório atual
    fr = f_getcwd(cwdbuf, sizeof(cwdbuf));
    if (fr != FR_OK) return;

    // Abre o diretório e procura por arquivos .csv
    fr = f_findfirst(&dir, &fno, cwdbuf, "*.csv");
    while (fr == FR_OK && fno.fname[0]) {
        // Adiciona ao vetor se couber
        if (csv_file_count < MAX_FILES) {
            strncpy(csv_files[csv_file_count], fno.fname, MAX_FILENAME_LEN - 1);
            csv_files[csv_file_count][MAX_FILENAME_LEN - 1] = '\0';
            csv_file_count++;
        }

        // Se for no formato datalogN.csv, tenta extrair o número
        int num;
        if (sscanf(fno.fname, "datalog%d.csv", &num) == 1) {
            if (num > max_index)
                max_index = num;
        }

        fr = f_findnext(&dir, &fno);
    }

    f_closedir(&dir);

    // Define o nome do próximo arquivo datalogN+1.csv
    snprintf(filename, sizeof(filename), "datalog%d.csv", max_index + 1);
    printf("Próximo nome de arquivo: %s\n", filename);
}

// Função para selecionar um arquivo CSV
void selecionar_arquivo_csv() {

    if (csv_file_count == 0) {
        set_led_magenta(); // Erro (magenta)
        beep(2000, 2, 100); // Beep de erro
        ssd1306_fill(&ssd, false);
        draw_centered_text(&ssd, "SEM CSV ENCONTRADO", 25);
        ssd1306_send_data(&ssd);
        sleep_ms(2000);
        return;
    }

    int index = 0;
    bool selecionar_arquivo = false;
    bool sair_menu = false;

    while (!selecionar_arquivo && !sair_menu) {
        // Mostrar arquivo atual
        ssd1306_fill(&ssd, false);
        draw_centered_text(&ssd, "SELECIONE CSV:", 5);
        draw_centered_text(&ssd, csv_files[index], 25);
        draw_centered_text(&ssd, "A: Selecionar", 45);
        draw_centered_text(&ssd, "B: Voltar", 55);
        ssd1306_send_data(&ssd);

        // Leitura joystick
        adc_select_input(1); // eixo X
        uint16_t x_value = adc_read();

        if (x_value < 500) {
            index = (index - 1 + csv_file_count) % csv_file_count;
        } else if (x_value > 2500) {
            index = (index + 1) % csv_file_count;
        }

        // Verifica botões
        if (selecionar) {
            selecionar = false;
            strncpy(filename, csv_files[index], sizeof(filename));
            filename[sizeof(filename) - 1] = '\0';
            selecionar_arquivo = true;
        } else if (BUTTON_B_PRESSED == true) {
            BUTTON_B_PRESSED = false; // Reseta o flag de botão B pressionado
            sair_menu = true;
        }

        sleep_ms(200); // debounce
    }

    if (selecionar_arquivo) {
        set_led_green(); // Pronto (verde)
        beep(3000, 3, 100); // Beep de sucesso
        ssd1306_fill(&ssd, false);
        draw_centered_text(&ssd, "ARQUIVO ATUAL:", 15);
        draw_centered_text(&ssd, filename, 35);
        ssd1306_send_data(&ssd);
        sleep_ms(2000);
    }
}

// Função para desenhar o menu na tela OLED
void draw_menu() {
    ssd1306_fill(&ssd, false);

    turn_off_leds(); // Desliga LEDs durante a exibição do menu
    
    switch (current_state) {    
        case MODO_MONTAR_DESMONTAR:
            draw_centered_text(&ssd, "<>", 0);
            draw_centered_text(&ssd, sd_card_is_mounted ? "DESMONTAR" : "MONTAR", 10);
            draw_centered_text(&ssd, "A: Confirmar", 40);
            
            break;
            
        case MODO_GRAVAR:
            draw_centered_text(&ssd, "<>", 0);
            draw_centered_text(&ssd, "GRAVAR DADOS", 10);
            draw_centered_text(&ssd, filename, 20);

            draw_centered_text(&ssd, "A: Iniciar", 40);
            break;
            
        case MODO_LER:
            draw_centered_text(&ssd, "<>", 0);
            draw_centered_text(&ssd, "LER ARQUIVO:", 10);
            draw_centered_text(&ssd, filename, 20);
            
            draw_centered_text(&ssd, "A: Abrir", 40);
            break;
        case MODO_ALTERAR_ARQUIVO:
            draw_centered_text(&ssd, "<>", 0);
            draw_centered_text(&ssd, "ALTERAR ARQUIVO", 10);
            draw_centered_text(&ssd, filename, 20);
            
            draw_centered_text(&ssd, "A: ALTERAR", 40);
            break;
        case MODO_BOOTSEL:
            draw_centered_text(&ssd, "<>", 0);
            draw_centered_text(&ssd, "HABILITAR", 10);
            draw_centered_text(&ssd, "MODO BOOTSEL", 20);
            draw_centered_text(&ssd, "A: Confirmar", 40);
            break;
    }
    ssd1306_send_data(&ssd);
}

// Função para atualizar o menu com base no joystick
void update_menu_from_joystick() {
    adc_select_input(1); // Pino do eixo X do joystick
    uint16_t x_value = adc_read();
    
    if (x_value < 500) { // Joystick para a ESQUERDA
        if (current_state > MODO_MONTAR_DESMONTAR) current_state--;
        else current_state = MODO_BOOTSEL; // Volta ao final
    } 
    else if (x_value > 2500) { // Joystick para a DIREITA
        if (current_state < MODO_BOOTSEL) current_state++;
        else current_state = MODO_MONTAR_DESMONTAR; // Volta ao início
    }
}

// Função para tocar um beep com frequência e duração especificadas
// Repetições é o número de vezes que o beep será tocado
void beep(uint frequency, uint repeticoes, uint duration) {
    for (uint i = 0; i < repeticoes; i++) {
        play_tone(BUZZER_PIN, frequency);
        sleep_ms(duration);
        stop_tone(BUZZER_PIN);
        sleep_ms(duration);
    }
}

// Função para ler o conteúdo de um arquivo e exibir no terminal
void read_file(const char *filename)
{

    set_led_cyan(); // Em processo (azul)
    beep(3000, 1, 100); // Beep de início

    // Verifica se o cartão SD está montado
    if (!sd_card_is_mounted) {
        ssd1306_fill(&ssd, false);
        draw_centered_text(&ssd, "ERRO", 20);
        draw_centered_text(&ssd, "SD CARD", 30);
        draw_centered_text(&ssd, "NAO MONTADO", 40);
        ssd1306_send_data(&ssd);
        set_led_magenta(); // Erro (magenta)
        beep(2000, 2, 100); // Beep de erro
        sleep_ms(2000);
        return;
    }


    ssd1306_fill(&ssd, false);
    draw_centered_text(&ssd, "LENDO ARQUIVO", 10);
    draw_centered_text(&ssd, "(no terminal)", 20);
    draw_centered_text(&ssd, filename, 30);
    ssd1306_send_data(&ssd);
    sleep_ms(2000); // Espera 2 segundos para mostrar a mensagem
    
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível abrir o arquivo para leitura. Verifique se o Cartão está montado ou se o arquivo existe.\n");
        set_led_magenta(); // Erro (magenta)
        beep(2000, 2, 100); // Beep de erro
        ssd1306_fill(&ssd, false);
        draw_centered_text(&ssd, "ERRO AO LER", 20);
        draw_centered_text(&ssd, "ARQUIVO", 30);
        ssd1306_send_data(&ssd);
        sleep_ms(2000);
        return;
    }
    char buffer[128];
    UINT br;
    printf("Conteúdo do arquivo %s:\n", filename);
    while (f_read(&file, buffer, sizeof(buffer) - 1, &br) == FR_OK && br > 0)
    {
        buffer[br] = '\0';
        printf("%s", buffer);
    }
    f_close(&file);
    ssd1306_fill(&ssd, false);
    draw_centered_text(&ssd, "ARQUIVO LIDO", 20);
    draw_centered_text(&ssd, "(no terminal)", 30);
    ssd1306_draw_string(&ssd, filename, 5, 40);
    ssd1306_send_data(&ssd);
    set_led_green(); // Pronto (verde)
    beep(3000, 3, 100); // Beep de sucesso

    printf("\nLeitura do arquivo %s concluída.\n\n", filename);
}

// Callback para os botões
static void gpio_button_handler(uint gpio, uint32_t events){
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
        if(gpio == BUTTON_A && (current_time - last_time_debounce_button_a > delay_debounce)){
            printf("\nBOTAO A PRESSIONADO\n");

            selecionar = true;
        

            last_time_debounce_button_a = current_time; // Atualiza o tempo do último debounce do botão A
        }
        else if(gpio == BUTTON_B && (current_time - last_time_debounce_button_b > delay_debounce)){
            printf("\nBOTAO B PRESSIONADO\n");

            if(MODO_ALTERAR_ARQUIVO) BUTTON_B_PRESSED = true; // Flag para indicar que o botão B foi pressionado

            last_time_debounce_button_b = current_time; // Atualiza o tempo do último debounce do botão B
        }
        else if(gpio == BUTTON_SW){// Habilita o modo de gravação
            
            
            /*
            printf("\nHABILITANDO O MODO GRAVAÇÃO\n");
            sd_unmount(); // Desmonta o SD Card

            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "  HABILITANDO", 5, 25);
            ssd1306_draw_string(&ssd, " MODO GRAVACAO", 5, 38);
            ssd1306_send_data(&ssd);

            reset_usb_boot(0, 0);
            */
        }
}
