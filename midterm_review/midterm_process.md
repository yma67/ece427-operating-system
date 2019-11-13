# 进程
## Process
- An operating system executes a variety of programs that run as a process.
- Process – a program in execution; process execution must progress in sequential fashion
- Multiple parts
  - The program code, also called text section
  - Current activity including program counter, processor registers
  - Stack containing temporary data
    - Function parameters, return addresses, local variables
  - Data section containing global variables
  - Heap containing memory dynamically allocated during run time
- As a process executes, it changes state
  - New:  The process is being created
  - Running:  Instructions are being executed
  - Waiting:  The process is waiting for some event to occur
  - Ready:  The process is waiting to be assigned to a processor
  - Terminated:  The process has finished execution

### Process Creation
- Address space
  - Child duplicate of parent
  - Child has a program loaded into it
- UNIX examples
  - ```fork()``` system call creates new process
  - ```exec()``` system call used after a ```fork()``` to replace the process’ memory space with a new program
  - Parent process calls ```wait()``` for the child to terminate

### Process Termination
- Process executes last statement and then asks the operating system to delete it using the ```exit()``` system call.
  - Returns status data from child to parent (via ```wait()```)
  - Process’ resources are deallocated by operating system
- Parent may terminate the execution of children processes using the ```abort()``` system call. Some reasons for doing so:
  - Child has exceeded allocated resources
  - Task assigned to child is no longer required
  - The parent is exiting and the operating systems does not allow a child to continue if its parent terminates
- Some operating systems do not allow child to exists if its parent has terminated.  If a process terminates, then all its children must also be terminated.
  - cascading termination.  All children, grandchildren, etc.  are  terminated.
  - The termination is initiated by the operating system.
- The parent process may wait for termination of a child process by using the wait()system call. The call returns status information and the pid of the terminated process
  -pid = wait(&status); 
- 我们知道在unix/linux中，正常情况下，子进程是通过父进程创建的，子进程在创建新的进程。子进程的结束和父进程的运行是一个异步过程,即父进程永远无法预测子进程 到底什么时候结束。 当一个 进程完成它的工作终止之后，它的父进程需要调用```wait()```或者```waitpid()```系统调用取得子进程的终止状态。
  - 孤儿进程(orphan)：一个父进程退出，而它的一个或多个子进程还在运行，那么那些子进程将成为孤儿进程。孤儿进程将被```init```进程(进程号为1)所收养，并由```init```进程对它们完成状态收集工作。
  - 僵尸进程(zombie)：一个进程使用```fork```创建子进程，如果子进程退出，而父进程并没有调用```wait```或```waitpid```获取子进程的状态信息，那么子进程的进程描述符仍然保存在系统中。这种进程称之为僵死进程

### Program
- Program is passive entity stored on disk (executable file); process is active 
  - Program becomes process when executable file loaded into memory
- Execution of program started via GUI mouse clicks, command line entry of its name, etc
- One program can be several processes
  - Consider multiple users executing the same program
```
(0x0000ish min) | text | data | heap | -> ... <- | stack | (max 0xFFFFish)
```

### Pipe
- Acts as a conduit allowing two processes to communicate
- Issues:
  - Is communication unidirectional or bidirectional?
  - In the case of two-way communication, is it half or full-duplex?
  - Must there exist a relationship (i.e., parent-child) between the communicating processes?
  - Can the pipes be used over a network?
- **Ordinary pipes** 
  - cannot be accessed from outside the process that created it. Typically, a parent process creates a pipe and uses it to communicate with a child process that it created. 
  - allow communication in standard producer-consumer style
    - Producer writes to one end (the write-end of the pipe)
    - Consumer reads from the other end (the read-end of the pipe)
    - Ordinary pipes are therefore unidirectional
    - Require parent-child relationship between communicating processes
- **Named pipes** 
  - can be accessed without a parent-child relationship.
  - Reader give up fifo only closed

