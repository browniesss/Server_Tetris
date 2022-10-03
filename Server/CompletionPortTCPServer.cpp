/*
18032064 이정우 게플 3B
테트리스 서버
*/

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

// 메세지들
#define INTRO_MSG "상대가 접속할때까지 기다려주세요\n"
#define ID_ERROR_MSG "없는 아이디입니다\n"
#define PW_ERROR_MSG "패스워드가 틀렸습니다.\n"
#define LOGIN_SUCCESS_MSG "로그인에 성공했습니다.방 인원(2명)이 가득차면 게임을 시작합니다.\n"
#define ID_EXIST_MSG "이미 있는 아이디 입니다.\n"
#define JOIN_SUCCESS_MSG "가입에 성공했습니다.\n"

#define SERVERPORT 9000
#define BUFSIZE 4096
#define IDSIZE 255
#define PWSIZE 255
#define NICKNAMESIZE 255
#define ERROR_DISCONNECTED -2
#define DISCONNECTED -1


enum STATE // 클라이언트의 스테이트
{
	NO_STATE = -1,
	MENU_STATE, // 메뉴 선택 스테이트
	WAIT_STATE, // 게임 시작 기다리는 스테이트
	GAME_STATE, // 게임 진행 중인 스테이트 
	END_STATE, // 게임 종료 스테이트 
	DISCONNECTED_STATE // 연결 종료 스테이트
};

enum ROOM_STATE // 게임 방 스테이트
{
	R_WAIT_STATE = 1, // 시작 기다리는 스테이트 
	R_PLAYING_STATE, // 게임 진행중인 스테이트
};

enum RESULT // 결과
{
	NODATA = -1,
	ID_EXIST = 1, // 아이디 존재 
	ID_ERROR, // 아이디 에러
	PW_ERROR, // 패스워드 에러
	JOIN_SUCCESS, // 가입 성공 
	LOGIN_SUCCESS, // 로그인 성공
};

enum PROTOCOL // 프로토콜
{
	NO_PROTOCOL = -1,
	REGISTER = 1, // 회원가입 프로토콜 
	LOGIN, // 로그인 프로토콜 
	REGISTER_RESULT, // 회원가입 결과 프로토콜
	LOGIN_RESULT, // 로그인 결과 프로토콜
	GAME_ROOM_ENTER, // 게임 방 입장 프로토콜 
	GAME_START, // 게임 시작 프로토콜 
	MY_UPDATE, // 내 테트리스 정보 업데이트 프로토콜 
	ENEMY_UPDATE, // 상대 테트리스 정보 업데이트 프로토콜
	GAME_OVER, // 게임 오버 프로토콜
	ENEMY_EXIT, // 상대방 종료 프로토콜
};

enum // 에러
{
	SOC_ERROR = 1,
	SOC_TRUE,
	SOC_FALSE
};

struct Tetris_Info // 테트리스 정보 
{
	int board[12][22]; // 테트리스 보드
	int brick, nbrick; // 테트리스 벽돌 정보
	int nx, ny; // 현재 이동중인 테트리스 좌표 
	int rot;		// 회전
	int score;		// 점수
	int bricknum;	// 벽돌 개수
};

struct _User_Info // 유저 정보.
{
	char id[IDSIZE];
	char pw[PWSIZE];
	char nickname[NICKNAMESIZE];
};

struct EXOVERLAPPED // 확장 구조체
{
	WSAOVERLAPPED overlapped;
	bool IOTYPE; // false = recv, true = send
	void* client_info;
};

// 소켓 정보 저장을 위한 구조체
struct SOCKETINFO
{
	// 확장 구조체
	EXOVERLAPPED recvEX;
	EXOVERLAPPED sendEX;

	SOCKET sock;
	SOCKADDR_IN		addr;
	_User_Info userInfo;
	STATE state;
	RESULT result;

	char recvbuf[BUFSIZE];
	char sendbuf[BUFSIZE];

	bool r_sizeflag;
	int recvbytes;
	int comp_recvbytes;
	int sendbytes;
	int comp_sendbytes;

