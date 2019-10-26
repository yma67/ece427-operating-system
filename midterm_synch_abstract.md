# 更高级的抽象
###### Made available under ```the Wang Licence (3-BSD)```
## 信号量
### 定义
```cpp
struct semaphore {
    int count;
    vector<process> queue;
    void wait();
    void post();
}

// uninterruptable
void semaphore::wait() {
    this->count -= 1;
    if (this->count < 0) {
        (this->queue).push_back(getproc());
        getproc().block();
    }
}

// uninterruptable
void semaphore::signal() {
    this->count += 1;
    if (s.count >= 0) {
        auto proc = (this->queue).pop_back();
        proc.ready();
    }
}
```
### 性质
- Can be initialized to a nonnegative value – set semaphore
- Wait and Signal primitives are assumed to be atomic
- 2 Types
    - Strict: FIFO
    - ???: Random wakeup
### 由管程实现
## 管程
### 定义
- A monitor is a high-level (programming language) abstraction that combines (and hides) the following
    - shared data
    - operations on the data
    - synchronization using ***condition variables***
### 性质
- A monitor ensures that only one process at a time can be active within the monitor. 
- Monitors use condition variables to provide user-tailored synchronization and manage each with a separate queue for each condition. 
- The only operations available on these variables are WAIT and SIGNAL
    - wait suspends process that executes it until someone else does a signal (different from semaphore wait!)
    - signal resumes one suspended process. no effect if nobody's waiting (different from semaphore free/signal)!
### 例子
```java
class Balance {

    private int balance; 
    private final Lock lock = new ReentrantLock();
    private final Condition positive = lock.newCondition();
    
    public Balance(int balance) {
        this.balance = balance;
    }
    
    public void deposit(int val) throws InterruptedException {
        lock.lock();
        try {
            this.balance += val;
            positive.signal();
        } finally {
            lock.unlock();
        }
    }
    
    public bool withdraw(int val) throws InterruptedException {
        lock.lock();
        try {
            while (this.balance - val < 0)
                positive.await();
            this.balance -= val;
        } finally {
            lock.unlock();
        }
    }
}
```
### Extra 🙄
- Use ```UMPLE``` to generate
- Use ```jUCMNav``` to model strategy
- Apply Feature Modeling
### 由信号量实现
