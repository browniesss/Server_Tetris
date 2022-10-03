/*
18032064 이정우 게플 3B
테트리스 클라이언트
회원가입 후 로그인 시 남아있는 방을 찾아서 방에 입장합니다.
방에 입장한 유저가 2명이 될 경우 게임을 시작합니다.
오른쪽에 상대방이 플레이하는 화면이 나오며 기존의 테트리스룰로 게임 오버 혹은 종료시 상대에게 게임종료 혹은
게임 오버 메세지가 가서 게임이 종료됩니다.
*/

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    4096
#define SIZE		16

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PauseChildProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ClientMain(LPVOID arg);
DWORD WINAPI TetrisInfoRecvThread(LPVOID arg);
DWORD WINAPI TetrisInfoSendThread(LPVOID arg);
HINSTANCE g_hInst;
HWND hWndMain;
LPCTSTR lpszClass = TEXT("Tetris4");
HANDLE hClientThread, RecvThread, SendThread;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance
	, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = 0;
	RegisterClass(&WndClass);

	WndClass.lpfnWndProc = PauseChildProc;
	WndClass.lpszClassName = "PauseChild";
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_SAVEBITS;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	DWORD ThreadId;
	hClientThread = CreateThread(NULL, 0, ClientMain, NULL, 0, &ThreadId);

	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	while (GetMessage(&Message, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &Message)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}
	return (int)Message.wParam;
}



enum PROTOCOL;
enum RESULT;

