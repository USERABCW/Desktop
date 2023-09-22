#include"HEAD_TFTP.h"

//VS环境下，避免对过时函数的警报和报错

time_t Time;//时间结构体
tm* time_now;//时间转化为便于记录的结构

FILE* LOG_F;//文件指针指向日志的文件路径
char LOG_LIST[512] = {0};//每条日志暂时存储的数组
UI ip_len;//记录IP地址的长度
UL opt = 1;//设定socket为非阻塞模式的参数
double Tbyte=0, Ttime=0;//传输的字节数以及传输所花费的时间
sockaddr_in SEV_Addr;//服务器IP地址
SOCKET sock;//函数调用的socket

void LOG_GET(int type4, char* filename, int type2, int block) {
	time(&Time);
	time_now = localtime(&Time);
	char Type[10] = { 0 };
	if (type2 == 1) {
		strcpy(Type, "netascii");
	}
	else {
		strcpy(Type, "octet");
	};
	if (type4 == 1) {
		sprintf(LOG_LIST, "%s错误：：上载文件：：%s,上载模式：：%s,%s\n", asctime(time_now), filename, Type, "文件不存在");
	}
	else if (type4 == 2) {
		sprintf(LOG_LIST, "%s错误：：上载文件：：%s,上载模式：：%s,%s,%d,%s\n", asctime(time_now), filename, Type, "发送包：：", block, "未收到回应！！");
	}
	else if (type4 == 3) {
		sprintf(LOG_LIST, "%s错误：：上载文件：：%s,上载模式：：%s,%s\n", asctime(time_now), filename, Type, "服务器无响应");
	}
	else if (type4 == 4) {
		sprintf(LOG_LIST, "%s成功：：上载文件：：%s,上载模式：：%s\n", asctime(time_now), filename, Type);
	}
	fwrite(LOG_LIST, 1, strlen(LOG_LIST), LOG_F);//将日志写入文件中
}

void LOG_GET(int type4, char* filename,char*filelocalname, int type2, int block) {
	time(&Time);
	time_now = localtime(&Time);
	char Type[10] = { 0 };
	if (type2 == 1) {
		strcpy(Type, "netascii");
	}
	else {
		strcpy(Type, "octet");
	};
	if (type4 == 1) {
		sprintf(LOG_LIST, "%s 错误：：下载文件：：%s,本地下载文件名：：%s,下载模式：：%s,%s",asctime(time_now),filename,filelocalname,Type,"无法生成本地文件！！");
	}
	else if (type4 == 2) {
		sprintf(LOG_LIST, "%s 错误：：下载文件：：%s,本地下载文件名：：%s,下载模式：：%s,%s", asctime(time_now), filename, filelocalname, Type, "服务器无响应！！");
	}
	else if (type4 == 3) {
		sprintf(LOG_LIST, "%s 错误：：下载文件：：%s,本地下载文件名：：%s,下载模式：：%s,%s,%d,%s", asctime(time_now), filename, filelocalname, Type, "无法接受数据包：",block,"请重传！！");
	}
	else if (type4 == 4) {
		sprintf(LOG_LIST, "%s 错误：：下载文件：：%s,本地下载文件名：：%s,下载模式：：%s,%s", asctime(time_now), filename, filelocalname, Type, "超时服务器未响应！！");
	}
	else if (type4 == 5) {
		sprintf(LOG_LIST, "%s 成功：：下载文件：：%s,本地下载文件名：：%s,下载模式：：%s", asctime(time_now), filename, filelocalname, Type);
	}
	fwrite(LOG_LIST, 1, strlen(LOG_LIST), LOG_F);//将日志写入文件中
}
//winsock的初始化
int INIT_winsock(int& nrc, WSADATA& wsadata) {//初始化失败或存在错误返回0，成功返回1
	nrc = WSAStartup(0x0101, &wsadata);//winsock API函数，完成一系列的初始化的工作
	if (nrc) {//nrc!=0表示函数初始化失败
		cout << "错误！！" << endl;
		cout << "客户端初始化winsock出错！！" << endl;
		return 0;
	}
	if (wsadata.wVersion != 0x0101) {//版本号出错
		cout << "错误！！" << endl;
		cout << "客户端winsock的版本号不匹配！！" << endl;
		WSACleanup();//清空初始化的工作
		return 0;
	}
	cout << "成功！！";
	cout << "客户端winsock初始化完成" << endl;
	cout << endl;
	return 1;
}
//IP地址的获取
int serverIP_GET(char* IP_server) {
	int type = 0;
	cout << "请输入设定服务器的IP地址::" << endl;
	cout << "键入1：：使用默认的IP地址(127.0.0.1)" << endl;
	cout << "键入2：：使用键入的新的IP地址" << endl;
	cin >> type;
	if (type == 1) {
		strcpy(IP_server, "127.0.0.1");
		cout << "成功！！" << endl;
		cout << "服务器IP地址已设置为127.0.0.1" << endl;
	}
	else if (type == 2) {
		cout << "请输入您所要设置的服务器IP地址：：" << endl;
		cin >> IP_server;
	}
	//设置服务器地址族（IPV4），ip地址和端口号
	SEV_Addr.sin_family = AF_INET;
	SEV_Addr.sin_addr.S_un.S_addr = inet_addr(IP_server);
	SEV_Addr.sin_port = htons(69);//初始端口号为69
	//创建socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//UDP套接字
	ioctlsocket(sock, FIONBIO, &opt);//设定进程为非阻塞模式
	if (sock == INVALID_SOCKET) {//socket创建失败
		cout << "错误！！" << endl;
		cout << "创建套接字失败！！" << endl;
		WSACleanup();
		return 0;
	}
	cout << "成功！！" << endl;
	cout << "创建套接字成功！！" << endl;
}

