#include "stemming.h"


uint8_t ADJ[26][7] = {
"ими", "ыми", "его", "ого", "ему",
"ому", "ее", "ие", "ые", "ое", "ей",
"ый", "ой", "ем", "им", "ым", "ом", 
"ий", "их", "ых", "ую", "юю",
"ая", "яя", "ою", "ею"
};
int ADJ_len = 26;

uint8_t PRT[15][7] = {
"аем", "анн", "авш", "ающ", "ивш", "ывш", "ующ",
"яем", "янн", "явш", "яющ", "ящ",
"ивш", "ывш", "ующ"

};
int PRT_len = 15;

uint8_t PG[12][14] = {
"авшись", "ившись", "явшись", "ывшись",  "явши",
 "ивши",  "ывши", "авши", "ав",  "яв", "ив", "ыв"
};
int PG_len = 12;

uint8_t REF[2][5] = {
"ся", "сь"
};
int REF_len = 2;

uint8_t VERB[63][9] = {
"аешь", "аете", "айте", "анно", "яете", 
"яйте", "уйте", "яешь", "янно", "ала", "ана",
"али", "аем",  "ало", "ано", "ает", "ают",
"аны", "ать",  "яла", "яна",  "яли", 
"яем",  "яло", "яно", "яет", "яют",
"яны", "ять", "ила", "ыла","ена", "ейте", "ите", "или",
"ыли", "ило", "ыло", "ено", "ует", "уют",
"ены", "ить","ыть", "ишь","ан", "ал", "ай",
"ял", "яй", "ян", "ей", "уй", "ил", "ыл",
"им", "ым", "ен", "ит", "ыт", "ят",  "ую", "ю"

};
int VERB_len = 63;

uint8_t NOUN[36][9] = {
"иями", "ями", "ами", "ией", "иям", "ием", "иях",
"ем", "ям", "ев", "ов", "ие", "ье",  "еи", "ии",
"ей", "ой", "ий",  "ию", "ью", "ия", "ья", "ах",
"ях",  "ам", "ом","а",  "е","и",  "й", "о", "у",
"ы","ь", "ю",  "я"
};
int NOUN_len = 36;

uint8_t SUPERL[2][9] = {
"ейш", "ейше"
};
int SUPERL_len = 2;

uint8_t DER[2][9] = {
"ость", "ост"
};
int DER_len = 2;

uint8_t* cmp_last(uint8_t* s, uint8_t* r) {
	int len_s, len_r;
	len_s = strlen((char*)s);
	len_r = strlen((char*)r);
	if (len_s < len_r) return NULL;
	uint8_t* ptr;
	s += len_s - len_r;
	ptr = s;

	while (*r)
	{
		if (*s++ != *r++) return NULL;
	}
	return ptr;
}


int Lemmatization(uint8_t* s) {

	int len = strlen((char*)s);
	uint8_t* ptr, * ptr_ext;
	ptr = s + len - 1;
	ptr_ext = ptr - 1;
	if (*ptr_ext == 0xd0 || *ptr_ext == 0xd1) {
	}
	else return 1;

	if (len <= 6) return 1;

	uint8_t flag = 0;

	for (int i = 0; i != PG_len; i++) {

		ptr = cmp_last(s, PG[i]);
		if (ptr != NULL) {
			*ptr = 0x00;
			flag = 1;
			break;
		}

	}

	if (!flag) {

		for (int i = 0; i != REF_len; i++) {

			ptr = cmp_last(s, REF[i]);
			if (ptr != NULL) {
				*ptr = 0x00;
				flag = 1;
				break;
			}
		}
	}

	if (!flag) {

		for (int i = 0; i != ADJ_len; i++) {

			ptr = cmp_last(s, ADJ[i]);
			if (ptr != NULL) {
				*ptr = 0x00;
				flag = 1;
				break;
			}
			if (flag) {
				for (int j = 0; j != PRT_len; j++) {
					ptr = cmp_last(s, ADJ[i]);
					if (ptr != NULL) {
						*ptr = 0x00;
						break;
					}
				}
			}
		}

	}


	if (!flag) {

		for (int i = 0; i != VERB_len; i++) {

			ptr = cmp_last(s, VERB[i]);
			if (ptr != NULL) {
				*ptr = 0x00;
				flag = 1;
				break;
			}
		}
	}

	if (!flag) {
		for (int i = 0; i != NOUN_len; i++) {

			ptr = cmp_last(s, NOUN[i]);
			if (ptr != NULL) {
				*ptr = 0x00;
				flag = 1;
				break;
			}
		}
	}

	ptr = cmp_last(s, (uint8_t*)&"и");
	if (ptr != NULL) *ptr = 0x00;


	for (int i = 0; i != DER_len; i++) {
		ptr = cmp_last(s, DER[i]);
		if (ptr != NULL) {
			*ptr = 0x00;
			break;
		}
	}

	for (int i = 0; i != SUPERL_len; i++) {
		ptr = cmp_last(s, SUPERL[i]);
		if (ptr != NULL) {
			*ptr = 0x00;
			break;
		}
	}

	ptr = cmp_last(s, (uint8_t*)&"нн");
	if (ptr != NULL) {
		*(ptr + 2) = 0x00;
		return 1;
	}

	ptr = cmp_last(s, (uint8_t*)&"ь");
	if (ptr != NULL) *ptr = 0x00;


	return 1;


}