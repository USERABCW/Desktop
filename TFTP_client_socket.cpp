#include"HEAD_TFTP.h"

//VS�����£�����Թ�ʱ�����ľ����ͱ���

time_t Time;//ʱ��ṹ��
tm* time_now;//ʱ��ת��Ϊ���ڼ�¼�Ľṹ

FILE* LOG_F;//�ļ�ָ��ָ����־���ļ�·��
char LOG_LIST[512] = {0};//ÿ����־��ʱ�洢������
UI ip_len;//��¼IP��ַ�ĳ���
UL opt = 1;//�趨socketΪ������ģʽ�Ĳ���
double Tbyte=0, Ttime=0;//������ֽ����Լ����������ѵ�ʱ��
sockaddr_in SEV_Addr;//������IP��ַ
SOCKET sock;//�������õ�socket

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
		sprintf(LOG_LIST, "%s���󣺣������ļ�����%s,����ģʽ����%s,%s\n", asctime(time_now), filename, Type, "�ļ�������");
	}
	else if (type4 == 2) {
		sprintf(LOG_LIST, "%s���󣺣������ļ�����%s,����ģʽ����%s,%s,%d,%s\n", asctime(time_now), filename, Type, "���Ͱ�����", block, "δ�յ���Ӧ����");
	}
	else if (type4 == 3) {
		sprintf(LOG_LIST, "%s���󣺣������ļ�����%s,����ģʽ����%s,%s\n", asctime(time_now), filename, Type, "����������Ӧ");
	}
	else if (type4 == 4) {
		sprintf(LOG_LIST, "%s�ɹ����������ļ�����%s,����ģʽ����%s\n", asctime(time_now), filename, Type);
	}
	fwrite(LOG_LIST, 1, strlen(LOG_LIST), LOG_F);//����־д���ļ���
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
		sprintf(LOG_LIST, "%s ���󣺣������ļ�����%s,���������ļ�������%s,����ģʽ����%s,%s",asctime(time_now),filename,filelocalname,Type,"�޷����ɱ����ļ�����");
	}
	else if (type4 == 2) {
		sprintf(LOG_LIST, "%s ���󣺣������ļ�����%s,���������ļ�������%s,����ģʽ����%s,%s", asctime(time_now), filename, filelocalname, Type, "����������Ӧ����");
	}
	else if (type4 == 3) {
		sprintf(LOG_LIST, "%s ���󣺣������ļ�����%s,���������ļ�������%s,����ģʽ����%s,%s,%d,%s", asctime(time_now), filename, filelocalname, Type, "�޷��������ݰ���",block,"���ش�����");
	}
	else if (type4 == 4) {
		sprintf(LOG_LIST, "%s ���󣺣������ļ�����%s,���������ļ�������%s,����ģʽ����%s,%s", asctime(time_now), filename, filelocalname, Type, "��ʱ������δ��Ӧ����");
	}
	else if (type4 == 5) {
		sprintf(LOG_LIST, "%s �ɹ����������ļ�����%s,���������ļ�������%s,����ģʽ����%s", asctime(time_now), filename, filelocalname, Type);
	}
	fwrite(LOG_LIST, 1, strlen(LOG_LIST), LOG_F);//����־д���ļ���
}
//winsock�ĳ�ʼ��
int INIT_winsock(int& nrc, WSADATA& wsadata) {//��ʼ��ʧ�ܻ���ڴ��󷵻�0���ɹ�����1
	nrc = WSAStartup(0x0101, &wsadata);//winsock API���������һϵ�еĳ�ʼ���Ĺ���
	if (nrc) {//nrc!=0��ʾ������ʼ��ʧ��
		cout << "���󣡣�" << endl;
		cout << "�ͻ��˳�ʼ��winsock������" << endl;
		return 0;
	}
	if (wsadata.wVersion != 0x0101) {//�汾�ų���
		cout << "���󣡣�" << endl;
		cout << "�ͻ���winsock�İ汾�Ų�ƥ�䣡��" << endl;
		WSACleanup();//��ճ�ʼ���Ĺ���
		return 0;
	}
	cout << "�ɹ�����";
	cout << "�ͻ���winsock��ʼ�����" << endl;
	cout << endl;
	return 1;
}
//IP��ַ�Ļ�ȡ
int serverIP_GET(char* IP_server) {
	int type = 0;
	cout << "�������趨��������IP��ַ::" << endl;
	cout << "����1����ʹ��Ĭ�ϵ�IP��ַ(127.0.0.1)" << endl;
	cout << "����2����ʹ�ü�����µ�IP��ַ" << endl;
	cin >> type;
	if (type == 1) {
		strcpy(IP_server, "127.0.0.1");
		cout << "�ɹ�����" << endl;
		cout << "������IP��ַ������Ϊ127.0.0.1" << endl;
	}
	else if (type == 2) {
		cout << "����������Ҫ���õķ�����IP��ַ����" << endl;
		cin >> IP_server;
	}
	//���÷�������ַ�壨IPV4����ip��ַ�Ͷ˿ں�
	SEV_Addr.sin_family = AF_INET;
	SEV_Addr.sin_addr.S_un.S_addr = inet_addr(IP_server);
	SEV_Addr.sin_port = htons(69);//��ʼ�˿ں�Ϊ69
	//����socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//UDP�׽���
	ioctlsocket(sock, FIONBIO, &opt);//�趨����Ϊ������ģʽ
	if (sock == INVALID_SOCKET) {//socket����ʧ��
		cout << "���󣡣�" << endl;
		cout << "�����׽���ʧ�ܣ���" << endl;
		WSACleanup();
		return 0;
	}
	cout << "�ɹ�����" << endl;
	cout << "�����׽��ֳɹ�����" << endl;
}

