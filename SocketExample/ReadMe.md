Select
--------------

#### 동작원리

소켓 함수 호출이 성공할 수 있는 시점을 미리 알 수 있다.  
따라서 소켓 함수 호출 시 조건이 만족되지 않아 생기는 문제를 해결할 수 있다.
	
1. 소켓 set 3개를 준비해야 한다.  
소켓 set은 소켓 디스크립터의 집합을 의미하며 호출할 함수의 종류에 따라 소켓을 적당한 셋에 넣어두어야 한다.  
예를들면 어떤 소켓에 대해 recv()함수를 호출하고 싶다면 읽기 set에 넣고, send()함수를 호출하고 싶다면 쓰기 set에 넣는 방식이다.

2. 소켓 set 3개를 인자로 select()함수를 호출하면 select()함수는 소켓 set에 포함된 소켓이 입출력을 위한 준비가 될 때까지 대기한다.  
적어도 한 소켓이 준비되면 select() 함수는 리턴한다.  
이때 소켓 set에는 입출력이 가능한 소켓만 남고 나머지는 모두 제거된다.

#### 3가지 소켓 set

1. 읽기 set
	- 함수 호출 시점 - 접속한 클라이언트가 있으므로 accept() 함수를 호출 할 수 있다.  
	소켓 수신 버퍼에 도착한 데이터가 있으므로 recv(), recvfrom()등의 함수를 호출해 읽을 수 있다.  
	TCP 연결이 종료되었으므로 recv(), recvfrom()등의 함수를 호출해 연결 종료를 감지할 수 있다.

2. 쓰기 set
	- 함수 호출 시점 - 소켓 송신 버퍼의 여유공간이 충분하므로 send(), sendto()등의 함수를 호출해 데이터를 보낼 수 있다.
	
	- 함수 호출 결과 - 넌블로킹 소켓을 사용한 connect() 함수 호출이 성공했다.

3. 예외 set
	- 함수 호출 시점 - OOB(Out-Of-Band) 데이터가 도착했으므로 recv(), recvfrom()등의 함수를 호출하여 OOB데이터를 받을 수 있다.  
	(OOB 데이터는 send()함수 마지막 인자에 MSG_OOB옵션을 사용하여 보내는 특별한 데이터)
	
	- 함수 호출 결과 - 넌블로킹 소켓을 사용한 connect() 함수 호출이 실패했다.

```
int select(
	int nfds,					- 리눅스/유닉스와의 호환성을 위해 존재
	fd_set *readfds,				- 읽기 set
	fd_set *writefds,				- 쓰기 set
	fd_set *exceptfds,				- 예외 set
	const struct timeval *timeout			- '초'와 마이크로초 단위로 타임아웃을 지정, 이 시간이 지나면 select() 함수는 무조건 리턴
);(return 성공 - 조건을 만족하는 소켓 개수 또는 0(타임아웃), 실패 - SOCKE_ERROR)

typedef struct timeval {
	long tv_sec,	
	long tv_usec
}
```
#### 타임 아웃 값에 따른 select() 함수 동작

	NULL	- 적어도 한 소켓이 조건을 만족할 때까지 무한정 대기, 리턴 값은 조건을 만족하는 소켓 개수
	{0, 0}	- 소켓 set에 포함된 모든 소켓을 검사 후 곧바로 리턴, 리턴 값은 조건을 만족하는 소켓 개수 또는 0(타임아웃)
	양수	- 적어도 한 소켓이 조건을 만족하거나 타임아웃으로 지정한 시간이 지나면 리턴, 리턴 값은 조건을 만족하는 소켓 개수 또는 0(타임아웃)

#### select()함수를 통한 소켓 입출력 절차

1. 소켓 set을 초기화

2. 소켓 set에 소켓을 넣는다. 넣을 수 있는 소켓의 최대 개수는 FD_SETSIZE(64)로 정의되어 있다.

3. select()함수 호출. timeout이 NULL이면 select() 함수는 조건 만족하는 소켓이 있을때까지 
무한정 대기