	WSABUF r_wsabuf;
	WSABUF s_wsabuf;

	Tetris_Info* tetris_Info; 
};

struct _Room_Info // 방 정보 구조체
{
	SOCKETINFO* users[2]; // 방에 들어온 유저 정보 포인터 
	ROOM_STATE state; // 현재 방의 상태
	int count = 0; // 유저 count
};

// 회원가입 리스트
_User_Info* Join_List[100];
int join_Count = 0;

SOCKETINFO* user[100]; // 유저 정보 리스트 
int usercount = 0;

// 방 리스트
_Room_Info* RoomInfo[50];
int room_count = 0;

CRITICAL_SECTION cs; // 크리티컬 섹션

int gamecount = 0;

// 작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg);
// 오류 출력 함수
void err_quit(char* msg);
void err_display(char* msg);
void err_display(int errcode);

bool Recv(SOCKETINFO* _ptr);
int CompleteRecv(SOCKETINFO* _ptr, int _completebyte);
bool Send(SOCKETINFO* _ptr, int _size);
int CompleteSend(SOCKETINFO* _ptr, int _completebyte);

void GetProtocol(const char* _ptr, PROTOCOL& _protocol);
int PackPacket(char* _buf, PROTOCOL _protocol, RESULT _result, const char* _str1);
int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str1);
int PackPacket(char* _buf, PROTOCOL _protocol);
void PackPacket(char* _buf, PROTOCOL _protocol, char* _str, Tetris_Info* info, int& _size);
void PackPacket(char* _buf, PROTOCOL _protocol, Tetris_Info* info, int& _size);
void UnPackPacket(const char* _buf, char* _str1, char* _str2, char* _str3);
void UnPackPacket(const char* _buf, char* _str1, char* _str2);
void UnPackPacket(const char* _buf, char* _str1);
void UnPackPacket(const char* _buf, int& _data);
void UnPackPacket(const char* buf, Tetris_Info* Enemy_info);
void RecvPacketProcess(SOCKETINFO* _ptr);
void JoinProcess(SOCKETINFO* _ptr);
void LoginProcess(SOCKETINFO* _ptr);
void GameEnterProcess(SOCKETINFO* _ptr);
void GameInfoUpdateProcess(SOCKETINFO* _ptr);
void Game_End_Process(SOCKETINFO* _ptr);
_Room_Info* Search_Room(SOCKETINFO* _ptr);
_Room_Info* Create_Room();
void User_Exit(SOCKETINFO* _ptr);
void Remove_Room(_Room_Info* _ptr);

// 클라이언트 초기화 및 정보 넣어주기
SOCKETINFO* AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr)
{
	EnterCriticalSection(&cs); // 크리티컬 섹션 진입
	SOCKETINFO* ptr = new SOCKETINFO;

	memset(ptr, 0, sizeof(SOCKETINFO));

	ptr->sock = _sock;
	memcpy(&ptr->addr, &_addr, sizeof(SOCKADDR_IN));
	ptr->state = MENU_STATE;

	ptr->r_sizeflag = false;

	ptr->recvEX.IOTYPE = false;
	ptr->sendEX.IOTYPE = true;
	// SOCKETINFO 구조체 포인터로 넣어줌.
	ptr->recvEX.client_info = (SOCKETINFO*)ptr;
	ptr->sendEX.client_info = (SOCKETINFO*)ptr;

	user[usercount++] = ptr;

	printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(ptr->addr.sin_addr), ntohs(ptr->addr.sin_port));

	LeaveCriticalSection(&cs); // 크리티컬 섹션 탈출

	return ptr;
}

