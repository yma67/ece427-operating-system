# 同步基础
###### Made available under ```the Wang Licence (GPL)```
## 1.1 几个概念
### 1.1.1 竞争
#### 定义 
Processes that do not work together cannot affect the execution of each other, but they can compete for devices and other resources. 
#### 性质
- Deterministic; reproducible. 
- Can stop and restart without side effects; 
- Can proceed at arbitrary rate.

### 1.1.2 合作
#### 定义 
Processes that are aware of each other, and directly (by exchanging messages) or indirectly (by sharing a common object) work together, may affect the execution of
each other. 
#### 性质 
- Share a common object or exchange messages
- Non-deterministic; May be irreproducible
- Subject to **race conditions**

### 1.1.3 Race Condition (重要)
#### 定义
When two or more processes are reading or writing, some shared data and final result depends on who runs precisely when. 
#### Example
银行存钱
#### 防止: Mutual Exclusion
- When one process is reading or writing a shared data, other processes should be prevented from doing the same. 

## 1.2 Critical Section 临界区 (重要)
### 定义
Part of the program that accesses shared data. If we arrange the processes runs such that no two processes are in their critical sections at the same time, race conditions can be avoided. 
### 1.2.1 四个性质 (非常重要)
#### 1.2.1a 互斥 (Mahesh, Bilibili)
同一时间临界区最多存在一个线程
##### 反例 
```cpp
// flag[k]: proc k entered critical section
bool flag[2] = {false, false};
...
// def Proc 0
...
while (flag[1]);
flag[0] = true;
// critical section
flag[0] = false;
...
// END Proc 0
...
// def Proc 1
...
while (flag[0]);
flag[1] = true;
// critical section
flag[1] = false;
...
// END Proc 1
```
#### 1.2.1b No. of CPUs, Execution Speed (Mahesh)
No assumptions be made about speeds or number of CPUs. 
#### 1.2.1c Progress (Mahesh, Bilibili)
如果一个线程想要进入临界区，那么他最终会成功 <br>
换句话说, **No process should have to wait forever to enter its critical section**
##### 反例1: 死锁
```cpp
// flag[k]: proc k entered critical section
bool flag[2] = {false, false};
...
// def Proc 0
...
flag[0] = true;
while (flag[1]);
// critical section
flag[0] = false;
...
// END Proc 0
...
// def Proc 1
...
flag[1] = true;
while (flag[0]);
// critical section
flag[1] = false;
...
// END Proc 1
```
- 进程0执行完flag[0] = true，调度
- 进程1执行完flag[1] = true
- 死锁
##### 反例2: 活锁
```cpp
// flag[k]: proc k entered critical section
bool flag[2] = {false, false};
...
// def Proc 0
...
flag[0] = true;
while (flag[1]) {
    flag[0] = false;
    sleep(random());
    flag[0] = true;
}
// critical section
flag[0] = false;
...
// END Proc 0
...
// def Proc 1
...
flag[1] = true;
while (flag[0]) {
    flag[1] = false;
    sleep(random());
    flag[1] = true;
}
// critical section
flag[1] = false;
...
// END Proc 1
```
- 当且仅当sleep(random())相等的时候才会出现卡死
#### 1.2.1d 有限等待 (Mahesh, Bilibili)
如果一个线程ti处于入口区，那么在ti的请求被接受前，其他线程进入临界区的时间是有限制的 <br>
换句话说, **No process running outside its critical section may block other processes**
##### 反例
```cpp
// turn = k: proc k entered critical section
int turn = 1;
...
// def Proc 0
...
while (turn != 0);
// critical section
turn = 1;
...
// END Proc 0
...
// Proc 1
...
while (turn != 1);
// critical section
turn = 0;
...
// END Proc 1
```
反证法 <br>
- 假设进程1进入临界区，完成后turn = 0
- 如果进程0是朱哥设计的，也就是进程0暂时不做为，进程1再次想要进入临界区那是不可能的
- QED
#### 1.2.1e 无忙等待 (可选) (Bilibili)
如果一个线程在等待进入临界区，那么在他可以进入之前会被挂起
### 1.2.2 软件解决方案: Peterson's Algorithm
```cpp
bool flags[2] = {false, false};
int turn = 0;
...
// process 0
flags[0] = true;
turn = 1;
while (flags[1] && turn == 1);
// Critical Section
flags[0] = false;
...
// process 1
flags[1] = true;
turn = 0;
while (flags[0] && turn == 0);
// Critical Section
flags[1] = false;
```
- Busy wait
- Dead lock
- Priority Inversion Problem
### 1.2.3 硬件解决方案: 原子指令 Test and set
Test and set指令不可中断
```asm
test_and_set: 
    PUSH {R1}
    LDR R1, [R0]
    STR R0, #1
    MOV R0, R1
    POP {R1}
    BX LR
```
```cpp
std::atomic_flag lock_stream = ATOMIC_FLAG_INIT;
while (lock_stream.test_and_set());
// critical section
lock_stream.clear();
```
Advantages
- applicable to any number of processes
- can be used with single processor or multiple processors that share a single memory
- simple and easy to verify
- can be used to support multiple critical sections, i.e., define a separate variable for each critical section

Disadvantages
- Busy waiting is employed – process waiting to get into a critical section consumes CPU time
- Starvation is possible – selection of entering process is arbitrary when multiple processes are contending to enter
