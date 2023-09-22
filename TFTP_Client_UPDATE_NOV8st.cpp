// TFTP_Client_update _Nov_8st.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "client.h"
#include <string>
#include <malloc.h>
using namespace std;

time_t rawTime;						//时间处理相关参数
tm* nowtime;
FILE* logFp;						//log日志记录文件路径
char logBuf[512];					//log日志记录开的字符数组
unsigned int addr_len;				//ip地址的长度
unsigned long Opt = 1;				//ioctlsocket函数的参数（用于设定sock为非阻塞模式）
double transByte, consumeTime;		//传输的字节数与花费的时间（吞吐率计算）
sockaddr_in server_add;				//服务器的IP地址
SOCKET sock;						//使用的socket

//LOG:操作类型；filename：操作文件；choose：传输模式；lo：只用在传输完成之上，计算遗失率
void LOG_UP(int LOG, char* filename, int choose,int block)//上传模式//根据当前是否接受到数据包分为三种操作//该函数实现了日志记录
{

	time(&rawTime);	// 获取时间
	nowtime = localtime(&rawTime);	// 转化为当地时间
	switch (LOG) {
	case LOG_UP_FILE_NOT_EXISTS:
		sprintf(logBuf, "%s ERROR: upload %s, mode: %s, %s\n", asctime(nowtime), filename, choose == 1 ? ("netascii") : ("octet"), "File not exists!");
		//logBuf即为为日志开的数组//记录信息：1.时间，文件状态（ERROR，OVER ），文件名，文件格式  文件当前状态具体信息 
		break;
	case LOG_UP_NOT_RCV_ACK:
		sprintf(logBuf, "%s OVER: upload %s, mode: %s, %s,%d,%s\n", asctime(nowtime), filename, choose == 1 ? ("netascii") : ("octet"), "Sending block",block,"but not rcv ack ");
		break;
	case LOG_UP_NOT_RCV_FROM_SERVER:
		sprintf(logBuf, "%s ERROR: upload %s, mode: %s, %s\n", asctime(nowtime), filename, choose == 1 ? ("netascii") : ("octet"), "Could not receive from server.");
		break;
	case LOG_UP_NOT_RCV_ACK_IN_RETRAN:
		sprintf(logBuf, "%s ERROR: upload %s, mode: %s, %s\n", asctime(nowtime), filename, choose == 1 ? ("netascii") : ("octet"), "Could not receive from server.");
		break;
	}
	for (int i = 0; i < 512; i++) {
		if (logBuf[i] == '\n') {
			logBuf[i] = ' ';
			break;
		}
	}
	fwrite(logBuf, 1, strlen(logBuf), logFp);//将日志信息写入文件logFp（字节为单位）
	return;
}

void LOG_DOWN(int LOG, char* remoteFile, char* localFile, int choose, int block)//下载模式
{
	time(&rawTime);	// 获取时间
	nowtime = localtime(&rawTime);	// 转化为当地时间
	switch (LOG) {
	case LOG_DOWN_FILE_NOT_CREATE:
		sprintf(logBuf, "%s ERROR: download %s as %s, mode: %s, Create file \"%s\" error.\n", asctime(nowtime), remoteFile, localFile, choose == 1 ? ("netascii") : ("octet"), localFile);
		break;
	case LOG_DOWN_NOT_RCV_FROM_SERVER:
		sprintf(logBuf, "%s ERROR: download %s as %s, mode: %s, Could not receive from server.\n", asctime(nowtime), remoteFile, localFile, choose == 1 ? ("netascii") : ("octet"));
		break;
	case LOG_DOWN_NOT_RCV_DATA:
		sprintf(logBuf, "%s WARN: download %s as %s, mode: %s, Can't receive DATA #%d, resent\n", asctime(nowtime), remoteFile, localFile, choose == 1 ? ("netascii") : ("octet"), block);
		break;
	case LOG_DOWN_RCV_DATA_OVERTIME:
		sprintf(logBuf, "%s ERROR: download %s as %s, mode: %s, Wait for DATA #%d timeout.\n", asctime(nowtime), remoteFile, localFile, choose == 1 ? ("netascii") : ("octet"), block);
		break;
	}
	for (int i = 0; i < 512; i++) {
		if (logBuf[i] == '\n') {
			logBuf[i] = ' ';
			break;
		}
	}
	fwrite(logBuf, 1, strlen(logBuf), logFp);
	return;
}