struct Tetris_Info;
// 함수 원형
void err_quit(char* msg);
void err_display(char* msg);
int recvn(SOCKET s, char* buf, int len, int flags);
void DrawScreen(HDC hdc);
void EnemyDrawScreen(HDC hdc);
void MakeNewBrick();
int GetAround(int x, int y, int b, int r, Tetris_Info* info);
BOOL MoveDown(Tetris_Info* info);
void TestFull(Tetris_Info* info);
void PrintTile(HDC hdc, int x, int y, int c);
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit);
void PlayEffectSound(UINT Sound);
void AdjustMainWindow();
void UpdateBoard();
void Update_Send();
void GameStart();
BOOL IsMovingBrick(int x, int y, Tetris_Info* info);
void Board_Check(Tetris_Info* info);
BOOL CALLBACK DlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProc2(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

// 패킹, 언패킹
void GetProtocol(char* _ptr, PROTOCOL& _protocol);
bool PacketRecv(SOCKET _sock, char* _buf);
int PackPacket(enum PROTOCOL protocol, struct _User_Info info, char* buf);
void PackPacket(char* _buf, PROTOCOL _protocol, Tetris_Info* info, int& _size);
void UnPackPacket(char* _buf, RESULT& _result, char* _str1);
void UnPackPacket(const char* buf, char* EnemyNickName, Tetris_Info* Enemy_info);
void UnPackPacket(const char* buf, Tetris_Info* Enemy_info);

// 매크로 및 전역 변수들
#define random(n) (rand()%n)
#define TS 24
struct Point {
	int x, y;
};

Point Shape[][4][4] = {
	{ {0,0,1,0,2,0,-1,0}, {0,0,0,1,0,-1,0,-2}, {0,0,1,0,2,0,-1,0}, {0,0,0,1,0,-1,0,-2} },
	{ {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1}, {0,0,1,0,0,1,1,1} },
	{ {0,0,-1,0,0,-1,1,-1}, {0,0,0,1,-1,0,-1,-1}, {0,0,-1,0,0,-1,1,-1}, {0,0,0,1,-1,0,-1,-1} },
	{ {0,0,-1,-1,0,-1,1,0}, {0,0,-1,0,-1,1,0,-1}, {0,0,-1,-1,0,-1,1,0}, {0,0,-1,0,-1,1,0,-1} },
	{ {0,0,-1,0,1,0,-1,-1}, {0,0,0,-1,0,1,-1,1}, {0,0,-1,0,1,0,1,1}, {0,0,0,-1,0,1,1,-1} },
	{ {0,0,1,0,-1,0,1,-1}, {0,0,0,1,0,-1,-1,-1}, {0,0,1,0,-1,0,-1,1}, {0,0,0,-1,0,1,1,1} },
	{ {0,0,-1,0,1,0,0,1}, {0,0,0,-1,0,1,1,0}, {0,0,-1,0,1,0,0,-1}, {0,0,-1,0,0,-1,0,1} },
	{ {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0} },
	{ {0,0,0,0,0,-1,1,0},{0,0,0,0,-1,0,0,-1},{0,0,0,0,0,1,-1,0},{0,0,0,0,0,1,1,0} },
};

struct Tetris_Info // 테트리스 정보
{
	int board[12][22]; // 테트리스 보드
	int brick, nbrick; // 테트리스 벽돌 정보
	int nx, ny;		 // 현재   이동중인 테트리스 좌표
	int rot;       // 회전
	int score;	   // 점수
	int bricknum;  // 벽돌 개수
};

struct _User_Info // 유저 정보 구조체
{
	char id[SIZE];
	char pw[SIZE];
	char nickname[SIZE];
	PROTOCOL protocol;
	SOCKET sock;
}info;

enum STATE { // 스테이트 
	MENU_SELECT_STATE = 1, // 메뉴 선택 스테이트
	GAME_WAIT_STATE, // 게임 입장 전 스테이트 
	GAME_PLAYING_STATE, // 게임 플레이 스테이트 
	DISCONNECTED_STATE // 연결 종료 스테이트
}state;

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

enum RESULT // 결과
{
	NODATA = -1,
	ID_EXIST = 1, // 아이디 존재
	ID_ERROR, // 아이디 에러 
	PW_ERROR, // 패스워드 에러 
	JOIN_SUCCESS, // 회원가입 성공 
	LOGIN_SUCCESS, // 로그인 성공
};


char msg[BUFSIZE + 1];

enum { EMPTY, BRICK, WALL = sizeof(Shape) / sizeof(Shape[0]) + 1 };
int arBW[] = { 8,10,12,15,20 };
int arBH[] = { 15,20,25,30,32 };
int BW = 10;
int BH = 20;
enum tag_Status { GAMEOVER, RUNNING, PAUSE };
tag_Status GameStatus;
int Interval;
HBITMAP hBit[11];
BOOL bShowSpace = TRUE;
BOOL bQuiet = FALSE;
HWND hPauseChild;
TCHAR enemy_Name[255];
Tetris_Info* MyBoardInfo = new Tetris_Info(); // 내 테트리스 정보
Tetris_Info* EnemyBoardInfo = new Tetris_Info(); // 상대 테트리스 정보
bool spacedown = false;

DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	info.sock = sock;

	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE + 1];
	char msg[BUFSIZE + 1];
	int len;
	int size;

	DWORD ThreadId2, ThreadId3, ThreadId;
	RESULT result;

	int count = 0;

	state = MENU_SELECT_STATE; // 스테이트를 메뉴 선택 스테이트로 변경

	PROTOCOL protocol;

	// 서버와 데이터 통신
	while (1)
	{
		memset(buf, 0, sizeof(buf));
		memset(msg, 0, sizeof(msg));

		if (GameStatus != RUNNING) // 게임 진행중이 아닐때만 이 쓰레드로 패킷을 받아옴
		{
			if (!PacketRecv(sock, buf))
			{
				break;
			}

			GetProtocol(buf, protocol);
		}

		switch (state) // 스테이트에 따라 진행 
		{
		case STATE::MENU_SELECT_STATE: // 메뉴 선택 스테이트라면 
		{
			switch (protocol)
			{
			case PROTOCOL::REGISTER_RESULT: // 회원가입 결과 프로토콜이라면 
				UnPackPacket(buf, result, msg); // 언패킹 후 
				MessageBox(hWndMain, msg, "회원가입 결과", MB_OK);
				break;
			case PROTOCOL::LOGIN_RESULT: // 로그인 결과 프로토콜이라면 
				UnPackPacket(buf, result, msg); // 언패킹 후 
				if (result == RESULT::LOGIN_SUCCESS) // 결과에 따라서 진행 
				{
					MessageBox(hWndMain, msg, "로그인 결과", MB_OK);
					state = GAME_WAIT_STATE;

					info.protocol = PROTOCOL::GAME_ROOM_ENTER; // 로그인 성공이라면 게임 방에 입장한다는
					// 프로토콜로 변경 후 
					// 패킹해서 send
					size = PackPacket(info.protocol, info, buf);
					retval = send(info.sock, buf, size, 0);
					if (retval == SOCKET_ERROR) {
						err_quit("send()");
						break;
					}
				}
				else
				{
					MessageBox(hWndMain, msg, "로그인 결과", MB_OK);
				}
				break;
			}
		}
		break;
		case STATE::GAME_WAIT_STATE: // 게임 시작 기다리는 스테이트라면 
		{
			switch (protocol)
			{
			case PROTOCOL::GAME_START: // 게임 스타트 프로토콜이라면 
				// 상대 닉네임과, 테트리스 정보를 언패킹 후 
				UnPackPacket(buf, enemy_Name, EnemyBoardInfo);
				GameStart(); // 게임을 시작하는 함수 호출 후 
				Update_Send(); // 내 정보 전송 함수 호출


				// recv와 send 담당 쓰레드 생성
				RecvThread = CreateThread(NULL, 0, TetrisInfoRecvThread, (LPVOID)sock, 0, &ThreadId2);
				SendThread = CreateThread(NULL, 0, TetrisInfoSendThread, (LPVOID)sock, 0, &ThreadId3);
				break;
			}
		}
		break;
		case STATE::GAME_PLAYING_STATE: // 게임 진행중인 스테이트 라면 
		{
			switch (protocol)
			{
			case PROTOCOL::ENEMY_UPDATE: // 상대 정보 업데이트 프로토콜이라면 
				UnPackPacket(buf, EnemyBoardInfo); // 상대 테트리스 정보를 언패킹 후 
				UpdateBoard(); // 보드 업데이트
				break;
			}
		}
		break;
		}
	}
	// closesocket()
	closesocket(sock);
	// 윈속 종료
	WSACleanup();
	return 0;
}

