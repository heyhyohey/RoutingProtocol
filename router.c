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

// 라우팅 테이블 엔트리 구조체
typedef struct entry {
	int port;		// 목적지		
	int dir;		// 방향
	int dist;		// 거리
}ENTRY;

// 코어 데이터 구조체
typedef struct socket_set {
	SOCKET socket;		// 소켓
	SOCKADDR_IN addr;	// 소켓 주소
	ENTRY* r_table;		// 라우팅 테이블
	int* r_size;		// 테이블의 크기
	int r_dir;			// 인터페이스 방향
}SOCK_SET;


// 함수정의
void send_thread(void*);
void recv_thread(void*);
void ErrorHandling(char*);
void flush_buffer();
void display_table(ENTRY*, int*);

int main(int argc, char *argv[])
{
	WSADATA wsaData;	// 윈도우 소켓 데이터
	SOCKET l_send_sock, r_send_sock, l_recv_sock, r_recv_sock;	// 소켓
	SOCK_SET l_send_sock_set, r_send_sock_set, l_recv_sock_set, r_recv_sock_set;	// 소켓 주소 세트
	ENTRY r_table[TABLE_SIZE] = { 0 };	// 라우팅 테이블
	char input;
	int r_size = 0;	// 테이블의 사이즈
	int l_send_port, r_send_port, l_recv_port, r_recv_port;	// 포트번호
	bool flag = true, isRun = false;

	SOCKADDR_IN l_send_addr, r_send_addr, l_recv_addr, r_recv_addr;

	for (int i = 0; i < TABLE_SIZE; i++)
		r_table[i].port = -1;

	// 1. 라우터 인터페이스의 포트번호를 입력
	printf("> My Node Configuration\n- Input Left Port: ");
	scanf("%d", &l_send_port);
	printf("- Input Right Port: ");
	scanf("%d", &r_send_port);

	// 2. 인접 라우터의 포트번호를 입력
	printf("\n> Neighbor Node Configuration\n- Input Left Neighbor Port: ");
	scanf("%d", &l_recv_port);
	printf("- Input Right Neighbor Port: ");
	scanf("%d", &r_recv_port);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	flush_buffer();

	// 3. 테이블 정보 초기화
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

	// 4. 왼쪽 인터페이스 소켓 생성 및 초기화
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

	// 5. 오른쪽 인터페이스 소켓 생성 및 초기화
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

	// 6. 왼쪽 라우터로부터 수신하는 소켓 생성 및 초기화
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

	// 7. 오른쪽 라우터로부터 수신하는 소켓 생성 및 초기화
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

	// 8. 수신 쓰레드 생성
	if (l_recv_port != 0)
		_beginthread(recv_thread, 0, (void*)&l_recv_sock_set);
	if (r_recv_port != 0)
		_beginthread(recv_thread, 0, (void*)&r_recv_sock_set);
	
	// 9. 사용자 입력 받음
	while (flag) {
		fprintf(stdout, "\n> Input Command(s:Start p:Print q:Quit): ");
		scanf("%c", &input);
		flush_buffer();
		switch (input) {
		case 's':
			// 9-1. 송신 쓰레드 생성
			if(!isRun) {
				_beginthread(send_thread, 0, (void*)&l_send_sock_set);
				_beginthread(send_thread, 0, (void*)&r_send_sock_set);
				isRun = true;
			}
			else
				fprintf(stdout, "- Already running\n");
			break;
		case 'p':
			// 9-2. 테이블 출력
			display_table(r_table, &r_size);
			break;
		case 'q':
			// 9-3. 프로그램 종료
			fprintf(stdout, "Quit\n\n");
			flag = false;
			break;
		default:
			fprintf(stdout, "Wrong Input\n\n");
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

// 표준 입력 버퍼 플러쉬 함수
void flush_buffer() {
	char c;

	do {
		c = getchar(stdin);
	} while (c != '\n');
}

// 에러 처리 함수
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

// 테이블 정보 출력 함수
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

// 데이터 송신 쓰레드
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

// 데이터 수신 스레드
void recv_thread(void* sock) {
	SOCK_SET* recv_sock_set = (SOCK_SET*)sock;
	SOCKET recv_sock = recv_sock_set->socket;
	SOCKADDR_IN addr = recv_sock_set->addr;
	ENTRY recv_table[TABLE_SIZE], *r_table = recv_sock_set->r_table;
	char message[BUF_SIZE];
	int i, j, addr_size, strLen, cnt = 0, *r_size = recv_sock_set->r_size, r_dir = recv_sock_set->r_dir;
	bool u_flag = true;	// 테이블 수정 여부 플래그

	// 1. 소켓 바인드
	if (bind(recv_sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");

	for (;;) {
		addr_size = sizeof(addr);
		// 2. 왼쪽 인터페이스로 부터 테이블 수신
		strLen = recvfrom(recv_sock, message, sizeof(message), 0, (SOCKADDR*)&addr, &addr_size);
		if (strLen < 0)
			continue;
		fprintf(stdout, "\n- Recive Rounting Table from %d\n\n> Input Command(s:Start p:Print q:Quit): ", addr.sin_port);
		memcpy(recv_table, message, sizeof(message));

		// 3. 테이블 수정
		for (i = 0; recv_table[i].port != -1; i++) {
			u_flag = true;
			for (j = 0;  j < *r_size; j++) {
				// 3-1. 포트번호가 같은 것이 있는지 확인
				if (recv_table[i].port == r_table[j].port) {
					// 3-1-1. 거리가 더 적은 것으로 업데이트
					if (recv_table[i].dist + 1 < r_table[j].dist) {
						r_table[j].dir = r_dir;
						r_table[j].dist = (recv_table[i].dist) + 1;
					}
					u_flag = false;
					break;
				}
			}
			// 3-2. 존제하지 않았다면 업데이트
			if (u_flag) {
				r_table[*r_size].port = recv_table[i].port;
				r_table[*r_size].dir = r_dir;
				r_table[*r_size].dist = (recv_table[i].dist) + 1;
				(*r_size)++;
			}
		}
	}
}