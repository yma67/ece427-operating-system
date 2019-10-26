# 经典同步问题
###### Made available under ```the Wang Licence (GPL)```
## 1. 生产者-消费者问题
### 信号量
```cpp
semaphore full(0), empty(N), mutex(1);
vector<item_t> queue;

void produce(item_t& item) {
    full.wait();
    mutex.wait();
    queue.push_back(item);
    mutex.signal();
    full.signal();
}

item_t consume() {
    empty.wait();
    mutex.wait();
    auto item = queue.pop_back();
    mutex.signal();
    empty.signal();
    return item;
}
```
### 管程
