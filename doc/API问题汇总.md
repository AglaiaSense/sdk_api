API问题汇总

1. 未处理right和left字段，获取未打开相机时按照非法ID提示

2. <span style="color:#FF0000;">Json格式错误，导致客户端和服务端卡住</span>

3. <span style="color:#FF0000;">set_camera_param_req，对设置数值无类型判断</span>

4. set_camera_param_req，设置参数只有ae_mode和framerate有效

5. <span style="color:#FF0000;">有些请求报8193后，没有退出内层循环，导致后续的请求或设置参数全部异常</span>

6. <span style="color:#FF0000;">set_model_pkg_req，解压后并没有放到指定文件夹</span>

7. <span style="color:#FF0000;">MD5校验失败后，未强制中断后续操作</span>

8. <span style="color:#FF0000;">程序异常或者报错后，会一直卡住，不会自动退出或者重新启动</span>

9. 传输过程中缺乏提示信息

10. <span style="color:#FF0000;">传入模型文件或者后处理文件后，无论文件是否正常，都会执行重启命令</span>