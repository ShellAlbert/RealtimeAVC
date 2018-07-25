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
}  
## 2.ARMLinux返回响应结果  
{  
	"ImgPro":"on/off"  返回当前图像处理功能的状态是开启还是关闭  
	"RTC":"2018/07/19 14:26:54"  返回当前设备的RTC时间   
	"DeNoise":"off/RNNoise/WebRTC/Bevis"  返回音频噪声抑制算法的当前状态  
	"BevisGrade":"1/2/3/4"  返回Bevis降噪算法等级  
	"DGain":"off/[0-90]"  返回数字增益当前值  
	"FlushUI":"on/off"  返回是否刷新本地UI  
}    

