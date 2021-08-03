#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>



#include "Utf8Func.h"
#include "hash.h"
#include "mmap_utils.h"

#define _FILE_OFFSET_BITS 64


int cmp_func(const void* v1, const void* v2) {

	if (*(uint32_t*)v1 > * (uint32_t*)v2) return 1;
	if (*(uint32_t*)v1 < *(uint32_t*)v2) return -1;
	return 0;
}


int removeDuplicatesHashes(uint32_t* hash_arr, uint32_t* cur_len) {


	uint32_t new_len = 0;
	qsort(hash_arr, *cur_len, sizeof(uint32_t), cmp_func);

	for (int i = 0; i < *cur_len - 1; i++) {
		if (*(hash_arr + i) < *(hash_arr + i + 1)) *(hash_arr + new_len++) = *(hash_arr + i);
	}
	*(hash_arr + new_len++) = *(hash_arr + *cur_len - 1);
	*cur_len = new_len;


	return 1;

}
	



int main(int argc, char *argv[]) {

	FILE* fin;
	fin = fopen("doc_processed.bin", "rm");
	if (fin == NULL) {
		printf("ERROR OPENING FILE!\n");
		return 0;
	}


	const uint32_t MAX_ALLOCATION_SIZE = 4096 * 32;
	uint32_t doc_id, title_qty, text_qty, cat_qty;
	uint32_t  *all_unique_tokens;

	uint32_t cur_tokens = 0;
	const uint32_t TOKENS_ALLOCATION_SIZE = 0x2ffffff;

	all_unique_tokens = calloc(TOKENS_ALLOCATION_SIZE, sizeof(uint32_t));

	size_t fread_val;

	//Нахождение всех уникальных хешей
	while (fread(&doc_id, sizeof(uint32_t), 1, fin) != 0) {
		if (cur_tokens > (TOKENS_ALLOCATION_SIZE - MAX_ALLOCATION_SIZE)) {
			removeDuplicatesHashes(all_unique_tokens, &cur_tokens);
		}


		fread_val = fread(&title_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread((all_unique_tokens + cur_tokens), sizeof(uint32_t), title_qty, fin);
		cur_tokens += title_qty;

		fread_val = fread(&text_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread((all_unique_tokens + cur_tokens), sizeof(uint32_t), text_qty, fin);
		cur_tokens += text_qty;

		fread_val = fread(&cat_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread((all_unique_tokens + cur_tokens), sizeof(uint32_t), cat_qty, fin);
		cur_tokens += cat_qty;
	}
	removeDuplicatesHashes(all_unique_tokens, &cur_tokens);

	printf("TOTAL UNIQUE HASHES=%d\n", cur_tokens);


	uint32_t* hash_doc_qty, *hash_freq, *temp_doc_hashes;
	hash_doc_qty = calloc(cur_tokens, sizeof(uint32_t));
	hash_freq = calloc(cur_tokens, sizeof(uint32_t));
	temp_doc_hashes = calloc(MAX_ALLOCATION_SIZE, sizeof(uint32_t));
	uint32_t temp_qty = 0, *hash_off_ptr;


	rewind(fin);

	//Calculate freq
	while (fread(&doc_id, sizeof(uint32_t), 1, fin) != 0) {
		temp_qty = 0;

		fread_val = fread(&title_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread(temp_doc_hashes, sizeof(uint32_t), title_qty, fin);
		temp_qty += title_qty;


		fread_val = fread(&text_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread((temp_doc_hashes + temp_qty), sizeof(uint32_t), text_qty, fin);
		temp_qty += text_qty;

		fread_val = fread(&cat_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread((temp_doc_hashes + temp_qty), sizeof(uint32_t), cat_qty, fin);
		temp_qty += cat_qty;

		qsort(temp_doc_hashes, temp_qty, sizeof(uint32_t), cmp_func);



		for (int i = 0; i < temp_qty; i++) {
			hash_off_ptr = bsearch((temp_doc_hashes + i), all_unique_tokens, cur_tokens, sizeof(uint32_t), cmp_func);
			size_t index = hash_off_ptr - all_unique_tokens;
			(*(hash_doc_qty + index))++;
			while (*(hash_off_ptr) == *(temp_doc_hashes + i)) {
				i++;
				(*(hash_freq + index))++;
			}
			i--;

		}
		if (!(doc_id % 10000))printf("DOC %d SERVED!\n", doc_id);
	}

	printf("DONE CALCULATING FREQUENCIES!!\n");



	size_t* hash_offset, *pos_offset, *doc_ids_offset, *entries_offset;
	hash_offset = calloc(cur_tokens, sizeof(size_t));


	doc_ids_offset = calloc(cur_tokens, sizeof(size_t));
	entries_offset = calloc(cur_tokens, sizeof(size_t));
	pos_offset = calloc(cur_tokens, sizeof(size_t));

	size_t file_size = (2+(*hash_doc_qty)*2+(*hash_freq));
	*pos_offset=*hash_offset+ 2 + (*hash_doc_qty) * 2;
	for (int i = 1; i < cur_tokens; i++) {
		*(hash_offset + i) = (*(hash_offset + i - 1)) + (2 + *(hash_doc_qty+i-1) * 2 + *(hash_freq+i-1));

		file_size += (2 + *(hash_doc_qty + i) * 2 + *(hash_freq + i));
	}




	file_size = file_size * sizeof(uint32_t);
	uint32_t* mmap_inverted_index_pos= (uint32_t*)createEmptyFilemmap("inverted_index_pos.bin", file_size);


	uint32_t* mmap_hashtable= (uint32_t*)createEmptyFilemmap("hashtable2.bin", (sizeof(uint32_t)+sizeof(size_t))*cur_tokens);
	for (int i = 0; i < cur_tokens; i++) {
		*(mmap_hashtable + 3 * i) = *(all_unique_tokens + i); //Hash
		*(size_t*)(mmap_hashtable + 3 * i + 1) = *(hash_offset + i); //Offset
	}


	if (msync(mmap_hashtable, (sizeof(uint32_t) + sizeof(size_t)) * cur_tokens, MS_SYNC) == -1) {
		printf("ERROR WRITING FROM MMAP!\n");
		return 1;
	}


	for (int i = 0; i < cur_tokens; i++) {
		*(mmap_inverted_index_pos + *(hash_offset + i)) = *(hash_freq + i);
		*(mmap_inverted_index_pos + *(hash_offset + i) + 1) = *(hash_doc_qty + i);



		*(doc_ids_offset + i) = (*(hash_offset + i)) + 2;
		*(entries_offset + i) = *(doc_ids_offset + i) + *(hash_doc_qty + i);
		*(pos_offset + i) = *(entries_offset + i) + *(hash_doc_qty + i);



		if (!(i % 1000000)) printf("WRITING TO OFFSETS!\n");
	}
	free(hash_offset);
	free(hash_freq);
	free(hash_doc_qty);


	rewind(fin);

	size_t cur_hash = 0;

	while (fread(&doc_id, sizeof(uint32_t), 1, fin) != 0) {
		temp_qty = 0;

		fread_val = fread(&title_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread(temp_doc_hashes, sizeof(uint32_t), title_qty, fin);
		temp_qty += title_qty;


		fread_val = fread(&text_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread((temp_doc_hashes + temp_qty), sizeof(uint32_t), text_qty, fin);
		temp_qty += text_qty;

		fread_val = fread(&cat_qty, sizeof(uint32_t), 1, fin);
		fread_val = fread((temp_doc_hashes + temp_qty), sizeof(uint32_t), cat_qty, fin);
		temp_qty += cat_qty;



		for (int i = 0; i < temp_qty; i++) {
			hash_off_ptr = bsearch((temp_doc_hashes + i), all_unique_tokens, cur_tokens, sizeof(uint32_t), cmp_func);

			cur_hash = hash_off_ptr - all_unique_tokens;

			*(mmap_inverted_index_pos + *(doc_ids_offset + cur_hash)) = doc_id;
			(*(mmap_inverted_index_pos + *(entries_offset + cur_hash)))++;
			*(mmap_inverted_index_pos + *(pos_offset + cur_hash)) = i;
			(*(pos_offset + cur_hash))++;


		}

		qsort(temp_doc_hashes, temp_qty, sizeof(uint32_t), cmp_func);
		for (int i = 0; i < temp_qty; i++) {
			hash_off_ptr = bsearch((temp_doc_hashes + i), all_unique_tokens, cur_tokens, sizeof(uint32_t), cmp_func);

			cur_hash = hash_off_ptr - all_unique_tokens;

			(*(doc_ids_offset + cur_hash))++;
			(*(entries_offset + cur_hash))++;

			while (*(hash_off_ptr) == *(temp_doc_hashes + i)) {
				i++;
			}
			i--;
		}

		if (!(doc_id % 10000))printf("DOC %d SERVED!\n", doc_id);
	}

	if (fread_val == 0) printf("THIS SHOULDN'T HAPPEN!\n");

	printf("DONE WRITING TO MMAP!\n");

	if (msync(mmap_inverted_index_pos, file_size, MS_SYNC) == -1) {
		printf("ERROR WRITING FROM MMAP!\n");
		return 1;
	}

	fclose(fin);

	return 1;
}