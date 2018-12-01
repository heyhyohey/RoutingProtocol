#define HAVE_STRUCT_TIMESPEC
#define BUF_SIZE 768
#define TABLE_SIZE 64
#define SELF -1
#define NONE 0
#define LEFT 1
#define RIGHT 2
#define SLEEP_TIME 20000

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

// ����� ���̺� ��Ʈ�� ����ü
typedef struct entry {
	int port;		// ������		
	int dir;		// ����
	int dist;		// �Ÿ�
}ENTRY;

// �ھ� ������ ����ü
typedef struct socket_set {
	SOCKET socket;		// ����
	SOCKADDR_IN addr;	// ���� �ּ�
	ENTRY* r_table;		// ����� ���̺�
	int* r_size;		// ���̺��� ũ��
	int r_dir;			// �������̽� ����
}SOCK_SET;


// �Լ�����
void send_thread(void*);
void recv_thread(void*);
void ErrorHandling(char*);
void flush_buffer();
void display_table(ENTRY*, int*);

int main(int argc, char *argv[])
{
	WSADATA wsaData;	// ������ ���� ������
	SOCKET l_send_sock, r_send_sock, l_recv_sock, r_recv_sock;	// ����
	SOCK_SET l_send_sock_set, r_send_sock_set, l_recv_sock_set, r_recv_sock_set;	// ���� �ּ� ��Ʈ
	ENTRY r_table[TABLE_SIZE] = { 0 };	// ����� ���̺�
	char input;
	int r_size = 0;	// ���̺��� ������
	int l_send_port, r_send_port, l_recv_port, r_recv_port;	// ��Ʈ��ȣ
	bool flag = true, isRun = false;

	SOCKADDR_IN l_send_addr, r_send_addr, l_recv_addr, r_recv_addr;

	for (int i = 0; i < TABLE_SIZE; i++)
		r_table[i].port = -1;

	// 1. ����� �������̽��� ��Ʈ��ȣ�� �Է�
	printf("> My Node Configuration\n- Input Left Port: ");
	scanf("%d", &l_send_port);
	printf("- Input Right Port: ");
	scanf("%d", &r_send_port);

	// 2. ���� ������� ��Ʈ��ȣ�� �Է�
	printf("\n> Neighbor Node Configuration\n- Input Left Neighbor Port: ");
	scanf("%d", &l_recv_port);
	printf("- Input Right Neighbor Port: ");
	scanf("%d", &r_recv_port);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	flush_buffer();

	// 3. ���̺� ���� �ʱ�ȭ
	if(l_send_port != 0) {
		r_table[r_size].port = l_send_port;
		r_table[r_size].dist = SELF;
		r_table[r_size++].dir = NONE;
	}

	if (r_send_port != 0) {
		r_table[r_size].port = r_send_port;
		r_table[r_size].dist = SELF;
		r_table[r_size++].dir = NONE;
	}

	// 4. ���� �������̽� ���� ���� �� �ʱ�ȭ
	l_send_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (l_send_sock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	memset(&l_send_addr, 0, sizeof(l_send_addr));
	l_send_addr.sin_family = AF_INET;
	l_send_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	l_send_addr.sin_port = htons(l_send_port);

	l_send_sock_set.socket = l_send_sock;
	l_send_sock_set.addr = l_send_addr;
	l_send_sock_set.r_table = r_table;
	l_send_sock_set.r_size = &r_size;
	l_send_sock_set.r_dir = LEFT;

	// 5. ������ �������̽� ���� ���� �� �ʱ�ȭ
	r_send_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (r_send_sock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	memset(&r_send_addr, 0, sizeof(r_send_addr));
	r_send_addr.sin_family = AF_INET;
	r_send_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	r_send_addr.sin_port = htons(r_send_port);

	r_send_sock_set.socket = r_send_sock;
	r_send_sock_set.addr = r_send_addr;
	r_send_sock_set.r_table = r_table;
	r_send_sock_set.r_size = &r_size;
	r_send_sock_set.r_dir = LEFT;

	// 6. ���� ����ͷκ��� �����ϴ� ���� ���� �� �ʱ�ȭ
	l_recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (l_recv_sock == INVALID_SOCKET)
		ErrorHandling("UDP socket creation error");

	memset(&l_recv_addr, 0, sizeof(l_recv_addr));
	l_recv_addr.sin_family = AF_INET;
	l_recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	l_recv_addr.sin_port = htons(l_recv_port);

	l_recv_sock_set.socket = l_recv_sock;
	l_recv_sock_set.addr = l_recv_addr;
	l_recv_sock_set.r_table = r_table;
	l_recv_sock_set.r_size = &r_size;
	l_recv_sock_set.r_dir = LEFT;

	// 7. ������ ����ͷκ��� �����ϴ� ���� ���� �� �ʱ�ȭ
	r_recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (r_recv_sock == INVALID_SOCKET)
		ErrorHandling("UDP socket creation error");

	memset(&r_recv_addr, 0, sizeof(r_recv_addr));
	r_recv_addr.sin_family = AF_INET;
	r_recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	r_recv_addr.sin_port = htons(r_recv_port);

	r_recv_sock_set.socket = r_recv_sock;
	r_recv_sock_set.addr = r_recv_addr;
	r_recv_sock_set.r_table = r_table;
	r_recv_sock_set.r_size = &r_size;
	r_recv_sock_set.r_dir = RIGHT;

	// 8. ���� ������ ����
	if (l_recv_port != 0)
		_beginthread(recv_thread, 0, (void*)&l_recv_sock_set);
	if (r_recv_port != 0)
		_beginthread(recv_thread, 0, (void*)&r_recv_sock_set);
	
	// 9. ����� �Է� ����
	while (flag) {
		fprintf(stdout, "\n> Input Command(s:Start p:Print q:Quit): ");
		scanf("%c", &input);
		flush_buffer();
		switch (input) {
		case 's':
			// 9-1. �۽� ������ ����
			if(!isRun) {
				_beginthread(send_thread, 0, (void*)&l_send_sock_set);
				_beginthread(send_thread, 0, (void*)&r_send_sock_set);
				isRun = true;
			}
			else
				fprintf(stdout, "- Already running\n");
			break;
		case 'p':
			// 9-2. ���̺� ���
			display_table(r_table, &r_size);
			break;
		case 'q':
			// 9-3. ���α׷� ����
			fprintf(stdout, "Quit\n\n");
			flag = false;
			break;
		default:
			fprintf(stdout, "Wrong Input\n\n");
			break;
		}
	}

	// 10. ���� �Ҵ� ����
	closesocket(l_send_sock);
	closesocket(r_send_sock);
	closesocket(l_recv_sock);
	closesocket(r_recv_sock);
	WSACleanup();

	return 0;
}

// ǥ�� �Է� ���� �÷��� �Լ�
void flush_buffer() {
	char c;

	do {
		c = getchar(stdin);
	} while (c != '\n');
}

// ���� ó�� �Լ�
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

// ���̺� ���� ��� �Լ�
void display_table(ENTRY* r_table, int* size) {
	char dir[BUF_SIZE];

	printf("\n- Print Rounting Table\nPort\tDirection\tDistance\n");
	for (int i = 0; i < *size; i++) {
		switch (r_table[i].dir) {
		case NONE:
			strcpy(dir, "-");
			break;
		case LEFT:
			strcpy(dir, "Left");
			break;
		case RIGHT:
			strcpy(dir, "Right");
			break;
		}
		if(r_table[i].dist == -1)
			fprintf(stdout, "%d\t%s\t\t-\n", r_table[i].port, dir);
		else
			fprintf(stdout, "%d\t%s\t\t%d\n", r_table[i].port, dir, r_table[i].dist);
	}
}

// ������ �۽� ������
void send_thread(void* sock) {
	SOCK_SET *send_sock_set = (SOCK_SET*)sock;
	SOCKET send_sock = send_sock_set->socket;
	SOCKADDR_IN addr = send_sock_set->addr;
	ENTRY *r_table = send_sock_set->r_table;
	char message[BUF_SIZE];
	int cnt = 0, *r_size = send_sock_set->r_size;

	for(;;) {
		memcpy(message, r_table, sizeof(ENTRY)*TABLE_SIZE);
		sendto(send_sock, message, sizeof(message), 0, (SOCKADDR*)&addr, sizeof(addr));
		fprintf(stdout, "\n- Send Routing Table to %d\n\n> Input Command(s:Start p:Print q:Quit): ", ntohs(addr.sin_port));
		Sleep(SLEEP_TIME);
	}
}

// ������ ���� ������
void recv_thread(void* sock) {
	SOCK_SET* recv_sock_set = (SOCK_SET*)sock;
	SOCKET recv_sock = recv_sock_set->socket;
	SOCKADDR_IN addr = recv_sock_set->addr;
	ENTRY recv_table[TABLE_SIZE], *r_table = recv_sock_set->r_table;
	char message[BUF_SIZE];
	int i, j, addr_size, strLen, cnt = 0, *r_size = recv_sock_set->r_size, r_dir = recv_sock_set->r_dir;
	bool u_flag = true;	// ���̺� ���� ���� �÷���

	// 1. ���� ���ε�
	if (bind(recv_sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");

	for (;;) {
		addr_size = sizeof(addr);
		// 2. ���� �������̽��� ���� ���̺� ����
		strLen = recvfrom(recv_sock, message, sizeof(message), 0, (SOCKADDR*)&addr, &addr_size);
		if (strLen < 0)
			continue;
		fprintf(stdout, "\n- Recive Rounting Table from %d\n\n> Input Command(s:Start p:Print q:Quit): ", addr.sin_port);
		memcpy(recv_table, message, sizeof(message));

		// 3. ���̺� ����
		for (i = 0; recv_table[i].port != -1; i++) {
			u_flag = true;
			for (j = 0;  j < *r_size; j++) {
				// 3-1. ��Ʈ��ȣ�� ���� ���� �ִ��� Ȯ��
				if (recv_table[i].port == r_table[j].port) {
					// 3-1-1. �Ÿ��� �� ���� ������ ������Ʈ
					if (recv_table[i].dist + 1 < r_table[j].dist) {
						r_table[j].dir = r_dir;
						r_table[j].dist = (recv_table[i].dist) + 1;
					}
					u_flag = false;
					break;
				}
			}
			// 3-2. �������� �ʾҴٸ� ������Ʈ
			if (u_flag) {
				r_table[*r_size].port = recv_table[i].port;
				r_table[*r_size].dir = r_dir;
				r_table[*r_size].dist = (recv_table[i].dist) + 1;
				(*r_size)++;
			}
		}
	}
}