void Remove_Client_Info(SOCKETINFO* _ptr) // 클라이언트 정보 지우기
{
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
		inet_ntoa(_ptr->addr.sin_addr), ntohs(_ptr->addr.sin_port));

	EnterCriticalSection(&cs); // 크리티컬 섹션 진입
	for (int i = 0; i < usercount; i++)
	{
		if (user[i] == _ptr)
		{
			closesocket(user[i]->sock);

			delete user[i];

			for (int j = i; j < usercount - 1; j++) // 클라이언트 정보 지우고 당겨줌.
			{
				user[j] = user[j + 1];
			}

			break;
		}
	}

	usercount--;
	LeaveCriticalSection(&cs); // 크리티컬 섹션 탈출
}

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	InitializeCriticalSection(&cs);

	// 입출력 완료 포트 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) return 1;

	// CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// (CPU 개수 * 2)개의 작업자 스레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);
	}

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes, flags;

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		// 소켓 정보 구조체 할당
		SOCKETINFO* ptr = AddClientInfo(client_sock, clientaddr);

		if (!Recv(ptr)) // 최초의 recv
		{
			continue;
		}
	}

	// 윈속 종료
	WSACleanup();
	return 0;
}

bool Recv(SOCKETINFO* _ptr) // recv 
{
	int retval;
	DWORD recvbytes;
	DWORD flags = 0;

	ZeroMemory(&_ptr->recvEX.overlapped, sizeof(_ptr->recvEX.overlapped));

	_ptr->r_wsabuf.buf = _ptr->recvbuf + _ptr->comp_recvbytes;

	if (_ptr->r_sizeflag)
	{
		_ptr->r_wsabuf.len = _ptr->recvbytes - _ptr->comp_recvbytes;
	}
	else
	{
		_ptr->recvbytes = sizeof(int) - _ptr->comp_recvbytes;
		_ptr->r_wsabuf.len = _ptr->recvbytes;
	}

	retval = WSARecv(_ptr->sock, &_ptr->r_wsabuf, 1, &recvbytes,
		&flags, &_ptr->recvEX.overlapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			err_display("WSARecv()");
			return false;
		}
	}

	return true;
}

int CompleteRecv(SOCKETINFO* _ptr, int _completebyte) // completerecv 
{
	if (!_ptr->r_sizeflag)
	{
		_ptr->comp_recvbytes += _completebyte;

		if (_ptr->comp_recvbytes == sizeof(int))
		{
			memcpy(&_ptr->recvbytes, _ptr->recvbuf, sizeof(int));
			_ptr->comp_recvbytes = 0;
			_ptr->r_sizeflag = true;
		}

		if (!Recv(_ptr))
		{
			return SOC_ERROR;
		}

		return SOC_FALSE;
	}

	_ptr->comp_recvbytes += _completebyte;

	if (_ptr->comp_recvbytes == _ptr->recvbytes)
	{
		_ptr->comp_recvbytes = 0;
		_ptr->recvbytes = 0;
		_ptr->r_sizeflag = false;

		return SOC_TRUE;
	}
	else
	{
		if (!Recv(_ptr))
		{
			return SOC_ERROR;
		}

		return SOC_FALSE;
	}
}

bool Send(SOCKETINFO* _ptr, int _size) // send
{
	int retval;
	DWORD sendbytes;
	DWORD flags;

	ZeroMemory(&_ptr->sendEX.overlapped, sizeof(_ptr->sendEX.overlapped));

	if (_ptr->sendbytes == 0)
	{
		_ptr->sendbytes = _size;
	}

	_ptr->s_wsabuf.buf = _ptr->sendbuf + _ptr->comp_sendbytes;
	_ptr->s_wsabuf.len = _ptr->sendbytes - _ptr->comp_sendbytes;

	retval = WSASend(_ptr->sock, &_ptr->s_wsabuf, 1, &sendbytes,
		0, &_ptr->sendEX.overlapped, NULL);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			err_display("WSASend()");
			return false;
		}
	}

	return true;
}

int CompleteSend(SOCKETINFO* _ptr, int _completebyte) // completesend
{
	_ptr->comp_sendbytes += _completebyte;
	if (_ptr->comp_sendbytes == _ptr->sendbytes)
	{
		_ptr->comp_sendbytes = 0;
		_ptr->sendbytes = 0;

		return SOC_TRUE;
	}
	if (!Send(_ptr, _ptr->sendbytes))
	{
		return SOC_ERROR;
	}

	return SOC_FALSE;
}