DWORD WINAPI TetrisInfoSendThread(LPVOID arg) // send 쓰레드
{
	SOCKET sock = (SOCKET)arg;
	int size;
	int retval;
	char buf[BUFSIZE + 1];

	while (1) {
		switch (info.protocol) // 프로토콜에 따라 
		{
		case PROTOCOL::MY_UPDATE: // 내 정보 업데이트 프로토콜이라면 
			memset(buf, 0, sizeof(buf));

			// 내 정보 업데이트 프로토콜과 내 테트리스 정보를 패킹 후 send
			PackPacket(buf, PROTOCOL::MY_UPDATE, MyBoardInfo, size);
			retval = send(sock, buf, size, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				break;
			}
			 // 프로토콜 초기화
			info.protocol = PROTOCOL::NO_PROTOCOL;
		}
	}
}

DWORD WINAPI TetrisInfoRecvThread(LPVOID arg) // recv 쓰레드
{
	SOCKET sock = (SOCKET)arg;

	PROTOCOL protocol;
	int result;
	char buf[BUFSIZE + 1];

	while (1)
	{
		if (!PacketRecv(sock, buf)) // 패킷을 받아와서 
		{
			break;
		}

		GetProtocol(buf, protocol);

		switch (protocol) // 프로토콜에 따라서 
		{
		case PROTOCOL::ENEMY_UPDATE: // 상대 정보 업데이트 프로토콜이라면 
			UpdateBoard();
			// 상대 테트리스 정보 언패킹 후 
			UnPackPacket(buf, EnemyBoardInfo);
			// 상대 테트리스 보드 체크해줌.
			Board_Check(EnemyBoardInfo);

			InvalidateRect(hWndMain, NULL, FALSE);
			break;
		case PROTOCOL::GAME_OVER: // 게임 오버 프로토콜이라면 
			KillTimer(hWndMain, 1); // 타이머 끈 뒤 
			GameStatus = GAMEOVER; // 게임 정보를 게임오버로 변경

			MessageBox(hWndMain, "승리했습니다. 확인 버튼 누를 시 종료됩니다.", "알림", MB_OK);
			break;
		case PROTOCOL::ENEMY_EXIT: // 상대 게임 종료 프로토콜이라면
			KillTimer(hWndMain, 1); // 타이머 끈 뒤 
			GameStatus = GAMEOVER; // 게임 정보를 게임오버로 변경

			MessageBox(hWndMain, "상대방이 게임에서 퇴장했습니다. 게임을 종료합니다.", "알림", MB_OK);
			break;
		}
	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	char buf[BUFSIZE + 1];
	int i;
	int size, retval;
	int trot;
	HDC hdc;
	PAINTSTRUCT ps;
	int x, y;

	switch (iMessage) {
	case WM_CREATE:
		hWndMain = hWnd;
		hPauseChild = CreateWindow("PauseChild", NULL, WS_CHILD | WS_BORDER,
			0, 0, 0, 0, hWnd, (HMENU)0, g_hInst, NULL);
		AdjustMainWindow();
		GameStatus = GAMEOVER;
		srand(GetTickCount());

		for (i = 0; i < 11; i++) {
			hBit[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1 + i)); // 벽돌 비트맵 로딩
		}
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDM_REGISTER: // 회원가입 대화상자 
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgProc);
			break;
		case IDM_LOGIN: // 로그인 대화상자
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), NULL, (DLGPROC)DlgProc2);
			break;
		case IDM_GAME_EXIT:
			DestroyWindow(hWnd);
			break;
		}
		return 0;
	case WM_INITMENU:
		CheckMenuItem((HMENU)wParam, IDM_GAME_VIEWSPACE,
			MF_BYCOMMAND | (bShowSpace ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem((HMENU)wParam, IDM_GAME_QUIET,
			MF_BYCOMMAND | (bQuiet ? MF_CHECKED : MF_UNCHECKED));
		return 0;
	case WM_TIMER:
		switch (wParam)
		{
		case 1:
			if (GameStatus == RUNNING) {
				if (MoveDown(MyBoardInfo) == TRUE) { // 타이머 호출 시 마다 
					MakeNewBrick();
				}
				info.protocol = PROTOCOL::MY_UPDATE; // 내 정보를 상대에게 업데이트 시켜주기 위해 프로토콜
				// 변경
			}
			else {
				if (IsWindowVisible(hPauseChild)) {
					ShowWindow(hPauseChild, SW_HIDE);
				}
				else {
					ShowWindow(hPauseChild, SW_SHOW);
				}
			}
			break;
		}
		return 0;
	case WM_KEYDOWN:
		if (GameStatus != RUNNING || MyBoardInfo->brick == -1)
			return 0;
		switch (wParam) { // 방향키와 스페이스로 내려가는 벽돌 조작과 변경되는 벽돌 정보들 상대에게 보내주기 위해 프로토콜도 변경
		case VK_LEFT:
			UpdateBoard();
			if (GetAround(MyBoardInfo->nx - 1, MyBoardInfo->ny, MyBoardInfo->brick, MyBoardInfo->rot, MyBoardInfo) == EMPTY) {
				/*if ((lParam & 0x40000000) == 0) {
					PlayEffectSound(IDR_WAVE4);
				}*/
				MyBoardInfo->nx--;
				InvalidateRect(hWnd, NULL, FALSE);
				info.protocol = PROTOCOL::MY_UPDATE;
			}
			break;
		case VK_RIGHT:
			UpdateBoard();
			if (GetAround(MyBoardInfo->nx + 1, MyBoardInfo->ny, MyBoardInfo->brick, MyBoardInfo->rot, MyBoardInfo) == EMPTY) {
				/*if ((lParam & 0x40000000) == 0) {
					PlayEffectSound(IDR_WAVE4);
				}*/
				MyBoardInfo->nx++;
				InvalidateRect(hWnd, NULL, FALSE);
				info.protocol = PROTOCOL::MY_UPDATE;
			}
			break;
		case VK_UP:
			UpdateBoard();
			trot = (MyBoardInfo->rot == 3 ? 0 : MyBoardInfo->rot + 1);
			if (GetAround(MyBoardInfo->nx, MyBoardInfo->ny, MyBoardInfo->brick, trot, MyBoardInfo) == EMPTY) {
				//PlayEffectSound(IDR_WAVE1);
				MyBoardInfo->rot = trot;
				InvalidateRect(hWnd, NULL, FALSE);
				info.protocol = PROTOCOL::MY_UPDATE;
			}
			break;
		case VK_DOWN:
			if (MoveDown(MyBoardInfo) == TRUE) {
				MakeNewBrick();
			}
			InvalidateRect(hWnd, NULL, FALSE);
			info.protocol = PROTOCOL::MY_UPDATE;
			break;
		case VK_SPACE:
			spacedown = true;
			//PlayEffectSound(IDR_WAVE3);
			while (MoveDown(MyBoardInfo) == FALSE)
			{
				;
			}
			MakeNewBrick();
			InvalidateRect(hWnd, NULL, FALSE);
			//info.protocol = PROTOCOL::MY_UPDATE;
			break;
		}
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawScreen(hdc);
		EnemyDrawScreen(hdc);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		KillTimer(hWndMain, 1);
		CloseHandle(hClientThread);
		CloseHandle(RecvThread);
		CloseHandle(SendThread);

		WSACleanup();
		delete MyBoardInfo;
		delete EnemyBoardInfo;

		for (i = 0; i < 11; i++) {
			DeleteObject(hBit[i]);
		}
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

// 회원가입 창
BOOL CALLBACK DlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	char buf[BUFSIZE + 1];
	int len;
	int size;

	int retval;

	switch (iMessage)
	{

	case WM_INITDIALOG:/*
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
		hEdit3 = GetDlgItem(hDlg, IDC_EDIT3);*/
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT_REGISTER_ID, info.id, SIZE + 1);
			GetDlgItemText(hDlg, IDC_EDIT_REGISTER_PW, info.pw, SIZE + 1);
			GetDlgItemText(hDlg, IDC_EDIT_REGISTER_NICKNAME, info.nickname, SIZE + 1);

			info.protocol = PROTOCOL::REGISTER;

			size = PackPacket(info.protocol, info, buf);
			retval = send(info.sock, buf, size, 0);
			if (retval == SOCKET_ERROR) {
				err_quit("send()");
				break;
			}
			EndDialog(hDlg, IDOK);
			break;
		}
		return TRUE;
	case WM_ACTIVATE:
		return TRUE;
	}

	return FALSE;
}

