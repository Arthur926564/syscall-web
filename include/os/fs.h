#ifndef OS_FS_H
#define OS_FS_H

#include <stddef.h>  // for size_t
#include <unistd.h>

/**
 * Open a file and return a file descriptor (fd)
 *
 * Example: int fd = os_open("index.html", O_RDONLY);
 */
int os_open(const char *path, int flags);

/**
 * Read up to 'size' bytes from 'fd' and store the data into 'buf'
 *
 * Returns the number of bytes read, or -1 on error
 */
ssize_t os_read(int fd, void *buf, size_t size);

/**
 * Write into the fd the value into the buffer
 */
ssize_t os_write(int fd, const void *buf, size_t size);

/**
 * Close a file descriptor (file or socket)
 *
 * Returns 0 on success, -1 on error
 */
int os_close(int fd);

/**
 * Put the file descriptor into non-blocking mode
 *
 * Returns 0 on success, -1 on error
 */
int os_set_nonblocking(int fd);

#endif // OS_FS_H