//����Ľṹ��WRQ�����ڴ���ʱ���ַ������ʹ���//����ʵ��WRQ���ĵĹ���ͷ���
int WRQ_create(TFTP_RRQorWRQ* WRQ_packet, char* filepointer, char* TEMP) {
	WRQ_packet = (struct TFTP_RRQorWRQ*)malloc(sizeof(TFTP_RRQorWRQ) + strlen(filepointer) + strlen(TEMP) + 2);//��̬����WRQ���ռ�
	if (!WRQ_packet) {
		return 0;//�ռ����ʱ��������
	}
	else  if (WRQ_packet) {
		string FILE_STR = filepointer;
		char filename_main[40] = { 0 };
		auto f = FILE_STR.find_last_of('\\');//ʹ��string���ٶ�λ�ļ�������λ��
		if (f == FILE_STR.npos) strcpy_s(filename_main, FILE_STR.c_str());
		else strcpy_s(filename_main, FILE_STR.substr(f + 1).c_str());//ʵ���ļ����Ļ�ȡ
		WRQ_packet->OPC = htons(TFTP_OC_WRQ);//����������䣬д�������
		sprintf(WRQ_packet->Filename_TransferMode, "%s%c%s%c", filename_main, 0, TEMP, 0);
		//���ļ����Լ��ļ������ʽд��WRQ������
		sendto(sock, (char*)WRQ_packet, sizeof(TFTP_RRQorWRQ) + strlen(filepointer) + strlen(TEMP) + 2, 0, (struct sockaddr*)&SEV_Addr, ip_len);
		//������ɹ���WRQ���ķ�����Ŀ��IP���Լ�������Ŀ�Ķ˿�Ϊ69��
		return 1;
	}
}

//����RRQ���ݰ������͵��������˿�
int RRQ_create(TFTP_RRQorWRQ* RRQ_packet, char* filepointer, char* TEMP) {
	RRQ_packet = (struct TFTP_RRQorWRQ*)malloc(sizeof(TFTP_RRQorWRQ) + strlen(filepointer) + strlen(TEMP) + 2);//��̬����RRQ���ռ�
	if (!RRQ_packet) {
		return 0;//�ռ����ʱ��������
	}
	else  if (RRQ_packet) {
		string FILE_STR = filepointer;
		RRQ_packet->OPC = htons(TFTP_OC_RRQ);//����������䣬д�������
		sprintf(RRQ_packet->Filename_TransferMode, "%s%c%s%c", filepointer, 0, TEMP, 0);
		//���ļ����Լ��ļ������ʽд��RRQ������
		sendto(sock, (char*)RRQ_packet, sizeof(TFTP_RRQorWRQ) + strlen(filepointer) + strlen(TEMP) + 2, 0, (struct sockaddr*)&SEV_Addr, ip_len);
		//������ɹ���RRQ���ķ�����Ŀ��IP���Լ�������Ŀ�Ķ˿�Ϊ69��
		return 1;
	}
}

