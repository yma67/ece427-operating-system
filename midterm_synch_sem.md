# 信号量
###### Made available under ```the Wang Licence (3-BSD)```
## 定义
```cpp
struct semaphore {
    int count;
    vector<process> queue;
}

// uninterruptabla
semaphore::wait(semaphore s) {
    s.count -= 1;
    if (s.count < 0) {
        queue.push_back(getproc());
        getproc().block();
    }
}

// uninterruptabla
semaphore::post(semaphore s) {
    s.count += 1;
    if (s.count >= 0) {
        auto proc = queue.pop_back();
        proc.ready();
    }
}
```
