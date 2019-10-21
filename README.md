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
./rwsim 60 30
```
##### With Reader Preference (Starvation Observed)
```bash
========[Reader Preference]======= 
[Reader]>>>>>>>>>>>>>>>>>>>>>>>>>> 
[Max wait] 4000.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 0.368559 ms 
[Writer]>>>>>>>>>>>>>>>>>>>>>>>>>> 
[Max wait] 1466000.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 48610.000000 ms
```
##### With Reader-Writer Fair (Almost No Starvation Observed)
``` bash
==============[Fair]============== 
[Reader]>>>>>>>>>>>>>>>>>>>>>>>>>> 
[Max wait] 416000.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 4714.309568 ms
[Writer]>>>>>>>>>>>>>>>>>>>>>>>>>> 
[Max wait] 385000.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 5753.333333 ms
```