//构造的结构体WRQ报文在传输时以字符串类型传输//函数实现WRQ报文的构造和发送
int WRQ_create(TFTP_RRQorWRQ* WRQ_packet, char* filepointer, char* TEMP) {
	WRQ_packet = (struct TFTP_RRQorWRQ*)malloc(sizeof(TFTP_RRQorWRQ) + strlen(filepointer) + strlen(TEMP) + 2);//动态分配WRQ包空间
	if (!WRQ_packet) {
		return 0;//空间分配时发生问题
	}
	else  if (WRQ_packet) {
		string FILE_STR = filepointer;
		char filename_main[40] = { 0 };
		auto f = FILE_STR.find_last_of('\\');//使用string快速定位文件名的首位置
		if (f == FILE_STR.npos) strcpy_s(filename_main, FILE_STR.c_str());
		else strcpy_s(filename_main, FILE_STR.substr(f + 1).c_str());//实现文件名的获取
		WRQ_packet->OPC = htons(TFTP_OC_WRQ);//报文类型填充，写入操作码
		sprintf(WRQ_packet->Filename_TransferMode, "%s%c%s%c", filename_main, 0, TEMP, 0);
		//将文件名以及文件传输格式写入WRQ报文中
		sendto(sock, (char*)WRQ_packet, sizeof(TFTP_RRQorWRQ) + strlen(filepointer) + strlen(TEMP) + 2, 0, (struct sockaddr*)&SEV_Addr, ip_len);
		//将构造成功的WRQ报文发出，目的IP有自己给出和目的端口为69号
		return 1;
	}
}

//构造RRQ数据包并发送到服务器端口
int RRQ_create(TFTP_RRQorWRQ* RRQ_packet, char* filepointer, char* TEMP) {
	RRQ_packet = (struct TFTP_RRQorWRQ*)malloc(sizeof(TFTP_RRQorWRQ) + strlen(filepointer) + strlen(TEMP) + 2);//动态分配RRQ包空间
	if (!RRQ_packet) {
		return 0;//空间分配时发生问题
	}
	else  if (RRQ_packet) {
		string FILE_STR = filepointer;
		RRQ_packet->OPC = htons(TFTP_OC_RRQ);//报文类型填充，写入操作码
		sprintf(RRQ_packet->Filename_TransferMode, "%s%c%s%c", filepointer, 0, TEMP, 0);
		//将文件名以及文件传输格式写入RRQ报文中
		sendto(sock, (char*)RRQ_packet, sizeof(TFTP_RRQorWRQ) + strlen(filepointer) + strlen(TEMP) + 2, 0, (struct sockaddr*)&SEV_Addr, ip_len);
		//将构造成功的RRQ报文发出，目的IP有自己给出和目的端口为69号
		return 1;
	}
}

