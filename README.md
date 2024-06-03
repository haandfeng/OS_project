# OS_project
 Inode-Unix-Like File System

更新思路：

1. 磁盘锁，给磁盘的操作 fileseek fileread filewrite加锁

2. 文件锁（通过Inode实现），不知道为什么写不出来，但是已经有雏形了，只是跑不通，截图吧

3. direct的fileentry 用读出来的时候B+树存储，这样搜索的速度可以加快。但是写入磁盘的时候，所有的fileentry还是线性存储。（还没写完）
4. direct读出来之后，用map<key = inode_id, value = direct>缓存一下这个direct，这样查找文件路径的时候，就

4. 只有相对路径跳转，没有绝对路径跳转。 懒得写了，老师应该不在乎，有空可以改



要求：

ppt 10分钟

提交的时候每组交一个报告 组号组名

好处：

- 速度快：速度上的改进操作，空块指针，找空块速度快
- 对文件目录的缓存技术
- 用B+树记录fileentry，大文件检索速度快
- 磁盘锁，支持多进程访问文件系统
- Inode读写锁，支持多进程对同一文件的安全操作

不足之处: 

- Undirect Inode 太少，不支持大文件

- 没有Inode ref count，不支持文件链接操作，也不支持多用户访问

鼓励独创性，新奇想法写在上面实现

测试代码

```
help
sum
mkdir /mydir/dir1/sub1
cd /mydir
mkfile /dir1/sub1/hello1.txt 3
cd /dir1/sub1
ls
cat hello1.txt
cd ..
cd ..
cp /dir1/sub1/hello1.txt /dir1/hello2.txt
cd dir1
ls
rmfile hello2.txt
ls
rmdir sub1
ls 
cd ..
sum
```

