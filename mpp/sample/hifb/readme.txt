1 hifb 包含两个示例sample
 1) api_sample_hifb
 2) api_sample_hifb_switchGlayer
 
2 如何运行sample? 
	1）运行sample/hifb/下的load脚本，该脚本加入了5个图形层G0~G4，分别对应设备/dev/fb0 ~ /dev/fb4
	2) 直接运行api_sample_hifb，可在高清设备上看到输出结果
	3) 直接运行api_sample_hifb_switchGlayer,可分别在HD、AD、SD设备上分别看到图形层G0，G2，G3。鼠标由
	   G4图形层实现，并支持其在HD和AD设备间的切换

  	注：图形层原理请参见文档《HiFB 开发指南》和《Hi3520 图形方案用户指南》
  	
  	
  	