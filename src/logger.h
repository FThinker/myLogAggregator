#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>

#define LOG_FILE_NAME "logs/current.log"
#define MAX_LOG_SIZE 51200 // 50KB

// ######################################################################################### //
//                                        FUNCTIONS                                          //
// ######################################################################################### //

/**
 * @brief initializes the logger by opening the log file for writing. 
 *
 * @param filename the name of the log file
 * @warning if the file already exists, it will be overwritten.
 * @return true on success, false on failure
 */
bool logger_init(const char *filename);


// ----------------------------------------------------------------------------------------- //


/**
 * @brief logs an entry for a new connection with the given sender ID.
 * 
 * @param sender_id the ID of the sender that established the connection
 * @param data the data to be logged
 * @return true on success, false on failure
 */
bool logger_write_data(int sender_id, double data);


// ----------------------------------------------------------------------------------------- //


/**
 * @brief logs an entry for a disconnection with the given sender ID.
 *
 * @param sender_id the ID of the sender that disconnected
 * @return true on success, false on failure
 */
bool logger_write_disconnect(int sender_id);


// ----------------------------------------------------------------------------------------- //


/**
 * @brief checks the size of the log file and rotates it if it exceeds the specified maximum size.
 *
 * A rotation consists in archiving the current log file with a timestamped name and creating a new log file
 * with the same orignal name to keep logging entries.
 *
 * @param max_size the maximum allowed size of the log file in bytes
 * @return true if the rotation is successful, false otherwise
 */
bool logger_check_and_rotate(size_t max_size);


// ----------------------------------------------------------------------------------------- //


/**
 * @brief closes the logger by closing the log file and releasing any resources used by the logger.
 *
 * @param void this function doesn't allow any parameters
 */
void logger_close(void);


// ----------------------------------------------------------------------------------------- //

#endif