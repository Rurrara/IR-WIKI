#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "Utf8Func.h"
#include "hash.h"
#include "stemming.h"

#define _FILE_OFFSET_BITS 64




int addToWriteBuffer(uint8_t* str, uint8_t* sep, uint8_t* w_buf, uint32_t* cur_len) {

	uint8_t* ptr;
	uint32_t cnt = 0, init_len = *cur_len;
	(*cur_len) += sizeof(uint32_t);
	strToLowerUtf8(str);
	ptr = (uint8_t*)strtok((char*)str, (char*)sep);
	while (ptr) {
		Lemmatization(ptr);
		*(uint32_t*)(w_buf + *cur_len) = Jenkins_hash(ptr);
		(*cur_len) += sizeof(uint32_t);
		cnt++;
		ptr = (uint8_t*)strtok(NULL, (char*)sep);
	}

	*(uint32_t*)(w_buf + init_len) = cnt;
	return cnt;
}


int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("NEED TO SPECIFY INPUT WIKI FILE!\n");
		return -1;
	}

	FILE* fin, * f_doc_text, * f_doc_processed, * f_doc_table;
	fin = fopen(argv[1], "r");
	if (fin == NULL) {
		printf("ERROR OPENING FILE111!\n");
		return 0;
	}

	f_doc_text = fopen("doc_text.bin", "wb");
	if (fin == NULL) {
		printf("ERROR CREATING FILE111!\n");
		return 0;
	}

	f_doc_processed = fopen("doc_processed.bin", "wb");
	if (f_doc_processed == NULL) {
		printf("ERROR CREATING FILE111!\n");
		return 0;
	}

	f_doc_table = fopen("doc_table.bin", "wb");
	if (f_doc_table == NULL) {
		printf("ERROR CREATING FILE111!\n");
		return 0;
	}


	const uint32_t MAX_ALLOCATION_SIZE = 4096 * 1024;

	const uint32_t WRITE_BUFFER_SIZE = 4096 * 1024 * 128;
	uint8_t* write_buffer;

	write_buffer = calloc(WRITE_BUFFER_SIZE, sizeof(uint8_t));
	uint32_t write_buffer_cur_len = 0;

	uint8_t* ptr, * ptr2, * ptr_wiki, sep[] = " ,.!?():;\"";



	uint8_t* buffer;
	buffer = calloc(MAX_ALLOCATION_SIZE, sizeof(uint8_t));
	uint8_t* title, * text, * category;


	title = calloc(MAX_ALLOCATION_SIZE, sizeof(uint8_t));
	text = calloc(MAX_ALLOCATION_SIZE, sizeof(uint8_t));
	category = calloc(MAX_ALLOCATION_SIZE, sizeof(uint8_t));


	uint32_t doc_id = 0;

	size_t cur_offset = 0;


	uint32_t title_tkn_qty = 0, text_tkn_qty = 0, cat_tkn_qty = 0, total_text_size = 0, title_len = 0, text_len = 0, cat_len = 0;

	while (fgets((char*)buffer, MAX_ALLOCATION_SIZE, fin)) {

		ptr_wiki = (uint8_t*)strstr((char*)buffer, "\"wiki\"");
		if (ptr_wiki != NULL) {

			ptr = (uint8_t*)strstr((char*)ptr_wiki, "\"title\":");
			if (ptr != NULL) {
				ptr2 = (uint8_t*)strstr((char*)ptr, "\",");
				if (ptr2 != NULL) {
					strncpy((char*)title, (char*)ptr + 8, ptr2 - ptr - 7);
					*(title + (ptr2 - ptr - 7)) = 0x00;
				}

			}
			else *title = 0x00;



			ptr = (uint8_t*)strstr((char*)buffer, ",\"text\":");
			if (ptr != NULL) {
				ptr2 = (uint8_t*)strstr((char*)ptr, "\",");
				strncpy((char*)text, (char*)ptr + 8, ptr2 - ptr - 7);
				*(text + (ptr2 - ptr - 7)) = 0x00;

			}
			else *text = 0x00;


			ptr = (uint8_t*)strstr((char*)buffer, "\"category\":[");
			if (ptr != NULL) {
				ptr2 = (uint8_t*)strstr((char*)ptr, "]");
				if (ptr2 != NULL) {
					strncpy((char*)category, (char*)ptr + 12, ptr2 - ptr - 12);
					*(category + (ptr2 - ptr - 12)) = 0x00;
				}
			}
			else *category = 0x00;


			if (*text == 0x00 || *title == 0x00) { printf("MISSING BODY OR TITLE!\n"); }
			else {

				if (!(doc_id%10000))printf("TOTAL=%d\n", doc_id);
				StrToUtf8(title, title);
				StrToUtf8(text, text);
				StrToUtf8(category, category);

				title_len = strlen((char*)title);
				text_len = strlen((char*)text);
				cat_len = strlen((char*)category);
				fwrite(title, sizeof(uint8_t), title_len, f_doc_text);
				fwrite(text, sizeof(uint8_t), text_len, f_doc_text);
				fwrite(category, sizeof(uint8_t), cat_len, f_doc_text);
				total_text_size = title_len + text_len + cat_len;




				if (write_buffer_cur_len > WRITE_BUFFER_SIZE * 0.8) {
					printf("WRITING TO FILE!\n");
					fwrite(write_buffer, sizeof(uint8_t), write_buffer_cur_len, f_doc_processed);
					write_buffer_cur_len = 0;
				}



				*(uint32_t*)(write_buffer + write_buffer_cur_len) = doc_id;
				write_buffer_cur_len += sizeof(uint32_t);




				title_tkn_qty = addToWriteBuffer(title, sep, write_buffer, &write_buffer_cur_len);
				text_tkn_qty = addToWriteBuffer(text, sep, write_buffer, &write_buffer_cur_len);
				cat_tkn_qty = addToWriteBuffer(category, sep, write_buffer, &write_buffer_cur_len);

				fwrite(&doc_id, sizeof(uint32_t), 1, f_doc_table);
				fwrite(&title_tkn_qty, sizeof(uint32_t), 1, f_doc_table);
				fwrite(&text_tkn_qty, sizeof(uint32_t), 1, f_doc_table);
				fwrite(&cat_tkn_qty, sizeof(uint32_t), 1, f_doc_table);
				fwrite(&total_text_size, sizeof(uint32_t), 1, f_doc_table);
				fwrite(&cur_offset, sizeof(size_t), 1, f_doc_table);
				fwrite(&title_len, sizeof(uint32_t), 1, f_doc_table);
				fwrite(&text_len, sizeof(uint32_t), 1, f_doc_table);
				fwrite(&cat_len, sizeof(uint32_t), 1, f_doc_table);

				cur_offset += total_text_size;

				doc_id++;

			}

		}
	}
	fwrite(write_buffer, sizeof(uint8_t), write_buffer_cur_len, f_doc_processed);


	free(write_buffer);
	free(buffer);
	fclose(f_doc_processed);
	fclose(f_doc_table);
	fclose(fin);
	fclose(f_doc_text);

	return 1;
}
