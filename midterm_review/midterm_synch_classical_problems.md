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
};
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
```cpp
semaphore fmutex(1), rmutex(1), wmutex(1);
int read_count = 0;

void read() {
    rmutex.wait();
    if (read_count == 0)
        fmutex.wait();
    read_count += 1;
    rmutex.signal();
    // Read
    rmutex.wait();
    read_count -= 1;
    if (read_count == 0)
        fmutex.post();
    rmutex.signal();
}

void write() {
    wmutex.wait();
    fmutex.wait();
    // write
    fmutex.signal();
    wmutex.signal();
}
```
#### 管程实现
```cpp
struct reader_writer {
private:
    int read_count, write_count;
    std::mutex lock;
    std::condition_variable read_queue, write_queue;
    
public:
    reader_writer() : read_count(0), write_count(0) {}
    
    void read_lock() {
        std::unique_lock<std::mutex> lc(lock);
        read_count += 1;
        read_queue.wait(lc, [this]() -> bool {
            return !(write_count > 0);
        });
        read_queue.notify_one();
    }
    
    void read_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        read_count -= 1;
        if (read_count == 0)
            write_queue.notify_one();
    }
    
    void write_lock() {
        std::unique_lock<std::mutex> lc(lock);
        write_queue.wait(lc, [this]() -> bool {
            return !(read_count > 0 || write_count > 0);
        });
        write_count += 1;
    }
    
    void write_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        write_count -= 1;
        if (read_count > 0)
            read_queue.notify_one();
        else
            write_queue.notify_one();
    }
};
```
### 写者优先
#### 信号量实现
```cpp
semaphore fmutex(1), rmutex(1), wmutex(1), qmutex(1);
int read_count = 0, write_count = 0;

void read() {
    qmutex.wait();
    rmutex.wait();
    if (read_count == 0)
        fmutex.wait();
    read_count += 1;
    rmutex.signal();
    qmutex.signal();
    // Read
    rmutex.wait();
    read_count -= 1;
    if (read_count == 0)
        fmutex.post();
    rmutex.signal();
}

void write() {
    wmutex.wait();
    if (write_count == 0)
        qmutex.wait();
    write_count += 1;
    wmutex.signal();
    fmutex.wait();
    // write
    fmutex.signal();
    wmutex.wait();
    write_count -= 1;
    if (write_count == 0)
        qmutex.wait();
    wmutex.signal();
}
```
#### 管程实现
```cpp
struct reader_writer {
private:
    int read_count, write_count, write_wait;
    std::mutex lock;
    std::condition_variable read_queue, write_queue;
    
public:
    reader_writer() : read_count(0), write_count(0), write_wait(0) {}
    
    void read_lock() {
        std::unique_lock<std::mutex> lc(lock);
        read_queue.wait(lc, [this]() -> bool {
            return !(write_count > 0 || write_wait > 0);
        });
        read_count += 1;
        read_queue.notify_one();
    }
    
    void read_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        read_count -= 1;
        if (read_count == 0)
            write_queue.notify_one();
    }
    
    void write_lock() {
        std::unique_lock<std::mutex> lc(lock);
        write_wait += 1;
        write_queue.wait(lc, [this]() -> bool {
            return !(read_count > 0 || write_count > 0);
        });
        write_wait -= 1;
        write_count += 1;
    }
    
    void write_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        write_count -= 1;
        if (write_count > 0 || write_wait > 0)
            write_queue.notify_one();
        else
            read_queue.notify_one();
    }
};
```
### 公平竞争
#### 信号量实现
```cpp
semaphore fmutex(1), rmutex(1), wmutex(1), qmutex(1);
int read_count = 0;

void read() {
    qmutex.wait();
    rmutex.wait();
    if (read_count == 0)
        fmutex.wait();
    read_count += 1;
    rmutex.signal();
    qmutex.signal();
    // Read
    rmutex.wait();
    read_count -= 1;
    if (read_count == 0)
        fmutex.post();
    rmutex.signal();
}

void write() {
    qmutex.wait();
    wmutex.wait();
    fmutex.wait();
    // write
    fmutex.signal();
    wmutex.signal();
    qmutex.signal();
}
```
#### 管程实现
```cpp
struct reader_writer {
private:
    int read_count, write_count, read_wait, write_wait;
    std::mutex lock;
    std::condition_variable read_queue, write_queue;
    
public:
    reader_writer() : read_count(0), write_count(0), read_wait(0), write_wait(0) {}
    
    void read_lock() {
        std::unique_lock<std::mutex> lc(lock);
        read_wait += 1;
        if (write_count > 0 || write_wait > 0)
            read_queue.wait(lc);
        read_wait -= 1;
        read_count += 1;
        read_queue.notify_one();
    }
    
    void read_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        read_count -= 1;
        if (read_count == 0)
            write_queue.notify_one();
    }
    
    void write_lock() {
        std::unique_lock<std::mutex> lc(lock);
        write_wait += 1;
        if (read_count > 0 || write_count > 0)
            write_queue.wait(lc);
        write_wait -= 1;
        write_count += 1;
    }
    
    void write_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        write_count -= 1;
        if (read_wait > 0)
            read_queue.notify_one();
        else
            write_queue.notify_one();
    }
};
```
## 哲学家进餐
### 信号量实现
```cpp
semaphore c(n);

void manger(int i) {
    if (i != n - 1) {
        wait(c[(i)]);
        wait(c[(i + 1) % n]); 
        // eat
        post(c[(i + 1) % n]);
        post(c[(i)]);
        // think
    } else {
        wait(c[(i + 1) % n]);
        wait(c[(i)]);
        // eat
        post(c[(i)]);
        post(c[(i + 1) % n]);
        // think
    }
}
```
### 管程实现
```cpp
struct dining_philosopher {
private:
    enum status_t {THINKING, HUNGRY, THINKING};
    int num_phil;
    std::vector<status_t> status;
    std::mutex lock;
    std::vector<std::condition_variable> self;

public:
    dining_philosopher(int n): num_phil(n), status(std::vector<status_t> (n, THINKING)), 
                               self(std::vector<std::condition_variable> (n)) {}
    void pick_up(int i) {
        std::unique_lock<std::mutex> lock(mutex);
        status[(i)] = HUNGRY;
        if (status[(i + num_phil - 1) % num_phil] != EATING && 
            status[(i + 1) % num_phil] != EATING) {
            status[(i)] = EATING;
            self[(i)].notify_one(); // optional
        } else {
            self[(i)].wait();
        }
    }
    
    void put_down(int i) {
        std::unique_lock<std::mutex> lock(mutex);
        status[(i)] = THINKING;
        if (status[(i + num_phil - 1) % num_phil] == HUNGRY && 
            status[(i + num_phil - 2) % num_phil] != EATING) {
            status[(i + num_phil - 1) % num_phil] = EATING;
            status[(i + num_phil - 1) % num_phil].notify_one();
        }
        if (status[(i + 1) % num_phil] == HUNGRY && 
            status[(i + 2) % num_phil] != EATING) {
            status[(i + 1) % num_phil] = EATING;
            status[(i + 1) % num_phil].notify_one();
        }
    }
}
```
