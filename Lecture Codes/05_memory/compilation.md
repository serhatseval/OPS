### Simple program compilation

```bash
gcc -o main main.c hello.c 
./main
```

### 4 compilation phases

```bash
gcc -E -o hello.i hello.c  # Stop after pre-processing
```

```bash
gcc -S -o hello.s hello.i  # Stop after compiler
```

```bash
gcc -c hello.s  # Stop after assembler
```

```bash
gcc -o hello hello.o  # Stop after linker
```

### Typical buildsystem behavior

```bash
gcc -c main.c
gcc -c hello.c
gcc -o main main.o hello.o
./main
```

### Inspect file types

```bash
file main.c
file main.o
file main
```

### Inspect dynamically linked libraries

```bash
ldd main
```

### Inspect ELF headers

```bash
readelf -h main.o
```

```bash
readelf -h main
```

### Inspect ELF symbol tables

```bash
readelf -s main.o
readelf -s hello.o
```

```bash
readelf -s main
```

```bash
objdump --disassemble=foo hello.o
```

```bash
objdump --disassemble=foo main
```

```bash
objdump -dj .text hello.o
```

```bash
readelf -r hello.o
```

```bash
readelf -r main
```

### Compile static version

```bash
gcc -o main.static -static main.o hello.o
./main.static
```

```bash
ldd main.static
```

```bash
file main.static
```

```bash
readelf -h main.static
```

```bash
readelf -s main.static
```

```bash
objdump --disassemble=foo main.static
```

### Runtime linking

```bash
gcc -g -o dynamic_rt dynamic_rt.c -ldl  
```
