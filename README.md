# VTD_RDB_Handler

- To get information from VTD via RDB messages and output csv file

- 根据vtd的官方教程修改，得到想要的数据并输出csv文件

## 操作步骤

1. 在`RDB_read.cpp`中根据自己需要的信息修改代码
2. 运行`compile.sh`进行编译
3. 打开VTD，运行编译后的`sampleClientRDB`的exe文件，终端显示`connected！`表示连接成功
4. 在VTD中运行场景，相关信息就会记录到csv文件中，csv路径在`RDB_read.cpp`中定义