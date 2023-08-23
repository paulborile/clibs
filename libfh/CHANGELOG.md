# Changelog

## 0.10.0 Aug 2023

- adopted wyhash (Go, Zig..) as the default hashfun
- added seeding to avoid DOS exploits
- added benchmarks for fh_get of 1 million keys

## 0.9.0 July 2023

- changed signature of hashfun (removed the hashtable size so that the hasfun just returns 
an hash and how to 'fit' it into the real table size is left to the hashtable implementation)

## 0.8.1

- introduced picobench for speed benchmarks
- `fh_getattr_string()` method

## 0.8.0

initial import of code from Paul repo