//构造DATA数据包
int DATA_create(TFTP_DATA* *DATA_packet, int packet_size, int block, char* TEMP) {
	(*DATA_packet) = (struct TFTP_DATA*)malloc(sizeof(TFTP_DATA) + packet_size);//为DATA数据包分配空间
	if ((*DATA_packet) != NULL) {
		memset((*DATA_packet), 0, sizeof(TFTP_DATA) + packet_size);//结构体初始化
		(*DATA_packet)->OPC = htons(TFTP_OC_DATA);
		(*DATA_packet)->Block = htons(block);
		memcpy((*DATA_packet)->Date, TEMP, packet_size);//DATA包内数据以及相关信息的写入
		return 1;
	}
	else {
		return 0;
	}
}

//构造ACK数据包
int ACK_create(TFTP_ACK* *ACK_packet) {
	(*ACK_packet) = (struct TFTP_ACK*)malloc(sizeof(struct TFTP_ACK));
	if ((*ACK_packet) != NULL) {
		(*ACK_packet)->OPC = htons(TFTP_OC_ACK);
		return 1;
	}
	else {
		return 0;
	}
}

//文件上载！！//
int upload(char* Filepointer) {
	clock_t begin, end;//保存时间变量，记载上载开始和结束时间，便于计算上载速率
	sockaddr_in serveIP;//记录服务器IP和端口号信息
	int Wtime, pack_size = 0;//等待时间以及接收包的字节大小
	TFTP_RRQorWRQ* WRQ_packet{};//构造首先发送的写请求报文
	TFTP_DATA* DATA_packet{};
	TFTP_rev REV_packet = { 0 };//接收包接受报文信息
	FILE* fp = NULL;//文件指针，用于打开文件
	int rpack_size;//记录每次DATA包中数据字节数
	int Rfreq;//DATA包重传次数
	char TEMP[DATA_MAX_SIZE] = { 0 };//临时存储
	US block = 1;//DATA包以及收到的ACK包中的编号
	US Ltime = 0;//丢包次数
	double Lpcent;//丢包率
	Tbyte = 0;//传输的数据包的总字节数，初始置为0


	cout << "<<请选择传输文件的传输格式：：>>" << endl;
	cout << "1.以netascii格式传输文件：：" << endl;
	cout << "2.以octet格式传输文件：：" << endl;


	int type3;
	cin >> type3;
	if (type3 == 1) {
		strcpy_s(TEMP, "netascii");
		cout << "<<传输格式已设为：netascii>>" << endl;
	}
	else if (type3 == 2) {
		strcpy_s(TEMP, "octet");
		cout << "<<传输格式已设为：octet>>" << endl;
	}
	cout << "<<<————文件上传中————>>>" << endl;


	if (WRQ_create(WRQ_packet, Filepointer, TEMP) == 0) {
		cout << "ERROR" << endl;
		cout << "程序运行时空间溢出！！" << endl;
		return 0;//传输失败
	};//函数构造WRQ报文并发送

	for (Wtime = 0; Wtime < RT_Ttimeout; Wtime += 20)//监听3s，每20ms 进行一次尝试接受
	{
		rpack_size = recvfrom(sock, (char*)&REV_packet, sizeof(TFTP_rev), 0, (struct sockaddr*)&serveIP, (int*)&ip_len);
		//接收到报文
		if (rpack_size > 0 && rpack_size < 4) {
			cout << "Warning！！" << endl;
			cout << "接受报文不完整！！" << endl;
		}//报文字节数太少,传送的报文结构不正确
		if (rpack_size >= 4 && REV_packet.OPC == htons(TFTP_OC_ACK) && REV_packet.block == htons(0)) {
			//如果接受到的报文结构完整而且报文类型及报文编号正确
			break;//接收成功，结束循环持续监听
		}
		Sleep(20);//避免资源浪费，每间隔20ms进行一次监听
	}

	if (Wtime >= RT_Ttimeout) {
		//超时未接收到报文信息
		cout << "Warning::文件传输失败！！" << endl;
		cout << "失败原因：：服务器未响应！！" << endl;
		LOG_GET(3, Filepointer, type3, block);//写入日志信息
		return 0;//表示传输失败
	}

	else {
		if (type3 == 1) fp = fopen(Filepointer, "r");//以文本形式读取数据
		else fp = fopen(Filepointer, "rb");//rb表示以二进制读取数据

		if (fp == NULL) {
			//文件未找到
			cout << "Warning::文件上传失败！！" << endl;
			cout << "失败原因：：文件不存在！！" << endl;
			LOG_GET(1, Filepointer, type3, block);
			return 0;
		}

		else {
			pack_size = 0; block = 1; Ltime = 0;//赋初值传送DATA数据包
			begin = clock();//开始计时
			do {
				memset(TEMP, 0, sizeof(TEMP));
				pack_size = fread(TEMP, 1, DATA_MAX_SIZE, fp);//从文件中读取数据写入TEMP中存储
				DATA_create(&DATA_packet, pack_size, block, TEMP);//构造DATA数据报文
				//循环传输DATA数据包//以接收到ACK包结束
				for (Rfreq = 0; Rfreq < RT_Ttime_MAX; Rfreq++) {//循环重传直到接收到ACK包
					sendto(sock, (char*)DATA_packet, pack_size + 4, 0, (struct sockaddr*)&serveIP, ip_len);//发送DATA报文
					if (Rfreq!=0) {
						cout << "第" << block << "个包丢失！！" << "请重传！！" << endl;
						Ltime++;
					}
					cout << "发送第" << block << "个包，待回应！！" << endl;
					for (Wtime = 0; Wtime < RT_Ttimeout; Wtime += 20) {
						//间隔20ms刷新一次查看是否有回应，等待3s
						rpack_size = recvfrom(sock, (char*)&REV_packet, sizeof(TFTP_rev), 0, (struct sockaddr*)&serveIP, (int*)&ip_len);
						//接受到Packet包
						if (rpack_size > 0 && rpack_size < 4) {
							cout << "错误！！" << endl;
							cout << "接受报文不完整！！"  << endl;
						}
						else if (rpack_size >= 4 && REV_packet.OPC == htons(TFTP_OC_ACK) && REV_packet.block == htons(block))
						{
							break;
						}
						Sleep(20);
					}
					if (Wtime < RT_Ttimeout) break;
					else {
						LOG_GET(2, Filepointer, type3, block);
						continue;
					}
				}
				if (Rfreq >= RT_Ttime_MAX) {
					cout << "<<文件上传失败>>：：" << endl;
					cout << "失败原因：：超时未响应" << endl;
					fclose(fp);
					LOG_GET(3, Filepointer, type3, block);
					free(DATA_packet);
					return 0;//传输失败
				}
				block++;//块的个数随着DATAT包的发送而增加
				Tbyte += pack_size;//计算传送的文档数据长度
				free(DATA_packet);
			} while (pack_size == DATA_MAX_SIZE);//循环构造

			fclose(fp);//关闭传送的文件
			end = clock();//结束传送的时间
			Lpcent = (double)(Ltime) / (double)(block - 1 + Ltime);//计算丢包率
			Ttime = ((double)(end - begin)) / 1000;//ms转s
			cout << "<<数据上载完成>>****" << endl;
			cout << "数据上载大小：：" << Tbyte / 1024 << "kB" << endl;
			cout << "数据上载用时：：" << Ttime << 's' << endl;
			cout << "数据上载速率：：" << Tbyte / (1024 * Ttime) << endl;
			cout << "丢包率：：" << Lpcent * 100 << "%" << endl;
			LOG_GET(4, Filepointer, type3, block);
			return 1;//传输成功
		}
	}
}

