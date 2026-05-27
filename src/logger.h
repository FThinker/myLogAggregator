#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>

// Configurazione (puoi cambiarle in base alle specifiche del progetto)
#define LOG_FILE_NAME "coordinatore.log"
#define MAX_LOG_SIZE 102400 // Esempio: 100 KB in byte

// Inizializza il logger (apre il file)
int logger_init(const char *filename);

// Scrive un dato nel log: [TIMESTAMP, ID_MITTENTE, DATO]
int logger_write_data(int id_mittente, int dato);

// Scrive la disconnessione nel log: [TIMESTAMP, ID_MITTENTE, "DISCONNECT"]
int logger_write_disconnect(int id_mittente);

// Controlla la dimensione e archivia se necessario (chiamata dall'handler di SIGALRM)
int logger_check_and_rotate(size_t max_size);

// Chiude il file di log (chiamata alla terminazione controllata SIGINT)
void logger_close(void);

#define LOGGER_H