//初始化winsock
int Init_winsock(int &nRC, WSADATA& wsaData)
{
	nRC = WSAStartup(0x0101, &wsaData);
	if (nRC)
	{
		printf("错误：客户端初始化winsock出错！\n");
		return 0;
	}
	if (wsaData.wVersion != 0x0101)
	{
		printf("错误：客户端winsock版本出错!\n");
		WSACleanup();
		return 0;
	}
	printf("客户端winsock成功初始化!\n\n");
	return 1;
}

//设置server的ip的函数
int Set_serverip(char* serverIP)//设置目标服务器相关信息，IP+host
{
	int flag;
	printf("******************************************\n");
	printf("** 请您设定服务器IP地址(用于创建套接字):**\n");
	printf("**（输入1：使用默认的IP地址(127.0.0.1)  **\n");
	printf("**（输入2：使用输入的新IP地址           **\n");
	printf("******************************************\n");
	scanf("%d", &flag);
	if (flag == 1) {
		strcpy(serverIP, "127.0.0.1");
		printf("①服务器IP地址设置为:127.0.0.1\n");
	}
	else if (flag == 2) {
		printf("①服务器IP地址设置为:");
		scanf("%s", serverIP);
	}
	//2.1 设置服务器的地址族(ipv4)，端口和ip
	server_add.sin_family = AF_INET;
	server_add.sin_port = htons(69);//初始HOST为69
	server_add.sin_addr.S_un.S_addr = inet_addr(serverIP);
	//2.2 创建socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ioctlsocket(sock, FIONBIO, &Opt);	//设定进程为非阻塞模式
	if (sock == INVALID_SOCKET)
	{
		// 创建失败
		printf("错误：创建套接字（socket）失败！\n\n");
		WSACleanup();
		return 0;
	}
	printf("创建套接字（socket)成功！\n\n");
	return 1;
}
int main()
{
	/*---------------------------------main函数定义区域-------------------------------------*/
	char serverIP[20];
	char buf[50];
	char buf2[50];
	char* token1, * token2;
	int nRC;
	WSADATA wsaData;
	int flag;								//选择（是使用默认ip地址还是自己输入IP地址）
	addr_len = sizeof(struct sockaddr_in);	//地址长度
	/*--------------第零步：准备好log记录的文件指针-----------------------------------------*/
	time(&rawTime);
	nowtime = localtime(&rawTime);
	logFp = fopen("tftpclient.log", "a");
	if (logFp == NULL)
	{
		printf("错误：不能打开log文件！\n");
	}
	printf("本地时间为:%s\n", asctime(nowtime));
	/*--------------第三步：上传/下载文件---------------------------------------------------*/
	while (1) {
		printf("******************************************\n");
		printf("**欢迎使用TFTP客户端系统！              **\n");
		printf("**请选择你要执行的操作：                **\n");
		printf("**1.上传文件（upload）                  **\n");
		printf("**2.下载文件（download）                **\n");
		printf("**0.退出系统                            **\n");
		printf("******************************************\n");
		scanf("%d", &flag);
		system("cls");//清屏操作
		if (flag == 1)			//上传文件
		{
			printf("您已经成功进入上传文件功能模块！\n");
			Init_winsock(nRC, wsaData);//初始化Winsock
			Set_serverip(serverIP);//设置服务器相关信息，serverIP为指向IP地址的指针
			printf("******************************************\n");
			printf("**请您设定待上传的文件路径:             **\n");
			printf("******************************************\n");
			printf("②待上传的文件路径设置为：");
			scanf("%s", buf);//获取上传文件的路径信息
			token1 = buf;//将上传文件路径信息赋值于token1
			if (token1 != NULL) {//上传文件路径信息不为NULL
				upload(token1);//调用上载文件函数，实现文件的上载（文件上载函数包含REQ报文的构造以及DATA报文的构造
				system("pause");system("cls");//执行清屏操作
			}
			else
				printf("错误：文件读取（read）失败!\n");
		}
		else if (flag == 2) {	//下载文件
			printf("您已经成功进入下载文件功能模块！\n");
			Init_winsock(nRC, wsaData);//同上载  初始化Winsock
			Set_serverip(serverIP);//同上载  设置服务器相关信息，serverIP为指向IP地址的指针
			printf("******************************************\n");
			printf("**请您设定远程的文件路径与本地的文件    **\n");
			printf("**路径：                                **\n");
			printf("******************************************\n");
			printf("②远程的文件路径(remote file name)设置为:");
			scanf("%s", buf);
			token1 = buf;
			printf("③本地的文件路径(local file name)设置为:");
			scanf("%s", buf2);
			token2 = buf2;//实现远程文件名的获取以及本地生成的文件名的获取
			if (token1 != NULL && token2 != NULL) {
				download(token1, token2);
				system("pause");system("cls");
			}
			else {
				printf("错误：文件读取（read）失败!\n");
			}
		}
		else if (flag == 0) {
			printf("----谢谢使用!----\n");
			break;
		}
	}
	fclose(logFp);
	return 0;
}

