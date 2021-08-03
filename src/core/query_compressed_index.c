#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <math.h>
#include <time.h>


#include "Utf8Func.h"
#include "hash.h"
#include "parse_expr.h"
#include "mmap_utils.h"
#include "compression.h"
#include "stemming.h"

#define _FILE_OFFSET_BITS 64

int cmp_func(const void* v1, const void* v2) {

	if (*(uint32_t*)v1 > * (uint32_t*)v2) return 1;
	if (*(uint32_t*)v1 < *(uint32_t*)v2) return -1;
	return 0;
}



typedef struct {
	uint32_t doc_id;
	uint32_t title_qty;
	uint32_t text_qty;
	uint32_t cat_qty;
	uint32_t total_qty;
	uint32_t text_bytes;
	size_t offset_doc_text;
	uint32_t title_len;
	uint32_t text_len;
	uint32_t cat_len;
	double score;
	double pos_weight;
} doc_table_entry;


typedef struct {
	uint8_t token[256];
	uint32_t hash;
	uint32_t freq;
	uint32_t doc_qty;
	uint32_t* doc_ids;
	uint32_t* entries_in_doc;
	uint32_t* positions;
	uint32_t* pos_offset;
} hash_docs_and_qty;

typedef struct {
	uint8_t b_free;
	uint32_t negotiation;
	uint32_t qty;
	uint32_t freq;
	uint32_t* doc_ids;
	uint32_t* pos_offset;
	uint32_t* positions;
	uint32_t* entries_in_doc;
	uint32_t* dist;
} resultDocs;




int dte_cmp_func(const void* v1, const void* v2) {

	if ((*(doc_table_entry*)v1).score > (*(doc_table_entry*)v2).score) return -1;
	if ((*(doc_table_entry*)v1).score < (*(doc_table_entry*)v2).score) return 1;
	return 0;
}


int allocResDoc(resultDocs* rd, uint32_t qty, uint32_t freq) {

	rd->doc_ids = calloc(qty, sizeof(uint32_t));
	rd->pos_offset = calloc(qty, sizeof(uint32_t));
	rd->positions = calloc(freq, sizeof(uint32_t));
	rd->dist = calloc(freq, sizeof(uint32_t));
	rd->entries_in_doc = calloc(qty, sizeof(uint32_t));
	return 1;
}


int fetchDTE(doc_table_entry* dte, uint32_t* offset) {

	dte->doc_id = *(uint32_t*)(offset);
	dte->title_qty = *(uint32_t*)(offset+1);
	dte->text_qty = *(uint32_t*)(offset+2);
	dte->cat_qty = *(uint32_t*)(offset+3);
	dte->text_bytes = *(uint32_t*)(offset+4);
	dte->offset_doc_text = *(size_t*)(offset+5);
	dte->title_len = *(uint32_t*)(offset+7);
	dte->text_len = *(uint32_t*)(offset+8);
	dte->cat_len = *(uint32_t*)(offset+9);
	dte->total_qty = dte->title_qty + dte->text_qty + dte->cat_qty;
	return 1;
}


int initializeStruct(hash_docs_and_qty* st, uint8_t* tkn, uint32_t hash, uint8_t* mmap_ptr, size_t offset) {

	uint32_t cur_len=0;

	st->hash = hash;
	memcpy(st->token, tkn, strlen((char*)tkn));

	cur_len += decompress_uint32((mmap_ptr + offset), &st->freq);
	cur_len += decompress_uint32((mmap_ptr + offset+cur_len), &st->doc_qty);



	st->doc_ids = calloc(st->doc_qty, sizeof(uint32_t));
	st->entries_in_doc = calloc(st->doc_qty, sizeof(uint32_t));
	st->positions = calloc(st->freq, sizeof(uint32_t));

	for (int i = 0; i != st->doc_qty; i++) {
		cur_len += decompress_uint32((mmap_ptr + offset + cur_len), &st->doc_ids[i]);
	}

	for (int i = 0; i != st->doc_qty; i++) {
		cur_len += decompress_uint32((mmap_ptr + offset + cur_len), &st->entries_in_doc[i]);
	}

	for (int i=0; i!=st->freq; i++) cur_len+= decompress_uint32((mmap_ptr + offset + cur_len), &st->positions[i]);

	st->pos_offset = calloc(st->doc_qty, sizeof(uint32_t));
	for (int i = 1; i != st->doc_qty; i++) st->pos_offset[i] = st->pos_offset[i - 1] + st->entries_in_doc[i - 1];

	return 1;

}

