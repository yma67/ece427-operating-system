### ECE427 Operating System
###### Made available under ```the Wang Licence (3-Clause BSD)```

#### Tiny Shell
##### Compile and execute
- to enable print of prompt
```bash
gcc tiny_shell.c -o tshell -DPRINT_PROMPT=1
``` 
- to enable error code printing
```bash
gcc tiny_shell.c -o tshell -DPRINT_RET=1
``` 
- to enable both
```bash
gcc tiny_shell.c -o tshell -DPRINT_PROMPT=1 -DPRINT_RET=1
``` 

##### Features
- change directory use ```chdir DIR``` or ```cd DIR```
- change limit in memory use of subprocess using ```limit NUM_BYTES```
- view history use ```history```
- other common system commands
- pipe supports two programs using FIFO 
```bash
mkfifo myfifo -m 755
./tshell myfifo
```
- ```Ctrl+C``` to exit program if there is one, exit shell if there is not
- ```./tshell myfifo < script.txt``` to run script
- command line prompt showing current working directory

#### Simulation of the Readers Writers Problem
##### compile
- to show starvation, compile with 
```bash 
gcc [this_filename].c -o rwsim -lpthread
```
- to show solve of starvation, compile with
```bash 
gcc [this_filename].c -o rwsim -lpthread -DEQUAL
```
- to show values in variables, append ```-DPNUM``` to any of the above commands
##### execute
```bash
./rwsim NUM_TRAILS_WRITER NUM_TRAILS_READER
```
##### With Reader Preference (Starvation Observed)
```bash
yma67@teaching:~/ws/ece427/a2$ gcc A2Q1.c -o a2q1 -lpthread
yma67@teaching:~/ws/ece427/a2$ ./a2q1 30 60
========[Reader Preference]=======
[Reader]>>>>>>>>>>>>>>>>>>>>>>>>>>
[Max wait] 12.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 0.010708 ms
[Writer]>>>>>>>>>>>>>>>>>>>>>>>>>>
[Max wait] 3291.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 109.443333 ms
```
##### With Reader-Writer Fair (Almost No Starvation Observed)
``` bash
yma67@teaching:~/ws/ece427/a2$ gcc A2Q3.c -o a2q3 -lpthread
yma67@teaching:~/ws/ece427/a2$ ./a2q3 30 60
==============[Fair]==============
[Reader]>>>>>>>>>>>>>>>>>>>>>>>>>>
[Max wait] 521.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 5.173833 ms
[Writer]>>>>>>>>>>>>>>>>>>>>>>>>>>
[Max wait] 485.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 7.930000 ms
```
