# Changelog

### 1.2.1 Aug 2024

No need for specialized unlock methods

### 1.2.0 Aug 2024

- fh_setattr(f, FH_SETATTR_USERWLOCKS, 1) to use rwlocks instead of mutexes. Performance is better with rwlocks but
only if you are using the hash massively concurrently with fh_search/fh_get. Benchmarks : 
[ RUN      ] FH.MultithreadGetSpeedMutex
1 threads - Total calls per second: 3602579.92
2 threads - Total calls per second: 6821306.72
3 threads - Total calls per second: 10726246.07
4 threads - Total calls per second: 11101978.70
5 threads - Total calls per second: 9364126.81
6 threads - Total calls per second: 10587052.48
7 threads - Total calls per second: 12205865.27
8 threads - Total calls per second: 10485972.71
[       OK ] FH.MultithreadGetSpeedMutex (34246 ms)
[ RUN      ] FH.MultithreadGetSpeedRWLocks
1 threads - Total calls per second: 2984249.83
2 threads - Total calls per second: 5458129.34
3 threads - Total calls per second: 7862844.13
4 threads - Total calls per second: 9204684.07
5 threads - Total calls per second: 10208603.19
6 threads - Total calls per second: 11846776.29
7 threads - Total calls per second: 12814910.64
8 threads - Total calls per second: 14076254.62
[       OK ] FH.MultithreadGetSpeedRWLocks (37175 ms)

### 1.1.0 June 2024

- introduced rwlocks (instead of standard mutexes) to reduce contention

### 1.0.3 Mar 2024

- restored critical section on elements and collisions : we need correct numbers for fh_clean to work
- fixed a problem on fh_insert when holes were present in buckets
- better MT tests the showed the problem on elements
- libch fixed size channel

### 1.0.2 Mar 2024

- removed critical section on elements number : we don't care if not super correct under concurrent access
- added fh_insertlock() : allow insert and manipulation of opaque under critical section
- fixes to libchannel : ch_get() in blocking mode should loop in cond_wait()

## 1.0.1 Jan 2024

- fixed seeding (was not working properly). Tests now show that seeding works
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