/*
* 文件的上传
* 输入：filename：上传的文件名字（路径）
*/
bool upload(char* filename)//上载文件，filename为指向文件路径的指针
{
	clock_t start, end;						//记录时间的变量，记录上载开始和结束时间，便于计算上载速率
	sockaddr_in sender;						//服务器发送的ip和端口号
	int time_wait_ack, r_size, choose;		//等待时间，接收包的大小，和选择
	tftpPacket_RRQ_WRQ* sendPacket_1;		//发送包一：用在RRQ和WRQ的发送之中
	tftpPacket_DATA_ACK_ERROR* sendPacket_2;//发送包二：用在DATA、ACK、ERROR的发送之中//根据五种包不同，定义两种不同结构体
	tftpPacket_rev rcv_packet;				//接收包//接受报文信息的记录
	char temp[DATA_SIZE] = { 0 };					//暂时存放Transfer Mode和DATA数据包中的数据//初始为0
	FILE* fp = NULL;						//文件指针，用来打开文件（在下载和上传中使用）
	int s_size;								//DATA包每次携带DATA的大小
	int rxmt;								//DATA包重传次数
	unsigned short block = 1;				//DATA包的block序号
	unsigned short lostnum = 0;				//DATA包遗失的次数
	double lo;								//传输成功的丢包率
	transByte = 0;							//传输字节数设置为0
	/*--------------第0步：选择文件传输格式---------------------------------------------------------*/
	printf("\n******************************************\n");
	printf("** 请传输选择文件的格式:                **\n");
	printf("** (输入1：netascii                     **\n");
	printf("** (输入2：octet                        **\n");
	printf("******************************************\n");
	scanf("%d", &choose);	//memset(temp, 0, sizeof(temp));		//有和没有其实没差
	if (choose == 1) {
		strcpy_s(temp, "netascii");//sizeof(temp)==8
		printf("③传输格式设置为：netascii\n");
	}
	else {
		strcpy_s(temp, "octet");//sizeof(temp)==5
		printf("③传输格式设置为：octet\n");
	}
	printf("*********************文件正在上传中*********************\n");
	/*--------------第一步：构造WRQ---------------------------------------------------------*/
	//1.1 为sendPacket_1申请空间（空间大小包括操作码大小（即sizeof（struct tf。。。。））和filename与transfer mode大小以及它们以0结尾的结束标志）*/
	sendPacket_1 = (struct tftpPacket_RRQ_WRQ*)malloc(sizeof(struct tftpPacket_RRQ_WRQ) + strlen(filename) + strlen(temp) + 2);//动态申请，根据filename长度和格式长度来分配+两个以0结尾
	//1.2 对输入的filename进行处理，使得在WRQ包里面只有最后的文件名
	string filepath = filename;
	char filename_1[250] = {};
	auto f = filepath.find_last_of('\\');//逆向查找在原字符串中最后一个与指定字符串（或字符）中的某个字符匹配的字符，返回它的位置。若查找失败，则返回npos
	if (f == filepath.npos) strcpy(filename_1, filepath.c_str());//表示只有文件名，即无需截取处理
	else strcpy(filename_1, filepath.substr(f + 1).c_str());//表示需要从f+1处获取其后的信息作为文件名
	//1.3 写入操作码
	sendPacket_1->cmd = htons(CMD_WRQ);//写请求报文置为2//htons::将一个无符号短整型数值转换为网络字节序，即大端模式(big-endian)
	//1.4 写入文件名（Filename）和传输格式（Transfer Mode）
	sprintf(sendPacket_1->filename_and_data, "%s%c%s%c", filename_1, 0, temp, 0);
	/*--------------第二步：发送WRQ---------------------------------------------------------*/
	sendto(sock, (char*)sendPacket_1, (sizeof(struct tftpPacket_RRQ_WRQ) + strlen(filename_1) + strlen(temp) + 2), 0, (struct sockaddr*)&server_add, addr_len);
	/*--------------第三步：等待ACK回应-----------------------------------------------------*/
	for (time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20)
	{  //最多等待3s，每隔20ms刷新一次*///超过三秒即认为TLE了
	//3.1 尝试接收
		r_size = recvfrom(sock, (char*)&rcv_packet, sizeof(tftpPacket_rev), 0, (struct sockaddr*)&sender, (int*)&addr_len);
		//关于r_size与sendup的详细介绍：：https://zhuanlan.zhihu.com/p/408369874

		if (r_size > 0 && r_size < 4)			//如果返回值在0与4之间，则说明接受包异常//ACK报文长度为4个字节
		{
			printf_s("Bad packet:r_size=%d\n", r_size);
		}
		if (r_size >= 4 && rcv_packet.cmd == htons(CMD_ACK) && rcv_packet.block == htons(0))//成功接受到报文，报文数据编号为0
		{
			break;
		}
		Sleep(20);
	}
	//3.2 如果超时了没有收到ACK-------*/
	if (time_wait_ack >= PKT_RCV_TIMEOUT)
	{
		printf("*********************文件上传失败！*********************\n");
		printf("失败原因：Could not receive from server!\n");
		LOG_UP(LOG_UP_NOT_RCV_FROM_SERVER, filename, choose,block);//记录日志信息
		return false;//结束
	}
	/*--------------第四步：构造并发送data包------------------------------------------------------*/
	//4.1 打开文件
	if (choose == 1) fp = fopen(filename, "r");//1表示文件格式为netascii   2表示文件格式为octet
	else fp = fopen(filename, "rb");//"r"是以文本形式读，"rb"是以二进制的形式读
	//4.2 如果文件不存在（打不开文件）
	if (fp == NULL)
	{
		printf("*********************文件上传失败！*********************\n");
		printf("失败原因：File not exists!\n");
		LOG_UP(LOG_UP_FILE_NOT_EXISTS, filename, choose,block);//记录日志信息
		return false;//返回错误，表示上载失败
	}
	//4.3 循环发送文件
	s_size = 0, block = 1, lostnum = 0;	//data包携带数据大小s_size设置为0
	start = clock();					//开始计时
	do {//循环发送文件
		//4.4 构建DATA数据包
		memset(temp, 0, sizeof(temp));//将临时存储区域置为0
		s_size = fread(temp, 1, DATA_SIZE, fp);		//将发送的文件部分内容读到temp之中
		sendPacket_2 = (struct tftpPacket_DATA_ACK_ERROR*)malloc(sizeof(struct tftpPacket_DATA_ACK_ERROR) + s_size /*+ 32*/);//对发送报文指针分配所需空间
		memset(sendPacket_2, 0, sizeof(struct tftpPacket_DATA_ACK_ERROR) + s_size);
		sendPacket_2->cmd = htons(CMD_DATA);		//写入参数//写入报文类型DATA（3）
		sendPacket_2->block = htons(block);//写入报文分段的序号block
		memcpy(sendPacket_2->data, temp, s_size);	//写入数据
		//4.5 传输data包（最多重传3次）
		for (rxmt = 0; rxmt < PKT_MAX_RXMT; rxmt++)
		{
			sendto(sock, (char*)sendPacket_2, s_size + 4, 0, (struct sockaddr*)&sender, addr_len);//发送构造的报文（DATA类型）
			if (rxmt != 0)
			{//表示不是第一次传输该段报文，即该段报文需要正处于重传状态，输出报文丢失信息
				printf("block %d lost ,retransmission\n", block);
				lostnum += 1;
			}//未收到ACK报文，发送报文丢失的通报信息
			printf("Send the %d block\n", block);//界面显示发送报文信息
			//4.6 等待接收ACK（最多等待3s，每隔20ms更新一次）
			for (time_wait_ack = 0; time_wait_ack < PKT_RCV_TIMEOUT; time_wait_ack += 20) {// 等待ACK,最多等待3s,每隔20ms刷新一次
				r_size = recvfrom(sock, (char*)&rcv_packet, sizeof(tftpPacket_rev), 0, (struct sockaddr*)&sender, (int*)&addr_len);//监听接受ACK回复报文
				if (r_size > 0 && r_size < 4) {//同上：报文数据长度有问题，通报信息（可能传输过程发生错误）
					printf("Bad packet: r_size=%d\n", r_size);
				}
				if (r_size >= 4 && rcv_packet.cmd == htons(CMD_ACK) && rcv_packet.block == htons(block)) {//成功接受到ACK报文，结束试探监听
					break;
				}
				Sleep(20);//时停20ms
			}
			if (time_wait_ack < PKT_RCV_TIMEOUT) {// 发送成功，结束重传，break结束循环
				break;
			}
			//4.7 未收到ACK，重传
			else {
				LOG_UP(LOG_UP_NOT_RCV_ACK, filename, choose,block);//写入日志信息
				continue;//重传
			}
		}
		//4.8 如果3次重传失败，结束
		if (rxmt >= PKT_MAX_RXMT)//3次重传失败
		{
			printf("*********************文件上传失败！*********************\n");
			printf("失败原因：Could not receive from server!\n");
			fclose(fp);
			LOG_UP(LOG_UP_NOT_RCV_ACK_IN_RETRAN, filename, choose,block);//更新日志
			free(sendPacket_2);			//释放
			return false;//当前无法传送数据，重传连续失败超过3次，直接退出上载过程
		}
		//4.9 （收到ACK)单个数据包发送成功，传输下一个
		block++;
		transByte += s_size;//计算传送的文件的数据长度
		free(sendPacket_2);
	} while (s_size == DATA_SIZE);//循环判断信息表示根据该段报文数据段长度来判断当前所传送的数据报文是否为最后一段

	/*--------------第五步：全部data包发送成功----------------------------------------------*/
	fclose(fp);//关闭文件（本地）
	end = clock();//记录传送完成的时间信息
	lo = (double)(lostnum * 100) / (double)(block-1 + lostnum);			//计算丢包率
	consumeTime = ((double)(end - start)) / CLK_TCK;					//计算总用时
	printf("send file end\n");
	printf("*********************文件上传成功！*********************\n");
	printf("upload file size: %.1f kB cost time: %.2f s\n", transByte / 1024, consumeTime);
	printf("upload speed: %.1f kB/s\n", transByte / (1024 * consumeTime));
	printf("upload lost : %.2f %%\n", lo);//上载成功输出上载信息
	time(&rawTime);									// 获取时间
	nowtime = localtime(&rawTime);					// 转化为当地时间
	sprintf(logBuf, "%s Succ: upload %s, mode: %s, size: %.1f kB, cost time: %.2f s,lost : %.2f %%\n", asctime(nowtime), filename, choose == 1 ? ("netascii") : ("octet"), transByte / 1024, consumeTime, lo);
	//记录上载成功的相关日志信息
	for (int i = 0; i < 512; i++) {
		if (logBuf[i] == '\n') {
			logBuf[i] = ' ';
			break;
		}
	}
	fwrite(logBuf, 1, strlen(logBuf), logFp);//将该日志信息写入文件logFp指向的文件中
	return true;
}//在该函数如果传输成功会返回TRUE，否则会返回FALSE


