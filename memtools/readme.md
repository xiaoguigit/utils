

# memtools



内存数据导入导出工具

### 使用方式：

##### 内存导出

memtools -m 0x40000000 -s 512 -f /tmp/xiaogui.bin -d

-m: 指定物理内存地址

-o: 指定偏移地址，可选

-s: 指定dump大小

-f: 指定输出文件名

-d: 使用导出方式(dump)

##### 内存导入

memtools -m 0x40000000 -s 512 -f /tmp/xiaogui.bin -w

-m: 指定物理内存地址

-o: 指定偏移地址，可选

-s: 指定物理内存块大小，防止写入时超出边界

-f: 指定输入文件名

-w: 使用写入方式(write)



### BAT脚本：

1、上传执行程序

2、赋予执行权限

3、执行数据导出

4、下载数据文件到本地

直接双击运行dumpMemory.bat可以将数据直接导出本地

导入内存的方式请参考上述说明和writeMemory.bat