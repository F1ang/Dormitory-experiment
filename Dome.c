#include reg52.h
//串口初始化
 void UsartInit()
{
  TMOD=0x20;         
   TH1=0xfd;
   TL1=0xfd;//9600
   TR1=1;
   REN=1;
   SM0=0;
   SM1=1;
   EA=1;
   ES=1;
}
 void Usart() interrupt 4
{
	u8 receiveData;
	RI = 0;//RXNE清
	receiveData=SBUF;
	SBUF=receiveData;
	while(!TI); //TXNE
	TI=0;
}//测试成功
//定时器0
 void Timer0Init()
{
	TMOD|=0X01;//tim0,方式1，仅用TR0启动

	TH0=0XFC;	//ARR--1ms
	TL0=0X18;	
	ET0=1;//tim0-enable
	EA=1;//总中断-enable
	TR0=1;//打开tim0			
}
 void Timer0() interrupt 1
{
	static u16 i;
	TH0=0XFC;	//ARR
	TL0=0X18;
	i++;
	if(i==1000)
	{
		i=0;
	}	
}
//定时器1
 void Timer1Init()
{
	TMOD|=0X10;//tim1,方式1，仅用TR1启动

	TH1=0XFC;
	TL1=0X18;	
	ET1=1;//tim1-enable
	EA=1;//总中断-enable
	TR1=1;//打开tim0			
}
 void Timer1() interrupt 3
{
	static u16 i;
	TH1=0XFC;	//重装载
	TL1=0X18;
	i++;
	if(i==1000)
	{
		i=0;
	}	
}//实现
//外部中断0  --//设置 INT0-P3.2
 void Int0Init()
{
IT0=1;//下降沿
EX0=1;//INT0中断允许
EA=1;//打开总中断
}
void Int0() interrupt 0 //中断处理函数
{
delay(1000); 
if(k3==0)
    {

    }
}
//外部中断1 --//设置 INT1-P3.3
 void Int1Init()
{
	IT1=1;//下降沿
	EX1=1;	
	EA=1;	
}
void Int1()	interrupt 2	
{
	delay(1000);	
	if(k4==0)
	{
		led=~led;
	}
}
//LCD1602说明：16*2--即2行，每行16个字符。  注：请配合附带pdf文件。
//Pin信息，DRAM地址操作，CGROM和CGRAM固化字模数据了。
//命令：清屏、光标归位、光标移位方向、显示开关、光标或屏幕移位、显示字体、读取状态等。
//控制线RS-命令或数据 、WR-写或读、使能E  数据线D0-D7 
//具体操作步骤：利用控制线和数据线时序，完成读取和写入工作时序代码->实现字模显示.
void LcdWriteCom(uchar com)	  //写入工作时序
{
	LCD1602_E = 0;    //控制线-E、RS、RW
	LCD1602_RS = 0;	   
	LCD1602_RW = 0;	   
	
	LCD1602_DATAPINS = com;  //命令
	Lcd1602_Delay1ms(1);

	LCD1602_E = 1;	          
	Lcd1602_Delay1ms(5);	  //延时
	LCD1602_E = 0;
}
void LcdWriteData(uchar dat)//读取工作时序
{
	LCD1602_E = 0;	
	LCD1602_RS = 1;	
	LCD1602_RW = 0;	

	LCD1602_DATAPINS = dat; //数据
	Lcd1602_Delay1ms(1);

	LCD1602_E = 1;  
	Lcd1602_Delay1ms(5); 
	LCD1602_E = 0;
}
void LcdInit()	//注：请配合pdf文件					 
{
 	LcdWriteCom(0x38);  //开显示
	LcdWriteCom(0x0c);  
	LcdWriteCom(0x06); 
	LcdWriteCom(0x01);  //清屏
	LcdWriteCom(0x80);  
}
//DS18B20温度模块：数字式   注：请配合附带pdf文件。
//Pin信息，DS18B20方框图，温度量化表和计算公式。
//单总线的分时复用，480us低电平则复位总线（下面时序所有的延迟可能都有类似作用）。
//初始化：主机发，从机应答。 ROM命令：读33h,寻55h，跳cch，搜F0h，警ECh。--为了配合指令表。
//注：或许可以跳过前面的直接来到存储器操作流程图等时序（推荐此时配合程序看）。
//具体操作：初始化DS18B20->读写时序->具体指令写入（看指令表）。
//注：数据H L字节读取，再组合（看方框图）， 读写时序为写0和写1时间片时序。
uchar Ds18b20Init()//初始化DS18B20
{
	uchar i;
	DSPORT = 0;			 //单总线DQ拉低480us~960us
	i = 70;	
	while(i--);//取642us
	DSPORT = 1;//等应答，15us~60us总线被拉高则有应答。
	i = 0;
	while(DSPORT)	//拉低
	{
		Delay1ms(1);
		i++;
		if(i>5)//等待>5MS
		{
			return 0;//初始化失败
		}
	
	}
	return 1;
}
void Ds18b20WriteByte(uchar dat)//写工作时序
{
	uint i, j;

	for(j=0; j<8; j++)
	{
		DSPORT = 0;	     	  //总线拉低1us
		i++;
		DSPORT = dat & 0x01;  //写入数据，最低位开始
		i=6;
		while(i--); //延时68us，至少60us--写0
		DSPORT = 1;	//释放总线    
		dat >>= 1;
	}
}//注：单总线分时复用，在总线被拉低下，总线的写时间片被写入1/0，15us~60us对DQ进行采样，写周期至少留1us恢复。
uchar Ds18b20ReadByte()//读工作时序
{
	uchar byte, bi;
	uint i, j;	
	for(j=8; j>0; j--)
	{
		DSPORT = 0;
		i++;
		DSPORT = 1;
		i++;
		i++;
		bi = DSPORT;
		byte = (byte >> 1) | (bi << 7);						  
		i = 4;		
		while(i--);
	}				
	return byte;
}
void  Ds18b20ChangTemp()//配合读写完成想完成的指令操作
{
	Ds18b20Init();
	Delay1ms(1);
	Ds18b20WriteByte(0xcc);	//ROM			 
	Ds18b20WriteByte(0x44);	//温度转换    
}
void  Ds18b20ReadTempCom()
{	

	Ds18b20Init();
	Delay1ms(1);
	Ds18b20WriteByte(0xcc);	 
	Ds18b20WriteByte(0xbe);	 //读取温度
}
int Ds18b20ReadTemp()//直接封装成API
{
	int temp = 0;
	uchar tmh, tml;
	Ds18b20ChangTemp();			 	
	Ds18b20ReadTempCom();			
	tml = Ds18b20ReadByte();		//L
	tmh = Ds18b20ReadByte();		//H
	temp = tmh;
	temp <<= 8;
	temp |= tml;
	return temp;
}
void datapros(int temp) //读取上面温度的处理（公式和量化关系），可直接用吧。	 
{
   	float tp;  
	if(temp< 0)
  	{
		DisplayData[0] = 0x40;
		temp=temp-1;
		temp=~temp;
		tp=temp;
		temp=tp*0.0625*100+0.5;	
  	}
 	else
  	{			
		DisplayData[0] = 0x00;
		tp=temp;
		temp=tp*0.0625*100+0.5;	
	}
	DisplayData[1] = smgduan[temp % 10000 / 1000];
	DisplayData[2] = smgduan[temp % 1000 / 100];
	DisplayData[3] = smgduan[temp %  100 / 10];
	DisplayData[4] = smgduan[temp %  10 / 1];
}
//DS1302时钟   注:配合pdf看
//Pin信息，控制线和数据线RST、SCLK、I/O，方框图，地址命令字-帧头（即命令地址）。
//注：SCLK和晶振一个为自己产生去驱动数据的I/O，一个是外部拿来驱动自己的滴答。
//注：RST、SCLK、I/O的模拟时序图，即产生单/多字节的I/O，帧头来达成指令操作。
//具体操作：写一字节实现->写多字节实现->读一字节实现->读多字节实现->DS1302初始化实现。