//文件下载！！//
int download(char* Filepointer, char* Filelocalpointer) {
	clock_t begin, end;//记录下载开始时间和结束时间
	Tbyte = 0;//记录下载的数据字节数
	sockaddr_in serveIP;//记录保存服务器IP地址
	int Wtime, pack_size = 0;//等待时间以及数据包接受大小
	TFTP_RRQorWRQ* RRQ_packet{};//构造首先发送的写请求报文
	TFTP_ACK* ACK_packet{};
	TFTP_rev REV_packet;//接收包接受报文信息
	char TEMP[DATA_MAX_SIZE] = { 0 };//临时存储
	US block = 1;//DATA包以及收到的ACK包中的编号

	cout << "<<请选择传输文件的传输格式：：>>" << endl;
	cout << "1.以netascii格式传输文件：：" << endl;
	cout << "2.以octet格式传输文件：：" << endl;

	int type3;
	cin >> type3;
	if (type3 == 1) {
		strcpy_s(TEMP, "netascii");
		cout << "<<传输格式已设为：netascii>>" << endl;
	}
	else if (type3 == 2) {
		strcpy_s(TEMP, "octet");
		cout << "<<传输格式已设为：octet>>" << endl;
	}
	cout << "<<<————文件下载中————>>>" << endl;

	if (RRQ_create(RRQ_packet, Filepointer, TEMP) == 0) {
		cout << "ERROR" << endl;
		cout << "空间溢出！！" << endl;
		return 0;//传输失败
	};//函数构造WRQ报文并发送，并检测是否发送成功

	FILE* fp = NULL;//本地创建文件接受数据
	if (type3 == 1)  fp = fopen(Filelocalpointer, "w");
	else fp = fopen(Filelocalpointer, "wb");
	if (fp == NULL) {
		//创建文件失败
		cout << "<<<错误：： 文件下载失败！！>>>" << endl;
		cout << "失败原因：：创建文件失败！！" << endl;
		LOG_GET(1, Filepointer,Filelocalpointer, type3, 0);
		return 0;
	}
	//接受文件
	begin = clock();//开始计时
	ACK_create(&ACK_packet);//构造ACK报文
	do {
		for (Wtime = 0; Wtime < RT_Ttimeout * RT_Ttime_MAX; Wtime += 50) {
			pack_size = recvfrom(sock, (char*)&REV_packet, sizeof(TFTP_rev), 0, (struct sockaddr*)&serveIP, (int*)&ip_len);
			//尝试接受报文（每间隔50ms监听一次）
			if (Wtime == RT_Ttimeout && block == 1) {
				//Timeout无法接受到报文，block==1表示无法接受到数据包
				cout << "<<<错误：：文件下载失败>>>" << endl;
				cout << "失败原因：：无法接受到数据包！！" << endl;
				LOG_GET(2, Filepointer, Filelocalpointer, type3, block);//服务器不响应
				fclose(fp);//关闭创建的文件
				return 0;
			}
			if (pack_size > 0 && pack_size < 4) {
				cout << "数据包不完整！！" << endl;
			}
			else if (pack_size >= 4 && REV_packet.OPC == htons(TFTP_OC_DATA) && REV_packet.block == htons(block)) {
				//成功接受到数据包DATA包
				cout << "数据包：：" << ntohs(REV_packet.block) << "\t数据包大小：：" << pack_size - 4<<endl;
				ACK_packet->block = htons(block);
				sendto(sock, (char*)(ACK_packet), sizeof(TFTP_ACK), 0, (struct sockaddr*)&serveIP, ip_len);
				fwrite(REV_packet.DATA, 1, pack_size - 4, fp);
				break;
			}
			else {
				if ((Wtime) && (Wtime % RT_Ttimeout == 0)) {
					Sleep(20);
					cout << "包:" << block << "丢失！！" << endl;
					cout << "重发ACK包:" << block - 1 << endl;
					//重发ACK包
					sendto(sock, (char*)(ACK_packet), sizeof(TFTP_ACK), 0, (struct sockaddr*)&serveIP, ip_len);
					LOG_GET(3, Filepointer, Filelocalpointer, type3, block);
				}
			}
			Sleep(50);//停等50ms
		}
		Tbyte += (pack_size - 4);
		if (Wtime >= RT_Ttimeout * RT_Ttime_MAX) {
			cout << "<<<文件下载失败！！>>>" << endl;
			cout << "数据包：：" << block << "超时！！" << endl;
			fclose(fp);
			LOG_GET(4, Filepointer, Filelocalpointer, type3, block);
			return 0;//文件下载失败的返回值
		}
		block++;
	} while (pack_size == DATA_MAX_SIZE + 4);
	fclose(fp);
	end = clock();
	Ttime = ((double)(end - begin)) / 1000;//ms转s
	cout << "<<数据下载完成>>****" << endl;
	cout << "数据下载大小：：" << Tbyte / 1024 << "kB" << endl;
	cout << "数据下载用时：：" << Ttime << 's' << endl;
	cout << "数据下载速率：：" << Tbyte / (1024 * Ttime) << endl;
	LOG_GET(5, Filepointer, Filelocalpointer, type3, block);
	return 1;//传输成功
}



