项目运行截图：

![image-20241226210444351](https://mypic-1306876708.cos.ap-guangzhou.myqcloud.com/img/202412262104439.png)

项目运行步骤：
1. 将项目git clone 到本地库中
2. 通过vs软件打开sln文件
3. 查看项目配置属性，是否导入了ffmpeg文件夹下的相关文件
  头文件include： ![image](https://github.com/user-attachments/assets/1d1d9321-5f5b-49ce-b14e-3bde153832ed)
  库文件lib：![image](https://github.com/user-attachments/assets/6fde0f1e-5cb8-4500-b3b5-c6dea9477755)
4. 编译该项目，生成可执行文件
5. 将ffmpeg/bin文件下的dll动态库拷贝到exe文件目录中，即可执行程序：![image](https://github.com/user-attachments/assets/c4152d36-ad5e-43a2-a0a9-953fda10d1ca)