4. select()함수가 리턴하면 소켓 set에 남아있는 모든 소켓에 대해 적절한 소켓 함수를 호출하여 
처리

5. 1 ~ 4를 반복

#### 소켓 set을 위한 매크로
```
FD_ZERO(fd_set *set)			- set을 초기화
FD_SET(SOCKET s, fd_set *set)		- set에 소켓 s를 넣는다.
FD_CLR(SOCKET s, fd_set *set)		- set에서 소켓 s를 제거한다.
FD_ISSET(SOCKET s, fd_set *set)		- 소켓 s가 set에 들어있으면 0이 아닌 값을 리턴한다. 그렇지 않으면 0을 리턴한다.
```
<hr>

WSAAsyncSelect
--------------
#### 동작원리

1. WSAAsyncSelect()함수를 호출하여 소켓 이벤트를 알려줄 윈도우 메시지와 관심있는 네트워크 이벤트를 등록한다.  
예를들면 소켓으로 데이터를 받을 수 있는 상황이 되면 (WM_USER+1)로 정의된 윈도우 메시지로 알려달라고 등록한다.

2. 등록한 네트워크 이벤트가 발생하면 윈도우 메시지가 발생하여 윈도우 프로시저가 호출된다.

3. 윈도우 프로시저에서는 받은 메시지의 종류에 따라 적절한 소켓 함수를 호출하여 처리한다.

```
int WSAAsynceSelect(
	SOCKET s,			- 네트워크를 처리하고자 하는 소켓
	HWND hWnd,			- 네트워크 이벤트가 발생하면 메시지를 받을 윈도우의 핸들
	unsigned int wMsg,		- 네트워크 이벤트가 발생하면 윈도우가 받을 메시지, 소켓을 위한 윈도우 메시지는 따로 정의되어 있지 않으므로 WM_USER+x(임의숫자) 형태의 사용자 정의 메시지를 이용하면 된다.
	long lEvent			- 관심있는 네트워크 이벤트를 비트마스크 조합으로 나타낸다.
);(return 성공 - 조건을 만족하는 소켓 개수 또는 0(타임아웃), 실패 - SOCKE_ERROR)
```
#### 네트워크 이벤트를 나타내는 상수
```
FD_ACCEPT	- 접속한 클라이언트가 있다.		(대응 함수 : accept())
FD_READ		- 데이터 수신이 가능하다.			(recv(), recvfrom())
FD_WRITE	- 데이터 송신이 가능하다.			(send(), sendto())
FD_CLOSE	- 상대가 접속을 종료했다.
FD_CONNECT	- 통신을 위한 연결 절차가 끝났다.
FD_OOB		- OOB 데이터가 도착했다.			(recv(), recvfrom())
```
#### 네트워크 이벤트 발생시 윈도우 프로시저에 전달되는 내용
```
LRESULT CALLBACK WndProc(HWND hWnd, UNIT uMsg, WPARAM wParam, LPARAM lParam) {...}

hWnd		- 메시지가 발생한 윈도우의 핸들
uMsg		- WSAAsyncSelect() 함수 호출시 등록했던 사용자 정의 메시지
wParam		- 네트워크 이벤트가 발생한 소켓. SOCKET으로 형변환하여 소켓 함수 호출에 그대로 사용
lParam		- 하위 16비트는 발생한 네트워크 이벤트, 상위 16비트는 오류 코드를 담고 있다. 항상 오류 코드를 먼저 확인 후 네트워크 이벤트를 처리해야 한다. 각각을 위한 정의 된 매크로가 있다. (WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam))
```
#### 유의점
1. WSAAsyncSelect()함수 호출시 해당 소켓은 자동으로 넌블로킹 모드로 전환된다.  
블로킹 소켓은 윈도우 메시지 루프를 정지시킬 가능성이 있기때문이다.

