#include "parse_expr.h"


int op_preced(const uint8_t c)
{
    switch (c) {
    case '|':
        return 3;
    case '&':
        return 2;
    case '!':
        return 1;
    }
    return 0;
}

int op_left_assoc(const uint8_t c)
{
    switch (c) {
        // left to right
    case '|': case '&':
        return 1;
        // right to left
    case '!':
        return 0;
    }
    return 0;
}


#define is_operator(c)  (c == '!' || c == '&' || c == '|')


int shunting_yard(const uint8_t* input, uint8_t* output) {

    const uint8_t* strpos = input, * strend = input + strlen((char*)input);
    uint8_t c, * outpos = output;
    const uint8_t* temp_ptr;


    uint8_t stack[32];
    uint32_t sl = 0;
    uint8_t sc;




    while (strpos < strend) {

        c = *strpos;

        if (is_operator(c)) {
            *(outpos++) = ' ';
            while (sl > 0) {
                sc = stack[sl - 1];
                if (is_operator(sc) &&
                    ((op_left_assoc(c) && (op_preced(c) >= op_preced(sc))) ||
                        (op_preced(c) > op_preced(sc)))) {

                    *(outpos++) = ' ';
                    *(outpos++) = sc;
                    *(outpos++) = ' ';
                    sl--;
                }
                else break;
            }
            stack[sl++] = c;
        }

        else if (c == '(') {
            stack[sl++] = c;
        }

        else if (c == ')') {
            int pmatch = 0;

            while (sl > 0)
            {
                sc = stack[sl - 1];
                if (sc == '(') {
                    pmatch = 1;
                    break;
                }
                else
                {
                    *(outpos++) = ' ';
                    *(outpos++) = sc;
                    sl--;
                }
            }
            if (!pmatch) {
                printf("Parentheses mismatched\n");
                return 0;
            }
            sl--;
        }
        else if (c == '\"') {
            int qmatch = 0;
            *(outpos++) = c;
            strpos++;
            while (strpos < strend)
            {
                c = *(strpos++);
                *(outpos++) = c;
                if (c == '\"') {
                    qmatch = 1;
                    break;
                }
            }
            if (!qmatch) {
                printf("Quotation mismatched\n");
                return 0;
            }
            temp_ptr = strpos;
            while (*temp_ptr == ' ') temp_ptr++;
            if (*temp_ptr == '/') {
                *(outpos++) = *(temp_ptr++);

                while (temp_ptr <= strend) {
                    if (*temp_ptr >= '0' && *temp_ptr <= '9') *(outpos++) = *(temp_ptr++);
                    else if (*temp_ptr == ' ' || *temp_ptr == 0) {
                        strpos = temp_ptr;
                        break;
                    }
                    else {
                        printf("THIS IS UNACCEPTABLE!\n");
                        return 0;

                    }
                }
            }
            *(outpos++) = ' ';
            strpos--;

        }
        else if (c != ' ') {
            *(outpos++) = c;
        }

        strpos++;
    }




    while (sl > 0) {
        sc = stack[sl - 1];
        if (sc == '(' || sc == ')') {
            printf("Parentheses mismatched\n");
            return 0;
        }
        *(outpos++) = ' ';
        *(outpos++) = sc;
        sl--;
    }
    *outpos = 0;
    return 1;
}

int ParsExpression(const uint8_t* input, uint8_t* output)
{
    uint8_t* ptr;
    ptr = (uint8_t*)strpbrk((char*)input, "!&|\"");

    uint8_t input_dup[8192], * cur_pos;

    if (ptr == NULL) {
        cur_pos = output;
        for (int i = 0; i != strlen((char*)input); i++)
        {
            *cur_pos++ = *(input + i);
        }

        strcpy((char*)input_dup, (char*)input);
        int tkn_qty = 0;
        ptr = (uint8_t*)strtok((char*)input_dup, " ");
        while (ptr)
        {
            tkn_qty++;
            ptr = (uint8_t*)strtok(NULL, " ");
        }
        for (int i = 0; i != tkn_qty-1; i++) {
            *cur_pos++ = ' ';
            *cur_pos++ = '|';
        }
        *cur_pos++ = 0x00;
        return 1;
    }
    else {
        cur_pos = &input_dup[0];
        uint8_t tkn_flag=0, quot_flag=0, op_flag=0;
        for (int i = 0; i != strlen((char*)input); i++)
        {
            if (*(input + i) != ' ' && *(input + i) != '!') tkn_flag = 1;
            if (*(input + i) == ' ') {
                if (!quot_flag) {
                    int j = 0;

                    while (*(input + i + j) == ' ')
                    {
                        j++;
                    }
                    switch (*(input + i + j)) {
                    case '&':
                    case '|':
                    case ')':
                        op_flag = 1;
                        break;
                    default:
                        i += j;
                        if (*(input + i) == '!' && tkn_flag == 1) {
                            *cur_pos++ = ' ';
                            *cur_pos++ = '&';
                            *cur_pos++ = ' ';
                            op_flag = 1;
                            break;
                        }
                        if (op_flag) {
                            op_flag = 0;
                            break;
                        }
                        if (tkn_flag) {
                            *cur_pos++ = ' ';
                            *cur_pos++ = '&';
                            *cur_pos++ = ' ';
                        }
                        break;
                    }
                }

            }

            if (*(input + i) == '\"') quot_flag = !quot_flag;
            *cur_pos++ = *(input + i);
        }

    }

    return shunting_yard(input_dup, output);
}