resultDocs DocsOr(resultDocs* d1, resultDocs* d2, uint32_t neg) {
	resultDocs res = {0};
	res.doc_ids = calloc(d1->qty + d2->qty, sizeof(uint32_t));
	res.negotiation = neg;
	uint32_t temp_qty = 0;

	res.b_free = 1;


	uint32_t i = 0, j = 0;

	while (i < d1->qty) {
		if (j == d2->qty) {
			break;
		}
		if (d1->doc_ids[i] == d2->doc_ids[j]) {
			res.doc_ids[temp_qty++] = d1->doc_ids[i];
			i++;
			j++;

		}
		else if (d2->doc_ids[j] < d1->doc_ids[i]) {
			res.doc_ids[temp_qty++] = d2->doc_ids[j];
			j++;
		}
		else
		{
			res.doc_ids[temp_qty++] = d1->doc_ids[i];
			i++;
		}
	}
	while (j < d2->qty)
	{
		res.doc_ids[temp_qty++] = d2->doc_ids[j];
		j++;
	}
	while (i < d1->qty)
	{
		res.doc_ids[temp_qty++] = d1->doc_ids[i];
		i++;
	}
	res.qty = temp_qty;
	return res;
}

resultDocs DocsAnd(resultDocs* d1, resultDocs* d2, uint32_t neg) {
	resultDocs res = {0}, *ptr_l_qty, * ptr_g_qty;
	res.negotiation = neg;
	uint32_t temp_qty = 0;

	res.b_free = 1;

	ptr_l_qty = d1;
	ptr_g_qty = d2;

	if (d1->qty > d2->qty) {
		ptr_l_qty = d2;
		ptr_g_qty = d1;
	}

	res.doc_ids = calloc(ptr_l_qty->qty, sizeof(uint32_t));
	uint32_t i = 0, j = 0;

	while (i < ptr_l_qty->qty){

		if (j == ptr_g_qty->qty) {
			break;
		}

		if (ptr_l_qty->doc_ids[i] == ptr_g_qty->doc_ids[j]) {
			res.doc_ids[temp_qty++] = ptr_l_qty->doc_ids[i];
			i++;
			j++;
		}
		else if (ptr_g_qty->doc_ids[j] < ptr_l_qty->doc_ids[i]) {
			j++;
		}
		else {
			i++;
		}
	}

	res.qty = temp_qty;

	return res;
}

resultDocs DocsRem(resultDocs* d1, resultDocs* d2, uint32_t neg) {
	resultDocs res = {0}, *ptr_rem_from, *ptr_rem_what, *temp_rd;
	uint32_t temp_qty = 0;

	res.b_free = 1;

	res.negotiation = neg;

	ptr_rem_from = d1;
	ptr_rem_what = d2;

	if (d1->negotiation) {
		ptr_rem_from = d2;
		ptr_rem_what = d1;
	}
	if (neg) {
		temp_rd = ptr_rem_from;
		ptr_rem_from = ptr_rem_what;
		ptr_rem_what = temp_rd;
	}


	res.doc_ids = calloc(ptr_rem_from->qty, sizeof(uint32_t));

	uint32_t i = 0, j = 0;

	while (i < ptr_rem_from->qty) {

		if (j == ptr_rem_what->qty) {
			while (i<ptr_rem_from->qty)
			{
				res.doc_ids[temp_qty++] = ptr_rem_from->doc_ids[i];
				i++;
			}
			break;
		}

		if (ptr_rem_from->doc_ids[i] == ptr_rem_what->doc_ids[j]) {
			i++;
			j++;
		}
		else if (ptr_rem_what->doc_ids[j] < ptr_rem_from->doc_ids[i]) {
			j++;
		}
		else {
			res.doc_ids[temp_qty++] = ptr_rem_from->doc_ids[i];
			i++;
		}
	}
	res.qty = temp_qty;


	return res;
}