2. accept()함수가 리턴하는 소켓은 연결 대기 소켓과 동일한 속성을 갖게된다.  
연결 대기 소켓은 직접 데이터 송수신을 하지 않으므로 FD_READ, FD_WRITE 이벤트를 처리하지 않는다.  
반면 accept()함수가 리턴하는 소켓은 FD_READ, FD_WRITE 이벤트를 처리해야하므로 다시 WSAAsyncSelect()함수를 호출하여 관심있는 이벤트를 등록해야한다.

3. 윈도우 메시지에 대응하여 소켓 함수를 호출하면 대부분 성공하나 WSAEWOULDBLOCK 오류 코드가 발생하는 경우가 드물게 있다.

4. 윈도우 메시지를 받았을때 적절한 소켓 함수를 호출하지 않으면, 다음번에 같은 윈도우 메시지가 발생하지 않는다.  
예를들어 FD_READ 이벤트에 대응하여 recv()함수를 호출하지 않으면 동일 소켓에 대한 FD_READ 이벤트는 다시 발생하지 않는다.  
따라서 윈도우 메시지가 발생하면 대응하는 함수를 호출해야 하며 그렇지 않을 경우 응용 프로그램이 나중에 직접 메시지를 발생시켜야 한다.  
(직접 메시지를 발생시킨다는 것은 PostMessage() API 함수를 사용해 자신의 윈도우 메시지 큐에 직접 메시지를 넣는다는 뜻)

5. Select 모델은 최대 FD_SETSIZE(64)개의 소켓을 처리할 수 있었으나 WSAAsyncSelect 모델에는 개수제한이 없다.
<hr>

WSAEventSelect
--------------
#### 동작원리
소켓과 관련된 네트워크 이벤트를 이벤트 객체를 통해 감지한다.  (소켓당 이벤트 객체를 하나씩 생성한다.)  
소켓 함수 호출이 성공할 수 있는 시점을 이벤트 객체를 통해 알 수 있다.  
네트워크 이벤트가 발생할 때마다 이벤트 객체가 신호 상태가 된다.  
따라서 이벤트 객체의 상태 변화를 관찰함으로써 네트워크 이벤트 발생을 감지할 수 있다.  
그러나 이것만으로는 구체적으로 어떤 종류의 이벤트가 발생했는지 혹은 어떤 오류가 발생했는지 알 수 없다는 문제가 있다.

#### WSAEventSelect모델을 통한 소켓 입출력 절차
1. 소켓을 생성할 때마다 WSACreateEvent() 함수를 이용해 이벤트 객체를 생성한다.

2. WSAEventSelect() 함수를 이용해 소켓과 이벤트 객체를 짝지음과 동시에 처리할 네트워크 이벤트를 등록한다. 예를들면 소켓을 통해 데이터를 받을 수 있는 상황이 되면 이벤트 객체를 신호상태로 변경하라는 내용을 등록한다.

3. WSAWaitForMultipleEvents() 함수를 호출해 이벤트 객체가 신호 상태가 되기를 기다린다. 등록한 네트워크 이벤트가 발생하면 해당 소켓과 연관된 이벤트 객체가 신호 상태가 된다.

4. WSAEnumNetworkEvents() 함수를 호출해 발생한 네트워크 이벤트를 알아내고, 적절한 소켓함수를 호출해 처리한다.

#### WSAEventSelect 모델의 필요 기능 및 관련 함수
	이벤트 객체 생성 및 제거			- WSACreateEvent(), WSACloseEvent()
	소켓과 이벤트 객체 짝짓기			- WSAEventSelect()
	이벤트 객체의 신호 상태 감지하기		- WSAWaitForMultipleEvents()
	구체적인 네트워크 이벤트 알아내기		- WSAEnumNetworkEvents()

##### 이벤트 객체 생성 및 제거

	WSAEVENT WSACreateEvent() (return 성공 - 이벤트 객체 핸들, 실패 - WSA_INVALID_EVENT)
	BOOL WSACloseEvent(WSAEVENT hEvent)	(return 성공 - TRUE, 실패 - FALSE)

