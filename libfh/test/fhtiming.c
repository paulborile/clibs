// WARNING!!!! Needed to use popen in this code. Without it, it's not defined
//#define _BSD_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>


#include    "fh.h"
#include    "timing.h"

// wyhash used by Go, Zig and more
#include  "wyhash.h"

uint64_t seed[4];

void wyhash_hash_init()
{
    make_secret(time(NULL), seed);
}

static unsigned int wyhash_hash(char *key, int dim)
{
    uint64_t h = wyhash(key, strlen(key), 0, _wyp);
    return h % dim;
}

// defined in libfh
extern unsigned int fh_default_hash(char *key, int dim);

// old hash function (used with prime size table)
unsigned int fh_default_hash_orig(char *name, int dim)
{
    unsigned long h = 0, g;
    while (*name)
    {
        h = (h << 4) + *name++;
        g = (h & 0xF0000000);
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h % dim;
}

// bernstein hash : fastest 71 micro
unsigned int djb_hash(char *key, int dim)
{
    char *p = key;
    unsigned long h = 0;
    int i;

    for (i = 0; p[i] != 0; i++)
    {
        h = 33 * h + p[i];
        //h = 33 * h ^ p[i];
    }

    return h & (dim - 1);
}

// Shift-Add-XOR hash : slow as default
unsigned int sax_hash(char *key, int dim)
{
    char *p = key;
    unsigned long h = 0;
    int i;

    for (i = 0; p[i] != 0; i++)
    {
        h ^= (h << 5) + (h >> 2) + p[i];
    }

    return h & (dim - 1);
}

static unsigned int jsw_tab[256] = {
    104805925, 1419716914, 206053503, 2000419012, 192076427, 101206147, 871560053, 1854340521,
    2035245537, 775182410, 610191041, 147258054, 1385217352, 265909536, 1432762316, 1471912891,
    1453316387, 2135769961, 396350688, 714161119, 62358911, 1655004337, 114455720, 270834609,
    61623156, 959496175, 821277289, 1704502911, 69714497, 1523250256, 1674699507, 174520422,
    795483522, 1880753010, 27455787, 987559949, 1981959157, 899015840, 694416822, 1869721047,
    1674198250, 1304607863, 2016979101, 911931954, 1570517400, 1302257769, 236361197, 876350139,
    1290544082, 632711886, 1590511258, 1352902993, 140232575, 1704966979, 1623737603, 201855731,
    516979506, 297531244, 1906358642, 586694003, 1820781500, 1433574502, 761214426, 468781375,
    1166843864, 788670213, 1456341324, 1001319374, 1687686053, 3274499, 723556773, 1214400655,
    1307882362, 593052226, 2126332610, 730916114, 1895309995, 215210159, 1607266254, 1038370429,
    847922045, 1050293864, 243789774, 988154620, 607777195, 1867527377, 1190010351, 1124756701,
    17574973, 948885346, 1711450705, 1838356474, 234976200, 325181483, 159654201, 1401820064,
    1113851696, 1615995525, 255655790, 654054101, 1619270024, 979212563, 1868454756, 779668739,
    1572264789, 1847303718, 1510584853, 1320091136, 2062513878, 970367459, 210977917, 762952275,
    2020661324, 454767692, 1751106896, 480954871, 174811421, 793633599, 1605711573, 192386395,
    1742518945, 1169678630, 2030742869, 1977495145, 1494860113, 42913422, 1231831562, 461228161,
    1658908947, 1487487352, 1115282262, 1130695324, 319216268, 836253370, 1910364063, 1891481057,
    536073441, 1273465268, 1064088546, 451103671, 96349080, 1275066463, 1214055946, 2117010404,
    1729834155, 817679194, 450481627, 1904645577, 1611312794, 2056193200, 2097031972, 1206348091,
    1078388182, 1980291193, 1036359589, 425764647, 2023204615, 120707503, 886992808, 1534629914,
    1608194855, 2002275070, 517841590, 1927411123, 691044793, 280722005, 1671408533, 1227118234,
    1554187274, 588013431, 1678221905, 1650536354, 1863079894, 744794203, 1620063110, 1445430402,
    1562473398, 2070544737, 1202592331, 1026302544, 1979254290, 1152140655, 85166987, 910158824,
    984948200, 1121526576, 1335923472, 860669167, 1242234079, 75432632, 247815433, 702945287,
    2077707703, 765657024, 482872762, 621268848, 1046379029, 6797647, 1848387082, 453082655,
    594811078, 1379125339, 2103619009, 310407325, 2123919542, 1576198471, 1755837727, 1538909292,
    1499259561, 810946410, 417728188, 1331030203, 1963087065, 502895176, 93705379, 800551617,
    1624421752, 1429628851, 1661220784, 719172184, 1505061484, 1909036217, 1422117471, 1435285539,
    527209593, 1904990233, 2056554387, 1573588623, 1911787881, 1757457821, 2026671278, 359115311,
    989099512, 1982806640, 669522636, 965535406, 1411521463, 277876715, 356961051, 763297376,
    1088823125, 774689239, 2094327579, 904426542, 1277584415, 40549311, 1704978159, 754522520,
    1470178162, 1218715295, 1473694704, 827755998, 980267865, 748328527, 115557889, 1507477458
};
unsigned jsw_hash(char *key, int dim)
{
    unsigned h = 16777551;
    int i;

    for (i = 0; key[i] != 0; i++)
    {
        h = (h << 1 | h >> 31) ^ jsw_tab[(int)key[i]];
    }

    return h & (dim - 1);
}

// psb_hash : a variant of jsw

unsigned psb_hash(char *key, int dim)
{
    size_t i;
    size_t len = strlen(key);

    // use standard jsw_hash
    if ( len < sizeof(int))
    {
        unsigned h = 16777551;
        int i;

        for (i = 0; key[i] != 0; i++)
        {
            h = (h << 1 | h >> 31) ^ jsw_tab[(int)key[i]];
        }

        return h & (dim - 1);
    }

    // unsigned int h = 5381;
    unsigned int h = 4294967295;
    unsigned int *kh = (unsigned int *) key;

    for (i = 0; i<(len - sizeof(int)); i+=sizeof(int))
    {
        //h ^= (h << 5) + (h >> 2) + (*kh * 16777551);
        //        h ^= (h << 5) + (h >> 2) + p[i];

        h ^= (h << 5) + (h >> 2) * *kh;  /* hash * 33 + c */
        kh += sizeof(int);

    }

    return h & (dim - 1);
}

/* simple: compute hash value of string */
unsigned simple_hash(char *key, int dim)
{
    unsigned int h;
    unsigned char *p;

    h = 0;
    for (p = (unsigned char *)key; *p != '\0'; p++)
        h = 37 * h + *p;
    return h & (dim - 1);
}


unsigned elf_hash(char *key, int dim)
{
    char *p = key;
    unsigned h = 0, g;
    int i;

    for (i = 0; key[i] != 0; i++)
    {
        h = (h << 4) + p[i];
        g = h & 0xf0000000L;

        if (g != 0)
        {
            h ^= g >> 24;
        }

        h &= ~g;
    }

    return h & (dim - 1);
}


//////////////////
#define hashsize(n) (1U << (n))
#define hashmask(n) (hashsize(n) - 1)

#define mix(a, b, c) \
    { \
        a -= b; a -= c; a ^= (c >> 13); \
        b -= c; b -= a; b ^= (a << 8); \
        c -= a; c -= b; c ^= (b >> 13); \
        a -= b; a -= c; a ^= (c >> 12); \
        b -= c; b -= a; b ^= (a << 16); \
        c -= a; c -= b; c ^= (b >> 5); \
        a -= b; a -= c; a ^= (c >> 3); \
        b -= c; b -= a; b ^= (a << 10); \
        c -= a; c -= b; c ^= (b >> 15); \
    }

unsigned jen_hash(char *k, int dim)
{
    unsigned a, b;
    unsigned c = 9787;
    unsigned len = strlen(k);
    int length = len;

    a = b = 0x9e3779b9;

    while (len >= 12)
    {
        a += (k[0] + ((unsigned)k[1] << 8) + ((unsigned)k[2] << 16) + ((unsigned)k[3] << 24));
        b += (k[4] + ((unsigned)k[5] << 8) + ((unsigned)k[6] << 16) + ((unsigned)k[7] << 24));
        c += (k[8] + ((unsigned)k[9] << 8) + ((unsigned)k[10] << 16) + ((unsigned)k[11] << 24));

        mix(a, b, c);

        k += 12;
        len -= 12;
    }

    c += length;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough="

    switch (len)
    {
    case 11: c += ((unsigned)k[10] << 24);
    case 10: c += ((unsigned)k[9] << 16);
    case 9: c += ((unsigned)k[8] << 8);
    /* First byte of c reserved for length */
    case 8: b += ((unsigned)k[7] << 24);
    case 7: b += ((unsigned)k[6] << 16);
    case 6: b += ((unsigned)k[5] << 8);
    case 5: b += k[4];
    case 4: a += ((unsigned)k[3] << 24);
    case 3: a += ((unsigned)k[2] << 16);
    case 2: a += ((unsigned)k[1] << 8);
    case 1: a += k[0];
    }

#pragma GCC diagnostic pop

    mix(a, b, c);

    return c & (dim - 1);
}
//////////////////

unsigned djb2_hash(char *key, int dim)
{
    unsigned hash = 5381;
    int c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;  /* hash * 33 + c */

    return hash & (dim - 1);
}

unsigned sdbm_hash(char *key, int dim)
{
    unsigned hash = 0;
    int c;

    while ((c = *key++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash & (dim - 1);
}

unsigned fnv_hash(char *key, int dim)
{
    unsigned h = 2166136261;
    int i;

    for (i = 0; key[i] != 0; i++)
    {
        h = (h * 16777619) ^ key[i];
    }

    return h & (dim - 1);
}

unsigned oat_hash(char *key, int dim)
{
    unsigned h = 0;
    int i;

    for (i = 0; key[i] != 0; i++)
    {
        h += key[i];
        h += (h << 10);
        h ^= (h >> 6);
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    return h & (dim - 1);
}

// crc32 hash

static const unsigned int crc32_table[] =
{
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

#include <stdint.h>

unsigned crc32_hash(char *buf, int dim)
{
    uint32_t h = 0;
    for (int i=0; buf[i] != 0; i++)
    {
        h = (h << 8) ^ crc32_table[((h >> 24) ^ buf[i]) & 255];
    }
    return h & (dim - 1);
}



#define BIG_CONSTANT(x) (x ## LLU)

unsigned int murmur64a_hash(char *key, int dim)
{
    const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
    const int r = 47;
    int len = strlen(key);
    uint64_t h = 0x1F0D3804 ^ (len * m);

    const uint64_t *data = (const uint64_t *)key;
    const uint64_t *end = data + (len/8);

    while (data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    const unsigned char *data2 = (const unsigned char *)data;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough="

    switch (len & 7)
    {
    case 7:
        h ^= (uint64_t) data2[6] << 48;
    case 6:
        h ^= (uint64_t) data2[5] << 40;
    case 5:
        h ^= (uint64_t) data2[4] << 32;
    case 4:
        h ^= (uint64_t) data2[3] << 24;
    case 3:
        h ^= (uint64_t) data2[2] << 16;
    case 2:
        h ^= (uint64_t) data2[1] << 8;
    case 1:
        h ^= (uint64_t) data2[0];
        h *= m;
    };

#pragma GCC diagnostic pop

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h & (dim - 1);
}


struct hash_fun
{
    char hash_fun_name[64];
    unsigned int (*hash_fun)(char *key, int dim);
    double hash_dim_factor;
};

struct hash_fun hash_funs[] = {
    // { "fh_default_hash", fh_default_hash, 1.0 },
    { "fh_default", fh_default_hash, 1.5 },
    { "crc32_hash", crc32_hash, 1.5 },

    // { "fh_default_hash", fh_default_hash, 2.5 },

    // { "fh_default_hash_orig", fh_default_hash_orig, 1.0 },
    // { "fh_default_hash_orig", fh_default_hash_orig, 1.5 },
    // { "fh_default_hash_orig", fh_default_hash_orig, 2.5 },

    // { "djb_hash", djb_hash, 1.0 },
    { "djb_hash", djb_hash, 1.5 },
    // { "djb_hash", djb_hash, 2.5 },

    // { "sax_hash", sax_hash, 1.0 },
    { "sax_hash", sax_hash, 1.5 },
    { "murmur64a_hash", murmur64a_hash, 1.5 },
    { "wyhash_hash", wyhash_hash, 1.5 },
    { "simple_hash", simple_hash, 1.5 },
    // { "psb_hash", psb_hash, 1.5 },
    { "jsw_hash", jsw_hash, 1.5 },
    // { "jsw_hash", jsw_hash, 2.5 },

    // { "elf_hash", elf_hash, 1.0 },
    // { "elf_hash", elf_hash, 1.5 },
    // { "elf_hash", elf_hash, 2.5 },

    // { "jen_hash", jen_hash, 1.0 },
    { "jen_hash", jen_hash, 1.5 },
    // { "jen_hash", jen_hash, 2.5 },

    // { "djb2_hash", djb2_hash, 1.0 },
    { "djb2_hash", djb2_hash, 1.5 },
    // { "djb2_hash", djb2_hash, 2.5 },

    // { "sdbm_hash", sdbm_hash, 1.0 },
    { "sdbm_hash", sdbm_hash, 1.5 },
    // { "sdbm_hash", sdbm_hash, 2.5 },

    // { "fnv_hash", fnv_hash, 1.0 },
    { "fnv_hash", fnv_hash, 1.5 },
    // { "fnv_hash", fnv_hash, 2.5 },

    // { "fnv_hash", fnv_hash, 1.0 },
    { "oat_hash", oat_hash, 1.5 },


    { "", NULL, 0.0 },
};


double  compute_average(double current_avg, int count, int new_value)
{
    if ( count == 0 )
    {
        return (new_value);
    }
    else
    {
        return (((current_avg * count) + new_value) / (count+1));
    }
}



int count_lines(const char *file)
{
    FILE *pin;
    char wc_command[4096];
    char wc_out[4096];
    int lines;

    // count lines with wc -l
    sprintf(wc_command, "wc -l %s", file);
    // printf("command to run : %s\n", wc_command);

    if (( pin = popen(wc_command, "r")) == NULL)
    {
        perror(wc_command);
        exit(EXIT_FAILURE);
    }

    fscanf(pin, "%d %s\n", &lines, wc_out);
    pclose(pin);

    // printf("file %s, lines %d\n", file, lines);

    return(lines);
}

#define ASCII0      'a'
#define ASCIISET    'z'

void generate_random_str(int seed, char *str, int min_len, int max_len)
{
    int i, irandom;
    char c;
    unsigned int s = seed;
    irandom = min_len + (rand_r(&s) % (max_len - min_len));

    for (i = 0; i < irandom; i++)
    {
        c = ASCII0 + (rand_r(&s) % (ASCIISET - ASCII0 + 1));
        str[i] = c;
    }
    str[i] = '\0';
    return;
}

static unsigned int fh_hash_size(unsigned int s)
{
    unsigned int i = 1;
    while (i < s) i <<= 1;
    return i;
}

int main( int argc, char **argv )
{
    struct mydata
    {
        char checksum[20];
        int i;
    };
    double hash_calc_time = 0;
    unsigned long long delta;
    void *t = timing_new_timer(1);

    int which = 3;
    if ( argc > 1 )
    {
        which = atoi(argv[1]);
    }

    wyhash_hash_init();

    printf("Tests are run on random keys\n");

    if (which & 1)
    {
        char keys[8*1024];

        printf("--- hash_function speed on random long (100-350) keys\n");
        printf("%10s%20s%15s%17s%15s%17s%18s\n", "Keys", "HashFunc", "HashSize", "AvgTime(ns)", "Collisions", "LongestChain", "HashDimFactor");

        int num_strings = 1000000; // simulating 1 million random keys

        for (int i = 0; hash_funs[i].hash_fun != NULL; i++)
        {
            int real_hash_size = fh_hash_size(num_strings*hash_funs[i].hash_dim_factor);
            int *coll = calloc(real_hash_size, sizeof(int));
            int collisions = 0;
            memset(coll, 0, real_hash_size * sizeof(int));
            hash_calc_time = 0;

            for ( int l = 0; l< num_strings; l++ )
            {
                generate_random_str(1000+l, keys, 100, 350);

                timing_start(t);
                int hashval = hash_funs[i].hash_fun(keys, real_hash_size);
                delta = timing_end(t);
                hash_calc_time = compute_average(hash_calc_time, l, delta);
                if (coll[hashval] != 0 )
                {
                    // collision
                    collisions++;
                }
                coll[hashval]++;
            }

            // calculate longest collision chain
            int longest = 0;
            for (int j=0; j<real_hash_size; j++)
            {
                if (coll[j] > longest )
                    longest = coll[j];
            }

            printf("%10d%20s%15d%17.2f%15d%17d%18f\n",
                   num_strings, hash_funs[i].hash_fun_name, real_hash_size, hash_calc_time,
                   collisions, longest, hash_funs[i].hash_dim_factor);

            free(coll);
        }

        printf("\n--- hash_function speed on short keys (10 to 45 char len)\n");
        printf("%10s%20s%15s%17s%15s%17s%18s\n", "Keys", "HashFunc", "HashSize", "AvgTime(ns)", "Collisions", "LongestChain", "HashDimFactor");

        num_strings = 1000000; // simulating 1 million random keys
        for (int i = 0; hash_funs[i].hash_fun != NULL; i++)
        {
            int real_hash_size = fh_hash_size(num_strings*hash_funs[i].hash_dim_factor);
            int *coll = calloc(real_hash_size, sizeof(int));
            int collisions = 0;
            memset(coll, 0, real_hash_size * sizeof(int));
            hash_calc_time = 0;

            for ( int l = 0; l< num_strings; l++ )
            {
                generate_random_str(1000+l, keys, 10, 45);
                // check if ua present in cache
                timing_start(t);
                int hashval = hash_funs[i].hash_fun(keys, real_hash_size);
                delta = timing_end(t);
                hash_calc_time = compute_average(hash_calc_time, l, delta);
                if (coll[hashval] != 0 )
                {
                    // collision
                    collisions++;
                }
                coll[hashval]++;
            }

            // calculate longest collision chain
            int longest = 0;
            for (int j=0; j<real_hash_size; j++)
            {
                if (coll[j] > longest )
                    longest = coll[j];
            }

            printf("%10d%20s%15d%17.2f%15d%17d%18f\n",
                   num_strings, hash_funs[i].hash_fun_name, real_hash_size, hash_calc_time,
                   collisions, longest, hash_funs[i].hash_dim_factor);

            free(coll);
        }
    }


    timing_delete_timer(t);
}
