#pragma once

#include <unistd.h>

#define PAGE_SIZE 4096

#define STUB_FILES_COUNT 1024
#define STUB_HUGEPAGES_COUNT 2

// note: we only support 1 pipe
#define STUB_PIPE_FD_READ (STUB_FILES_COUNT + 1)
#define STUB_PIPE_FD_WRITE (STUB_FILES_COUNT + 2)

void stub_pipe_write(const void* buf, size_t len);