##### 소켓과 이벤트 객체 짝짓기
	int WSAEventSelect(
		SOCKET s,			- 네트워크 이벤트를 처리하고자 하는 소켓
		WSAEVENT hEventObject,		- 소켓과 연관시킬 이벤트 객체의 핸들
		long lNetworkEvents		- 관심 있는 네트워크 이벤트를 비트 마스크 조합으로 나타낸다. (WSAAsyncSelect 모델과 동일)
	);

##### 네트워크 이벤트를 나타내는 상수
	FD_ACCEPT	- 접속한 클라이언트가 있다.		(대응 함수 : accept())
	FD_READ		- 데이터 수신이 가능하다.			(recv(), recvfrom())
	FD_WRITE	- 데이터 송신이 가능하다.			(send(), sendto())
	FD_CLOSE	- 상대가 접속을 종료했다.
	FD_CONNECT	- 통신을 위한 연결 절차가 끝났다.
	FD_OOB		- OOB 데이터가 도착했다.			(recv(), recvfrom())

#### WSAEventSelect() 함수 사용시 유의점
1. WSAEventSelect() 함수 호출시 해당 소켓은 자동으로 넌블로킹 모드로 전환된다.

2. accept()함수가 리턴하는 소켓은 연결 대기 소켓과 동일한 속성을 갖게된다. 연결 대기 소켓은 직접 데이터 송수신을 하지 않으므로 FD_READ, FD_WRITE 이벤트를 처리하지 않는다. 반면 accept()함수가 리턴하는 소켓은 FD_READ, FD_WRITE 이벤트를 처리해야하므로 다시 WSAEventSelect()함수를 호출하여 관심있는 이벤트를 등록해야한다.

3. 네트워크 이벤트에 대응하여 소켓 함수를 호출하면 대부분 성공하나 WSAEWOULDBLOCK 오류 코드가 발생하는 경우가 드물게 있다.

4. 네트워크 이벤트 발생시 적절한 소켓 함수를 호출하지 않으면, 다음번에 같은 네트워크 이벤트가 발생하지 않는다. 예를들어 FD_READ 이벤트에 대응하여 recv()함수를 호출하지 않으면 동일 소켓에 대한 FD_READ 이벤트는 다시 발생하지 않는다. 따라서 네트워크 이벤트가 발생하면 대응하는 함수를 호출해야 하며 그렇지 않을 경우 응용 프로그램이 네트워크 이벤트 발생 사실을 기록해두고 나중에 대응 함수를 호출해야한다. 