resultDocs DocsAndWithDist(resultDocs* d1, resultDocs* d2, uint32_t dist) {
	resultDocs res = { 0 };
	uint32_t temp_qty = 0, temp_total_pos=0, temp_entries=0;

	allocResDoc(&res, d1->qty, d1->freq);
	uint32_t i = 0, j = 0;

	while (i < d1->qty) {

		if (j == d2->qty) {
			break;
		}

		if (d1->doc_ids[i] == d2->doc_ids[j]) {
			uint32_t bflag = 0;
			temp_entries = 0;

			uint32_t k = 0, l = 0;

			while (k < d1->entries_in_doc[i]) {
				if (l == d2->entries_in_doc[j]) break;

				if (d2->positions[d2->pos_offset[j] + l] <= d1->positions[d1->pos_offset[i] + k]) l++;
				else if (
					((d2->positions[d2->pos_offset[j] + l] - d1->positions[d1->pos_offset[i] + k]) <= dist) &&
					((d2->positions[d2->pos_offset[j] + l] - d1->positions[d1->pos_offset[i] + k]) > d1->dist[d1->pos_offset[i] + k])
					) {
					bflag = 1;
					res.doc_ids[temp_qty] = d1->doc_ids[i];
					res.positions[temp_total_pos] = d1->positions[d1->pos_offset[i] + k];
					res.dist[temp_total_pos++] = (d2->positions[d2->pos_offset[j] + l] - d1->positions[d1->pos_offset[i] + k]);
					temp_entries++;

					l++;
				}
				else k++;
			}


			if (bflag) {

				res.entries_in_doc[temp_qty]=temp_entries;
				temp_qty++;
				res.pos_offset[temp_qty] = temp_total_pos;

			}

			i++;
			j++;
		}
		else if (d2->doc_ids[j] < d1->doc_ids[i]) {
			j++;
		}
		else {
			i++;
		}
	}

	res.qty = temp_qty;
	res.freq = temp_total_pos;

	return res;
}

resultDocs getCitation(resultDocs* rd_arr, uint32_t arr_len, uint32_t dist) {

	resultDocs res = {0};
	if (dist == 0) dist = arr_len;

	if (dist == 0) {
		res = rd_arr[0];
		return res;
	}

	if (dist < arr_len) return res;

	allocResDoc(&res, rd_arr[0].qty, rd_arr[0].freq);
	res.qty = rd_arr[0].qty;
	res.freq = rd_arr[0].freq;
	memcpy(res.doc_ids, rd_arr[0].doc_ids, sizeof(uint32_t) * res.qty);
	memcpy(res.entries_in_doc, rd_arr[0].entries_in_doc, sizeof(uint32_t) * res.qty);
	memcpy(res.positions, rd_arr[0].positions, sizeof(uint32_t) * res.freq);
	memcpy(res.pos_offset, rd_arr[0].pos_offset, sizeof(uint32_t) * res.qty);




	for (int i = 1; i != arr_len; i++) {
		res = DocsAndWithDist(&res, &rd_arr[i], dist);
	}
	return res;

}

