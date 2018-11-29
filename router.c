#define HAVE_STRUCT_TIMESPEC
#define BUF_SIZE 512
#define TABLE_SIZE 1024

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>

// 라우팅 테이블 엔트리 구조체
typedef struct entry {
	int dest;		// 목적지		
	char dir[10];	// 방향
	int dist;		// 거리
}ENTRY;

// 소켓 주소 구조체
typedef struct socket_set {
	SOCKET socket;
	SOCKADDR_IN addr;
}SOCK_SET;

ENTRY rounting_table[TABLE_SIZE] = { 0 };	// 라우팅 테이블

// 함수정의
void send_thread(void*);
void recv_thread(void*);
void ErrorHandling(char*);
void flush_buffer();

int main(int argc, char *argv[])
{
	//pthread_t thread[4];	// 스레드
	WSADATA wsaData;	// 윈도우 소켓 데이터
	SOCKET l_send_sock, r_send_sock, l_recv_sock, r_recv_sock;	// 소켓
	SOCK_SET l_send_sock_set, r_send_sock_set, l_recv_sock_set, r_recv_sock_set;	// 소켓 주소 세트
	char message[BUF_SIZE], input;
	int strLen, i;
	int l_send_port, r_send_port, l_recv_port, r_recv_port;	// 포트번호
	bool flag = true;

	SOCKADDR_IN l_send_addr, r_send_addr, l_recv_addr, r_recv_addr;

	// 1. 라우터의 포트번호를 입력
	printf("> My Node Configuration\n- Input Left Port: ");
	scanf("%d", &l_send_port);
	printf("- Input Right Port: ");
	scanf("%d", &r_send_port);

	// 2. 이웃한 라우터의 포트번호를 입력
	printf("\n> Neighbor Node Configuration\n- Input Left Neighbor Port: ");
	scanf("%d", &l_recv_port);
	printf("- Input Right Neighbor Port: ");
	scanf("%d", &r_recv_port);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	flush_buffer();

	// 3. 왼쪽 인터페이스 소켓 생성 및 초기화
	l_send_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (l_send_sock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	memset(&l_send_addr, 0, sizeof(l_send_addr));
	l_send_addr.sin_family = AF_INET;
	l_send_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	l_send_addr.sin_port = htons(l_send_port);

	l_send_sock_set.socket = l_send_sock;
	l_send_sock_set.addr = l_send_addr;

	//connect(l_send_sock, (SOCKADDR*)&l_send_addr, sizeof(l_send_addr));

	// 4. 오른쪽 인터페이스 소켓 생성 및 초기화
	r_send_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (r_send_sock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	memset(&r_send_addr, 0, sizeof(r_send_addr));
	r_send_addr.sin_family = AF_INET;
	r_send_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	r_send_addr.sin_port = htons(r_send_port);

	r_send_sock_set.socket = r_send_sock;
	r_send_sock_set.addr = r_send_addr;

	//connect(r_send_sock, (SOCKADDR*)&r_send_addr, sizeof(r_send_addr));

	// 5. 왼쪽 라우터로부터 수신하는 소켓 생성 및 초기화
	l_recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (l_recv_sock == INVALID_SOCKET)
		ErrorHandling("UDP socket creation error");

	memset(&l_recv_addr, 0, sizeof(l_recv_addr));
	l_recv_addr.sin_family = AF_INET;
	l_recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	l_recv_addr.sin_port = htons(l_recv_port);

	//if (bind(l_recv_sock, (SOCKADDR*)&l_recv_addr, sizeof(l_recv_addr)) == SOCKET_ERROR)
	//	ErrorHandling("bind() error");

	l_recv_sock_set.socket = l_recv_sock;
	l_recv_sock_set.addr = l_recv_addr;

	// 6. 오른쪽 라우터로부터 수신하는 소켓 생성 및 초기화
	r_recv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (r_recv_sock == INVALID_SOCKET)
		ErrorHandling("UDP socket creation error");

	memset(&r_recv_addr, 0, sizeof(r_recv_addr));
	r_recv_addr.sin_family = AF_INET;
	r_recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	r_recv_addr.sin_port = htons(r_recv_port);

	r_recv_sock_set.socket = r_recv_sock;
	r_recv_sock_set.addr = r_recv_addr;

	// 7. 수신 쓰레드 생성
	if (l_recv_port != 0)
		_beginthread(recv_thread, 0, (void*)&l_recv_sock_set);
	if (r_recv_port != 0)
		_beginthread(recv_thread, 0, (void*)&r_recv_sock_set);
	
	// 8. 사용자 입력 받음
	while (flag) {
		printf("> Input Command(s:Start p:Print q:Quit): ");
		scanf("%c", &input);
		flush_buffer();
		switch (input) {
		case 's':
			// 8-1. 송신 쓰레드 생성
			_beginthread(send_thread, 0, (void*)&l_send_sock_set);
			_beginthread(send_thread, 0, (void*)&r_send_sock_set);
			break;
		case 'p':
			// 8-2. 테이블 출력
			// display_table();
			break;
		case 'q':
			printf("Quit\n\n");
			flag = false;
			break;
		default:
			printf("Wrong Input\n\n");
			break;
		}
	}

	// 10. 소켓 할당 해제
	closesocket(l_send_sock);
	closesocket(r_send_sock);
	closesocket(l_recv_sock);
	closesocket(r_recv_sock);
	WSACleanup();
	return 0;
}

void flush_buffer() {
	char c;

	do {
		c = getchar(stdin);
	} while (c != '\n');
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void send_thread(void* sock) {
	SOCK_SET *send_sock_set = (SOCK_SET*)sock;
	SOCKET send_sock = send_sock_set->socket;
	SOCKADDR_IN addr = send_sock_set->addr;
	char message[BUF_SIZE] = "asdf";
	int strLen;

	for(;;) {
		fprintf(stdout, "send massage\n");
		sendto(send_sock, message, strlen(message), 0, (SOCKADDR*)&addr, sizeof(addr));
		Sleep(1000);
	}
}

void recv_thread(void* sock) {
	SOCK_SET* recv_sock_set = (SOCK_SET*)sock;
	SOCKET recv_sock = recv_sock_set->socket;
	SOCKADDR_IN addr = recv_sock_set->addr;
	char message[BUF_SIZE];
	int addr_size, strLen;
	if (bind(recv_sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");

	for (;;) {
		addr_size = sizeof(addr);
		strLen = recvfrom(recv_sock, message, BUF_SIZE, 0,
			(SOCKADDR*)&addr, &addr_size);
		message[strLen] = '\0';
		fprintf(stdout, "message from client : %s \n", message);
	}
}