##### 이벤트 객체의 신호 상태 감지하기
```
DWORD WSAWaitForMultipleEvents(			- WSAWaitForMultipleEvents() 함수 사용시 이베느 객체 핸들을 모두 배열에 넣어 전달해야한다			
	DWORD cEvents,				- 배열 원소 개수(최댓값은 WSA_MAXIMUM_WAIT_EVENTS(64))
	const WSAEVENT *lphEvents,		- 배열의 시작 주소
	BOOL fWaitAll,				- TRUE면 모든 이벤트 객체가 신호 상태가 될 때까지 기다린다. FALSE면 이벤트 객체 중 하나가 신호 상태가 되는 즉시 리턴한다.
	DWORD dwTimeout,			- 대기 시간으로 밀리초 단위를 사용한다. 네트워크 이벤트가 발생하지 않아도 설정한 시간이 지나면 리턴한다. 대기 시간으로 WSA_INFINITE를 사용하면 조건이 만족될 때까지 무한히 대기한다.
	BOOL fAlertable				- 입출력 완료 루틴(I/0 completion routine)과 관련된 부분, WSAEventSelect 모델에서는 사용하지 않으므로 항상 FALSE를 전달한다.
);	(return 성공 - WSA_WAIT_EVENT_0 ~ WSA_WAIT_EVENT_0 + cEvents - 1 또는 WSA_WAIT_TIMEOUT, 실패 - WSA_WAIT_FAILED)
```
##### 구체적인 네트워크 이벤트 알아내기
```
int WSAEnumNetworkEvents(
	SOCKET s,						- 대상 소켓
	WSAEVENT hEventObject,					- 대상 소켓 s와 짝지어둔 이벤트 객체 핸들을 넘겨주면 이벤트 객체가 자동으로 비신호 상태가 된다. 선택 사항이므로 사용하지 않으려면 NULL을 넘겨주면 된다.
	LPWSANETWORKEVENTS lpNetworkEvents			- WSANETWORKEVENTS 구조체 변수 주소값을 전달하면 발생한 네트어크 이벤트와 오류 정보가 이 변수에 저장된다.
); (return 성공 - 0, 실패 - SOCKET_ERROR)

typedef struct _WSANETWORKEVENTS {
	long lNetworkEvents;					- 상수값이 조합된 형태로 저장되어 발생한 이벤트를 알려준다.
	int iErrorCode[FD_MAX_EVENTS];				- 네트워크 이벤트와 연관된 오류 정보가 저장된다. 오류 정보 참조시 배열 인덱스 값을 사용해야 한다.
}WSANETWORKEVENTS, *LPWSANETWORKEVENES;
```	
	네트워크 이벤트 - FD_ACCEPT, FD_READ, FD_WRITE, FD_CLOSE, FD_CONNECT, FD_OOB
	배열 인덱스 - FD_ACCEPT_BIT, FD_READ_BIT, FD_WRITE_BIT, FD_CLOSE_BIT, FD_CONNECT_BIT, FD_OOB_BIT
<hr>
Overlapped Model
------------------
#### 동작원리
원래 Overlapped 입출력 방식은 윈도우 운영체제에서 고성능 파일 입출력을 위해 제공하는데, 이를 소켓 입출력에도 사용할 수 있게 만든 것.

#### 입출력 방식
- 동기 입출력  
	응용 프로그램은 입출력 함수를 호출한 후 입출력 작업이 끝날 때까지 대기한다.   
	입출력 작업이 끝나면 입출력 함수가 리턴하고 응용 프로그램은 입출력 결과를 처리하거나 다른 작업을 할 수 있다.  
	(Select, WSAAsyncSelect, WSAEventSelect은 모두 동기 입출력 방식)  
	입출력 함수를 성공적으로 호출할 수 있는 시점을 운영체제가 알려주기 때문에 좀 더 입출력을 편하게 처리할 수 있다.  
	이와 같이 운영체제가 함수 호출 시점을 알려주는 개념을 비동기 통지라고 한다.  
	Select, WSAAsyncSelect, WSAEventSelect는 동기 입출력과 비동기 통지를 결합한 형태.

- 비동기 입출력  
	중첩 입출력(Overlapped I/O)라고도 부른다.  
	응용 프로그램은 입출력 함수를 호출한 후 입출력 작업의 여부와 무관하게 다른 작업을 진행할 수 있다.  
	입출력 작업이 끝나면 운영체제는 작업 완료를 응용 프로그램에 알려준다.   이때 응용 프로그램은 하던 작업을 중단하고 입출력 결과를 처리하면 된다.  
	(Overlapped, Completion Port는 모두 비동기 입출력 방식)  
	비동기 입출력 방식에서는 입출력 완료를 운영체제가 알려주는 개념이 필요하므로 비동기 통지도 사용한다 볼 수 있다.

#### Overlapped Model의 종류
1. Overlapped Model 1 - 소켓 입출력 작업이 완료되면 운영체제는 응용 프로그램이 등록해둔 이벤트 객체를 신호 상태로 바꾼다.  
응용 프로그램은 이벤트 객체를 관찰함으로써 입출력 작업 완료를 감지할 수 있다.  
Overlapped 모델을 사용하는 주된 이유는 데이터를 보내고 받는 작업을 효율적으로 처리하기 위해서다.

