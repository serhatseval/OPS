### Deadlocks

#### Debugging with GDB

```shell
gcc -g 16_dining_philosophers.c -o /tmp/03synchro_16_dining_philosophers -pthread
```

Option 1) Run process under GDB

```shell
gdb /tmp/03synchro_16_dining_philosophers
```

Use `r` command to start the execution, then `C-c` to exit to prompt once program hangs.
Use `bt` and `thread apply all bt` to analyze where all threads are hanged.
Use `q` to quit GDB.

Option 2) Attach to a running process

```shell
# Start independent process
/tmp/03synchro_16_dining_philosophers
```

```shell
# Attach GDB to a running, deadlocked process
gdb -p $(pidof 03synchro_16_dining_philosophers)
```

Use `thread apply all bt` to analyze where all threads are hanged. 

Option 3) Create and debug a coredump file

```shell
# Generate a coredump of a running process with gcore
gcore -o dining_philosophers.core $(pidof 03synchro_16_dining_philosophers)
ls -lah dining_philosophers.core.$(pidof 03synchro_16_dining_philosophers)
```

```shell
gdb /tmp/03synchro_16_dining_philosophers -c dining_philosophers.core.$(pidof 03synchro_16_dining_philosophers)
```

Try also debugging a coredump file using your IDE!
