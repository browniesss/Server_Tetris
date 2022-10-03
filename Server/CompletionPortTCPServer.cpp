/*
18032064 ������ ���� 3B
��Ʈ���� ����
*/

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

// �޼�����
#define INTRO_MSG "��밡 �����Ҷ����� ��ٷ��ּ���\n"
#define ID_ERROR_MSG "���� ���̵��Դϴ�\n"
#define PW_ERROR_MSG "�н����尡 Ʋ�Ƚ��ϴ�.\n"
#define LOGIN_SUCCESS_MSG "�α��ο� �����߽��ϴ�.�� �ο�(2��)�� �������� ������ �����մϴ�.\n"
#define ID_EXIST_MSG "�̹� �ִ� ���̵� �Դϴ�.\n"
#define JOIN_SUCCESS_MSG "���Կ� �����߽��ϴ�.\n"

#define SERVERPORT 9000
#define BUFSIZE 4096
#define IDSIZE 255
#define PWSIZE 255
#define NICKNAMESIZE 255
#define ERROR_DISCONNECTED -2
#define DISCONNECTED -1


enum STATE // Ŭ���̾�Ʈ�� ������Ʈ
{
	NO_STATE = -1,
	MENU_STATE, // �޴� ���� ������Ʈ
	WAIT_STATE, // ���� ���� ��ٸ��� ������Ʈ
	GAME_STATE, // ���� ���� ���� ������Ʈ 
	END_STATE, // ���� ���� ������Ʈ 
	DISCONNECTED_STATE // ���� ���� ������Ʈ
};

enum ROOM_STATE // ���� �� ������Ʈ
{
	R_WAIT_STATE = 1, // ���� ��ٸ��� ������Ʈ 
	R_PLAYING_STATE, // ���� �������� ������Ʈ
};

enum RESULT // ���
{
	NODATA = -1,
	ID_EXIST = 1, // ���̵� ���� 
	ID_ERROR, // ���̵� ����
	PW_ERROR, // �н����� ����
	JOIN_SUCCESS, // ���� ���� 
	LOGIN_SUCCESS, // �α��� ����
};

enum PROTOCOL // ��������
{
	NO_PROTOCOL = -1,
	REGISTER = 1, // ȸ������ �������� 
	LOGIN, // �α��� �������� 
	REGISTER_RESULT, // ȸ������ ��� ��������
	LOGIN_RESULT, // �α��� ��� ��������
	GAME_ROOM_ENTER, // ���� �� ���� �������� 
	GAME_START, // ���� ���� �������� 
	MY_UPDATE, // �� ��Ʈ���� ���� ������Ʈ �������� 
	ENEMY_UPDATE, // ��� ��Ʈ���� ���� ������Ʈ ��������
	GAME_OVER, // ���� ���� ��������
	ENEMY_EXIT, // ���� ���� ��������
};

enum // ����
{
	SOC_ERROR = 1,
	SOC_TRUE,
	SOC_FALSE
};

struct Tetris_Info // ��Ʈ���� ���� 
{
	int board[12][22]; // ��Ʈ���� ����
	int brick, nbrick; // ��Ʈ���� ���� ����
	int nx, ny; // ���� �̵����� ��Ʈ���� ��ǥ 
	int rot;		// ȸ��
	int score;		// ����
	int bricknum;	// ���� ����
};

struct _User_Info // ���� ����.
{
	char id[IDSIZE];
	char pw[PWSIZE];
	char nickname[NICKNAMESIZE];
};

struct EXOVERLAPPED // Ȯ�� ����ü
{
	WSAOVERLAPPED overlapped;
	bool IOTYPE; // false = recv, true = send
	void* client_info;
};