2. Overlapped Model 2 - 소켓 입출력 작업이 완료되면 운영체제는 응용 프로그램이 등록해둔 함수를 자동으로 호출한다.  
일반적으로 운영체제가 호출하는 응용 프로그램 함수를 콜백 함수(callback function)라 하는데, 특별히 Overlapped Model 에서는 완료 루틴(completion routine)이라 부른다.

#### Overlapped Model 공통 절차
1. 비동기 입출력을 지원하는 소켓을 생성한다. (socket() 함수로 생성한 소켓은 기본적으로 비동기 입출력을 지원한다.)

2. 비동기 입출력을 지원하는 소켓 함수를 호출한다. (AcceptEx(), ConnectEx(), DisconnectEx(), TransmitFile(), TransmitPackets(), WSAIoctl(), WSANSPIoctl(), WSAProviderConfigChange(), WSARecv(), WSARecvFrom(), WSARecvMsg(), WSASend(), WSASendTo())

3. 운영체제가 입출력 작업 완료를 응용 프로그램에 알려주면(비동기 통지), 응용 프로그램은 입출력 결과를 처리한다.

#### 핵심 함수
```
int WSASend(
	SOCKET s,				- 비동기 입출력을 할 대상 소켓
	LPWSABUF lpBuffers,			- WSABUF 구조체 배열의 시작 주소, 각 배열 원소(WSABUF 타입)는 버퍼의 시작 주소와 길이(바이트 단위)를 담고 있다.
	DWORD dwBufferCount,			- WSABUF 구조체 배열의 원소 개수
	LPDWORD lpNumberOfBytesSent,		- 함수 호출이 성공하면 보낸 바이트 수를 저장한다.
	DWORD dwFlags,				- 옵션으로 MSG_* 형태의 상수를 전달할 수 있는데, 각각 send(), recv() 함수의 마지막 인자와 같은 기능을 한다. 대부분의 경우 0을 사용
	LPWSAOVERLAPPED lpOverlapped,		- WSAOVERLAPPED 구조체의 주소 값, WSAOVERLAPPED 구조체는 비동기 입출력을 위한 정보를 운영체제에 전달하거나, 운영체제가 비동기 입출력 결과를 응용 프로그램에 알려줄 때 사용
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine	- 입출력 작업이 완료되면 운영체제가 자동으로 호출할 완료 루틴(콜백 함수)의 주소 값.
)	(return 성공 - 0, 실패 - SOCKET_ERROR)

int WSARecv( 
	SOCKET s,
	LPWSABUF lpBuffers,
	DWORD dwBufferCount,
	LPDWORD lpNumberOfBytesRecvd,
	LPDWORD lpFlags,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)	(return 성공 - 0, 실패 - SOCKET_ERROR)

typedef struct __WSABUF {
	u_long len,		- 길이(바이트 단위)
	char *buf		- 버퍼 시작 주소
}WSABUF, *LPWSABUF;

typedef struct _WSAOVERLAPPED {
	DWORD Internal,
	DWORD InternalHigh,
	DWORD Offset,
	DWORD OffsetHigh,
	WSAEVENT hEvent			- 이벤트 객체의 핸들 값, Overlapped Model 1에서만 사용, 입출력 작업이 완료되면 hEvent가 가리키는 이벤트 객체는 신호 상태가 된다.
}WSAOVERLAPPED, *LPWSAOVERLAPPED;
```

#### WSASend(), WSARecv() 함수의 특징
1. Scatter / Gather 입출력을 지원
	- 송신 측에서 WSABUF 구조체 배열을 사용하면, 여러 버퍼에 저장된 데이터를 모아서(Gather) 보낼 수 있다.
	```
	char buf1[128];
	char buf2[256];
	WSABUF wsabuf[2];
	wsabuf[0].buf = buf1;
	wsabuf[0].len = 128;
	wsabuf[1].buf = buf2;
	wsabuf[1].len = 256;
	WSASend(sock, wsabuf, 2, ...);
	```
	 - 수신 측에서 WSABUF 구조체 배열을 사용하면, 받은 데이터를 여러 버퍼에 흩뜨려(Scatter) 저장할 수 있다.

	```
	char buf1[128];
	char buf2[256];
	WSABUF wsabuf[2];
	wsabuf[0].buf = buf1;
	wsabuf[0].len = 128;
	wsabuf[1].buf = buf2;
	wsabuf[1].len = 256;
	WSARecv(sock, wsabuf, 2, ...);
	```
