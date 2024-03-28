# Linux VFS  

https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h

`struct inode`
https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h#L1754

`struct file`
https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h#L942

`struct file_operations`
https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h#L1754

`struct super_block`
https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h#L1136

`struct dentry`
https://elixir.bootlin.com/linux/latest/source/include/linux/dcache.h#L82

# FUSE

Required installation of `libfuse3-dev` on Ubuntu. Build with `-lfuse3`:

```bash
gcc fuse_hello.c -o /tmp/fuse_hello -lfuse3 -g -O2
```

```bash
mkdir -p mountpoint
sudo /tmp/fuse_hello -f -o allow_other mountpoint --name=sop.txt
```

# Master Boot Record

https://pl.wikipedia.org/wiki/Master_Boot_Record

List block devices:
```bash
lsblk
```

```bash
ll /dev/sdb
```

Dump MBR contents:
```bash
sudo xxd -l512 /dev/sda
```

Create MBR
```bash
sudo cfdisk /dev/sdb
```

... and destroy it:
```bash
sudo dd if=/dev/urandom of=/dev/sdb bs=512 count=1024 status=progress
```

Observe contents of the disk via `lsblk` and `xxd`.

# Mounting

```bash
sudo mkfs.ext4 /dev/sdb1
```

```bash
sudo mount /dev/sdb1 mountpoint
ll
```

```bash
mount
```

```bash
stat .
stat mountpoint
```

```bash
sudo chown saqq:saqq mountpoint
echo "Hello, world!" > mountpoint/sop.txt
```

```bash
sudo umount mountpoint
```

# FAT File Systems. FAT32, FAT16, FAT12

https://www.ntfs.com/fat_systems.htm

# ext4 Data Structures and Algorithms

https://www.kernel.org/doc/html/latest/filesystems/ext4/index.html

Display filesystem metadata:
```bash
sudo dumpe2fs /dev/sda1
```

```bash
sudo tune2fs -o journal_data /dev/sda1
```

```bash
sudo mount /dev/sda1 mountpoint
```

```bash
sudo dd if=/dev/urandom of=mountpoint/hello.txt bs=4K status=progress
```

```bash
sudo tune2fs -o journal_data_ordered /dev/sda1
```

```bash
sudo tune2fs -o journal_data_writeback /dev/sda1
```