/*
* 文件的下载
* 输入：remoteFile：远程文件名（路径） localFile：本地文件名（路径）
*/
bool download(char* remoteFile, char* localFile)
{
	clock_t start, end;								//用来记录时间的量start和end
	transByte = 0;									//传输的字节数设置为0
	sockaddr_in sender;								//用来保存服务器发送的ip和端口号
	int time_wait_ack, r_size, choose, resent = 0;	//等待时间、接受包大小、模式选择、
	tftpPacket_RRQ_WRQ* sendPacket_1;				//RRQ和WEQ
	tftpPacket_DATA_ACK_ERROR* sendPacket_2;		//DATA和ACK以及ERROR包//在这里学长采用union实现对于三种不同类型的报文结构的统一
	tftpPacket_rev rcv_packet;						//接收包//存放接受获得的报文的相关数据（相当于对于报文的所有数据的获取）
	char temp[DATA_SIZE];							//需要准备的缓存区

	int time_wait_data;								//等待接收单个数据包的用时
	unsigned short block = 1;						//接收的block块//发送报文ACK从0开始,但是接受报文ACK序号从1开始
	/*--------------第0步：选择文件传输模式---------------------------------------------------------*/
	printf("\n******************************************\n");
	printf("** 请传输选择文件的格式:                **\n");
	printf("** (输入1：netascii                     **\n");
	printf("** (输入2：octet                        **\n");
	printf("******************************************\n");
	scanf("%d", &choose);
	if (choose == 1) {//这里的choose与上载中的choose作用一致
		strcpy_s(temp, "netascii");
		printf("④传输格式设置为：netascii\n");
	}
	else {
		strcpy_s(temp, "octet");
		printf("④传输格式设置为：octet\n");
	}
	printf("*********************文件正在下载中*********************\n");
	/*--------------第一步：构造RRQ---------------------------------------------------------*/
	//1.1 为sendPacket_1申请空间  空间大小包括操作码大小（即sizeof（struct tf。。。。））和filename与transfer mode大小以及它们以0结尾的结束标志）*/
	sendPacket_1 = (struct tftpPacket_RRQ_WRQ*)malloc(sizeof(struct tftpPacket_RRQ_WRQ) + strlen(remoteFile) + strlen(temp) + 2);//由于对于所有数据之前都只是定义了一个指针，故均需分配空间
	//1.2 填写sendPacket_1包的相关信息
	sendPacket_1->cmd = htons(CMD_RRQ);												//写入操作码//这里是读请求报文
	sprintf(sendPacket_1->filename_and_data, "%s%c%s%c", remoteFile, 0, temp, 0);	//写入文件名（Filename）和传输格式（Transfer Mode）
	/*--------------第二步：发送RRQ---------------------------------------------------------*/
	sendto(sock, (char*)sendPacket_1, (sizeof(struct tftpPacket_RRQ_WRQ) + strlen(remoteFile) + strlen(temp) + 2), 0, (struct sockaddr*)&server_add, addr_len);
	/*--------------第三步：本地创建接收的文件----------------------------------------------*/
	FILE* fp = NULL;
	//3.1 fopen创建文件
	if (choose == 1) fp = fopen(localFile, "w");
	else fp = fopen(localFile, "wb");//这里与上载处相同，下载得到文件根据所需格式进行写入
	if (fp == NULL)
	{//3.2 如果打开不了文件
		printf("*********************文件下载失败！*********************\n");
		printf("失败原因：创建文件\"%s\"失败！\n", localFile);
		LOG_DOWN(LOG_DOWN_FILE_NOT_CREATE, remoteFile, localFile, choose, 0);//写入下载失败日志
		return false;
	}
	/*--------------第四步：接收文件--------------------------------------------------------*/
	start = clock();
	//4.1 在循环前提前创建好ACK（以反复利用）
	sendPacket_2 = (struct tftpPacket_DATA_ACK_ERROR*)malloc(sizeof(struct tftpPacket_DATA_ACK_ERROR));
	sendPacket_2->cmd = htons(CMD_ACK);//提前分配空间，并将opcode的值提前赋值
	do {
		for (time_wait_data = 0; time_wait_data < PKT_RCV_TIMEOUT * PKT_MAX_RXMT; time_wait_data += 50)
		{
			//4.2 尝试接收（Nonblock receive）
			r_size = recvfrom(sock, (char*)&rcv_packet, sizeof(tftpPacket_rev), 0, (struct sockaddr*)&sender, (int*)&addr_len);
			//4.5 超时重传不涉及第一个ACK1
			if (time_wait_data == PKT_RCV_TIMEOUT && block == 1)
			{//超时无法收到DATA包,此处对第一个包进行分类解决，原因是若无法收到第一个DATA包无法只能重新发送REQ报文
				printf("*********************文件下载失败！*********************\n");
				printf("失败原因：Could not receive from server!\n");
				LOG_DOWN(LOG_DOWN_NOT_RCV_FROM_SERVER, remoteFile, localFile, choose, 0);//写入下载日志，与上载日志相差不大
				fclose(fp);
				return false;
			}
			//4.3 如果接收有误 则打印提醒
			if (r_size > 0 && r_size < 4)	//DATA包头有Operation Code和Block Number两个项，共四个字节
			{//DATA包内容不完整
				printf("Bad packet: r_size=%d", r_size);//同样的如果报文低于4个字节那么表明该报文不完整
			}
			//4.4 如果接收数据包成功 则发送ACK 并将数据包写进本地文件
			else if (r_size >= 4 && rcv_packet.cmd == htons(CMD_DATA) && rcv_packet.block == htons(block))//接受的报文的相关条件均满足
			{//成功接受到DATA数据包
				printf("DATA: block=%d, data_size=%d\n", ntohs(rcv_packet.block), r_size - 4);
				//发送ACK！
				sendPacket_2->block = rcv_packet.block;//构造ACK报文
				sendto(sock, (char*)sendPacket_2, sizeof(tftpPacket_DATA_ACK_ERROR), 0, (struct sockaddr*)&sender, addr_len);//之前已经提前构造了ACK报文，现在只需修改ACK报文中的block信息即可
				fwrite(rcv_packet.data, 1, r_size - 4, fp);//将接收到的DATA包中的数据段写入新建的文件中
				break;
			}
			else {
				//4.5 如果单次超时，则重传ACK//这里除去了第一次超时的情况
				if ((time_wait_data != 0)&& (time_wait_data % PKT_RCV_TIMEOUT == 0))
				{
					Sleep(20);
					printf("block %d 丢失\n", block);
					//重发ACK!
					printf("send ACK %d\n", block - 1);
					sendto(sock, (char*)sendPacket_2, sizeof(tftpPacket_DATA_ACK_ERROR), 0, (struct sockaddr*)&sender, addr_len);
					//LOG_8	不能收到data数据包	
					LOG_DOWN(LOG_DOWN_NOT_RCV_DATA, remoteFile, localFile, choose, block);
					resent++;
				}
			}
			Sleep(50);
		}
		transByte += (r_size - 4);//记录接受的DATA包内数据的总长度
		//4.6 三次超时，接收数据失败
		if (time_wait_data >= PKT_RCV_TIMEOUT * PKT_MAX_RXMT)
		{
			printf("*********************文件下载失败！*********************\n");
			printf("失败原因：wait for data #%d timeout.\n", block);
			fclose(fp);
			LOG_DOWN(LOG_DOWN_RCV_DATA_OVERTIME, remoteFile, localFile, choose, block);
			return false;
		}
		block++;
	} while (r_size == DATA_SIZE + 4);//通过判断当前接受的DATA报文长度来判断是否为最后一个DATA报文
	/*--------------第五步：接收文件成功----------------------------------------------------*/
	fclose(fp);//文件写入完成关闭文件指针
	end = clock();//计算接受完成的时间，便于计算传输速率
	time(&rawTime);					// 获取时间
	nowtime = localtime(&rawTime);	// 转化为当地时间
	consumeTime = ((double)(end - start)) / CLK_TCK;//计算总花费时间
	printf("download file end\n");
	printf("*********************文件下载成功！*********************\n");
	printf("download file size: %.1f kB cost time: %.2f s\n", transByte / 1024, consumeTime);
	printf("download speed: %.1f kB/s\n", transByte / (1024 * consumeTime));
	sprintf(logBuf, "%s Succ: download %s as %s, mode: %s, size: %.1f kB, cost time: %.2f s\n", asctime(nowtime), remoteFile, localFile, choose == 1 ? ("netascii") : ("octet"), transByte / 1024, consumeTime);//写入下载完成的日志信息
	for (int i = 0; i < 512; i++) {
		if (logBuf[i] == '\n') {
			logBuf[i] = ' ';
			break;
		}
	}
	fwrite(logBuf, 1, strlen(logBuf), logFp);//将日志信息写入logFp指向的文档中
	return true;
}