// 작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1) {
		// 비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET client_sock;
		EXOVERLAPPED* tempOverlapped;

		SOCKETINFO* ptr;

		retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
			(LPDWORD)&client_sock, (LPOVERLAPPED*)&tempOverlapped, INFINITE);

		ptr = (SOCKETINFO*)tempOverlapped->client_info;

		// 클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);


		// 비동기 입출력 결과 확인
		if (retval == 0 || cbTransferred == 0)
		{
			if (retval == 0)
			{ // 실패 , cbtransferred 가 0은 연결이 끊긴것.
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &tempOverlapped->overlapped,
					&temp1, FALSE, &temp2); // 구체적인 에러코드의 발생목적으로 함수 호출
				err_display("WSAGetOverlappedResult()");
			}

			ptr->state = DISCONNECTED_STATE;
		}

		if (ptr->state != DISCONNECTED_STATE) // 연결 종료 상태가 아니라면
		{
			if (!tempOverlapped->IOTYPE) // IOTYPE에 따라서 RECV,SEND
			{
				int result = CompleteRecv(ptr, cbTransferred);
				switch (result)
				{
				case SOC_ERROR:
					continue;
				case SOC_FALSE:
					continue;
				case SOC_TRUE:
					break;
				}

				RecvPacketProcess(ptr);

				if (!Recv(ptr))
				{
					ptr->state = DISCONNECTED_STATE;
				}
			}
			else
			{
				int result = CompleteSend(ptr, cbTransferred);
				switch (result)
				{
				case SOC_ERROR:
					continue;
				case SOC_FALSE:
					continue;
				case SOC_TRUE:
					break;
				}
			}
		}

		if (ptr->state == DISCONNECTED_STATE)
		{
			// 유저 정보 삭제
			User_Exit(ptr);

			Remove_Client_Info(ptr);
		}
	}

	return 0;
}

void RecvPacketProcess(SOCKETINFO* _ptr) // recv 프로세스
{
	PROTOCOL protocol;

	GetProtocol(_ptr->recvbuf, protocol); // 프로토콜을 가져와서

	switch (_ptr->state) // 해당 소켓정보의 state에 따라서
	{
	case MENU_STATE: // 메뉴 선택 스테이트 
		switch (protocol)
		{
		case REGISTER: // 회원가입 프로토콜이라면 
			JoinProcess(_ptr); // 회원가입 프로세스 진행
			break;
		case LOGIN: // 로그인 프로토콜이라면 
			LoginProcess(_ptr); // 로그인 프로세스 진행
			break;
		}
		break;
	case WAIT_STATE: // 게임 시작 기다리는 스테이트
		switch (protocol) // 프로토콜에 따라
		{
		case GAME_ROOM_ENTER: // 게임 입장 프로토콜이라면 
			GameEnterProcess(_ptr); // 게임 입장 프로세스 진행
			break;
		}
		break;
	case GAME_STATE: // 게임 스테이트라면 
		switch (protocol)
		{
		case MY_UPDATE: // 내 정보 업데이트 프로토콜이라면 
			GameInfoUpdateProcess(_ptr); // 게임 정보 업데이트 프로세스 진행
			break;
		case GAME_OVER: // 게임 오버 프로토콜 이라면 
			Game_End_Process(_ptr); // 게임 종료 프로세스 진행
			break;
		}
		break;
	}
}

