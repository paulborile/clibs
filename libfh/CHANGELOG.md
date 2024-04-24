# Changelog

### 1.0.2 Mar 2024

- fixes to liblru for new libfh
- removed critical section on elements : we don't care if not super correct under concurrent access
- added fh_insertlock() : allow insert and manipulation of opaque under critical section
- fixes to libchannel : ch_get() in blocking mode should loop in cond_wait()

## 1.0.1 Jan 2024

- fixed seeding (was not working properly). Tests now show taht seeding works
- breaking change on the hashfun signature. Additional void * used to pass fh_t where seed/secret is kept


## 1.0.0 Sept 2023

- introduced buckets for collisions (size 8) heavily inspired by go map source
- had to deprecate fh_scan_start/fh_scan_next since signature is not good anymore (at least for now).
  Use enumerators to scan the hashtable.

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