uchar TIME[7] = {0, 0, 0x12, 0x07, 0x05, 0x06, 0x16};//s,min,h,ri,yue,zhou,year.
uchar code READ_RTC_ADDR[7] = {0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d}; //写入和读取地址
uchar code WRITE_RTC_ADDR[7] = {0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c};//时分秒日月周年
void Ds1302Write(uchar addr, uchar dat)//写地址+数据
{
	uchar n;
	RST = 0;
	_nop_();

	SCLK = 0;
	_nop_();
	RST = 1; 
	_nop_();

	for (n=0; n<8; n++)
	{
		DSIO = addr & 0x01;
		addr >>= 1;
		SCLK = 1;
		_nop_();
		SCLK = 0;
		_nop_();
	}
	for (n=0; n<8; n++)
	{
		DSIO = dat & 0x01;
		dat >>= 1;
		SCLK = 1;
		_nop_();
		SCLK = 0;
		_nop_();	
	}	
		 
	RST = 0;
	_nop_();
}
uchar Ds1302Read(uchar addr)//读数据
{
	uchar n,dat,dat1;
	RST = 0;
	_nop_();

	SCLK = 0;
	_nop_();
	RST = 1;
	_nop_();

	for(n=0; n<8; n++)
	{
		DSIO = addr & 0x01;
		addr >>= 1;
		SCLK = 1;
		_nop_();
		SCLK = 0;
		_nop_();
	}
	_nop_();
	for(n=0; n<8; n++)
	{
		dat1 = DSIO;
		dat = (dat>>1) | (dat1<<7);
		SCLK = 1;
		_nop_();
		SCLK = 0;
		_nop_();
	}
	RST = 0;
	_nop_();	
	SCLK = 1;
	_nop_();
	DSIO = 0;
	_nop_();
	DSIO = 1;
	_nop_();
	return dat;	
}
void Ds1302ReadTime()//读多字节-时间  注：写多字节也可for实现
{
	uchar n;
	for (n=0; n<7; n++)
	{
		TIME[n] = Ds1302Read(READ_RTC_ADDR[n]);
	}
		
}
void Ds1302Init()//初始化Ds1302
{
	uchar n;
	Ds1302Write(0x8E,0X00);	
	for (n=0; n<7; n++)
	{
		Ds1302Write(WRITE_RTC_ADDR[n],TIME[n]);	
	}
	Ds1302Write(0x8E,0x80);	
}
//极客魔方--2021.11.30