void JoinProcess(SOCKETINFO* _ptr) // 회원가입 프로세스
{
	RESULT join_result = NODATA;
	char msg[BUFSIZE];
	PROTOCOL protocol;
	int size;

	memset(&_ptr->userInfo, 0, sizeof(_User_Info));
	// 받아온 정보들을 유저의 아이디,비밀번호,닉네임에 언패킹해줌.
	UnPackPacket(_ptr->recvbuf, _ptr->userInfo.id, _ptr->userInfo.pw, _ptr->userInfo.nickname);

	// 아래에서 회원가입을 진행한 후
	for (int i = 0; i < join_Count; i++)
	{
		if (!strcmp(Join_List[i]->id, _ptr->userInfo.id)) 
		{
			join_result = ID_EXIST;
			strcpy(msg, ID_EXIST_MSG);
			break;
		}
	}

	if (join_result == NODATA) // 없었다면 아이디 생성 후 
	{
		_User_Info* user = new _User_Info;
		memset(user, 0, sizeof(_User_Info));
		strcpy(user->id, _ptr->userInfo.id);
		strcpy(user->pw, _ptr->userInfo.pw);
		strcpy(user->nickname, _ptr->userInfo.nickname);

		Join_List[join_Count++] = user;

		join_result = JOIN_SUCCESS;
		strcpy(msg, JOIN_SUCCESS_MSG);
	}

	protocol = REGISTER_RESULT;

	// 회원가입 결과를 패킹해서 
	size = PackPacket(_ptr->sendbuf, protocol, join_result, msg);

	_ptr->state = MENU_STATE; // 다시 메뉴스테이트로 변경해준 후 

	if (!Send(_ptr, size)) // send
	{
		_ptr->state = DISCONNECTED_STATE;
	}
}

void LoginProcess(SOCKETINFO* _ptr) // 로그인 프로세스
{
	RESULT login_result = NODATA;

	char msg[BUFSIZE];
	PROTOCOL protocol;
	int size;

	memset(&_ptr->userInfo, 0, sizeof(_User_Info));
	// 받아온 패킷을 유저의 아이디와 비밀번호에 언패킹해준 후 
	UnPackPacket(_ptr->recvbuf, _ptr->userInfo.id, _ptr->userInfo.pw);
	// 로그인 프로세스 진행 해준 후 
	for (int i = 0; i < join_Count; i++)
	{
		if (!strcmp(Join_List[i]->id, _ptr->userInfo.id))
		{
			if (!strcmp(Join_List[i]->pw, _ptr->userInfo.pw))
			{
				login_result = LOGIN_SUCCESS;
				strcpy(msg, LOGIN_SUCCESS_MSG);
				strcpy(_ptr->userInfo.nickname, Join_List[i]->nickname);
			}
			else
			{
				login_result = PW_ERROR;
				strcpy(msg, PW_ERROR_MSG);
			}
			break;
		}
	}

	if (login_result == NODATA)
	{
		login_result = ID_ERROR;
		strcpy(msg, ID_ERROR_MSG);
	}

	protocol = LOGIN_RESULT;
	// 로그인 결과를 패킹
	size = PackPacket(_ptr->sendbuf, protocol, login_result, msg);

	// 로그인 성공이라면 게임시작을 기다리는 스테이트로 변경 해준 후 
	if (login_result == LOGIN_SUCCESS)
		_ptr->state = STATE::WAIT_STATE;

	if (!Send(_ptr, size)) // 패킹한 패킷을 send
	{
		_ptr->state = DISCONNECTED_STATE;
	}
}

