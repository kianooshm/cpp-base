# cpp-base
Collection of C++ utilities for faster development. A good fraction of the codes is taken from here and there (and sometimes adapted ot our needs). See the copyright notice in each file. The default is Apache License v2; see LICENSE file.

If you discovered issues or made improvements, your feedback and contributions to the repository are very welcome.

- **Utility data structures**:
  - Collection of **producer-consumer queues** optimized for concurrency and efficiency:
    - Non-blocking, lock-free single-producer single-consumer queue based on Facebook Folly library.
    - Non-blocking, lock-free single-producer single-consumer queue by Dr Dobb's. (though Folly's is much faster.)
    - Non-blocking, multiple-producer multiple-consumer queue -- a circular array protected by a mutex.
    - Blocking queue wrapper over any of the three above by either spin lock or semaphore. See the comments in the header files as to when to use which.
  - **Bloom Filter**: approximate set allowing Insert() and Contains() operations with a governable tradeoff between accuracy and memory usage. See [this](https://en.wikipedia.org/wiki/Bloom_filter).
  - **Expandable Bloom Filter**: if you are not sure about max num items that your Bloom Filter is to store, this utilitiy allows to start small and grow as needed. Like C++ std::vector/Java ArrayList.
  - **Cuckoo Filter**: like Bloom Filter but also allows Delete() operation as well. As memory efficient and as fast as Bloom filter, if not faster. See [this](https://www.cs.cmu.edu/~binfan/papers/login_cuckoofilter.pdf).
  - **Count-min Sketch**: Like Bloom/Cukoo filter but allows 'counting' num occurences of each key, rather than just telling a 0-1 presence. See [this](https://en.wikipedia.org/wiki/Count%E2%80%93min_sketch).
  - **LRU Map** and **LRU Set**: a bounded size (i.e., TTL-ed) hash map/hash set that guarantees to never exceed a given capacity -- upon new insertions evicts the least recently used entry. Considering LRU as a measure of popularity (esp. where data has temporal locality), this is useful for working on large streams where e.g. tracking metadata for only popular keys is feasible.
  - **Ring Buffer**: a.k.a. circular buffer/array. Fixed size FIFO *without* any memory allocation at runtime.
  - **Vector Map**: a vector backed by a hash map from element to their index in the vector.
  - **ZVector**: anconvenient class for defining arrays with custom index ranges; recall Pascal syntax array[min_index..max_index].
- **File utilities**:
  - Convenient classes for file ops e.g. Exists(), Remove(), Size().
  - Convenient classes for file I/O: FileInputStream and FileOutputStream that wrap C system calls and also provide buffering on top.
- **Hash utilities**:
  - Hashing/mixing strings or numbers.
  - MD5 for 128-bit hashing.
  - FingerPrint2011 (by Google) for 64-bit hashing.
- **Simple HTTP Server**:
  - C++ Socket wrapper over (unpleasant) POSIX sockets. (not for Windows.)
  - URL/URI/stream utilities (e.g., Cord).
  - HTTP server for writing e.g. a REST service.
- **Management and Monitoring**: allows to export 2 types of data over HTTP:
  - Stat variables: exposing these allow to know the program's internal state at runtime -- like Google's `/varz` or Java JMX.
  - Config variables: expose these allow to configure a program at runtime -- like Google's `/configz` or Java JMX.
- **String utilities**:
  - join operations, e.g., StrCat(...), split, tokenize, trim, ...
  - Conversion to/from numeric types.
  - StringPiece, StringPrintf, ...
- **Threading and concurrency**:
  - Mutex, condition variable and barrier wrappers.
  - Scoped mutex lock -- a great tool to avoid the headache of releasing locks in different execution paths.
  - ThreadPool.
- **Test tools**: assuming the use of Google test framework and Google build system (Bazel), Bash script `autotest.sh` discovers all test targets in BUILD files, builds and runs them, collects the report in XML format, and generates code coverage reports using gcovr. Like a mini CI system (we've been using it with Jenkins).
- **Miscellaneous utilities**: some notable ones are:
  - Clock: Interface for sleep, wait and notify operations with injectable real or simulated clock -- for production code and unit testing, respectively.
  - Map utilities: to avoid direct use of std's long/unintuitive/insufficient (hash_)map API.
  - PeriodicClosure: to schedule a callback to run asynchronously at exact certain times.
  - triple<T1, T2, T3>: extension of std::pair<T1, T2> to 3 items -- not really best coding practice and can grow a bad habit to overuse! :grinning:
  - Type traits like is_integral, has_trivial_constructor, is_convertible, etc.
- **ZooKeeper**:
  - C++ wrapper over ZooKeeper's unfriendly C library.
  - Membership management for writing distributed systems: multiple instances of a program across different machines can compete for leader election on a number of ZK nodes so each instance knows which role it should take (active server, backup server, type A, type B, etc.)

**Note on building**: there are files named BUILD in each directory which are for the Google Bazel build system. If you use `make`, you need to add your Makefile. I myself have migrated from make to bazel a long ago.

**Note on unit tests**: they are written using Google's test framework which is a small code stub copied in the present code base (see gtest) so don't worry about that.

