## LYFS - A simple kernel file system example run in 4.* version Linux.
### Important Tips!
- **The example can only be used to study. It can not work as tools. The author has no reponsibilty to your file & device damage.**
- The code has been tested in linux-4.4.
- If you met some errors, please contact QQ 934367813 or QQ group 479198558.

### How run it?
- `Makefile` file own `KDIR` variable. Please make sure it is configured to your kenel header directory.

```
KDIR := ~/compile/linux-4.4
```

- Jump to the source dir. Excute `make build` command to compile the module. Then, make sure there exists `lyfs.ko` module file.

![](https://github.com/iDalink/lyfs/blob/master/img/%EF%BC%92%E7%BC%96%E8%AF%91.png?raw=true)
![](https://github.com/iDalink/lyfs/blob/master/img/1%E8%BF%9B%E5%85%A5%E7%9B%AE%E5%BD%95.png?raw=true)

- Excute `make mount`. It will `insmod`, and mount `output/disk.img` to `output/point`. `output/disk.img` has been formated, it contains some files. Of course, you can run `mkfs.lyfs` to format the img file. Excute `touch` to create file.

![](https://github.com/iDalink/lyfs/blob/master/img/%EF%BC%93%E5%AE%89%E8%A3%85%E5%B9%B6%E6%8C%82%E8%BD%BD.png?raw=true)

![](https://github.com/iDalink/lyfs/blob/master/img/%EF%BC%94%E8%BF%9B%E5%85%A5%E5%AD%90%E7%9B%AE%E5%BD%95.png?raw=true)

- Now you can operate the mounted file system as you want.  Excute `echo` to write some string into one file and excute `cat` to read the file. 

![](https://github.com/iDalink/lyfs/blob/master/img/%EF%BC%95%E8%AF%BB%E5%86%99%E6%96%87%E4%BB%B6.png?raw=true)

- Now, you can unmount the file system and remove the module from your system.

![](https://github.com/iDalink/lyfs/blob/master/img/%EF%BC%96%E5%8F%96%E6%B6%88%E6%8C%82%E8%BD%BD%E5%B9%B6%E5%8D%B8%E8%BD%BD.png?raw=true)

### LYFS architecture

```
super_block:
	every_block is 4KB.
	super_block has 4KB.
	first meta is root. it has no parent and name.
	meta_area is from 4KB.
	partion_size [4B]
	meta_area size = .
	block_area index.

meta_area:
	file name has 16 char. [16B]
	is file or dentry. [1B]
	block_area index. [4B]
	parent meta_are index.[4B]
	16 child meta_are index. [4B * 16]

data_area:
	file_size. [4B]
	....
	next block index [4B]


----------------------------------------------------------------------
|				|			  |			   |			 |			 |
|  super_block	| meta_bitmap |	meta_area  | data_bitmap | data_area |
|				|		 	  |			   |			 |			 |
----------------------------------------------------------------------

```