// 로그인창
BOOL CALLBACK DlgProc2(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	// 로그인 창
	char buf[BUFSIZE + 1];
	int len;
	int size;

	int retval;

	switch (iMessage)
	{

	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT_LOGIN_ID, info.id, SIZE + 1);
			GetDlgItemText(hDlg, IDC_EDIT_LOGIN_PW, info.pw, SIZE + 1);
			info.protocol = PROTOCOL::LOGIN;

			size = PackPacket(info.protocol, info, buf);
			retval = send(info.sock, buf, size, 0);
			if (retval == SOCKET_ERROR) {
				err_quit("send()");
				break;
			}
			EndDialog(hDlg, IDOK);
			break;
		}
		return TRUE;

	case WM_ACTIVATE:
		return TRUE;
	}

	return FALSE;
}

void EnemyDrawScreen(HDC hdc) // 상대 화면 그리는 함수
{
	int x, y, i;
	int OTHERBW = BW + 25;
	TCHAR str[128];

	// 테두리 그림
	for (x = 0; x < BW + 1; x++) {
		PrintTile(hdc, x + OTHERBW, 0, WALL);
		PrintTile(hdc, x + OTHERBW, BH + 1, WALL);
	}
	for (y = 0; y < BH + 2; y++) {
		PrintTile(hdc, OTHERBW - 9, y, WALL);
		PrintTile(hdc, OTHERBW - 10, y, WALL);
		PrintTile(hdc, OTHERBW - 11, y, WALL);

		PrintTile(hdc, OTHERBW, y, WALL);
		PrintTile(hdc, (BW + 1) + OTHERBW, y, WALL);
	}

	// 게임판과 이동중인 벽돌 그림
	for (x = 1; x < BW + 1; x++) {
		for (y = 1; y < BH + 1; y++) {
			if (IsMovingBrick(x, y, EnemyBoardInfo)) {
				PrintTile(hdc, x + OTHERBW, y, EnemyBoardInfo->brick + 1);
			}
			else {
				PrintTile(hdc, x + OTHERBW, y, EnemyBoardInfo->board[x][y]); // 점으로 초기화 해놓고 블럭이 채워지면
				// 블럭이 그려짐.
			}
		}
	}

	// 정보 출력
	wsprintf(str, "상대 닉네임 : %s   ", enemy_Name);
	TextOut(hdc, (BW + 3 + OTHERBW / 2) * TS, 30, str, lstrlen(str));
	wsprintf(str, "점수 : %d   ", EnemyBoardInfo->score);
	TextOut(hdc, (BW + 3 + OTHERBW / 2) * TS, 60, str, lstrlen(str));
	wsprintf(str, "벽돌 : %d 개   ", EnemyBoardInfo->bricknum);
	TextOut(hdc, (BW + 3 + OTHERBW / 2) * TS, 80, str, lstrlen(str));
}

