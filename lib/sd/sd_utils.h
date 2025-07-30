#ifndef SD_UTILS_H
#define SD_UTILS_H

#include "ff.h"
#include "diskio.h"
#include "rtc.h"
#include "FatFs_SPI/sd_driver/hw_config.h"

// Códigos de retorno
#define SD_OK             0      // Operação concluída com sucesso
#define SD_ERR_MOUNT     -1     // Falha ao montar o SD (cartão não reconhecido ou sistema de arquivos inválido)
#define SD_ERR_UNMOUNT   -2     // Falha ao desmontar o SD (possivelmente cartão removido durante operação)
#define SD_ERR_FORMAT    -3     // Erro durante formatação (cartão protegido contra gravação ou danificado)
#define SD_ERR_OPEN      -4     // Não conseguiu abrir o arquivo (não existe ou caminho inválido)
#define SD_ERR_READ      -5     // Falha na leitura (arquivo corrompido ou erro de E/S)
#define SD_ERR_WRITE     -6     // Falha na gravação (espaço insuficiente ou cartão protegido)
#define SD_ERR_NOTFOUND  -7     // Arquivo ou diretório não encontrado
#define SD_ERR_UNKNOWN   -99    // Erro genérico (para casos não especificados)

// Monta o sistema de arquivos do SD card
int sd_mount(void);

// Desmonta o sistema de arquivos do SD card
int sd_unmount(void);

// Formata o SD card (CUIDADO: apaga todos os dados!)
int sd_format(void);

// Obtém espaço livre em bytes
int sd_get_free(uint64_t *total_bytes, uint64_t *free_bytes);

// Lista arquivos no diretório atual
int sd_ls(void);

// Exibe conteúdo de um arquivo
int sd_cat(const char *filename);

// Função utilitária para mensagens de erro
const char* sd_strerror(int err_code);

// Funções auxiliares internas
sd_card_t* _sd_get_by_name(const char *name);
FATFS* _sd_get_fs_by_name(const char *name);

#endif