void GameEnterProcess(SOCKETINFO* _ptr) // 게임 입장 프로세스
{
	PROTOCOL protocol;
	RESULT result;
	int size1, size2;
	_Room_Info* room_ptr = Search_Room(_ptr); // 방을 받아오는 함수로 방 포인터에 방을 받아옴.

	for (int i = 0; i < 2; i++) // 받아온 방의 유저 빈자리를 검사해 빈자리에 유저를 넣어줌.
	{
		if (room_ptr->users[i] == nullptr)
		{
			room_ptr->users[i] = _ptr;
			if (i == 1) // 만약 빈자리가 배열의 1번째 자리 = 즉 2명이 가득찼다면 해당 방의 스테이트를
				// 게임 진행 스테이트로 변경해줌
			{
				room_ptr->state = ROOM_STATE::R_PLAYING_STATE;
			}
			break;
		}
	}

	// 방에 2명이 다 찬 경우
	if (room_ptr->state == ROOM_STATE::R_PLAYING_STATE) {
		protocol = PROTOCOL::GAME_START; // 게임 시작 프로토콜을 담아준 후

		// 유저 2명의 테트리스 정보 구조체 생성
		room_ptr->users[0]->tetris_Info = new Tetris_Info();
		room_ptr->users[1]->tetris_Info = new Tetris_Info();

		room_ptr->users[0]->tetris_Info->score = 0;
		room_ptr->users[0]->tetris_Info->bricknum = 0;
		room_ptr->users[1]->tetris_Info->score = 0;
		room_ptr->users[1]->tetris_Info->bricknum = 0;

		// 패킹
		PackPacket(room_ptr->users[0]->sendbuf, protocol, room_ptr->users[1]->userInfo.nickname,
			room_ptr->users[1]->tetris_Info, size1);
		PackPacket(room_ptr->users[1]->sendbuf, protocol, room_ptr->users[0]->userInfo.nickname,
			room_ptr->users[0]->tetris_Info, size2);

		// 해당 방의 유저들의 스테이트를 변경
		room_ptr->users[0]->state = STATE::GAME_STATE;
		room_ptr->users[1]->state = STATE::GAME_STATE;

		// 유저들에게 send
		if (!Send(room_ptr->users[0], size1))
		{
			room_ptr->users[0]->state = DISCONNECTED_STATE;
		}

		if (!Send(room_ptr->users[1], size2))
		{
			room_ptr->users[1]->state = DISCONNECTED_STATE;
		}
	}
}

void GameInfoUpdateProcess(SOCKETINFO* _ptr) // 게임 정보 업데이트 프로세스
{
	PROTOCOL protocol;
	int size, other_index;
	_Room_Info* room_ptr = new _Room_Info();

	for (int i = 0; i < room_count; i++) // 방들을 검사해 해당 유저가 접속해있는 방 정보를 포인터로 받아옴.
	{
		for (int k = 0; k < 2; k++)
		{
			if (RoomInfo[i]->users[k] == _ptr)
			{
				room_ptr = RoomInfo[i];

				if (k == 0) // 해당 유저 말고 다른유저의 배열 인덱스를 받아옴.
					other_index = 1;
				else
					other_index = 0;

				break;
			}
		}
	}

	protocol = PROTOCOL::ENEMY_UPDATE;

	// 받아온 패킷을 해당 유저의 테트리스 정보로 언패킹 해줌.
	UnPackPacket(_ptr->recvbuf, _ptr->tetris_Info);

	// 같은 방의 다른 유저에게 받아온 유저의 테트리스 정보를 패킹해준 후 
	PackPacket(room_ptr->users[other_index]->sendbuf, protocol, _ptr->tetris_Info, size);

	// 전송
	if (!Send(room_ptr->users[other_index], size))
	{
		room_ptr->users[other_index]->state = DISCONNECTED_STATE;
	}
	printf("%d 유저가 자기 보드 정보 전송 성공\n", other_index);
}

void Game_End_Process(SOCKETINFO* _ptr) // 게임 종료 프로세스
{
	PROTOCOL protocol;
	int size, other_index;
	_Room_Info* room_ptr = new _Room_Info(); 

	// 해당 유저가 들어가있던 방 정보를 받아옴
	for (int i = 0; i < room_count; i++)
	{
		for (int k = 0; k < 2; k++)
		{
			if (RoomInfo[i]->users[k] == _ptr)
			{
				room_ptr = RoomInfo[i];

				if (k == 0)
					other_index = 1;
				else
					other_index = 0;

				break;
			}
		}
	}

	protocol = PROTOCOL::GAME_OVER;

	// 해당 방의 다른 유저에게 게임 종료 프로토콜을 패킹해서
	size = PackPacket(room_ptr->users[other_index]->sendbuf, protocol);

	// 전송
	if (!Send(room_ptr->users[other_index], size))
	{
		room_ptr->users[other_index]->state = DISCONNECTED_STATE;
	}
}

_Room_Info* Search_Room(SOCKETINFO* _ptr) // 빈 방을 찾는 함수
{
	for (int i = 0; i < room_count; i++)
	{
		if (RoomInfo[i]->state == ROOM_STATE::R_WAIT_STATE) // 방중 게임 시작을 기다리는 방이 있다면 
		{
			return RoomInfo[i]; // 해당 방 정보를 return 
		}
	}

	return Create_Room(); // 빈 방이 없다면 방을 생성하는 함수를 호출해서 return
}

