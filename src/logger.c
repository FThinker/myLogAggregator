#include <logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// Variabile globale statica (visibile solo in questo file) per mantenere il descrittore del file
static int log_fd = -1;
static char current_filename[256];

// Funzione interna per ottenere il timestamp formattato
static void get_current_timestamp(char *buffer, size_t max_len) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    // Formato: YYYY-MM-DD HH:MM:S
    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Funzione interna per gestire il lock (F_SETLKW blocca il processo/thread finché non ottiene il lock)
static int apply_lock(int fd, int lock_type) {
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = lock_type;    // F_WRLCK (scrittura) o F_UNLCK (rilascio)
    lock.l_whence = SEEK_SET;   // Blocca l'intero file
    lock.l_start = 0;
    lock.l_len = 0;             // 0 significa fino alla fine del file
    
    return fcntl(fd, F_SETLKW, &lock);
}

int logger_init(const char *filename) {
    strncpy(current_filename, filename, sizeof(current_filename) - 1);
    
    // O_WRONLY: solo scrittura, O_CREAT: crea se non esiste, O_APPEND: scrive sempre alla fine
    log_fd = open(current_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("Errore nell'apertura del file di log");
        return -1;
    }
    return 0;
}

int logger_write_data(int id_mittente, int dato) {
    if (log_fd == -1) return -1;

    char timestamp[20];
    get_current_timestamp(timestamp, sizeof(timestamp));

    // Prepariamo la stringa da scrivere
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%s, %d, %d]\n", timestamp, id_mittente, dato);

    // --- SEZIONE CRITICA ---
    // 1. Prendi il lock di scrittura
    apply_lock(log_fd, F_WRLCK);

    // 2. Scrivi nel file
    ssize_t bytes_written = write(log_fd, buffer, strlen(buffer));

    // 3. Rilascia il lock
    apply_lock(log_fd, F_UNLCK);
    // ------------------------

    return (bytes_written > 0) ? 0 : -1;
}

int logger_write_disconnect(int id_mittente) {
    if (log_fd == -1) return -1;

    char timestamp[20];
    get_current_timestamp(timestamp, sizeof(timestamp));

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%s, %d, \"DISCONNECT\"]\n", timestamp, id_mittente);

    apply_lock(log_fd, F_WRLCK);
    ssize_t bytes_written = write(log_fd, buffer, strlen(buffer));
    apply_lock(log_fd, F_UNLCK);

    return (bytes_written > 0) ? 0 : -1;
}

int logger_check_and_rotate(size_t max_size) {
    if (log_fd == -1) return -1;

    struct stat st;
    
    // Prendiamo il lock per evitare che altri scrivano mentre controlliamo/ruotiamo
    apply_lock(log_fd, F_WRLCK);

    // fstat ottiene le informazioni del file (tra cui la dimensione in byte)
    if (fstat(log_fd, &st) == -1) {
        perror("Errore nel controllo dimensione file");
        apply_lock(log_fd, F_UNLCK);
        return -1;
    }

    if ((size_t)st.st_size >= max_size) {
        // Il file ha superato il limite: dobbiamo ruotarlo
        close(log_fd);

        // Creiamo un nome per l'archivio usando il timestamp corrente
        char archive_name[512];
        char timestamp[20];
        get_current_timestamp(timestamp, sizeof(timestamp));
        // Sostituiamo gli spazi e i due punti per evitare problemi con i nomi dei file
        for(int i=0; timestamp[i] != '\0'; i++) {
            if(timestamp[i] == ' ' || timestamp[i] == ':') timestamp[i] = '_';
        }
        
        snprintf(archive_name, sizeof(archive_name), "archive_%s_%s", timestamp, current_filename);

        // Rinominiamo il vecchio file (lo archiviamo)
        rename(current_filename, archive_name);

        // Riapriamo un nuovo file di log vuoto
        log_fd = open(current_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (log_fd == -1) {
            perror("Errore nella ricreazione del file di log dopo rotazione");
            return -1;
        }
        printf("[LOGGER] File ruotato con successo. Vecchio file archiviato come: %s\n", archive_name);
    } else {
        // Se non serve ruotarlo, rilasciamo semplicemente il lock
        apply_lock(log_fd, F_UNLCK);
    }

    return 0;
}

void logger_close(void) {
    if (log_fd != -1) {
        close(log_fd);
        log_fd = -1;
    }
}