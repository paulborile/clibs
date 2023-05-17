# libfh 

libfh : fast hashtable, advanced multi threading support, key is only string and is always copied inside (unless FH_SETATTR_DONTCOPYKEY is set), opaque data is allocated and copied inside hash and can be string (datalen = FH_DATALEN_STRING), fixed lenght (datalen = sizeof data) or datalen = FH_DATALEN_VOIDP just copies void pointer. hashfunction is oat hash (one at a time hash) by Bob Jenkins but you can set your hash function in fh_create()

May 2023 : introduced Murmurhash and benchmarks with picobench ( https://github.com/iboB/picobench )

```
cd libfh
make -f libfh.mk releaseclean releaselinux debugclean debuglinux

Parsing: fh.c as language C
Parsing: fh.h as language C
rm -rf fh.o libfh.a
cc -DNDEBUG -O3 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread -I .    -c -o fh.o fh.c
ar crs libfh.a fh.o
rm -rf fh.do fh.mo libfh-debug.a
cc -DDEBUG -g3 -fsanitize=address -O0 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread -I .  -c fh.c -o fh.do
ar crs libfh-debug.a fh.do
```

To run the tests and benchmarks (in debug mode) : 

```
cd libfh/test
make -f libfh-test.mk testclean testlinux testrun
```

To run only the benchmarks (in release mode)

```
cd libfh/test
make -f libfh-test rtestclean rtestlinux
./rtestfh --gtest_filter=notest
```

Comparison of hash functions speeds : 

```
cd libfh/test
make -f fh-benchmark.mk benchclean benchlinux benchrun
```
Run the hash benchmarks : 

```
./fhtiming 
Tests are run on random keys
--- hash_function speed on random long (100-350) keys
      Keys            HashFunc       HashSize      AvgTime(ns)     Collisions     LongestChain     HashDimFactor
   1000000          fh_default        2097152           388.05         204172                7          1.500000
   1000000          crc32_hash        2097152           660.62         204485                7          1.500000
   1000000            djb_hash        2097152           239.87         204877                7          1.500000
   1000000            sax_hash        2097152           310.33         205178                7          1.500000
   1000000      murmur64a_hash        2097152            78.64         204716                8          1.500000
   1000000         wyhash_hash        2097152            65.82         204878                6          1.500000
   1000000         simple_hash        2097152           231.24         204711                6          1.500000
   1000000            jsw_hash        2097152           168.55         204278                6          1.500000
   1000000            jen_hash        2097152           197.70         204221                7          1.500000
   1000000           djb2_hash        2097152           242.19         204243                7          1.500000
   1000000           sdbm_hash        2097152           301.65         204183                7          1.500000
   1000000            fnv_hash        2097152           303.03         205402                7          1.500000
   1000000            oat_hash        2097152           375.15         204172                7          1.500000

--- hash_function speed on short keys (10 to 45 char len)
      Keys            HashFunc       HashSize      AvgTime(ns)     Collisions     LongestChain     HashDimFactor
   1000000          fh_default        2097152            63.00         204554                7          1.500000
   1000000          crc32_hash        2097152            94.11         204777                7          1.500000
   1000000            djb_hash        2097152            46.46         204611                7          1.500000
   1000000            sax_hash        2097152            53.95         204570                6          1.500000
   1000000      murmur64a_hash        2097152            44.99         204592                7          1.500000
   1000000         wyhash_hash        2097152            45.93         204871                6          1.500000
   1000000         simple_hash        2097152            43.59         204564                7          1.500000
   1000000            jsw_hash        2097152            42.39         204840                7          1.500000
   1000000            jen_hash        2097152            55.45         205070                6          1.500000
   1000000           djb2_hash        2097152            44.63         204368                7          1.500000
   1000000           sdbm_hash        2097152            50.10         204024                6          1.500000
   1000000            fnv_hash        2097152            49.85         204702                7          1.500000
   1000000            oat_hash        2097152            62.16         204554                7          1.500000
```

