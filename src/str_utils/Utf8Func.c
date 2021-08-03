#include "Utf8Func.h"

uint8_t* strToLowerUtf8(uint8_t* pString)
{
    if (pString && *pString) {
        uint8_t* p = pString;
        uint8_t* ptr_extr = 0;
        while (*p) {
            if ((*p >= 0x41) && (*p <= 0x5a)) //ASCII
                (*p) += 0x20;
            else if (*p > 0xc0) {
                ptr_extr = p;
                p++;
                switch (*ptr_extr) { //Русский
                case 0xd0:
                    if ((*p >= 0x80) && (*p <= 0x8f)) {
                        *ptr_extr = 0xd1;
                        (*p) += 0x10;
                    }
                    else if ((*p >= 0x90) && (*p <= 0x9f))
                        (*p) += 0x20; 
                    else if ((*p >= 0xa0) && (*p <= 0xaf)) {
                        *ptr_extr = 0xd1;
                        (*p) -= 0x20;
                    }
                    break;
                
                
                default:
                    break;
                }
            }
            p++;
        }
    }
    return (uint8_t*)pString;
}

uint8_t* UnicodeToUtf8(uint8_t* pos, uint32_t val) {

    if (val <= 0x7F) {
        *pos++ = (uint8_t)val;
        return pos;
    }

    if (val <= 0x07FF) {
        *pos++ = (uint8_t)(((val >> 6) & 0x1F) | 0xC0);
        *pos++ = (uint8_t)(((val >> 0) & 0x3F) | 0x80);
        return pos;
    }

    *pos++ = (uint8_t)(((val >> 12) & 0x0F) | 0xE0);
    *pos++ = (uint8_t)(((val >> 6) & 0x3F) | 0x80);
    *pos++ = (uint8_t)(((val >> 0) & 0x3F) | 0x80);
    return pos;

}

uint8_t* StrToUtf8(uint8_t* dst, uint8_t* src) {

    uint8_t* pExtr;
    uint8_t temp=0;
    uint32_t unicode;
    while (*src && src) {
        pExtr = src + 1;
        if (*src == '\\') {
            switch (*pExtr)
            {
            case 'u':
                unicode = 0;
                src += 2;
                for (int i = 0; i != 4; i++) {
                    if (*src >= '0' && *src <= '9') temp = *src - '0';
                    else if (*src >= 'a' && *src <= 'f') temp = *src - 'a' + 10;
                    else if (*src >= 'A' && *src <= 'F') temp = *src - 'A' + 10;
                    unicode = (unicode << 4) | (temp & 0xF);
                    src++;
                }
                switch (unicode)
                {

                        //Удаление кавычек
                    case 8212:
                    case 160:
                    case 171:
                    case 187:
                        //Удаление знака ударения
                    case 769:
                    case 180:
                        break;

                        //Замена Ё/ё на е
                    case 0x401:
                    case 0x451:
                        unicode = 0x435; //FALLTHROUGH

                    default:
                        dst = UnicodeToUtf8(dst, unicode);
                        break;
                }

                break;
                // Замена на пробел
            case 'r':
            case 't':
            case 'n':
            case 'b':
                *dst++ = ' ';
                src += 2;
                break;

                // /" и //
            default:
                *dst++ = *pExtr;
                src += 2;
                break;
            }
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst++ = 0x00;

    return dst;

}
