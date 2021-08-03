#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include<sys/mman.h>
#include <sys/stat.h>

void* createEmptyFilemmap(char* filename, size_t file_size);
void* openFilemmap(char* filename, size_t* file_size);