_Room_Info* Create_Room() // 방 생성 함수
{
	EnterCriticalSection(&cs); // 크리티컬 섹션 진입

	_Room_Info* ptr = new _Room_Info;
	ZeroMemory(ptr, sizeof(_Room_Info));

	ptr->state = ROOM_STATE::R_WAIT_STATE; // 방을 생성해서 state를 게임 시작을 기다리는 state로 해준 후

	memset(ptr->users, 0, sizeof(ptr->users));

	RoomInfo[room_count++] = ptr; // 해당 방을 방 리스트에 추가

	LeaveCriticalSection(&cs); // 크리티컬 섹션 탈출

	return ptr;
}

void User_Exit(SOCKETINFO* _ptr) // 유저 연결 종료시 호출 하는 함수
{
	int other_index, index = -1, size, retval;
	_Room_Info* room_ptr = new _Room_Info();

	for (int i = 0; i < room_count; i++) // 해당 유저가 접속해있던 방 정보를 받아옴
	{
		for (int k = 0; k < 2; k++)
		{
			if (RoomInfo[i]->users[k] == _ptr)
			{
				room_ptr = RoomInfo[i];

				index = k;
				if (k == 0)
					other_index = 1;
				else
					other_index = 0;
				break;
			}
		}
	}

	if (index == -1)
		return;

	if (room_ptr->state == ROOM_STATE::R_WAIT_STATE) // 방이 게임을 시작하기 전이었다면
	{
		// 해당 방의 유저 정보에서 해당 유저 정보를 nullptr로 변경해줌
		room_ptr->users[index] = nullptr;
		room_ptr->count = 0;
	}
	else
	{
		// 이미 게임이 시작해있었다면 상대 유저에게 해당 유저가 게임을 종료했다는 프로토콜을 패킹해서
		size = PackPacket(room_ptr->users[other_index]->sendbuf, PROTOCOL::ENEMY_EXIT);

		// send
		if (!Send(room_ptr->users[other_index], size))
		{
			room_ptr->users[other_index]->state = DISCONNECTED_STATE;
		}

		// 해당 방 스테이트를 변경해준 후 
		room_ptr->state = ROOM_STATE::R_WAIT_STATE;
		// 해당 방 삭제
		Remove_Room(room_ptr);

	}
}