void DrawScreen(HDC hdc) // 내 화면 그리는 함수 
{
	int x, y, i;
	TCHAR str[128];

	// 테두리 그림
	for (x = 0; x < BW + 1; x++) {
		PrintTile(hdc, x, 0, WALL);
		PrintTile(hdc, x, BH + 1, WALL);
	}
	for (y = 0; y < BH + 2; y++) {
		PrintTile(hdc, 0, y, WALL);
		PrintTile(hdc, BW + 1, y, WALL);
	}

	// 게임판과 이동중인 벽돌 그림
	for (x = 1; x < BW + 1; x++) {
		for (y = 1; y < BH + 1; y++) {
			if (IsMovingBrick(x, y, MyBoardInfo)) {
				PrintTile(hdc, x, y, MyBoardInfo->brick + 1);
			}
			else {
				PrintTile(hdc, x, y, MyBoardInfo->board[x][y]);
			}
		}
	}

	// 다음 벽돌 그림
	for (x = BW + 3; x <= BW + 11; x++) {
		for (y = BH - 5; y <= BH + 1; y++) {
			if (x == BW + 3 || x == BW + 11 || y == BH - 5 || y == BH + 1) {
				PrintTile(hdc, x, y, WALL);
			}
			else {
				PrintTile(hdc, x, y, 0);
			}
		}
	}

	// 다음 나올 벽돌
	if (GameStatus != GAMEOVER) {
		for (i = 0; i < 4; i++) {
			PrintTile(hdc, BW + 7 + Shape[MyBoardInfo->nbrick][0][i].x, BH - 2 + Shape[MyBoardInfo->nbrick][0][i].y, MyBoardInfo->nbrick + 1);
		}
	}

	// 정보 출력
	wsprintf(str, "닉네임 : %s   ", info.nickname);
	TextOut(hdc, (BW + 4) * TS, 30, str, lstrlen(str));
	wsprintf(str, "점수 : %d   ", MyBoardInfo->score);
	TextOut(hdc, (BW + 4) * TS, 60, str, lstrlen(str));
	wsprintf(str, "벽돌 : %d 개   ", MyBoardInfo->bricknum);
	TextOut(hdc, (BW + 4) * TS, 80, str, lstrlen(str));
}

