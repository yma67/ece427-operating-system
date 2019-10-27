# 经典同步问题
###### Made available under ```the Wang Licence (GPL)```
## 生产者-消费者问题
### 信号量
```cpp
// 像个stack一样
semaphore full(0), empty(N), mutex(1);
item_t queue[N];
size_t index = 0;

void produce(item_t* item) {
    full.wait();
    mutex.wait();
    queue[index++] = std::move(item);
    mutex.signal();
    full.signal();
}

item_t consume() {
    empty.wait();
    mutex.wait();
    item_t item = std::move(queue[--index]);
    mutex.signal();
    empty.signal();
    return item;
}

// 像个queue一样
semaphore full(0), empty(N), mutex(1);
item_t queue[N];
size_t front = 0, count = 0;

void produce(item_t* item) {
    full.wait();
    mutex.wait();
    queue[(front + count++) % N] = std::move(item);
    mutex.signal();
    full.signal();
}

item_t* consume() {
    empty.wait();
    mutex.wait();
    item_t* item = std::move(queue[front]);
    front = (front + 1) % N;
    count = count + 1;
    mutex.signal();
    empty.signal();
    return item;
}
```
### 管程
#### Version A
##### Implementation 1
```cpp
template <class T>
struct bounded_buffer {
private:
    T** buffer;
    int slots, out_ptr, in_ptr, count;
    std::mutex lock;
    std::condition_variable full, empty;
public:
    bounded_buffer(int c) : slots(c), out_ptr(0), in_ptr(0), count(0) {
        buffer = new T*[slots + 1];
    }

    ~bounded_buffer() {
        delete[] buffer;
    }

    void produce(T* item_ptr){
        std::unique_lock<std::mutex> lc(lock);
        full.wait(lc, [this]() -> bool {
            return count < slots;
        });
        buffer[in_ptr] = item_ptr;
        in_ptr = (in_ptr + 1) % slots;
        count += 1;
        empty.notify_one();
    }

    T* consume(){
        std::unique_lock<std::mutex> lc(lock);
        empty.wait(lc, [this]() -> bool {
            return count > 0;
        });
        T* item_ptr = buffer[out_ptr];
        out_ptr = (out_ptr + 1) % slots;
        count -= 1;
        full.notify_one();
        return item_ptr;
    }
};
```
##### Implementation 2
```cpp
template <class T>
struct bounded_buffer {
private:
    T* buffer;
    int slots, out_ptr, in_ptr, count;
    std::mutex lock;
    std::condition_variable full, empty;
public:
    bounded_buffer(int c) : slots(c), out_ptr(0), in_ptr(0), count(0) {
        buffer = new T[slots];
    }

    ~bounded_buffer() {
        delete[] buffer;
    }

    void produce(T& item){
        std::unique_lock<std::mutex> lc(lock);
        full.wait(lc, [this]() -> bool {
            return count < slots;
        });
        buffer[in_ptr] = std::move(item);
        in_ptr = (in_ptr + 1) % slots;
        count += 1;
        empty.notify_one();
    }

    T consume(){
        std::unique_lock<std::mutex> lc(lock);
        empty.wait(lc, [this]() -> bool {
            return count > 0;
        });
        T item = std::move(buffer[out_ptr]);
        out_ptr = (out_ptr + 1) % slots;
        count -= 1;
        full.notify_one();
        return item;
    }
};
```
解释
- ```RAII```所以不需要释放锁
- ```wait```到了可以的时候就放开了
- Horase管程会引发overwrite
    - 解决1: ```buffer = new T[capacity + 1];```
    - 解决2: 把pointer换成object并使用移动语义以兼顾效率

##### Usage
```cpp
bounded_buffer<int> m(3);

void producer_thread() {
    int item = rand();
    m.produce(item);
}

void consumer_thread() {
    int item = m.consume();
    cout << item << endl;
}
```
#### Version B
##### Implementation
```cpp
template <class T>
struct bounded_buffer {
private:
    int slots, out_ptr, in_ptr, count;
    std::mutex lock;
    std::condition_variable full, empty;
public:
    bounded_buffer(int c) : slots(c), out_ptr(0), in_ptr(0), count(0) {}
    
    void producer_lock() {
        std::unique_lock<std::mutex> lc(lock);
        full.wait(lc, [this]() -> bool {
            return count < slots;
        });
        return in_ptr;
    }
    
    void producer_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        in_ptr = (in_ptr + 1) % capacity;
        count += 1;
        empty.notify_one();
    }
    
    void consumer_lock() {
        std::unique_lock<std::mutex> lc(lock);
        empty.wait(lc, [this]() -> bool {
            return count > 0;
        });
        return out_ptr;
    }
    
    void consumer_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        out_ptr = (out_ptr + 1) % slots;
        count -= 1;
        full.notify_one();
    }
}
```
注意
- Allocate one more slot outside monitor
##### Usage
```cpp
bounded_buffer<int> m(3);
vector<int> buffer(3 + 1);

void producer_thread() {
    int item = rand();
    int item_ptr = m.producer_lock();
    buffer[item_ptr] = item;
    m.producer_unlock();
}

void consumer_thread() {
    int item_ptr = m.consumer_lock();
    cout << buffer[item_ptr] << endl;
    m.consumer_unlock();
}
```
## 读者-写者问题
### 读者优先
#### 信号量实现
#### 管程实现
### 写者优先
#### 信号量实现
#### 管程实现
### 公平竞争
#### 信号量实现
#### 管程实现
