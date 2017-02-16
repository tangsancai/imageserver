		说明：
			0.不会用qt，所以客户端写得比较渣。
			1.TCP粘包问题在客户端没有解决，所以服务器端用sleep(1)来防止粘包了。服务器端二级任务队列，每个队列在没工作时会sleep(2)，所以综上10*1+2*2这14秒的响应时间是可以缩减的。
			2.服务器在载入时，会提取simgupload中的图片的特征，读取sxml中的图片特征向量，所以开启时服务器比较慢，出现open server complete!字样时表示开启成功
			3.我会努力继续改进。学生作品轻喷。
		目标：编写一个图像检索服务器
		环境：ubuntu14.04.4\opencv2.4.9\make\g++\gdb\qt等
		功能：
			0.定时写回的错误日志
			1.保存客户端发送地图片
			2.提取图片特征向量，与特征库匹配
			3.向客户端发送topK相似度的图片
			4.更新图片库
		其他：
			0.使用epollET+非阻塞套接字+工作线程事件驱动的服务端模型
       			1.使用任务队列，分割任务，提升检索效率
			2.使用专用线程处理除硬件错误信号外的信号
        		3.使用signal处理硬件错误信号
        		4.编写、使用线程池减少线程创建销毁的开销
        		5.使用c++对象生命周期管理MUTEX、文件流等
			6.SIFT算法提取特征向量
		拓展：
			0.编写代理服务器，将客户端上传图片按图片ID hash后存储到对应的检索服务器群中的一台

![image](https://github.com/tangsancai/imageserver/blob/master/result/result.jpg)
![image](https://github.com/tangsancai/imageserver/blob/master/result/result2.jpg)