//����DATA���ݰ�
int DATA_create(TFTP_DATA* *DATA_packet, int packet_size, int block, char* TEMP) {
	(*DATA_packet) = (struct TFTP_DATA*)malloc(sizeof(TFTP_DATA) + packet_size);//ΪDATA���ݰ�����ռ�
	if ((*DATA_packet) != NULL) {
		memset((*DATA_packet), 0, sizeof(TFTP_DATA) + packet_size);//�ṹ���ʼ��
		(*DATA_packet)->OPC = htons(TFTP_OC_DATA);
		(*DATA_packet)->Block = htons(block);
		memcpy((*DATA_packet)->Date, TEMP, packet_size);//DATA���������Լ������Ϣ��д��
		return 1;
	}
	else {
		return 0;
	}
}

//����ACK���ݰ�
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

//�ļ����أ���//
int upload(char* Filepointer) {
	clock_t begin, end;//����ʱ��������������ؿ�ʼ�ͽ���ʱ�䣬���ڼ�����������
	sockaddr_in serveIP;//��¼������IP�Ͷ˿ں���Ϣ
	int Wtime, pack_size = 0;//�ȴ�ʱ���Լ����հ����ֽڴ�С
	TFTP_RRQorWRQ* WRQ_packet{};//�������ȷ��͵�д������
	TFTP_DATA* DATA_packet{};
	TFTP_rev REV_packet = { 0 };//���հ����ܱ�����Ϣ
	FILE* fp = NULL;//�ļ�ָ�룬���ڴ��ļ�
	int rpack_size;//��¼ÿ��DATA���������ֽ���
	int Rfreq;//DATA���ش�����
	char TEMP[DATA_MAX_SIZE] = { 0 };//��ʱ�洢
	US block = 1;//DATA���Լ��յ���ACK���еı��
	US Ltime = 0;//��������
	double Lpcent;//������
	Tbyte = 0;//��������ݰ������ֽ�������ʼ��Ϊ0


	cout << "<<��ѡ�����ļ��Ĵ����ʽ����>>" << endl;
	cout << "1.��netascii��ʽ�����ļ�����" << endl;
	cout << "2.��octet��ʽ�����ļ�����" << endl;


	int type3;
	cin >> type3;
	if (type3 == 1) {
		strcpy_s(TEMP, "netascii");
		cout << "<<�����ʽ����Ϊ��netascii>>" << endl;
	}
	else if (type3 == 2) {
		strcpy_s(TEMP, "octet");
		cout << "<<�����ʽ����Ϊ��octet>>" << endl;
	}
	cout << "<<<���������ļ��ϴ��С�������>>>" << endl;


	if (WRQ_create(WRQ_packet, Filepointer, TEMP) == 0) {
		cout << "ERROR" << endl;
		cout << "��������ʱ�ռ��������" << endl;
		return 0;//����ʧ��
	};//��������WRQ���Ĳ�����

	for (Wtime = 0; Wtime < RT_Ttimeout; Wtime += 20)//����3s��ÿ20ms ����һ�γ��Խ���
	{
		rpack_size = recvfrom(sock, (char*)&REV_packet, sizeof(TFTP_rev), 0, (struct sockaddr*)&serveIP, (int*)&ip_len);
		//���յ�����
		if (rpack_size > 0 && rpack_size < 4) {
			cout << "Warning����" << endl;
			cout << "���ܱ��Ĳ���������" << endl;
		}//�����ֽ���̫��,���͵ı��Ľṹ����ȷ
		if (rpack_size >= 4 && REV_packet.OPC == htons(TFTP_OC_ACK) && REV_packet.block == htons(0)) {
			//������ܵ��ı��Ľṹ�������ұ������ͼ����ı����ȷ
			break;//���ճɹ�������ѭ����������
		}
		Sleep(20);//������Դ�˷ѣ�ÿ���20ms����һ�μ���
	}

	if (Wtime >= RT_Ttimeout) {
		//��ʱδ���յ�������Ϣ
		cout << "Warning::�ļ�����ʧ�ܣ���" << endl;
		cout << "ʧ��ԭ�򣺣�������δ��Ӧ����" << endl;
		LOG_GET(3, Filepointer, type3, block);//д����־��Ϣ
		return 0;//��ʾ����ʧ��
	}

	else {
		if (type3 == 1) fp = fopen(Filepointer, "r");//���ı���ʽ��ȡ����
		else fp = fopen(Filepointer, "rb");//rb��ʾ�Զ����ƶ�ȡ����

		if (fp == NULL) {
			//�ļ�δ�ҵ�
			cout << "Warning::�ļ��ϴ�ʧ�ܣ���" << endl;
			cout << "ʧ��ԭ�򣺣��ļ������ڣ���" << endl;
			LOG_GET(1, Filepointer, type3, block);
			return 0;
		}

		else {
			pack_size = 0; block = 1; Ltime = 0;//����ֵ����DATA���ݰ�
			begin = clock();//��ʼ��ʱ
			do {
				memset(TEMP, 0, sizeof(TEMP));
				pack_size = fread(TEMP, 1, DATA_MAX_SIZE, fp);//���ļ��ж�ȡ����д��TEMP�д洢
				DATA_create(&DATA_packet, pack_size, block, TEMP);//����DATA���ݱ���
				//ѭ������DATA���ݰ�//�Խ��յ�ACK������
				for (Rfreq = 0; Rfreq < RT_Ttime_MAX; Rfreq++) {//ѭ���ش�ֱ�����յ�ACK��
					sendto(sock, (char*)DATA_packet, pack_size + 4, 0, (struct sockaddr*)&serveIP, ip_len);//����DATA����
					if (Rfreq!=0) {
						cout << "��" << block << "������ʧ����" << "���ش�����" << endl;
						Ltime++;
					}
					cout << "���͵�" << block << "����������Ӧ����" << endl;
					for (Wtime = 0; Wtime < RT_Ttimeout; Wtime += 20) {
						//���20msˢ��һ�β鿴�Ƿ��л�Ӧ���ȴ�3s
						rpack_size = recvfrom(sock, (char*)&REV_packet, sizeof(TFTP_rev), 0, (struct sockaddr*)&serveIP, (int*)&ip_len);
						//���ܵ�Packet��
						if (rpack_size > 0 && rpack_size < 4) {
							cout << "���󣡣�" << endl;
							cout << "���ܱ��Ĳ���������"  << endl;
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
					cout << "<<�ļ��ϴ�ʧ��>>����" << endl;
					cout << "ʧ��ԭ�򣺣���ʱδ��Ӧ" << endl;
					fclose(fp);
					LOG_GET(3, Filepointer, type3, block);
					free(DATA_packet);
					return 0;//����ʧ��
				}
				block++;//��ĸ�������DATAT���ķ��Ͷ�����
				Tbyte += pack_size;//���㴫�͵��ĵ����ݳ���
				free(DATA_packet);
			} while (pack_size == DATA_MAX_SIZE);//ѭ������

			fclose(fp);//�رմ��͵��ļ�
			end = clock();//�������͵�ʱ��
			Lpcent = (double)(Ltime) / (double)(block - 1 + Ltime);//���㶪����
			Ttime = ((double)(end - begin)) / 1000;//msתs
			cout << "<<�����������>>****" << endl;
			cout << "�������ش�С����" << Tbyte / 1024 << "kB" << endl;
			cout << "����������ʱ����" << Ttime << 's' << endl;
			cout << "�����������ʣ���" << Tbyte / (1024 * Ttime) << endl;
			cout << "�����ʣ���" << Lpcent * 100 << "%" << endl;
			LOG_GET(4, Filepointer, type3, block);
			return 1;//����ɹ�
		}
	}
}

//�ļ����أ���//
int download(char* Filepointer, char* Filelocalpointer) {
	clock_t begin, end;//��¼���ؿ�ʼʱ��ͽ���ʱ��
	Tbyte = 0;//��¼���ص������ֽ���
	sockaddr_in serveIP;//��¼���������IP��ַ
	int Wtime, pack_size = 0;//�ȴ�ʱ���Լ����ݰ����ܴ�С
	TFTP_RRQorWRQ* RRQ_packet{};//�������ȷ��͵�д������
	TFTP_ACK* ACK_packet{};
	TFTP_rev REV_packet;//���հ����ܱ�����Ϣ
	char TEMP[DATA_MAX_SIZE] = { 0 };//��ʱ�洢
	US block = 1;//DATA���Լ��յ���ACK���еı��

	cout << "<<��ѡ�����ļ��Ĵ����ʽ����>>" << endl;
	cout << "1.��netascii��ʽ�����ļ�����" << endl;
	cout << "2.��octet��ʽ�����ļ�����" << endl;

	int type3;
	cin >> type3;
	if (type3 == 1) {
		strcpy_s(TEMP, "netascii");
		cout << "<<�����ʽ����Ϊ��netascii>>" << endl;
	}
	else if (type3 == 2) {
		strcpy_s(TEMP, "octet");
		cout << "<<�����ʽ����Ϊ��octet>>" << endl;
	}
	cout << "<<<���������ļ������С�������>>>" << endl;

	if (RRQ_create(RRQ_packet, Filepointer, TEMP) == 0) {
		cout << "ERROR" << endl;
		cout << "�ռ��������" << endl;
		return 0;//����ʧ��
	};//��������WRQ���Ĳ����ͣ�������Ƿ��ͳɹ�

	FILE* fp = NULL;//���ش����ļ���������
	if (type3 == 1)  fp = fopen(Filelocalpointer, "w");
	else fp = fopen(Filelocalpointer, "wb");
	if (fp == NULL) {
		//�����ļ�ʧ��
		cout << "<<<���󣺣� �ļ�����ʧ�ܣ���>>>" << endl;
		cout << "ʧ��ԭ�򣺣������ļ�ʧ�ܣ���" << endl;
		LOG_GET(1, Filepointer,Filelocalpointer, type3, 0);
		return 0;
	}
	//�����ļ�
	begin = clock();//��ʼ��ʱ
	ACK_create(&ACK_packet);//����ACK����
	do {
		for (Wtime = 0; Wtime < RT_Ttimeout * RT_Ttime_MAX; Wtime += 50) {
			pack_size = recvfrom(sock, (char*)&REV_packet, sizeof(TFTP_rev), 0, (struct sockaddr*)&serveIP, (int*)&ip_len);
			//���Խ��ܱ��ģ�ÿ���50ms����һ�Σ�
			if (Wtime == RT_Ttimeout && block == 1) {
				//Timeout�޷����ܵ����ģ�block==1��ʾ�޷����ܵ����ݰ�
				cout << "<<<���󣺣��ļ�����ʧ��>>>" << endl;
				cout << "ʧ��ԭ�򣺣��޷����ܵ����ݰ�����" << endl;
				LOG_GET(2, Filepointer, Filelocalpointer, type3, block);//����������Ӧ
				fclose(fp);//�رմ������ļ�
				return 0;
			}
			if (pack_size > 0 && pack_size < 4) {
				cout << "���ݰ�����������" << endl;
			}
			else if (pack_size >= 4 && REV_packet.OPC == htons(TFTP_OC_DATA) && REV_packet.block == htons(block)) {
				//�ɹ����ܵ����ݰ�DATA��
				cout << "���ݰ�����" << ntohs(REV_packet.block) << "\t���ݰ���С����" << pack_size - 4<<endl;
				ACK_packet->block = htons(block);
				sendto(sock, (char*)(ACK_packet), sizeof(TFTP_ACK), 0, (struct sockaddr*)&serveIP, ip_len);
				fwrite(REV_packet.DATA, 1, pack_size - 4, fp);
				break;
			}
			else {
				if ((Wtime) && (Wtime % RT_Ttimeout == 0)) {
					Sleep(20);
					cout << "��:" << block << "��ʧ����" << endl;
					cout << "�ط�ACK��:" << block - 1 << endl;
					//�ط�ACK��
					sendto(sock, (char*)(ACK_packet), sizeof(TFTP_ACK), 0, (struct sockaddr*)&serveIP, ip_len);
					LOG_GET(3, Filepointer, Filelocalpointer, type3, block);
				}
			}
			Sleep(50);//ͣ��50ms
		}
		Tbyte += (pack_size - 4);
		if (Wtime >= RT_Ttimeout * RT_Ttime_MAX) {
			cout << "<<<�ļ�����ʧ�ܣ���>>>" << endl;
			cout << "���ݰ�����" << block << "��ʱ����" << endl;
			fclose(fp);
			LOG_GET(4, Filepointer, Filelocalpointer, type3, block);
			return 0;//�ļ�����ʧ�ܵķ���ֵ
		}
		block++;
	} while (pack_size == DATA_MAX_SIZE + 4);
	fclose(fp);
	end = clock();
	Ttime = ((double)(end - begin)) / 1000;//msתs
	cout << "<<�����������>>****" << endl;
	cout << "�������ش�С����" << Tbyte / 1024 << "kB" << endl;
	cout << "����������ʱ����" << Ttime << 's' << endl;
	cout << "�����������ʣ���" << Tbyte / (1024 * Ttime) << endl;
	LOG_GET(5, Filepointer, Filelocalpointer, type3, block);
	return 1;//����ɹ�
}



//+++++++++++++������++++++++++++//
int main() {

	char IP_serve[20];//���ڴ洢IP��ַ
	int nrc;
	WSADATA  wsadata;
	int type2;
	ip_len = sizeof(struct sockaddr_in);//�����ַ���Ȳ���������У����ڲ���
	time(&Time);//���ú�����⵱ǰʱ��
	time_now = localtime(&Time);//�ñ�������ת��ʱ��
	LOG_F = fopen("tftpclient.log", "a");//����LOG�ļ��洢��־
	if (LOG_F == NULL) {//����LOG�ļ�ʧ��//һ���ò���
		cout << "���󣡣�" << endl;
		cout << "����ʱ��Ϊ��" << asctime(time_now) << endl;//�����ǰʱ��
	}
	while (1) {
		cout << "��ѡ����Ҫִ�еĲ�������" << endl;
		cout << "1.�����ļ�(upload)��������" << endl;
		cout << "2.�����ļ�(dowload)��������" << endl;
		cout << "0.�˳�ϵͳ��������" << endl;
		cin >> type2;
		system("cls");//��������ϵͳ
		if (type2 == 0) {
			cout << "<<��лʹ�ã������˳�>>" << endl;
			break;
		}
		else if (type2 == 1) {
			cout << "<<��ӭʹ�ñ��ͻ��������ļ�>>" << endl;
			INIT_winsock(nrc, wsadata);//Winsock��ʼ��
			serverIP_GET(IP_serve);//ѯ�����÷����������Ϣ��IP_serveָ��洢IP��ַ�����ݿ�
			cout << "���������Ҫ���ص��ļ�·������" << endl;
			char Filesourse[60];//�洢�ļ���ַ��Ϣ
			char* Filepointer;//ָ���ļ���ַ��Ϣ��ָ��
			cin >> Filesourse;
			Filepointer = Filesourse;
			if (Filepointer != NULL) {//�����ļ���Ϣ��ΪNULL
				upload(Filepointer);
				system("pause");
				system("cls");//����
			}
			else {
				cout << "���󣡣�" << endl;
				cout << "�޷���ȡ�����ļ�����" << endl;
			}
		}
		else if (type2 == 2) {
			cout << "<<��ӭʹ�ñ��ͻ��������ļ�>>" << endl;
			INIT_winsock(nrc, wsadata);//Winsock��ʼ��
			serverIP_GET(IP_serve);//ѯ�����÷����������Ϣ��IP_serveָ��洢IP��ַ�����ݿ�
			cout << "���������Ҫ���ص��ļ�·������" << endl;
			char Filesourse[60];//�洢�ļ���ַ��Ϣ
			char* Filepointer;//ָ��Ŀ���ļ���ַ��Ϣ��ָ��
			cin >> Filesourse;
			cout << "���������Ҫ���ļ�����ı��ص�ַ����" << endl;
			char Filelocal[60];
			char* Filelocalpointer;//ָ�򱾵��ļ���ַ��Ϣ��ָ��
			cin >> Filelocal;
			Filepointer = Filesourse;
			Filelocalpointer = Filelocal;
			if (Filepointer != NULL && Filelocalpointer != NULL) {//�����ļ���Ϣ��ΪNULL
				download(Filepointer, Filelocalpointer);
				system("pause");
				system("cls");//����
			}
			else {
				cout << "���󣡣�" << endl;
				cout << "�ļ���Ϣ���󣡣�" << endl;
			}
		}
		else if (type2 == 0) {
			cout << "__��лʹ�ã���__" << endl;
			break;//����ѭ������
		}
		fclose(LOG_F);//�ر�LOG��־�ļ�
		return 0;
	}
}