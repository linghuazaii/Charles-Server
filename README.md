Charles Server
==============

### Charles Server Library!  
It is a simple server library, you can use it to write rpc or other funny things, it is a server anyway.

### Usage
 - make with `make`.
 - you will get all header files in `include`.
 - you will get all library files in `lib`.
 - `main.cpp` is a simple example, you can make it a rpc server without change much code.

### Benchmarking
I don't know how to make [wrk](https://github.com/wg/wrk) happy. Here is the result using `wrk -c400 -d30 -t12 charles_server:port`, QPS is about `368002 / 30 = 12266`.
```
Running 30s test @ http://172.31.43.244:19920/
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     0.00us    0.00us   0.00us    -nan%
    Req/Sec     0.00      0.00     0.00      -nan%
  0 requests in 30.03s, 0.00B read
  Socket errors: connect 0, read 368002, write 0, timeout 0
Requests/sec:      0.00
Transfer/sec:       0.00B
```