// ���� ���� ������ ���� ����ü
struct SOCKETINFO
{
	// Ȯ�� ����ü
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

struct _Room_Info // �� ���� ����ü
{
	SOCKETINFO* users[2]; // �濡 ���� ���� ���� ������ 
	ROOM_STATE state; // ���� ���� ����
	int count = 0; // ���� count
};

// ȸ������ ����Ʈ
_User_Info* Join_List[100];
int join_Count = 0;

SOCKETINFO* user[100]; // ���� ���� ����Ʈ 
int usercount = 0;

// �� ����Ʈ
_Room_Info* RoomInfo[50];
int room_count = 0;

CRITICAL_SECTION cs; // ũ��Ƽ�� ����

int gamecount = 0;

// �۾��� ������ �Լ�
DWORD WINAPI WorkerThread(LPVOID arg);
// ���� ��� �Լ�
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

// Ŭ���̾�Ʈ �ʱ�ȭ �� ���� �־��ֱ�
SOCKETINFO* AddClientInfo(SOCKET _sock, SOCKADDR_IN _addr)
{
	EnterCriticalSection(&cs); // ũ��Ƽ�� ���� ����
	SOCKETINFO* ptr = new SOCKETINFO;

	memset(ptr, 0, sizeof(SOCKETINFO));

	ptr->sock = _sock;
	memcpy(&ptr->addr, &_addr, sizeof(SOCKADDR_IN));
	ptr->state = MENU_STATE;

	ptr->r_sizeflag = false;

	ptr->recvEX.IOTYPE = false;
	ptr->sendEX.IOTYPE = true;
	// SOCKETINFO ����ü �����ͷ� �־���.
	ptr->recvEX.client_info = (SOCKETINFO*)ptr;
	ptr->sendEX.client_info = (SOCKETINFO*)ptr;

	user[usercount++] = ptr;

	printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		inet_ntoa(ptr->addr.sin_addr), ntohs(ptr->addr.sin_port));

	LeaveCriticalSection(&cs); // ũ��Ƽ�� ���� Ż��

	return ptr;
}

void Remove_Client_Info(SOCKETINFO* _ptr) // Ŭ���̾�Ʈ ���� �����
{
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		inet_ntoa(_ptr->addr.sin_addr), ntohs(_ptr->addr.sin_port));

	EnterCriticalSection(&cs); // ũ��Ƽ�� ���� ����
	for (int i = 0; i < usercount; i++)
	{
		if (user[i] == _ptr)
		{
			closesocket(user[i]->sock);

			delete user[i];

			for (int j = i; j < usercount - 1; j++) // Ŭ���̾�Ʈ ���� ����� �����.
			{
				user[j] = user[j + 1];
			}

			break;
		}
	}

	usercount--;
	LeaveCriticalSection(&cs); // ũ��Ƽ�� ���� Ż��
}

int main(int argc, char* argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	InitializeCriticalSection(&cs);

	// ����� �Ϸ� ��Ʈ ����
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) return 1;

	// CPU ���� Ȯ��
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// (CPU ���� * 2)���� �۾��� ������ ����
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

	// ������ ��ſ� ����� ����
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

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		// ���� ���� ����ü �Ҵ�
		SOCKETINFO* ptr = AddClientInfo(client_sock, clientaddr);

		if (!Recv(ptr)) // ������ recv
		{
			continue;
		}
	}

	// ���� ����
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

// �۾��� ������ �Լ�
DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1) {
		// �񵿱� ����� �Ϸ� ��ٸ���
		DWORD cbTransferred;
		SOCKET client_sock;
		EXOVERLAPPED* tempOverlapped;

		SOCKETINFO* ptr;

		retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
			(LPDWORD)&client_sock, (LPOVERLAPPED*)&tempOverlapped, INFINITE);

		ptr = (SOCKETINFO*)tempOverlapped->client_info;

		// Ŭ���̾�Ʈ ���� ���
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);


		// �񵿱� ����� ��� Ȯ��
		if (retval == 0 || cbTransferred == 0)
		{
			if (retval == 0)
			{ // ���� , cbtransferred �� 0�� ������ �����.
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &tempOverlapped->overlapped,
					&temp1, FALSE, &temp2); // ��ü���� �����ڵ��� �߻��������� �Լ� ȣ��
				err_display("WSAGetOverlappedResult()");
			}

			ptr->state = DISCONNECTED_STATE;
		}

		if (ptr->state != DISCONNECTED_STATE) // ���� ���� ���°� �ƴ϶��
		{
			if (!tempOverlapped->IOTYPE) // IOTYPE�� ���� RECV,SEND
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
			// ���� ���� ����
			User_Exit(ptr);

			Remove_Client_Info(ptr);
		}
	}

	return 0;
}