resultDocs getDocs(uint8_t* tkn, hash_docs_and_qty* hdq_arr, uint32_t* unique_cnt,  uint8_t* ii_mmap_ptr, uint32_t* ht_mmap_ptr, size_t ht_size) {

	uint32_t* ht_off_ptr, temp_hash;
	size_t ii_off_ptr = 0;

	resultDocs res = { 0 };

	strToLowerUtf8(tkn);
	Lemmatization(tkn);
	temp_hash = Jenkins_hash(tkn);
	ht_off_ptr = bsearch(&temp_hash, ht_mmap_ptr, ht_size / 12, 12, cmp_func);

	if (ht_off_ptr != NULL) {

		for (int i = 0; i < *unique_cnt; i++) {
			if (temp_hash == hdq_arr[i].hash) {

				res.doc_ids = hdq_arr[i].doc_ids;
				res.qty= hdq_arr[i].doc_qty;
				res.freq = hdq_arr[i].freq;
				res.positions = hdq_arr[i].positions;
				res.pos_offset = hdq_arr[i].pos_offset;
				res.entries_in_doc = hdq_arr[i].entries_in_doc;

				return res;
			}
		}

		ii_off_ptr = *(size_t*)(ht_off_ptr + 1);
		initializeStruct((hdq_arr + *unique_cnt), tkn, temp_hash, ii_mmap_ptr, ii_off_ptr);

		res.doc_ids = hdq_arr[*unique_cnt].doc_ids;
		res.qty = hdq_arr[*unique_cnt].doc_qty;
		res.freq = hdq_arr[*unique_cnt].freq;
		res.positions = hdq_arr[*unique_cnt].positions;
		res.pos_offset = hdq_arr[*unique_cnt].pos_offset;
		res.entries_in_doc = hdq_arr[*unique_cnt].entries_in_doc;
		(*unique_cnt)++;

		return res;

	}
	return res;

}

#define is_operator(c)  (c == '!' || c == '&' || c == '|')

resultDocs getAnswer(uint8_t* expr, hash_docs_and_qty* hdq_arr, uint32_t* unique_cnt, uint8_t* ii_mmap_ptr, uint32_t* ht_mmap_ptr, size_t ht_size) {

	resultDocs res = { 0 };

	uint8_t* ptr = expr, *ptr2, *temp_buf;


	resultDocs *rd_arr, temp_rd;
	rd_arr = calloc(32, sizeof(resultDocs));


	uint32_t stack_length = 0;


	temp_buf = calloc(4096, sizeof(uint8_t));

	while (*ptr && ptr) {
		if (is_operator(*ptr)) {
			//printf("%c\n", *ptr);


			switch (*ptr)
			{
			case '!':
				rd_arr[stack_length - 1].negotiation = !(rd_arr[stack_length - 1].negotiation);
				break;

			case '&':

				if (rd_arr[stack_length - 1].negotiation == rd_arr[stack_length - 2].negotiation) {

					if (rd_arr[stack_length - 1].negotiation == 0) {
						//AND (P & Q)

						temp_rd = DocsAnd(&rd_arr[stack_length - 1], &rd_arr[stack_length - 2], 0);
					}
					else
					{
						//OR (~P & ~Q) -> ~(P | Q)
						temp_rd = DocsOr(&rd_arr[stack_length - 1], &rd_arr[stack_length - 2], 1);
					}

				}
				else {
					//REMOVE FROM NEG0 NEG1 (~P & Q)  --/--  (P & ~Q)
					temp_rd = DocsRem(&rd_arr[stack_length - 1], &rd_arr[stack_length - 2], 0);

				}
				//FREEING
				for (int i = 0; i != 2; i++) {
					if (rd_arr[stack_length - 1 - i].b_free) free(rd_arr[stack_length - 1 - i].doc_ids);

				}
				rd_arr[stack_length - 2] = temp_rd;
				stack_length--;


				break;

			case '|':

				if (rd_arr[stack_length - 1].negotiation == rd_arr[stack_length - 2].negotiation) {

					if (rd_arr[stack_length - 1].negotiation == 0) {

						//OR (P | Q)

						temp_rd = DocsOr(&rd_arr[stack_length - 1], &rd_arr[stack_length - 2], 0);
					}
					else
					{
						//AND (~P | ~Q) -> ~(P & Q)
						temp_rd = DocsAnd(&rd_arr[stack_length - 1], &rd_arr[stack_length - 2], 1);
					}

				}
				else {
					//REMOVE FROM NEG1 NEG0 (~Q | P) -> ~(Q & ~P)  --/-- (Q | ~P) -> ~(~Q & P)
					temp_rd = DocsRem(&rd_arr[stack_length - 1], &rd_arr[stack_length - 2], 1);

				}
				//FREEING
				for (int i = 0; i != 2; i++) {
					if (rd_arr[stack_length - 1 - i].b_free) free(rd_arr[stack_length - 1 - i].doc_ids);
				}
				rd_arr[stack_length - 2] = temp_rd;
				stack_length--;
				break;

			default:
				break;
			}

		}

		//CITATION
		else if (*ptr == '\"') {
			uint32_t dist = 0;
			ptr2 = (uint8_t*)strchr((char*)ptr + 1, '\"');
			memcpy(temp_buf, ptr+1, ptr2 - ptr-1);
			*(temp_buf + (ptr2 - ptr)) = 0x00;
			if (*(ptr2 + 1) == '/') {
				dist = strtoul((char*)ptr2+2, (char**)&ptr, 10);
			}
			else ptr = ptr2;



			uint32_t cit_term_qty=0;
			uint8_t cit_tokens[50][256], *str_ptr;
			str_ptr = (uint8_t*)strtok((char*)temp_buf, " ");
			while (str_ptr)
			{
				strcpy((char*)cit_tokens[cit_term_qty++], (char*)str_ptr);
				str_ptr = (uint8_t*)strtok(NULL, " ");
			}

			resultDocs* cit_rd_arr;
			cit_rd_arr = calloc(cit_term_qty, sizeof(resultDocs));
			for (int i = 0; i != cit_term_qty; i++) {
				cit_rd_arr[i] = getDocs(cit_tokens[i], hdq_arr, unique_cnt, ii_mmap_ptr, ht_mmap_ptr, ht_size);
			}

			rd_arr[stack_length++] = getCitation(cit_rd_arr, cit_term_qty, dist);

			
		}
		else if (*ptr != ' ') {
			ptr2 = (uint8_t*)strchr((char*)ptr, ' ');
			if (ptr2 == NULL) ptr2 = (uint8_t*)strchr((char*)ptr, '\0');
			memcpy(temp_buf, ptr, ptr2 - ptr);
			*(temp_buf + (ptr2 - ptr)) = 0x00;
			ptr = ptr2;

			rd_arr[stack_length++] = getDocs(temp_buf, hdq_arr, unique_cnt, ii_mmap_ptr, ht_mmap_ptr, ht_size);


			//printf("%d STACK = %d\n", rd_arr[0].negotiation, stack_length);
			//stack_length++;
		}
		ptr++;
	}


	if (stack_length != 0) {
		res = rd_arr[0];
	}

	free(rd_arr);

	return res;
}

