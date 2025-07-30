#include "sd_utils.h"
#include <stdio.h>
#include <string.h>

// Implementação das funções auxiliares
sd_card_t* _sd_get_by_name(const char *name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    return NULL;
}

FATFS* _sd_get_fs_by_name(const char *name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return &sd_get_by_num(i)->fatfs;
    return NULL;
}

// Implementação das funções principais
int sd_mount(void) {
    const char *drive = sd_get_by_num(0)->pcName;
    FATFS *p_fs = _sd_get_fs_by_name(drive);
    
    if (!p_fs) return SD_ERR_MOUNT;
    
    FRESULT fr = f_mount(p_fs, drive, 1);
    if (FR_OK != fr) return SD_ERR_MOUNT;
    
    sd_card_t *pSD = _sd_get_by_name(drive);
    if (!pSD) return SD_ERR_MOUNT;
    
    pSD->mounted = true;
    return SD_OK;
}

// Implementação da função de erro
const char* sd_strerror(int err_code) {
    static const char* errors[] = {
        "Operação bem-sucedida",
        "Erro ao montar SD",
        "Erro ao desmontar SD",
        "Erro ao formatar",
        "Erro ao abrir arquivo",
        "Erro de leitura",
        "Erro de escrita",
        "Arquivo não encontrado"
    };
    
    if (err_code <= SD_OK && err_code >= SD_ERR_NOTFOUND)
        return errors[-err_code];
    return "Erro desconhecido";
}

int sd_unmount(void) {
    const char *drive = sd_get_by_num(0)->pcName;
    FATFS *p_fs = _sd_get_fs_by_name(drive);
    
    if (!p_fs) return SD_ERR_UNMOUNT;
    
    FRESULT fr = f_unmount(drive);
    if (FR_OK != fr) return SD_ERR_UNMOUNT;
    
    sd_card_t *pSD = _sd_get_by_name(drive);
    if (!pSD) return SD_ERR_UNMOUNT;
    
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT;
    return SD_OK;
}

int sd_format(void) {
    const char *drive = sd_get_by_num(0)->pcName;
    FATFS *p_fs = _sd_get_fs_by_name(drive);
    
    if (!p_fs) return SD_ERR_FORMAT;
    
    FRESULT fr = f_mkfs(drive, 0, 0, FF_MAX_SS * 2);
    return (FR_OK == fr) ? SD_OK : SD_ERR_FORMAT;
}

int sd_get_free(uint64_t *total_bytes, uint64_t *free_bytes) {
    const char *drive = sd_get_by_num(0)->pcName;
    DWORD fre_clust;
    FATFS *p_fs = _sd_get_fs_by_name(drive);
    
    if (!p_fs) return SD_ERR_UNKNOWN;
    
    FRESULT fr = f_getfree(drive, &fre_clust, &p_fs);
    if (FR_OK != fr) return SD_ERR_UNKNOWN;


    *total_bytes = (uint64_t)(p_fs->n_fatent - 2) * p_fs->csize;
    *free_bytes = (uint64_t)(fre_clust * p_fs->csize);;
    return SD_OK;
}

int sd_ls(void) {
    DIR dj;
    FILINFO fno;
    FRESULT fr;
    char cwdbuf[FF_LFN_BUF] = {0};
    
    fr = f_getcwd(cwdbuf, sizeof(cwdbuf));
    if (FR_OK != fr) return SD_ERR_READ;
    
    fr = f_findfirst(&dj, &fno, cwdbuf, "*");
    if (FR_OK != fr) return SD_ERR_READ;
    
    while (fr == FR_OK && fno.fname[0]) {
        printf("%s [%s] [size=%llu]\n", 
               fno.fname,
               (fno.fattrib & AM_DIR) ? "directory" : 
               (fno.fattrib & AM_RDO) ? "read only" : "writable",
               fno.fsize);
               
        fr = f_findnext(&dj, &fno);
    }
    
    f_closedir(&dj);
    return SD_OK;
}

int sd_cat(const char *filename) {
    FIL fil;
    FRESULT fr = f_open(&fil, filename, FA_READ);
    if (FR_OK != fr) return SD_ERR_OPEN;
    
    char buf[256];
    while (f_gets(buf, sizeof(buf), &fil)) {
        printf("%s", buf);
    }
    
    fr = f_close(&fil);
    return (FR_OK == fr) ? SD_OK : SD_ERR_READ;
}