void RecvPacketProcess(SOCKETINFO* _ptr) // recv ���μ���
{
	PROTOCOL protocol;

	GetProtocol(_ptr->recvbuf, protocol); // ���������� �����ͼ�

	switch (_ptr->state) // �ش� ���������� state�� ����
	{
	case MENU_STATE: // �޴� ���� ������Ʈ 
		switch (protocol)
		{
		case REGISTER: // ȸ������ ���������̶�� 
			JoinProcess(_ptr); // ȸ������ ���μ��� ����
			break;
		case LOGIN: // �α��� ���������̶�� 
			LoginProcess(_ptr); // �α��� ���μ��� ����
			break;
		}
		break;
	case WAIT_STATE: // ���� ���� ��ٸ��� ������Ʈ
		switch (protocol) // �������ݿ� ����
		{
		case GAME_ROOM_ENTER: // ���� ���� ���������̶�� 
			GameEnterProcess(_ptr); // ���� ���� ���μ��� ����
			break;
		}
		break;
	case GAME_STATE: // ���� ������Ʈ��� 
		switch (protocol)
		{
		case MY_UPDATE: // �� ���� ������Ʈ ���������̶�� 
			GameInfoUpdateProcess(_ptr); // ���� ���� ������Ʈ ���μ��� ����
			break;
		case GAME_OVER: // ���� ���� �������� �̶�� 
			Game_End_Process(_ptr); // ���� ���� ���μ��� ����
			break;
		}
		break;
	}
}

void JoinProcess(SOCKETINFO* _ptr) // ȸ������ ���μ���
{
	RESULT join_result = NODATA;
	char msg[BUFSIZE];
	PROTOCOL protocol;
	int size;

	memset(&_ptr->userInfo, 0, sizeof(_User_Info));
	// �޾ƿ� �������� ������ ���̵�,��й�ȣ,�г��ӿ� ����ŷ����.
	UnPackPacket(_ptr->recvbuf, _ptr->userInfo.id, _ptr->userInfo.pw, _ptr->userInfo.nickname);

	// �Ʒ����� ȸ�������� ������ ��
	for (int i = 0; i < join_Count; i++)
	{
		if (!strcmp(Join_List[i]->id, _ptr->userInfo.id)) 
		{
			join_result = ID_EXIST;
			strcpy(msg, ID_EXIST_MSG);
			break;
		}
	}

	if (join_result == NODATA) // �����ٸ� ���̵� ���� �� 
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

	// ȸ������ ����� ��ŷ�ؼ� 
	size = PackPacket(_ptr->sendbuf, protocol, join_result, msg);

	_ptr->state = MENU_STATE; // �ٽ� �޴�������Ʈ�� �������� �� 

	if (!Send(_ptr, size)) // send
	{
		_ptr->state = DISCONNECTED_STATE;
	}
}

void LoginProcess(SOCKETINFO* _ptr) // �α��� ���μ���
{
	RESULT login_result = NODATA;

	char msg[BUFSIZE];
	PROTOCOL protocol;
	int size;

	memset(&_ptr->userInfo, 0, sizeof(_User_Info));
	// �޾ƿ� ��Ŷ�� ������ ���̵�� ��й�ȣ�� ����ŷ���� �� 
	UnPackPacket(_ptr->recvbuf, _ptr->userInfo.id, _ptr->userInfo.pw);
	// �α��� ���μ��� ���� ���� �� 
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
	// �α��� ����� ��ŷ
	size = PackPacket(_ptr->sendbuf, protocol, login_result, msg);

	// �α��� �����̶�� ���ӽ����� ��ٸ��� ������Ʈ�� ���� ���� �� 
	if (login_result == LOGIN_SUCCESS)
		_ptr->state = STATE::WAIT_STATE;

	if (!Send(_ptr, size)) // ��ŷ�� ��Ŷ�� send
	{
		_ptr->state = DISCONNECTED_STATE;
	}
}

