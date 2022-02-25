#include <iostream>
#include <WinSock2.h>
#include <process.h>

#pragma comment(lib,"ws2_32.lib")	//ws2_32.lib 파일경로 명시, 속성창에서 라이브러리 추가하는것과 유사함

#define MAX_PACKETLEN 1024	//패킷 길이
#define PORT 3500	//포트번호
#define MAX_CLNT 256	//보관가능한 최대 클라이언트 갯수


unsigned WINAPI t_func(void* data);//클라이언트 처리 스레드 함수
void SendMsg(char* msg, int len);//메시지 보내는 함수

int clientCount = 0;	//현제까지 접속한 클라이언트 갯수
SOCKET clientSocks[MAX_CLNT];//클라이언트 소켓 보관용 배열
HANDLE hMutex;//뮤텍스

void SendMsg(char* msg, int len) { //메시지를 모든 클라이언트에게 보낸다.
	int i;
	WaitForSingleObject(hMutex, INFINITE);//뮤텍스 실행
	for (i = 0; i < clientCount; i++)//클라이언트 개수만큼
		send(clientSocks[i], msg, len, 0);//클라이언트들에게 메시지를 전달한다.
	ReleaseMutex(hMutex);//뮤텍스 중지
}


unsigned WINAPI t_func(void* data) {//클라이언트 처리 스레드 함수	
	SOCKET sockfd = *((SOCKET*)data);
	int strLen;	//받은메세지 길이 보관
	char szReceiveBuffer[MAX_PACKETLEN];//메세지 받을 버퍼

	while ((strLen = recv(sockfd, szReceiveBuffer, sizeof(szReceiveBuffer), 0)) != 0) { //클라이언트로부터 메시지를 받을때까지 기다린다.
		SendMsg(szReceiveBuffer, strLen);//SendMsg에 받은 메시지를 전달한다.
		//printf("받은 메세지 : %s \n", szReceiveBuffer);
	}

	WaitForSingleObject(hMutex, INFINITE);//뮤텍스 실행
	for (int i = 0; i < clientCount; i++) {//배열의 갯수만큼
		if (sockfd == clientSocks[i]) {//만약 현재 clientSock값이 배열의 값과 같다면
			while (i++ < clientCount - 1)//클라이언트 개수 만큼
				clientSocks[i] = clientSocks[i + 1];//앞으로 땡긴다.
			break;
		}
	}
	clientCount--;//클라이언트 개수 하나 감소
	ReleaseMutex(hMutex);//뮤텍스 중지

	printf("Thread Close : %d\n", GetCurrentThreadId());

	closesocket(sockfd); 
	return 0;

}

int main()
{
	WSADATA wsaData; //윈도우 소켓 주소의 정보가 들어있는 구조체.
	SOCKET listen_s, client_s;	//듣기소켓 클라이언트 소켓 과 클라이언트 소켓
	struct sockaddr_in		server_addr, client_addr;	//서버주소, 클라이언트주소
	HANDLE hTread;	//스레드 핸들
	int addr_len;	//주소길이 저장

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { //윈속을 사용하기 전에 ws2_32.dll을 초기화 시키기
		return 1;
	}
	listen_s = socket(AF_INET, SOCK_STREAM, 0);	//듣기소켓에 소켓 할당
	if (listen_s == INVALID_SOCKET) {
		return 1;
	}

	ZeroMemory(&server_addr, sizeof(struct sockaddr_in));	//서버주소 구조체 초기화

	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(listen_s, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {	//소켓에 주소할당
		return 0;
	}
	if (listen(listen_s, 5) == SOCKET_ERROR) {	//연결대기
		return 0;
	}

	while (1) {
		addr_len = sizeof(client_addr);
		client_s = accept(listen_s, (SOCKADDR*)&client_addr, &addr_len);//서버에게 전달된 클라이언트 소켓 연결 허용하고 clientSock에 전달
		WaitForSingleObject(hMutex, INFINITE);//뮤텍스 실행
		clientSocks[clientCount++] = client_s;//클라이언트 소켓배열에 방금 가져온 소켓 주소를 전달
		ReleaseMutex(hMutex);//뮤텍스 중지
		hTread = (HANDLE)_beginthreadex(NULL, 0, t_func, (void*)&client_s, 0, NULL);//HandleClient 쓰레드 실행, clientSock을 매개변수로 전달
		printf("Connected Client IP : %s\n", inet_ntoa(client_addr.sin_addr));
	}

	closesocket(listen_s);
	WSACleanup();
	return 0;
}
