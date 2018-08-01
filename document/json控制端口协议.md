# json控制端口协议
>tcp流边界:length+json data
>采用request-response通信方式，Android端作为请求方，ARMLinux端作为应答方。  
# json协议格式规定
{  
	"name":"zhangshaoyan",   
	"age":30,  
	"country":"America"  
}  
# json协议
## 1.Android请求设置数据  
{  
	"ImgPro":"on/off/query"  请求开启/关闭/查询图像处理功能  
	"RTC":"2018/07/19 14:26:53"  请求更新ARMLinux的硬件时间  
	"DeNoise":"off/RNNoise/WebRTC/Bevis/query"  请求关闭或打开音频噪声抑制算法  
	"BevisGrade":"1/2/3/4/query"  设置Bevis降噪算法等级  
	"DGain":"[0-90]/query"  设置音频数字增益,有效范围[0-90],query为查询当前值  
	"FlushUI":"on/off/query"   刷新本地UI     
	"Cam1CenterXY":"320,240/query"  设置或查询1号摄像头标定中心点坐标   
	"Cam2CenterXY":"320,240/query"  设置或查询2号摄像头标定中心点坐标   
	"Accumulated":"query"  查询设备累计运行秒数   
}  
## 2.ARMLinux返回响应结果  
{  
	"ImgPro":"on/off"  返回当前图像处理功能的状态是开启还是关闭  
	"RTC":"2018/07/19 14:26:54"  返回当前设备的RTC时间   
	"DeNoise":"off/RNNoise/WebRTC/Bevis"  返回音频噪声抑制算法的当前状态   
	"BevisGrade":"1/2/3/4"  返回Bevis降噪算法等级  
	"DGain":"off/[0-90]"  返回数字增益当前值  
	"FlushUI":"on/off"  返回是否刷新本地UI   
	"Cam1CenterXY":"320,240"  返回1号摄像头标定中心点坐标   
	"Cam2CenterXY":"320,240"  返回2号摄像头标定中心点坐标   
	"Accumulated":"1202323"  返回设备累计运行秒数   
	"ImgMatched":"x1,y1,w1,h1,x2,y2,w2,h2,diffX,diffY,costMs" 返回图像比对结果数据   
}    
>(x1,y1,w1,h1):1号摄像头的模板选定区域  
>(x2,y2,w2,h2):2号摄像头的匹配区域  
>(diffX,diffY):在2号摄像头图像中匹配到的区域中心点坐标  
>costMs:算法实际消耗的时间(毫秒)  