void GameEnterProcess(SOCKETINFO* _ptr) // ���� ���� ���μ���
{
	PROTOCOL protocol;
	RESULT result;
	int size1, size2;
	_Room_Info* room_ptr = Search_Room(_ptr); // ���� �޾ƿ��� �Լ��� �� �����Ϳ� ���� �޾ƿ�.

	for (int i = 0; i < 2; i++) // �޾ƿ� ���� ���� ���ڸ��� �˻��� ���ڸ��� ������ �־���.
	{
		if (room_ptr->users[i] == nullptr)
		{
			room_ptr->users[i] = _ptr;
			if (i == 1) // ���� ���ڸ��� �迭�� 1��° �ڸ� = �� 2���� ����á�ٸ� �ش� ���� ������Ʈ��
				// ���� ���� ������Ʈ�� ��������
			{
				room_ptr->state = ROOM_STATE::R_PLAYING_STATE;
			}
			break;
		}
	}

	// �濡 2���� �� �� ���
	if (room_ptr->state == ROOM_STATE::R_PLAYING_STATE) {
		protocol = PROTOCOL::GAME_START; // ���� ���� ���������� ����� ��

		// ���� 2���� ��Ʈ���� ���� ����ü ����
		room_ptr->users[0]->tetris_Info = new Tetris_Info();
		room_ptr->users[1]->tetris_Info = new Tetris_Info();

		room_ptr->users[0]->tetris_Info->score = 0;
		room_ptr->users[0]->tetris_Info->bricknum = 0;
		room_ptr->users[1]->tetris_Info->score = 0;
		room_ptr->users[1]->tetris_Info->bricknum = 0;

		// ��ŷ
		PackPacket(room_ptr->users[0]->sendbuf, protocol, room_ptr->users[1]->userInfo.nickname,
			room_ptr->users[1]->tetris_Info, size1);
		PackPacket(room_ptr->users[1]->sendbuf, protocol, room_ptr->users[0]->userInfo.nickname,
			room_ptr->users[0]->tetris_Info, size2);

		// �ش� ���� �������� ������Ʈ�� ����
		room_ptr->users[0]->state = STATE::GAME_STATE;
		room_ptr->users[1]->state = STATE::GAME_STATE;

		// �����鿡�� send
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

void GameInfoUpdateProcess(SOCKETINFO* _ptr) // ���� ���� ������Ʈ ���μ���
{
	PROTOCOL protocol;
	int size, other_index;
	_Room_Info* room_ptr = new _Room_Info();

	for (int i = 0; i < room_count; i++) // ����� �˻��� �ش� ������ �������ִ� �� ������ �����ͷ� �޾ƿ�.
	{
		for (int k = 0; k < 2; k++)
		{
			if (RoomInfo[i]->users[k] == _ptr)
			{
				room_ptr = RoomInfo[i];

				if (k == 0) // �ش� ���� ���� �ٸ������� �迭 �ε����� �޾ƿ�.
					other_index = 1;
				else
					other_index = 0;

				break;
			}
		}
	}

	protocol = PROTOCOL::ENEMY_UPDATE;

	// �޾ƿ� ��Ŷ�� �ش� ������ ��Ʈ���� ������ ����ŷ ����.
	UnPackPacket(_ptr->recvbuf, _ptr->tetris_Info);

	// ���� ���� �ٸ� �������� �޾ƿ� ������ ��Ʈ���� ������ ��ŷ���� �� 
	PackPacket(room_ptr->users[other_index]->sendbuf, protocol, _ptr->tetris_Info, size);

	// ����
	if (!Send(room_ptr->users[other_index], size))
	{
		room_ptr->users[other_index]->state = DISCONNECTED_STATE;
	}
	printf("%d ������ �ڱ� ���� ���� ���� ����\n", other_index);
}

void Game_End_Process(SOCKETINFO* _ptr) // ���� ���� ���μ���
{
	PROTOCOL protocol;
	int size, other_index;
	_Room_Info* room_ptr = new _Room_Info(); 

	// �ش� ������ ���ִ� �� ������ �޾ƿ�
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

	// �ش� ���� �ٸ� �������� ���� ���� ���������� ��ŷ�ؼ�
	size = PackPacket(room_ptr->users[other_index]->sendbuf, protocol);

	// ����
	if (!Send(room_ptr->users[other_index], size))
	{
		room_ptr->users[other_index]->state = DISCONNECTED_STATE;
	}
}

_Room_Info* Search_Room(SOCKETINFO* _ptr) // �� ���� ã�� �Լ�
{
	for (int i = 0; i < room_count; i++)
	{
		if (RoomInfo[i]->state == ROOM_STATE::R_WAIT_STATE) // ���� ���� ������ ��ٸ��� ���� �ִٸ� 
		{
			return RoomInfo[i]; // �ش� �� ������ return 
		}
	}

	return Create_Room(); // �� ���� ���ٸ� ���� �����ϴ� �Լ��� ȣ���ؼ� return
}

_Room_Info* Create_Room() // �� ���� �Լ�
{
	EnterCriticalSection(&cs); // ũ��Ƽ�� ���� ����

	_Room_Info* ptr = new _Room_Info;
	ZeroMemory(ptr, sizeof(_Room_Info));

	ptr->state = ROOM_STATE::R_WAIT_STATE; // ���� �����ؼ� state�� ���� ������ ��ٸ��� state�� ���� ��

	memset(ptr->users, 0, sizeof(ptr->users));

	RoomInfo[room_count++] = ptr; // �ش� ���� �� ����Ʈ�� �߰�

	LeaveCriticalSection(&cs); // ũ��Ƽ�� ���� Ż��

	return ptr;
}

void User_Exit(SOCKETINFO* _ptr) // ���� ���� ����� ȣ�� �ϴ� �Լ�
{
	int other_index, index = -1, size, retval;
	_Room_Info* room_ptr = new _Room_Info();

	for (int i = 0; i < room_count; i++) // �ش� ������ �������ִ� �� ������ �޾ƿ�
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

	if (room_ptr->state == ROOM_STATE::R_WAIT_STATE) // ���� ������ �����ϱ� ���̾��ٸ�
	{
		// �ش� ���� ���� �������� �ش� ���� ������ nullptr�� ��������
		room_ptr->users[index] = nullptr;
		room_ptr->count = 0;
	}
	else
	{
		// �̹� ������ �������־��ٸ� ��� �������� �ش� ������ ������ �����ߴٴ� ���������� ��ŷ�ؼ�
		size = PackPacket(room_ptr->users[other_index]->sendbuf, PROTOCOL::ENEMY_EXIT);

		// send
		if (!Send(room_ptr->users[other_index], size))
		{
			room_ptr->users[other_index]->state = DISCONNECTED_STATE;
		}

		// �ش� �� ������Ʈ�� �������� �� 
		room_ptr->state = ROOM_STATE::R_WAIT_STATE;
		// �ش� �� ����
		Remove_Room(room_ptr);

	}
}

void Remove_Room(_Room_Info* _ptr) // �� ���� �Լ�
{
	EnterCriticalSection(&cs); // ũ��Ƽ�� ���� ����

	for (int i = 0; i < room_count; i++) 
	{
		if (RoomInfo[i] == _ptr) // �ش� �� ���� �� �迭 �����
		{
			delete RoomInfo[i];

			for (int j = i; j < room_count - 1; j++)
				RoomInfo[j] = RoomInfo[j + 1];
			break;
		}
	}
	room_count--; // �� count �ٿ���

	LeaveCriticalSection(&cs); // ũ��Ƽ�� ���� Ż��
}

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ���� �Լ� ���� ���
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[����] %s", (char*)lpMsgBuf);
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
