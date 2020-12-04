#include "fileio.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <iostream>

#define BLOCK_SIZE 1024

static uint8_t block_buf[BLOCK_SIZE];

/****************************************************************
 * print_pointers: 
 *   prints the current and end positions of file with file 
 * descriptor fd. 
 ****************************************************************/
void print_pointers(int fd) {
    off_t curr = lseek(fd, 0, SEEK_CUR);

    printf("Curr: %lld End: %lld\n", curr, lseek(fd, 0, SEEK_END));
    lseek(fd, curr, SEEK_SET);
    return;
}

/****************************************************************
 *  copy_prefix: 
 *   internal helper function for add_bytes/remove_bytes. Copies
 * bytes [0, curr) from the file with file descriptor fd to 
 * the file with file descriptor fd2.
 ****************************************************************/
void copy_prefix(int fd, int fd2, off_t curr) {
    uint32_t bytes_read = 0;
    int offset = 0;
    int end = curr;

    // Write by a block until only less than a block remains
    while (end - bytes_read > BLOCK_SIZE) {
        read(fd, block_buf, BLOCK_SIZE);
        write(fd2, block_buf, BLOCK_SIZE);
        bytes_read += BLOCK_SIZE;
    }
    read(fd, block_buf, end - bytes_read);
    write(fd2, block_buf, end - bytes_read);
}

/****************************************************************
 * remove_bytes: 
 *   removes num_bytes bytes from fd and places the resulting file
 * at location path. 
 ****************************************************************/
void remove_bytes(int fd, size_t num_bytes, char* path) {
    int fd2 = open("tmp", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd2 == -1) {
        return;
    }
    assert(fd != fd2);

    // Get the current position
    off_t curr = lseek(fd, 0, SEEK_CUR);

    // Copy the prefix
    lseek(fd, 0, SEEK_SET);
    copy_prefix(fd, fd2, curr);

    // Get the start of bytes following the deleted range
    off_t rd_start = lseek(fd, num_bytes, SEEK_CUR);
    assert(rd_start != -1);

    size_t bytes_read;

    // Read a block
    while ((bytes_read = read(fd, &block_buf, BLOCK_SIZE))) {
        write(fd2, &block_buf, bytes_read);
    }

    close(fd);
    close(fd2);

    unlink(path);
    rename("tmp",  path);
    fd2 = open(path, O_RDWR, S_IRUSR | S_IWUSR);

    // dup fd2 to be the original file desciptor
    if (fd2 != fd) {
        dup2(fd2, fd);
        close(fd2);
    }

    // Set pointer to current or end
    off_t end = lseek(fd, 0, SEEK_END);
    if (end > curr) {
        lseek(fd, curr, SEEK_SET);
    }
}

/****************************************************************
 * remove_bytes: 
 *   removes num_bytes bytes from fd at offset and places 
 * the resulting file at location path. 
 ****************************************************************/
void remove_bytes_at(int fd, size_t num_bytes, off_t offset, char* path) {
    lseek(fd, offset, SEEK_SET);
    remove_bytes(fd, num_bytes, path);
}

/****************************************************************
 * add_bytes: 
 *   adds num_bytes bytes from buf to the file file descriptor fd 
 * at the current position and places the resulting file at 
 * location path. 
 ****************************************************************/
void add_bytes(int fd, size_t num_bytes, uint8_t* buf, char* path) {

    // Open a temporary file
    int fd2 = open("tmp", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd2 == -1) {
        printf("error\n");
        return;
    }
    assert(fd != fd2);

    // Get the current position
    off_t curr = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);

    copy_prefix(fd, fd2, curr);

    // TODO: batch this? 
    write(fd2, buf, num_bytes);
    size_t bytes_read;

    while ((bytes_read = read(fd, block_buf, BLOCK_SIZE))) {
        write(fd2, block_buf, bytes_read);
    }

    close(fd); 
    close(fd2);

    rename("tmp", path);
    fd2 = open(path, O_RDWR, S_IRUSR | S_IWUSR);

    if (fd2 != fd) {
        dup2(fd2, fd);
        close(fd2);
    }
    lseek(fd, curr + num_bytes, SEEK_SET);
}

/****************************************************************
 * add_bytes_at: 
 *   adds num_bytes bytes from buf to the file file descriptor fd 
 * at offset and places the resulting file at location path. 
 ****************************************************************/
void add_bytes_at(int fd, size_t num_bytes, uint8_t* buf, off_t offset, char* path) {
    lseek(fd, offset, SEEK_SET);
    add_bytes(fd, num_bytes, buf, path);
}