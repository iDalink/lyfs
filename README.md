##LYFS - A simple kernel file system exapmle run in 4.* version Linux.
### Important Tips!
- **The example can only used to study. It can not work as tools. The author has no reponsibilty with your losing files.**
- The code are tested in linux-4.4.
- If you met some errors, please contact QQ 934367813 or QQ group 479198558.

### How run it?
- `Makefile` file own `KDIR` variable. Please make sure it is configured to your kenel header directory.

```
KDIR := ~/compile/linux-4.4
```

- Jump to the source dir. Excute `make build` command to compile the module. Then, make sure there exists `lyfs.ko` module file.
![](https://github.com/iDalink/xx.png?raw=true)
![](https://github.com/iDalink/xx.png?raw=true)

- Excute `make mount`. It will `insmod`, and mount `output/disk.img` to `output/point`. `output/disk.img` has been formated, it contains some files. Of course, you can run `mkfs.lyfs` to format the img file. Excute `touch` to create file.
![](https://github.com/iDalink/xx.png?raw=true)

- Now you can operate the mounted file system as you want.  Excute `echo` to write some string into one file and excute `cat` to read the file. 
![](https://github.com/iDalink/xx.png?raw=true)

- Now, you can unmount the file system and remove the module from your system.
![](https://github.com/iDalink/xx.png?raw=true)

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