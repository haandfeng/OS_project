# OS_project
 Inode-Unix-Like File System

一些设想的优化

- [ ] 用数据结构优化搜索Inode

- [ ] 现成线程安全版

- [ ] 建立buffer，存读出来的block，使用LRU替换block

- [ ] 每次创建一个文件都从block里面读directory里面的block数据，这是不是太慢了

- [ ] dir查找文件是线性查找，会不会太慢了

- [ ] 不能直接返回头部

  ppt 10分钟

  提交的时候每组交一个报告 组号组名

​	   好处

​	不足之处: Undirect Inode 太少，不支持大文件

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