void MakeNewBrick() // 새로운 벽돌 만드는 함수
{
	char buf[BUFSIZE];
	int size, retval;

	MyBoardInfo->bricknum++;
	MyBoardInfo->brick = MyBoardInfo->nbrick;
	MyBoardInfo->nbrick = random(sizeof(Shape) / sizeof(Shape[0]));
	MyBoardInfo->nx = BW / 2;
	MyBoardInfo->ny = 3;
	MyBoardInfo->rot = 0;

	InvalidateRect(hWndMain, NULL, FALSE);
	if (GetAround(MyBoardInfo->nx, MyBoardInfo->ny, MyBoardInfo->brick, MyBoardInfo->rot, MyBoardInfo) != EMPTY) { // 꽉차서 못나올때 
		KillTimer(hWndMain, 1);
		GameStatus = GAMEOVER;
		// 게임 오버시 상대에게 게임 오버를 보내기 위해 게임오버 프로토콜과 내 보드 정보를 패킹해서 서버에게 전송
		PackPacket(buf, PROTOCOL::GAME_OVER, MyBoardInfo, size);
		retval = send(info.sock, buf, size, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
		}

		MessageBox(hWndMain, "패배했습니다. 확인 버튼 누를 시 종료됩니다.", "알림", MB_OK);
	}
}

int GetAround(int x, int y, int b, int r, Tetris_Info* info) // 돌려져 있는 상태를 받아서 주변 검사하는 함수
{
	int i, k = EMPTY;

	for (i = 0; i < 4; i++) {
		k = max(k, info->board[x + Shape[b][r][i].x][y + Shape[b][r][i].y]);
	}
	return k;
}

BOOL MoveDown(Tetris_Info* t_info)
{
	if (GetAround(t_info->nx, t_info->ny + 1, t_info->brick, t_info->rot, t_info) != EMPTY) {
		if (spacedown) // 스페이스바를 눌렀을 경우
		{
			InvalidateRect(hWndMain, NULL, FALSE);
			info.protocol = PROTOCOL::MY_UPDATE;
			UpdateBoard();
			UpdateWindow(hWndMain);
			spacedown = !spacedown;
		}
		TestFull(t_info);
		return TRUE;
	}
	t_info->ny++;
	// 즉시 그려서 벽돌이 내려가는 모양을 보여 준다.
	UpdateBoard();
	UpdateWindow(hWndMain);
	return FALSE;
}

