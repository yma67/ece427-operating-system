# 更高级的抽象
###### Made available under ```the Wang Licence (GPL)```
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
```java
```
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
    - wait suspends process that executes it until someone else does a signal (different from semaphore wait)
    - signal resumes one suspended process. no effect if nobody's waiting (different from semaphore free/signal)
### 示例
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
### What's Happened? Version 1
Process is entering Monitor
- Case 1: No process inside monitor
    - Incoming process enter monitor
- Case 2: 1 or more process(es) inside monitor
    - Incoming process goes to wait in **entry queue**
Process is leaving Monitor
- Process unlocks the monitor, so a process waiting in a entry queue can get into monitor
### What's Happened? Version 2: Allow a process to wait inside condition variable
- Process inside monitor wants to wait
    - unlock the monitor
    - signaling process to wake up a process that is sleeping on a conditional variable 
    - signaling process needs to go to sleep immediately after signaling
### 由信号量实现 (Hoare)
```cpp
struct monitor {

    struct condition {
        semaphore x_sem;
        int x_count;
    public:
        condition(): x_sem(0), x_count(0) {}
        void wait();
        void signal();
    }
    
    int next_count;
    semaphore next, mutex;
    monitor(): next_count(0), next(0), mutex(1) {}
    
    void condition::wait() {
        x_count += 1;
        x_sem.wait();
        if (next_count > 0)
            next.signal();
        else
            mutex.signal();
        x_count -= 1;
    }
    
    void condition::signal() {
        if (x_count > 0) {
            next_count += 1;
            x_sem.signal();
            next.wait();
            next_count -= 1;
        }
    }
    
    void monitor_func() {
        mutex.wait();
        // Function Body
        if (next_count > 0)
            next.signal();
        else 
            mutex.signal();
    }
}
```
### 🙄 Extra 
- Use ```UMPLE``` to generate
- Use ```jUCMNav``` to model strategy
- Apply Feature Modeling