int docScoring(resultDocs* docs, doc_table_entry* dte_arr, uint32_t* doc_table, uint32_t total_doc_qty, hash_docs_and_qty* hdq_arr, uint32_t unique_tokens_qty) {

	int res = docs->qty;
	uint32_t* index_ptr, *doc_table_offset;
	for (int i = 0; i != docs->qty; i++) {
		doc_table_offset = bsearch(&docs->doc_ids[i], doc_table, total_doc_qty, sizeof(uint32_t) * 10, cmp_func);
		if (doc_table_offset == NULL) {
			printf("Не найден такой документ!\n");
			return -1;
		}
		fetchDTE(&dte_arr[i], doc_table_offset);

		for (int j = 0; j != unique_tokens_qty; j++) {

			index_ptr = bsearch(&dte_arr[i].doc_id, &hdq_arr[j].doc_ids[0], hdq_arr[j].doc_qty, sizeof(uint32_t), cmp_func);
			if (index_ptr != NULL) {
				uint32_t index = index_ptr - &hdq_arr[j].doc_ids[0];
				//TF-IDF
				dte_arr[i].score += (hdq_arr[j].entries_in_doc[index] * log(total_doc_qty / (hdq_arr[j].doc_qty)));
				uint32_t entries_in_doc = hdq_arr[j].entries_in_doc[index];


				//POSITION WEIGHTING!
				if (entries_in_doc > 1) {
					if (hdq_arr[j].positions[hdq_arr[j].pos_offset[index]] < dte_arr[i].title_qty) dte_arr[i].pos_weight += 0.5;
					if (hdq_arr[j].positions[hdq_arr[j].pos_offset[index] + 1] < (dte_arr[i].title_qty + dte_arr[i].text_qty)) dte_arr[i].pos_weight += 0.35;
					if (hdq_arr[j].positions[hdq_arr[j].pos_offset[index] + entries_in_doc - 1] > (dte_arr[i].title_qty + dte_arr[i].text_qty)) dte_arr[i].pos_weight += 0.15;

				}
				else {
					if (hdq_arr[j].positions[hdq_arr[j].pos_offset[index]] < dte_arr[i].title_qty) dte_arr[i].pos_weight += 0.5;
					else if (hdq_arr[j].positions[hdq_arr[j].pos_offset[index]] > (dte_arr[i].title_qty + dte_arr[i].text_qty - 1)) dte_arr[i].pos_weight += 0.15;
					else dte_arr[i].pos_weight += 0.35;
				}

			}

		}
	}

	//Окончательная оценка
	for (int i = 0; i != docs->qty; i++) {
		dte_arr[i].score = ((dte_arr[i].score / dte_arr[i].text_bytes) + dte_arr[i].pos_weight);
	}


	//Освобождение памяти
	if (docs->b_free) {
		free(docs->doc_ids);
		free(docs->entries_in_doc);
		free(docs->positions);
		free(docs->pos_offset);
	}
	for (int i = 0; i != unique_tokens_qty; i++) {
		free(hdq_arr[i].doc_ids);
		free(hdq_arr[i].entries_in_doc);
		free(hdq_arr[i].positions);
		free(hdq_arr[i].pos_offset);
	}

	qsort(&dte_arr[0], docs->qty, sizeof(doc_table_entry), dte_cmp_func);

	return res;
}