void GameStart() // 게임 시작 함수
{
	int x, y;
	if (GameStatus != GAMEOVER) {
		return;
	}
	for (x = 0; x < BW + 2; x++) {
		for (y = 0; y < BH + 2; y++) {
			MyBoardInfo->board[x][y] = (y == 0 || y == BH + 1 || x == 0 || x == BW + 1) ? WALL : EMPTY;
			EnemyBoardInfo->board[x][y] = (y == 0 || y == BH + 1 || x == 0 || x == BW + 1) ? WALL : EMPTY;
		}
	}
	MyBoardInfo->score = 0;
	MyBoardInfo->bricknum = 0;
	GameStatus = RUNNING;
	MyBoardInfo->nbrick = random(sizeof(Shape) / sizeof(Shape[0]));
	MakeNewBrick();
	Interval = 1000;
	SetTimer(hWndMain, 1, Interval, NULL);

	state = STATE::GAME_PLAYING_STATE; // 스테이트를 게임 플레이 중인 스테이트로 변경
}

void TestFull(Tetris_Info* info) // 한줄 체크
{
	int i, x, y, ty;
	int count = 0;
	static int arScoreInc[] = { 0,1,3,8,20 };

	for (i = 0; i < 4; i++) { // 바닥에 안착시키는 역할
		info->board[info->nx + Shape[info->brick][info->rot][i].x][info->ny + Shape[info->brick][info->rot][i].y] = info->brick + 1;
	}
	// 이동중인 벽돌이 잠시 없는 상태. 
	info->brick = -1;

	for (y = 1; y < BH + 1; y++) { // 한줄 완성 검사
		for (x = 1; x < BW + 1; x++) { // y값 한줄 검사 후 
			if (info->board[x][y] == EMPTY) break;
		}
		if (x == BW + 1) { // 전부 빈칸이 아니라면
			//PlayEffectSound(IDR_WAVE2);
			count++;
			for (ty = y; ty > 1; ty--) {  //  한칸씩 당김
				for (x = 1; x < BW + 1; x++) {
					info->board[x][ty] = info->board[x][ty - 1];
				}
			}
			UpdateBoard();
			UpdateWindow(hWndMain);
			//Sleep(150);
		}
	}
	info->score += arScoreInc[count];

	if (info->bricknum % 10 == 0 && Interval > 200) {
		Interval -= 50;
		SetTimer(hWndMain, 1, Interval, NULL);
	}
}

void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit)
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx, by;
	BITMAP bit;

	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);

	GetObject(hBit, sizeof(BITMAP), &bit);
	bx = bit.bmWidth;
	by = bit.bmHeight;

	BitBlt(hdc, x, y, bx, by, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}

void PrintTile(HDC hdc, int x, int y, int c)
{
	DrawBitmap(hdc, x * TS, y * TS, hBit[c]);
	if (c == EMPTY && bShowSpace)
		Rectangle(hdc, x * TS + TS / 2 - 1, y * TS + TS / 2 - 1, x * TS + TS / 2 + 1, y * TS + TS / 2 + 1);
	return;
}

void PlayEffectSound(UINT Sound)
{
	if (!bQuiet) {
		PlaySound(MAKEINTRESOURCE(Sound), g_hInst, SND_RESOURCE | SND_ASYNC);
	}
}

void AdjustMainWindow()
{
	RECT crt;

	SetRect(&crt, 0, 0, (BW + 12) * TS, (BH + 2) * TS);
	AdjustWindowRect(&crt, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);
	SetWindowPos(hWndMain, NULL, 0, 0, 600 + crt.right - crt.left, crt.bottom - crt.top,
		SWP_NOMOVE | SWP_NOZORDER);
	SetWindowPos(hPauseChild, NULL, (BW + 12) * TS / 2 - 100,
		(BH + 2) * TS / 2 - 50, 200, 100, SWP_NOZORDER);
}

