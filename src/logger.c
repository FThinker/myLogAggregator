#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>

#include "logger.h"

static int log_fd = -1;
static char current_filename[256];


// ----------------------------------------------------------------------------------------- //


//Formatted timestamp
static void get_current_timestamp(char *buffer, size_t max_len) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    // Format: YYYY-MM-DD HH:MM:S
    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", timeinfo);
}


// ----------------------------------------------------------------------------------------- //


//Lock for the file log (F_SETLKW stop the thread without the lock)
static int apply_lock(int fd, int lock_type) {
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = lock_type;    // F_WRLCK (writing) o F_UNLCK (unlock)
    lock.l_whence = SEEK_SET;   // Lock file
    lock.l_start = 0;
    lock.l_len = 0;             // End of the file
    
    return fcntl(fd, F_SETLKW, &lock);
}


// ----------------------------------------------------------------------------------------- //


bool logger_init(const char *filename) {
    // Ensure directory exists (if filename contains a '/'), create it if necessary
    const char *slash = strrchr(filename, '/');
    if (slash) {
        char dir[256];
        size_t dir_len = slash - filename;
        if (dir_len >= sizeof(dir)) dir_len = sizeof(dir) - 1;
        strncpy(dir, filename, dir_len);
        dir[dir_len] = '\0';
        if (mkdir(dir, 0755) == -1) {
            if (errno != EEXIST) {
                perror("Error creating log directory");
                return false;
            }
        }
    }

    strncpy(current_filename, filename, sizeof(current_filename) - 1);
    current_filename[sizeof(current_filename) - 1] = '\0';

    // O_WRONLY: only writing, O_CREAT: create if not exist, O_APPEND: append
    log_fd = open(current_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("Error: Failed to open the file");
        return false;
    }
    return true;
}


// ----------------------------------------------------------------------------------------- //


bool logger_write_data(int sender_id, double data) {
    if (log_fd == -1) return false;

    char timestamp[20];
    get_current_timestamp(timestamp, sizeof(timestamp));

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%s, %d, %f]\n", timestamp, sender_id, data);

    apply_lock(log_fd, F_WRLCK);

    ssize_t bytes_written = write(log_fd, buffer, strlen(buffer));

    apply_lock(log_fd, F_UNLCK);
    // ------------------------

    return (bytes_written > 0);
}


// ----------------------------------------------------------------------------------------- //


bool logger_write_disconnect(int sender_id) {
    if (log_fd == -1) return false;

    char timestamp[20];
    get_current_timestamp(timestamp, sizeof(timestamp));

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%s, %d, \"DISCONNECT\"]\n", timestamp, sender_id);

    apply_lock(log_fd, F_WRLCK);
    ssize_t bytes_written = write(log_fd, buffer, strlen(buffer));
    apply_lock(log_fd, F_UNLCK);

    return (bytes_written > 0);
}


// ----------------------------------------------------------------------------------------- //


bool logger_check_and_rotate(size_t max_size) {
    if (log_fd == -1) return false;

    struct stat st;
    
    // Lock the log file to prevent other users from writing to it
    apply_lock(log_fd, F_WRLCK);

    // fstat give information about the file
    if (fstat(log_fd, &st) == -1) {
        perror("Error checking log file size");
        apply_lock(log_fd, F_UNLCK);
        return false;
    }

    if ((size_t)st.st_size >= max_size) {
        // Close current file descriptor (this implicitly releases the lock as well)
        close(log_fd);

        // Generate a unique filename for the archived log using the current timestamp
        char archive_name[512];
        char timestamp[20];
        get_current_timestamp(timestamp, sizeof(timestamp));
        // replace spaces and colons with underscores for file system compatibility
        for(int i=0; timestamp[i] != '\0'; i++) {
            if(timestamp[i] == ' ' || timestamp[i] == ':') timestamp[i] = '_';
        }
        // Determine directory and base filename
        char dir[256] = ".";
        char base[256];
        char *slash = strrchr(current_filename, '/');
        if (slash) {
            size_t dir_len = slash - current_filename;
            if (dir_len >= sizeof(dir)) dir_len = sizeof(dir) - 1;
            strncpy(dir, current_filename, dir_len);
            dir[dir_len] = '\0';
            strncpy(base, slash + 1, sizeof(base) - 1);
            base[sizeof(base) - 1] = '\0';
        } else {
            strncpy(base, current_filename, sizeof(base) - 1);
            base[sizeof(base) - 1] = '\0';
        }

        snprintf(archive_name, sizeof(archive_name), "%s/archive_%s_%s", dir, timestamp, base);

        // Rename the current log file to archive it
        rename(current_filename, archive_name);

        // Reopen a brand new, empty log file with the original name
        log_fd = open(current_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (log_fd == -1) {
            perror("Error recreating log file after rotation");
            return false;
        }
        printf("[LOGGER] File successfully rotated. Archived as: %s\n", archive_name);
    } else {
        // If the file is still small enough, simply release the lock and do nothing
        apply_lock(log_fd, F_UNLCK);
    }

    return true;
}


// ----------------------------------------------------------------------------------------- //


void logger_close(void) {
    // Safely closes the log file descriptor and resets its state
    if (log_fd != -1) {
        close(log_fd);
        log_fd = -1;
    }
}


// ----------------------------------------------------------------------------------------- //