# OS基础
## OS Startup/operation
- Bootstrap program – simple code to initialize the system, load the kernel
- Kernel loads
- Starts system daemons (services provided outside of the kernel)
- Kernel interrupt driven (hardware and software)
  - Hardware interrupt by one of the devices 
  - Software interrupt (exception or trap):
    - Software error (e.g., division by zero)
    - Request for operating system service – system call
    - Other process problems include infinite loop, processes modifying each other or the operating system

## OS functions
- Control access and provide interfaces
- Manage resources
- Provide abstractions
- Creating and deleting both user and system processes
- Suspending and resuming processes
- Providing mechanisms for process synchronization
- Providing mechanisms for process communication
- Providing mechanisms for deadlock handling

## IO
- After I/O starts, control returns to user program only upon I/O completion (Synchronous)
  - Wait instruction idles the CPU until the next interrupt
  - Wait loop (contention for memory access)
  - At most one I/O request is outstanding at a time, no simultaneous I/O processing
- After I/O starts, control returns to user program without waiting for I/O completion (Asynchronous)
  - **System call** – request to the OS to allow user to wait for I/O completion
  - **Device-status table** contains entry for each I/O device indicating its type, address, and state, and interrupt
  - OS indexes into I/O device table to determine device status and to modify table entry to include interrupt

## Multiprocessors
- Multiprocessors systems growing in use and importance
  - Also known as parallel systems, tightly-coupled systems
  - Advantages include:
    - Increased throughput
    - Economy of scale
    - Increased reliability – graceful degradation or fault tolerance
  - Two types:
    - Asymmetric Multiprocessing – each processor is assigned a special task.
    - Symmetric Multiprocessing – each processor performs all tasks
    
## Cluster
- Like multiprocessor systems, but multiple systems working together
  - Usually sharing storage via a storage-area network (SAN)
  - Provides a high-availability service which survives failures
    - Asymmetric clustering has one machine in hot-standby mode
    - Symmetric clustering has multiple nodes running applications, monitoring each other
  - Some clusters are for high-performance computing (HPC)
  - Applications must be written to use parallelization
  - Some have distributed lock manager (DLM) to avoid conflicting operations

## Multiprogramming
- Multiprogramming (Batch system) needed for efficiency
  - Single user cannot keep CPU and I/O devices busy at all times
  - Multiprogramming organizes jobs (code and data) so CPU always has one to execute
  - A subset of total jobs in system is kept in memory
  - One job selected and run via job scheduling
  - When it has to wait (for I/O for example), OS switches to another job

- Timesharing (multitasking) is logical extension in which CPU switches jobs so frequently that users can interact with each job while it is running, creating interactive computing
  - Response time should be < 1 second
  - Each user has at least one program executing in memory process
  - If several jobs ready to run at the same time  CPU scheduling
  - If processes don’t fit in memory, swapping moves them in and out to run
  - Virtual memory allows execution of processes not completely in memory

## Dual/multi mode
- Dual-mode operation allows OS to protect itself and other system components
  - **User mode** and **kernel mode**
  - Mode bit provided by hardware
    - Provides ability to distinguish when system is running user code or kernel code
    - Some instructions designated as privileged, only executable in kernel mode
    - System call changes mode to kernel, return from call resets it to user
- Increasingly CPUs support multi-mode operations
  - i.e. virtual machine manager (VMM) mode for guest VMs
  
## From user to kernel
- User mode is prevented from engaging in privileged operations (e.g., file access)
- Program needs to switch to kernel mode to carry out those operations
- Need to protect the system by restricting what application can do

## How system call is processed?
1. system service is requested (system call)
2. switch mode; verify arguments and service
3. branch to the service function via system call table
4. return from service function; switch mode
5. return from system call

background program runs in user context