2. 마지막 두 인자에 모두 NULL 값을 사용하면 send() / recv() 함수처럼 동기 함수로 동작한다.

3. Overlapped Model 1에서는 OVERLAPPED 구조체의 hEvent 변수를, Overlapped Model 2에서는 lpCompletionRoutine 인자를 사용한다.   
단, lpCompletionRoutine 인자의 우선순위가 높으므로 이 값이 NULL이 아니면 WSAOVERLAPPED 구조체의 hEvent 변수는 사용되지 않는다.

#### Overlapped Model 1 소켓 입출력 절차
1. 비동기 입출력을 지원하는 소켓을 생성한다.  
이때 WSACreateEvent() 함수를 호출하여 대응하는 이벤트 객체도 같이 생성한다.

2. 비동기 입출력을 지원하는 소켓 함수를 호출한다.  
이때 WSAOVERLAPPED 구조체의 hEvent 변수에 이벤트 객체 핸들 값을 넣어 전달한다.  
비동기 입출력 작업이 곧바로 완료되지 않으면, 소켓 함수는 오류를 리턴하고 오류 코드는 WSA_IO_PENDING으로 설정된다.  
나중에 비동기 입출력 작업이 완료되면, 운영체제는 이벤트 객체를 신호 상태로 만들어 이 사실을 응용 프로그램에 알린다.

3. WSAWaitForMutipleEvents() 함수를 호출하여 이벤트 객체가 신호 상태가 되기를 기다린다.

4. 비동기 입출력 작업이 완료하여 WSAWaitForMultipleEvents() 함수가 리턴하면, WSAGetOverlappedResult() 함수를 호출해 비동기 입출력 결과를 확인하고 데이터를 처리한다.

5. 새로운 소켓을 생성하면 1 ~ 4 단계를, 그렇지 않으면 2 ~ 4단계를 반복한다.

```
BOOL WSAGetOverlappedResult(
	SOCKET s,					- 비동기 입출력 함수 호출에 사용했던 소켓을 넣는다.
	LPWSAOVERLAPPED lpOverlapped,			- 비동기 입출력 함수 호출에 사용했던 WSAOVEERLAPPED 구조체를 다시 넣는다.
	LPDWORD lpcbTransfer,				- 전송된 바이트 수가 여기에 저장된다.
	BOOL fWait,					- 비동기 입출력 작업이 끝날 때까지 대기하려면 TRUE, 그렇지 않으면 FALSE를 사용한다. WSAWaitForMultipleEvents() 함수를 이전에 호출해서 리턴했다면(3단계) 비동기 입출력 작업이 끝난다는 뜻이므로 FALSE를 사용하면 된다.
	LPDWORD lpdwFlags				- 비동기 입출력 작업과 관련된 부가적인 정보가 여기에 저장된다. 이 값은 거의 사용하지 않으므로 무시해도 좋다.
) (return 성공 - TRUE, 실패 - FALSE)
```

#### Overlapped Model 2의 동작 원리
1. 비동기 입출력 함수를 호출함으로써 운영체제에 입출력 작업을 요청한다.

2. 해당 스레드는 곧바로 alertable wait 상태에 진입한다.   
여기서 alertable wait 상태는 비동기 입출력을 위한 특별한 대기 상태로, 비동기 입출력 함수를 호출한 스레드가 이 상태에 있어야 완료 루틴이 호출될 수 있다.  
스레드를 alertable wait 상태로 만드는 함수는 다양하다.  
몇가지 예를 들면 WSAWaitForMultipleEvents(), WaitForSingleObjectEx(), WaitForMultipleObjectEx(), SleepEx()등이 있다.  
첫번째 함수인 WSAWaitForMultipleEvents()는 마지막 인자에 TRUE를 사용하면 해당 스레드는 alertable wait 상태가 된다.

