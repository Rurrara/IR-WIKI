#include "mmap_utils.h"

void* createEmptyFilemmap(char* filename, size_t file_size) {

	void* mmap_ptr;
	int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (fd == -1) {
		printf("COULDN'T OPEN FILE!\n");
		return NULL;
	}

	uint8_t* zero_buf;
	zero_buf = calloc(4096, sizeof(uint8_t));
	int write_res;

	for (uint64_t i = 0; i < (uint64_t)(file_size / 4096); i++) {
		write_res=write(fd, zero_buf, sizeof(uint8_t) * 4096);
	}
	write_res=write(fd, zero_buf, sizeof(uint8_t) * (file_size % 4096));
	free(zero_buf);
	mmap_ptr = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mmap_ptr == MAP_FAILED) {
		printf("MMAP FAILED! write_res = %d\n", write_res);
		return NULL;
	}
	return mmap_ptr;
}

void* openFilemmap(char* filename, size_t* file_size){


	void* mmap_ptr;
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf("COULDN'T OPEN FILE!\n");
		return NULL;
	}

	struct stat st;
	if (fstat(fd, &st) < 0) {
		printf("PANIC!\n");
		return NULL;
	}
	*file_size = st.st_size;

	mmap_ptr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (mmap_ptr == MAP_FAILED) {
		printf("MMAP FAILED!\n");
		return NULL;
	}
	return mmap_ptr;
}