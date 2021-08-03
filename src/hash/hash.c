#include "hash.h"

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}


#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}



uint32_t lookup3(const void* key, uint32_t length)
{
    uint32_t a, b, c;
    a = b = c = 0xdeadbeef + length;
    const uint32_t* k = (const uint32_t*)key;
    const uint8_t* k8;

    while (length > 12)
    {
        a += k[0];
        b += k[1];
        c += k[2];
        mix(a, b, c);
        length -= 12;
        k += 3;
    }

    k8 = (const uint8_t*)k;
    switch (length)
    {
        case 12: c += k[2]; b += k[1]; a += k[0]; break;
        case 11: c += ((uint32_t)k8[10]) << 16;
        case 10: c += ((uint32_t)k8[9]) << 8;
        case 9: c += k8[8];
        case 8: b += k[1]; a += k[0]; break;
        case 7: b += ((uint32_t)k8[6]) << 16;
        case 6: b += ((uint32_t)k8[5]) << 8;
        case 5: b += k8[4];
        case 4: a += k[0]; break;
        case 3: a += ((uint32_t)k8[2]) << 16;
        case 2: a += ((uint32_t)k8[1]) << 8;
        case 1: a += k8[0]; break;
        case 0: return c;
    }


    final(a, b, c);
    return c;
}

uint32_t Jenkins_hash(const uint8_t *s) {
  uint32_t hash = 0;
  while (*s) {
    hash +=*s++;
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}