3. 비동기 입출력 작업이 완료되면, 운영체제는 스레드의 APC 큐에 결과를 저장한다.   
여기서 APC 큐(asynchronous procedure call queue)는 비동기 입출력 결과 저장을 위해 운영체제가 각 스레드에 할당하는 메모리 영역이다.

4. 비동기 입출력 함수를 호출한 스레드가 alertable wait 상태에 있으면, 운영체제는 APC 큐에 저장된 정보(완료 루틴의 주소)를 참조하여 완료 루틴을 호출한다.  
완료 루틴 내부에서는 데이터를 처리한 후 다시 비동기 입출력 함수를 호출할 수 있다.

5. APC 큐에 저장된 정보를 토대로 모든 완료 루틴 호출이 끝나면, 스레드는 alertable wait 상태에서 빠져나온다.  
스레드가 비동기 입출력 결과를 계속 처리하려면 다시 alertable wait 상태에 진입해야 한다.

#### Overlapped Model 2 소켓 입출력 절차
1. 비동기 입출력을 지원하는 소켓을 생성한다.  
(socket() 함수로 생성한 소켓은 기본적으로 비동기 입출력을 지원한다.)

2. 비동기 입출력을 지원하는 소켓 함수를 호출한다.  
이때 완료 루틴의 시작 주소를 함수 인자로 전달한다.  
비동기 입출력 작업이 곧바로 완료되지 않으면, 소켓 함수를 SOCKET_ERROR를 리턴하고 오류 코드는 WSA_IO_PENDING으로 설정된다.

3. 비동기 입출력 함수를 호출한 스레드를 alertable wait 상태로 만든다.  
앞에서 소개한 WSAWaitForMultipleEvents(), WaitForSingleObjectEx(), WaitForMultipleObjectEx(), SleepEx()등의 함수 중에서 적절한 것을 선택하여 사용하면 된다.

4. 비동기 입출력 작업이 완료되면, 운영체제는 완료 루틴을 호출한다.  
완료 루틴에서는 비동기 입출력 결과를 확인하고 후속 처리를 한다.

5. 완료 루틴 호출이 모두 끝나면, 스레드는 alertable wait 상태에서 빠져나온다.

6. 새로운 소켓을 생성하면 1 ~ 5 단계를, 그렇지 않으면 2 ~ 5 단계를 반복한다.

#### alertable wait
alertable wait은 스레드의 대기 상태(wait state) 중 하나이다.  
Sleep() 함수 등을 호출하여 진입하는 일반적인 wait 상태와의 차이는 무엇인가?  
일반적인 wait은 대기 상태가 끝날 때까지 cpu 시간을 사용하지 못하는데 반해 alertable wait은 대기 상태가 끝나기 전에 cpu를 할당받아 완료 루틴을 호출하고, 더 처리할 내용이 없으면 대기 상태가 끝난다는 차이가 있다.

#### 운영체제가 호출하는 완료 루틴의 형태
```
void CALLBACK CompletionRoutine(
	DWORD dwError,					- 비동기 입출력 결과, 오류가 발생하면 0이 아닌 값이 된다.
	DWORD cbTransferred,				- 전송 바이트 수, 통신 상대가 접속을 종료하면 이 값은 0이 된다
	LPWSAOVERLAPPED lpOverlapped,			- 비동기 입출력 함수 호출 시 넘겨준 WSAOVERLAPPED 구조체의 주소값이 이 인자를 통해 다시 응용 프로그램에 넘어온다. (Overlapped Model 2 에서는 이벤트 객체를 사용하지 않으므로 WSAOVERLAPPED 구조체를 완료 루틴 내부에서 직접 사용하는 일은 없다.)
	DWORD dwFlags					- 항상 0이므로 적어도 현재까지는 사용하지 않는다.
)
```