uint8_t* makeSnippet(uint8_t* str, hash_docs_and_qty* hdq_arr, uint32_t unique_tokens_qty) {

	uint8_t *ptr, *str1, *str2, *ptr_rem, *first_str, str_temp[4096];
	strToLowerUtf8(str);
	memcpy(str_temp, str, 4096);
	ptr = (uint8_t*)strtok((char*)str, ".");
	str1 = ptr+1;
	first_str = ptr+1;
	uint32_t entries_max=0, entries_cur=0;

	ptr_rem = (uint8_t*)strpbrk((char*)first_str, "(");
	if (ptr_rem != NULL) *ptr_rem = ' ';

	for (int i = 0; i != unique_tokens_qty; i++) {
		if (strstr((char*)first_str, (char*)hdq_arr->token) != NULL) entries_max++;
	}

	while (ptr) {
		entries_cur = 0;
		for (int i = 0; i != unique_tokens_qty; i++) {
			if (strstr((char*)ptr, (char*)hdq_arr->token) != NULL) entries_cur++;
		}
		if (entries_cur > entries_max) {
			str1 = ptr;
			entries_max = entries_cur;
		}

		ptr = (uint8_t*)strtok(NULL, ".");
	}
	if (first_str != str1) {
		strcat((char*)first_str, (char*)str1);
		return first_str;
	}

	str2 = (uint8_t*)strtok((char*)str_temp, ".");
	str2 = (uint8_t*)strtok(NULL, ".");
	if (str2!=NULL) strcat((char*)first_str, (char*)str2);
	return first_str;


}

