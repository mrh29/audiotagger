#ifndef FILEIO_H
#define FILEIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h> 

/****************************************************************
 * print_pointers: 
 *   prints the current and end positions of file with file 
 * descriptor fd. 
 ****************************************************************/
void print_pointers(int fd);

/****************************************************************
 * remove_bytes: 
 *   removes num_bytes bytes from fd and places the resulting file
 * at location path. 
 ****************************************************************/
void remove_bytes(int fd, size_t num_bytes, char* path);

/****************************************************************
 * remove_bytes_at: 
 *   removes num_bytes bytes from fd at offset and places 
 * the resulting file at location path. 
 ****************************************************************/
void remove_bytes_at(int fd, size_t num_bytes, off_t offset, char* path);

/****************************************************************
 * add_bytes: 
 *   adds num_bytes bytes from buf to the file file descriptor fd 
 * at the current position and places the resulting file at 
 * location path. 
 ****************************************************************/
void add_bytes(int fd, size_t num_bytes, uint8_t* buf, char* path); 


/****************************************************************
 * add_bytes_at: 
 *   adds num_bytes bytes from buf to the file file descriptor fd 
 * at offset and places the resulting file at location path. 
 ****************************************************************/
void add_bytes_at(int fd, size_t num_bytes, uint8_t* buf, off_t offset, char* path);

#endif