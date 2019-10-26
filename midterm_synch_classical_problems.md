# 经典同步问题
###### Made available under ```the Wang Licence (GPL)```
## 1. 生产者-消费者问题
### 信号量
```cpp
// 像个stack一样
semaphore full(0), empty(N), mutex(1);
item_t* queue[N];
size_t index = 0;

void produce(item_t* item) {
    full.wait();
    mutex.wait();
    queue[index++] = item;
    mutex.signal();
    full.signal();
}

item_t* consume() {
    empty.wait();
    mutex.wait();
    item_t* item = queue[--index];
    mutex.signal();
    empty.signal();
    return item;
}

// 像个queue一样
semaphore full(0), empty(N), mutex(1);
item_t* queue[N];
size_t front = 0, count = 0;

void produce(item_t* item) {
    full.wait();
    mutex.wait();
    queue[(front + count++) % N] = item;
    mutex.signal();
    full.signal();
}

item_t* consume() {
    empty.wait();
    mutex.wait();
    item_t* item = queue[front];
    front = (front + 1) % N;
    count = count + 1;
    mutex.signal();
    empty.signal();
    return item;
}
```
### 管程