int main(int argc, char* argv[]) {

	clock_t start_clk, result_clk, sort_clk, output_clock;
	start_clk = clock();

	uint32_t* hashtable, *doc_table;
	uint8_t* inverted_index, *doc_text;
	size_t ht_size, ii_size, dt_size, dtext_size;
	hashtable = (uint32_t*)openFilemmap("hashtable3.bin", &ht_size);
	if (hashtable == NULL) {
		printf("ERROR!\n");
		return 0;
	}
	inverted_index = (uint8_t*)openFilemmap("compressed_index.bin", &ii_size);
	if (inverted_index == NULL) {
		printf("ERROR!\n");
		return 0;
	}

	doc_table = (uint32_t*)openFilemmap("doc_table.bin", &dt_size);
	if (doc_table == NULL) {
		printf("ERROR!\n");
		return 0;
	}

	doc_text = (uint8_t*)openFilemmap("doc_text.bin", &dtext_size);
	if (doc_text == NULL) {
		printf("ERROR!\n");
		return 0;
	}

	//printf("%ld %ld\n", ht_size, ii_size);


	if (argc < 2) {
		printf("YOU NEED TO SPECIFY QUERY!\n");
		return 0;
	}

	int page_num = 0;

	if (argc == 3) {
		page_num = atoi(argv[2]);
	}

	//SHOULD REPLACE WITH ACTUAL SIZE!!
	const uint32_t total_doc_qty = dt_size/40;

	hash_docs_and_qty* hdq_arr;
	hdq_arr = calloc(32, sizeof(hash_docs_and_qty));

	uint32_t unique_tokens_qty = 0;
	resultDocs docs;

	uint8_t post_fix[8192] = {0};
	if (!ParsExpression((uint8_t*)argv[1], &post_fix[0])) return 1;
	//printf("%s\n", qqq);
	docs=getAnswer(&post_fix[0], hdq_arr, &unique_tokens_qty, inverted_index, hashtable, ht_size);
	if (docs.negotiation) {
		resultDocs allDocs = { 0 };
		allDocs.qty = total_doc_qty;
		allDocs.doc_ids = calloc(total_doc_qty, sizeof(uint32_t));
		for (int i = 0; i != total_doc_qty; i++) {
			allDocs.doc_ids[i] = i;
		}
		docs = DocsRem(&docs, &allDocs, 0);
		free(allDocs.doc_ids);
	}

	result_clk = clock();
	printf("Документов, удовлетворяющих запросу: %d\n", docs.qty);

	doc_table_entry* dte_arr;
	dte_arr = calloc(docs.qty, sizeof(doc_table_entry));
	uint8_t text_title[4096] = {0};
	size_t offset_temp = 0;
	uint32_t title_len = 0;


	uint32_t res_doc_qty=docScoring(&docs, dte_arr, doc_table, total_doc_qty, hdq_arr, unique_tokens_qty);
	sort_clk = clock();
	if (res_doc_qty == -1) {
		printf("Один из файлов поврежден!\n");
		return 1;
	}

	if (res_doc_qty == 0) {
		printf("Нет документов, удовлетворяющих запросу!\n");
		return 1;
	}


	printf("Поиск документов занял: %Lf секунд\n", (long double)(result_clk - start_clk) / CLOCKS_PER_SEC);
	printf("Сортировка заняла: %Lf секунд\n", (long double)(sort_clk - result_clk) / CLOCKS_PER_SEC);
	uint8_t* body_text;
	body_text = calloc(4096 * 4096, sizeof(uint8_t));

	uint32_t docs_per_page = 50;
	uint32_t start_pos = docs_per_page * page_num;
	uint32_t cur_doc = start_pos;

	while ((cur_doc < res_doc_qty) && (cur_doc < (start_pos+docs_per_page)))
	{
		offset_temp = dte_arr[cur_doc].offset_doc_text;
		title_len = dte_arr[cur_doc].title_len;
		memcpy(&text_title, (doc_text + offset_temp), title_len);
		memcpy(body_text, (doc_text + offset_temp + title_len), dte_arr[cur_doc].text_len);
		*(body_text + dte_arr[cur_doc].text_len) = 0x00;
		*(text_title + title_len) = 0x00;
		body_text = makeSnippet(body_text, hdq_arr, unique_tokens_qty);

		printf("%f\n", dte_arr[cur_doc].score);
		printf("%d\n", dte_arr[cur_doc].doc_id);
		printf("%s\n", text_title);
		printf("%s\n", body_text);
		cur_doc++;
	}
	output_clock = clock();
	printf("Вывод занял: %Lf\n", (long double)(output_clock - sort_clk) / CLOCKS_PER_SEC);



	//getchar();

	return 1;

	
}