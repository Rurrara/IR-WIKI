#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "compression.h"
#include "mmap_utils.h"

#define _FILE_OFFSET_BITS 64

int cmp_func(const void* v1, const void* v2) {

	if (*(uint32_t*)v1 > * (uint32_t*)v2) return 1;
	if (*(uint32_t*)v1 < *(uint32_t*)v2) return -1;
	return 0;
}


int main(int argc, char *argv[]) {


	uint32_t* new_ht;
	uint32_t* hashtable, * inverted_index;
	size_t ht_size, ii_size;
	hashtable = (uint32_t*)openFilemmap("hashtable2.bin", &ht_size);
	if (hashtable == NULL) {
		printf("ERROR!\n");
		return 1;
	}
	inverted_index = (uint32_t*)openFilemmap("inverted_index_pos.bin", &ii_size);
	if (inverted_index == NULL) {
		printf("ERROR!\n");
		return 1;
	}
	printf("%ld %ld\n", ht_size, ii_size);

	FILE* f_cmpr_index;
	f_cmpr_index = fopen("compressed_index.bin", "wb");
	if (f_cmpr_index == NULL) {
		printf("COULDN'T CREATE FILE!\n");
		return 1;
	}


	new_ht = (uint32_t*)createEmptyFilemmap("hashtable3.bin", ht_size);
	memcpy(new_ht, hashtable, ht_size);

	const uint32_t hash_qty = ht_size / (sizeof(uint32_t) + sizeof(size_t));


	size_t temp_ptr, new_offset_ht = 0;

	uint32_t freq = 0, docs = 0, len_compr_docs=0, len_comp_pos=0, len_head=0;
	uint8_t* compr_buf;
	compr_buf = calloc(0x3ffffff, sizeof(uint8_t));

	for (int i = 0; i != hash_qty; i++) {
		temp_ptr = *(size_t*)(hashtable + 3 * i + 1);

		len_compr_docs = 0;
		len_comp_pos = 0;
		len_head = 0;
		
		freq = *(inverted_index + temp_ptr);
		docs = *(inverted_index + temp_ptr + 1);
		//if (! i%10000)printf("SERVING HASH #%d\n", i);

		//DOC_IDS
		for (int j = 0; j != docs; j++) {
			len_compr_docs+=compress_uint32(*(inverted_index+ temp_ptr +2+j), (compr_buf+ len_compr_docs));
		}

		//ENTRIES IN DOC
		for (int j = 0; j != docs; j++) {
			len_compr_docs += compress_uint32(*(inverted_index + temp_ptr + 2 + j + docs), (compr_buf + len_compr_docs));
		}

		for (int j = 0; j != freq; j++) {
			len_comp_pos += compress_uint32(*(inverted_index + temp_ptr + 2 + 2 * docs + j), (compr_buf+len_compr_docs+len_comp_pos));
		}

		len_head += compress_uint32(freq, (compr_buf + len_compr_docs + len_comp_pos));
		len_head += compress_uint32(docs, (compr_buf + len_compr_docs + len_comp_pos+ len_head));




		*(size_t*)(new_ht + 1 + 3 * i) = new_offset_ht;

		new_offset_ht += (len_head + len_compr_docs + len_comp_pos);

		fwrite(compr_buf + len_compr_docs + len_comp_pos, sizeof(uint8_t), len_head, f_cmpr_index);
		fwrite(compr_buf, sizeof(uint8_t), len_compr_docs + len_comp_pos, f_cmpr_index);

	}



	printf("WRITING MMAP!\n");
	if (msync(new_ht, ht_size, MS_SYNC) == -1) {
		printf("ERROR WRITING FROM MMAP!\n");
		return 1;
	}

	return 1;
}