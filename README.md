# ece427-operating-system
Made available under ```the Wang Licence```

## Assignment 2 Test Report
### With Reader Preference (Starvation Observed)
```bash
yma67@teaching:~/ws/ece427/a2$ gcc rwx.c -lpthread
yma67@teaching:~/ws/ece427/a2$ ./a.out 60 30
=============[SUMMARY]============
[READER]>>>>>>>>>>>>>>>>>>>>>>>>>>
[Max wait] 1000.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 0.133373 ms
[Writer]>>>>>>>>>>>>>>>>>>>>>>>>>>
[Max wait] 3107000.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 103390.000000 ms
```
### With Reader-Writer Fair (Almost No Starvation Observed)
```bash
yma67@teaching:~/ws/ece427/a2$ gcc rwx.c -lpthread -DEQUAL
yma67@teaching:~/ws/ece427/a2$ ./a.out 60 30
=============[SUMMARY]============
[READER]>>>>>>>>>>>>>>>>>>>>>>>>>>
[Max wait] 328000.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 3708.077641 ms
[Writer]>>>>>>>>>>>>>>>>>>>>>>>>>>
[Max wait] 322000.000000 ms
[Min wait] 0.000000 ms
[Avg wait] 6243.333333 ms
```