LRESULT CALLBACK PauseChildProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT crt;
	TEXTMETRIC tm;

	switch (iMessage) {
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &crt);
		SetTextAlign(hdc, TA_CENTER);
		GetTextMetrics(hdc, &tm);
		TextOut(hdc, crt.right / 2, crt.bottom / 2 - tm.tmHeight / 2, "PAUSE", 5);
		EndPaint(hWnd, &ps);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void UpdateBoard()
{
	RECT rt;
	RECT allrt;

	GetClientRect(hWndMain, &allrt);

	//SetRect(&rt, TS, TS, (BW + 1) * TS, (BH + 1) * TS);
	InvalidateRect(hWndMain, &allrt, FALSE);
}

void Update_Send() // 내 정보 업데이트 패킷을 보내는 함수
{
	int size;
	int retval;
	char buf[BUFSIZE + 1];

	info.protocol = PROTOCOL::MY_UPDATE;

	PackPacket(buf, info.protocol, MyBoardInfo, size);
	retval = send(info.sock, buf, size, 0);
	if (retval == SOCKET_ERROR) {
		err_quit("send()");
	}
}

void Board_Check(Tetris_Info* info)
{
	if (GetAround(info->nx, info->ny + 1, info->brick, info->rot, info) != EMPTY) {
		TestFull(info);
	}
	InvalidateRect(hWndMain, NULL, FALSE);
	UpdateWindow(hWndMain);
}

BOOL IsMovingBrick(int x, int y, Tetris_Info* info)
{
	int i;

	if (GameStatus == GAMEOVER || info->brick == -1) {
		return FALSE;
	}

	for (i = 0; i < 4; i++) {
		if (x == info->nx + Shape[info->brick][info->rot][i].x &&
			y == info->ny + Shape[info->brick][info->rot][i].y)
			return TRUE;
	}
	return FALSE;
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

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

bool PacketRecv(SOCKET _sock, char* _buf)
{
	int size;

	int retval = recvn(_sock, (char*)&size, sizeof(size), 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv error()");
		return false;
	}
	else if (retval == 0)
	{
		return false;
	}

	retval = recvn(_sock, _buf, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("recv error()");
		return false;

	}
	else if (retval == 0)
	{
		return false;
	}

	return true;
}

int PackPacket(enum PROTOCOL protocol, _User_Info info, char* buf)
{
	int idsize = strlen(info.id);
	int pwsize = strlen(info.pw);
	int nicksize = strlen(info.nickname);

	char* ptr;
	int size = 0;
	ptr = buf + sizeof(int);							

	memcpy(ptr, &protocol, sizeof(enum PROTOCOL));		
	size = size + sizeof(enum PROTOCOL);				
	ptr = ptr + sizeof(enum PROTOCOL);					

	memcpy(ptr, &idsize, sizeof(int));					
	size = size + sizeof(int);							
	ptr = ptr + sizeof(int);							

	memcpy(ptr, info.id, idsize);						
	size = size + idsize;								
	ptr = ptr + idsize;									

														
	memcpy(ptr, &pwsize, sizeof(int));
	size = size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, info.pw, pwsize);
	size = size + pwsize;
	ptr = ptr + pwsize;


	memcpy(ptr, &nicksize, sizeof(int));
	size = size + sizeof(int);
	ptr = ptr + sizeof(int);

	memcpy(ptr, info.nickname, nicksize);
	size = size + nicksize;
	ptr = ptr + nicksize;

	ptr = buf;
	memcpy(ptr, &size, sizeof(int));

	return size + sizeof(int);							
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

void GetProtocol(char* _ptr, PROTOCOL& _protocol)
{
	memcpy(&_protocol, _ptr, sizeof(PROTOCOL));
}

void UnPackPacket(char* _buf, RESULT& _result, char* _str1)
{
	int strsize1;

	char* ptr = _buf + sizeof(PROTOCOL);

	memcpy(&_result, ptr, sizeof(_result));
	ptr = ptr + sizeof(_result);

	memcpy(&strsize1, ptr, sizeof(strsize1));
	ptr = ptr + sizeof(strsize1);

	memcpy(_str1, ptr, strsize1);
	ptr = ptr + strsize1;
}

void UnPackPacket(const char* buf, char* EnemyNickName, Tetris_Info* Enemy_info)
{
	int strsize;
	const char* ptr = buf;
	ptr = ptr + sizeof(enum PROTOCOL);

	memcpy(&strsize, ptr, sizeof(int));
	ptr = ptr + sizeof(int);

	memcpy(EnemyNickName, ptr, strsize);
	ptr = ptr + strsize;

	EnemyNickName[strsize] = '\0';

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