void Remove_Room(_Room_Info* _ptr) // 방 삭제 함수
{
	EnterCriticalSection(&cs); // 크리티컬 섹션 진입

	for (int i = 0; i < room_count; i++) 
	{
		if (RoomInfo[i] == _ptr) // 해당 방 삭제 후 배열 당겨줌
		{
			delete RoomInfo[i];

			for (int j = i; j < room_count - 1; j++)
				RoomInfo[j] = RoomInfo[j + 1];
			break;
		}
	}
	room_count--; // 방 count 줄여줌

	LeaveCriticalSection(&cs); // 크리티컬 섹션 탈출
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 소켓 함수 오류 출력
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[오류] %s", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void GetProtocol(const char* _ptr, PROTOCOL& _protocol)
{
	memcpy(&_protocol, _ptr, sizeof(PROTOCOL));
}

int PackPacket(char* _buf, PROTOCOL _protocol, const char* _str1)
{
	char* ptr = _buf;
	int strsize1 = strlen(_str1);
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	size = size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	size = size + strsize1;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

int PackPacket(char* _buf, PROTOCOL _protocol)
{
	char* ptr = _buf;
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

int PackPacket(char* _buf, PROTOCOL _protocol, RESULT _result, const char* _str1)
{
	char* ptr = _buf;
	int strsize1 = strlen(_str1);
	int size = 0;

	ptr = ptr + sizeof(size);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	ptr = ptr + sizeof(_protocol);
	size = size + sizeof(_protocol);

	memcpy(ptr, &_result, sizeof(_result));
	ptr = ptr + sizeof(_result);
	size = size + sizeof(_result);

	memcpy(ptr, &strsize1, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);
	size = size + sizeof(strsize1);

	memcpy(ptr, _str1, strsize1);
	ptr = ptr + strsize1;
	size = size + strsize1;

	ptr = _buf;
	memcpy(ptr, &size, sizeof(size));

	size = size + sizeof(size);

	return size;
}

void PackPacket(char* _buf, PROTOCOL _protocol,
	char* _str, Tetris_Info* info, int& _size)
{
	int strsize = strlen(_str);
	char* ptr;
	_size = 0;
	ptr = _buf + sizeof(int);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	_size = _size + sizeof(_protocol);
	ptr = ptr + sizeof(_protocol);

	memcpy(ptr, &strsize, sizeof(strsize));
	ptr = ptr + sizeof(strsize);
	_size = _size + sizeof(strsize);

	memcpy(ptr, _str, strsize);
	ptr = ptr + strsize;
	_size = _size + strsize;

	memcpy(ptr, &info->brick, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->nbrick, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->nx, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->ny, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->rot, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->score, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->bricknum, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	ptr = _buf;
	memcpy(ptr, &_size, sizeof(int));

	_size = _size + sizeof(_size);
}

void PackPacket(char* _buf, PROTOCOL _protocol, Tetris_Info* info, int& _size)
{
	char* ptr;
	_size = 0;
	ptr = _buf + sizeof(int);

	memcpy(ptr, &_protocol, sizeof(_protocol));
	_size = _size + sizeof(_protocol);
	ptr = ptr + sizeof(_protocol);

	memcpy(ptr, &info->brick, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->nbrick, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->nx, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->ny, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->rot, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->score, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, &info->bricknum, sizeof(int));
	_size = _size + sizeof(int);
	ptr = ptr + sizeof(int);

	ptr = _buf;
	memcpy(ptr, &_size, sizeof(int));

	_size = _size + sizeof(_size);
}

void UnPackPacket(const char* _buf, char* _str1, char* _str2, char* _str3)
{
	int str1size, str2size, str3size;

	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&str1size, ptr, sizeof(str1size));
	ptr = ptr + sizeof(str1size);

	memcpy(_str1, ptr, str1size);
	ptr = ptr + str1size;

	memcpy(&str2size, ptr, sizeof(str2size));
	ptr = ptr + sizeof(str2size);

	memcpy(_str2, ptr, str2size);
	ptr = ptr + str2size;

	memcpy(&str3size, ptr, sizeof(str3size));
	ptr = ptr + sizeof(str3size);

	memcpy(_str3, ptr, str3size);
	ptr = ptr + str3size;
}

void UnPackPacket(const char* _buf, char* _str1, char* _str2)
{
	int str1size, str2size;

	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&str1size, ptr, sizeof(str1size));
	ptr = ptr + sizeof(str1size);

	memcpy(_str1, ptr, str1size);
	ptr = ptr + str1size;

	memcpy(&str2size, ptr, sizeof(str2size));
	ptr = ptr + sizeof(str2size);

	memcpy(_str2, ptr, str2size);
	ptr = ptr + str2size;
}

void UnPackPacket(const char* _buf, char* _str1)
{
	int str1size, str2size;

	const char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&str1size, ptr, sizeof(str1size));
	ptr = ptr + sizeof(str1size);

	memcpy(_str1, ptr, str1size);
	ptr = ptr + str1size;
}

void UnPackPacket(const char* _buf, int& _data)
{
	const char* ptr = _buf + sizeof(PROTOCOL);
	memcpy(&_data, ptr, sizeof(_data));
	ptr = ptr + sizeof(_data);
}

void UnPackPacket(const char* buf, Tetris_Info* Enemy_info)
{
	const char* ptr = buf;
	ptr = ptr + sizeof(enum PROTOCOL);

	memcpy(&Enemy_info->brick, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(&Enemy_info->nbrick, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(&Enemy_info->nx, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(&Enemy_info->ny, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(&Enemy_info->rot, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(&Enemy_info->score, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(&Enemy_info->bricknum, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

}