//+++++++++++++主函数++++++++++++//
int main() {

	char IP_serve[20];//用于存储IP地址
	int nrc;
	WSADATA  wsadata;
	int type2;
	ip_len = sizeof(struct sockaddr_in);//计算地址长度并存入变量中，便于操作
	time(&Time);//调用函数求解当前时间
	time_now = localtime(&Time);//用本地市区转换时间
	LOG_F = fopen("tftpclient.log", "a");//创建LOG文件存储日志
	if (LOG_F == NULL) {//创建LOG文件失败//一般用不到
		cout << "错误！！" << endl;
		cout << "本地时间为：" << asctime(time_now) << endl;//输出当前时间
	}
	while (1) {
		cout << "请选择你要执行的操作：：" << endl;
		cout << "1.上载文件(upload)————" << endl;
		cout << "2.下载文件(dowload)————" << endl;
		cout << "0.退出系统————" << endl;
		cin >> type2;
		system("cls");//清屏进入系统
		if (type2 == 0) {
			cout << "<<感谢使用：：已退出>>" << endl;
			break;
		}
		else if (type2 == 1) {
			cout << "<<欢迎使用本客户端上载文件>>" << endl;
			INIT_winsock(nrc, wsadata);//Winsock初始化
			serverIP_GET(IP_serve);//询问设置服务器相关信息，IP_serve指向存储IP地址的数据块
			cout << "请键入您需要上载的文件路径：：" << endl;
			char Filesourse[60];//存储文件地址信息
			char* Filepointer;//指向文件地址信息的指针
			cin >> Filesourse;
			Filepointer = Filesourse;
			if (Filepointer != NULL) {//上载文件信息不为NULL
				upload(Filepointer);
				system("pause");
				system("cls");//清屏
			}
			else {
				cout << "错误！！" << endl;
				cout << "无法读取当地文件！！" << endl;
			}
		}
		else if (type2 == 2) {
			cout << "<<欢迎使用本客户端下载文件>>" << endl;
			INIT_winsock(nrc, wsadata);//Winsock初始化
			serverIP_GET(IP_serve);//询问设置服务器相关信息，IP_serve指向存储IP地址的数据块
			cout << "请键入您需要下载的文件路径：：" << endl;
			char Filesourse[60];//存储文件地址信息
			char* Filepointer;//指向目的文件地址信息的指针
			cin >> Filesourse;
			cout << "请键入您需要将文件保存的本地地址：：" << endl;
			char Filelocal[60];
			char* Filelocalpointer;//指向本地文件地址信息的指针
			cin >> Filelocal;
			Filepointer = Filesourse;
			Filelocalpointer = Filelocal;
			if (Filepointer != NULL && Filelocalpointer != NULL) {//上载文件信息不为NULL
				download(Filepointer, Filelocalpointer);
				system("pause");
				system("cls");//清屏
			}
			else {
				cout << "错误！！" << endl;
				cout << "文件信息有误！！" << endl;
			}
		}
		else if (type2 == 0) {
			cout << "__感谢使用！！__" << endl;
			break;//结束循环！！
		}
		fclose(LOG_F);//关闭LOG日志文件
		return 0;
	}
}