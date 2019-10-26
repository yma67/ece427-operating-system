# 信号量
###### Made available under ```the Wang Licence (3-BSD)```
## 定义
```cpp
struct semaphore {
    int count;
    vector<process> queue;
    void semaphore::wait();
    void semaphore::post();
}

// uninterruptabla
void semaphore::wait() {
    this->count -= 1;
    if (this->count < 0) {
        (this->queue).push_back(getproc());
        getproc().block();
    }
}

// uninterruptabla
void semaphore::post() {
    this->count += 1;
    if (s.count >= 0) {
        auto proc = (this->queue).pop_back();
        proc.ready();
    }
}
```
