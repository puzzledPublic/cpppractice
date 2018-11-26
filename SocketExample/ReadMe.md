TCP 서버 함수
-------------  
일반적으로 TCP 서버는 다음과 같은 순서로 소켓 함수를 호출한다.
1. socket() 함수로 소켓을 생성함으로써 사용할 프로토콜을 결정한다.

2. bind() 함수로 지역 IP 주소와 지역 포트 번호를 결정한다.

3. listen() 함수로 TCP를 LISTENING 상태로 변경한다.

4. accept() 함수로 자신에게 접속한 클라이언트와 통신할 수 있는 새로운 소켓을 생성한다.

5. send(), recv() 등의 데이터 전송 함수로 클라이언트와 통신을 수행한 후, closesocket() 함수로 소켓을 닫는다.

6. 새로운 클라이언트 접속이 들어올 때마다 4 ~ 5 과정을 반복한다.

#### bind()함수
```
int bind(
	SOCKET s,
	const struct sockaddr *name,
	int namelen
) (return 성공 - 0, 실패 - SOCKET_ERROR)
```
- s  
클라이언트 접속을 수용할 목적으로 만든 소켓으로, 지역 IP 주소와 지역 포트 번호가 아직 결정되지 않은 상태이다.

- name  
소켓 주소 구조체(TCP/IP의 경우 SOCKADDR_IN, SOCKADDR_IN6)를 지역 IP 주소와 지역 포트 번호로 초기화하여 전달한다.

- namelen  
소켓 주소 구조체의 길이(바이트 단위)다.

#### listen() 함수
소켓의 TCP 포트 상태를 LISTENING으로 바꾼다.  
이는 클라이언트 접속을 받을 수 있는 상태가 됨을 의미한다.
```
int listen(
	SOCKET s,
	int backlog
) (return 성공 - 0, 실패 - SOCKET_ERROR)
```
- s  
클라이언트 접속을 수용할 목적으로 만든 소켓으로, bind() 함수로 지역 IP 주소와 지역 포트 번호를 설정한 상태이다.

- backlog  
서버가 당장 처리하지 않더라도 접속 가능한 클라이언트 개수다.  
클라이언트의 접속 정보는 연결 큐(connection queue)에 저장되는데, backlog는 이 연결 큐의 길이를 나타낸다.  
하부 프로토콜에서 지원 가능한 최댓값을 사용하려면 SOMAXCONN 값을 대입한다.  
(서버가 당장 처리하지 않는다는 것은 accep() 함수를 호출하지 않음을 뜻한다. backlog 값을 바꾸려면 언제든지 listen() 함수를 다시 호출하면 된다.)

#### accept() 함수
접속한 클라이언트와 통신할 수 있도록 새로운 소켓을 생성하여 리턴한다.  
접속한 클라이언트의 주소 정보(서버 입장에서는 [원격 IP 주소, 원격 포트 번호] 클라이언트 입장에서는 [지역 IP 주소, 지역 포트 번호])도 알려 준다.
```
SOCKET accept(
	SOCKET s,
	struct sockaddr *addr,
	int *addrlen
)
```
- s  
클라이언트 접속을 수용할 목적으로 만든 소켓으로, bind() 함수로 지역 IP 주소와 지역 포트 번호를 설정하고 listen() 함수로 TCP 포트 상태를 LISTENING으로 변경한 상태다.

- addr  
소켓 주소 구조체를 전달하면 접속한 클라이언트의 주소 정보(IP 주소와 포트 번호)로 채워진다.

- addrlen  
정수형 변수를 addr이 가리키는 소켓 주소 구조체의 크기로 초기화한 후 전달한다.  
accept() 함수가 리턴하면 *addrlen 변수는 주소 정보의 크기(바이트 단위)를 갖게 된다.  
(클라이언트의 IP 주소와 포트 번호를 알 필요가 없다면 NULL 값을 전달하면 된다.)

접속한 클라이언트가 없다면 accept() 함수는 서버를 대기 상태(wait state 또는 suspended state)로 만든다.

TCP 클라이언트 함수
-------------------  
일반적으로 TCP 클라이언트는 다음과 같은 순서로 소켓 함수를 호출한다.
1. socket() 함수로 소켓을 생성함으로써 사용할 프로토콜을 결정한다.

2. connect() 함수로 서버에 접속한다.  
이때 원격 IP 주소와 원격 포트 번호는 물론, 지역 IP 주소와 지역 포트 번호도 결정된다.

3. send(), recv()등의 데이터 전송 함수로 서버와 통신한 후, closesocket() 함수로 소켓을 닫는다. 

#### connect() 함수
TCP 프로토콜 수준에서 서버와 논리적 연결을 설정한다.  
```
int connect(
	SOCKET s,
	const struct sockaddr *name,
	int namelen
) (return 성공 - 0, 실패 - SOCKET_ERROR)
```
- s  
서버와 통신할 목적으로 만든 소켓이다.

- name  
소켓 주소 구조체를 서버 주소(즉, 원격 IP 주소와 원격 포트 번호)로 초기화하여 전달한다.

- namelen  
소켓 주소 구조체의 길이(바이트 단위)다.

서버와 달리 bind() 함수는 호출하지 않는다.  
connect() 함수를 호출하면, 운영체제가 자동으로 지역 IP 주소와 지역 포트 번호를 할당해준다.

TCP 데이터 전송 함수
--------------------  
가장 기본이 되는 함수는 send(), recv()이며, 그 외에 sendto(), recvfrom(), WSASend(), WSARecv() 형태의 확장 함수가 존재한다.  
send(), recv() 함수는 소켓 버퍼에 접근할 수 있게 만든 함수라고 보면 된다.  
TCP 프로토콜은 응용 프로그램이 보낸 데이터의 경계를 구분하지 않는다는 특징이 있다.  
(이와 반대로 UDP 프로토콜은 경계를 구분한다.)  
따라서 TCP 서버-클라이언트를 작성할 때는 데이터 경계 구분을 위한 상호 약속이 필요하며, 이를 응용 프로그램 수준에서 처리해야 한다.

#### send() 함수  
send() 함수는 응용 프로그램 데이터를 운영체제 송신 버퍼에 복사함으로써 데이터를 전송한다.  
send() 함수는 데이터 복사가 성공하면 곧바로 리턴한다.  
따라서 send() 함수가 리턴했다고 실제 데이터가 전송된 것은 아니며, 일정 시간이 지나야만 하부 프로토콜(예를들면 TCP/IP)을 통해 전송이 완료된다.
```
int send(
	SOCKET s,
	const char *buf,
	int len,
	int flags
) (return 성공 - 보낸 바이트 수, 실패 - SOCKET_ERROR)
```
- s  
통신할 대상과 연결된(connected) 소켓이다.

- buf  
보낼 데이터를 담고 있는 응용 프로그램 버퍼의 주소다.

- len  
보낼 데이터의 크기(바이트 단위)다.

- flags  
send() 함수의 동작을 바꾸는 옵션으로, 대부분 0을 사용하면 된다.  
사용 가능한 값으로 MSG_DONTROUTE(윈속에서는 사용해도 무시됨)와 MSG_OOB가 있다.

send() 함수는 전달하는 소켓(s)의 특성에 따라 두 종류의 성공적인 리턴을 할 수 있다.
- 블로킹 소켓  
블로킹 소켓을 대상으로 호출하면, 송신 버퍼의 여유 공간이 send() 함수의 세 번째 인자인 len 보다 작을 경우 해당 프로세스는 대기 상태가 된다.  
송신 버퍼에 충분한 공간이 생기면 프로세스는 깨어나고 len 크기만큼 복사가 일어난 후 send() 함수가 리턴한다.  
이 경우 함수의 리턴 값은 len과 같다.

- 넌블로킹 소켓  
ioctlsock() 함수를 이용하면 넌블로킹 소켓으로 바꿀 수 있다.  
넌블로킹 대상으로 호출하면, 송신 버퍼의 여유 공간만큼 데이터를 복사한 후 실제 복사한 바이트 수를 리턴한다.  
이 경우 함수의 리턴 값은 최소 1, 최대 len이다.

#### recv() 함수
운영체제의 수신 버퍼에 도착한 데이터를 응용 프로그램 버퍼에 복사한다.  
```
int recv(
	SOCKET s,
	char *buf,
	int len,
	int flags
) (return 성공 - 받은 바이트 수 또는 0(연결 종료시), 실패 - SOCKET_ERROR)
```
- s  
통신할 대상과 연결된(connected) 소켓이다.

- buf  
받은 데이터를 저장할 응용 프로그램 버퍼의 주소다.

- len  
운영체제의 수신 버퍼로부터 복사할 최대 데이터 크기(바이트 단위)다.  
이 값은 buf가 가리키는 응용 프로그램 버퍼보다 크지 않아야 한다.  

- flags  
recv() 함수의 동작을 바꾸는 옵션으로, 대부분 0을 사용하면 된다.  
사용 가능한 값으로 MSG_PEEK, MSG_OOB, MSG_WAITALL이 있다.  
recv()함수의 기본 동작은 수신 버퍼의 데이터를 응용 프로그램 버퍼에 복사한 후 해당 데이터를 수신 버퍼에서 삭제하는 것이다.  
하지만 MSG_PEEK 옵션을 사용하면 수신 버퍼에 데이터가 계속 남는다.  
MSG_WAITALL 옵션을 사용하면 세 번째 인자로 설정한 len 크기만큼 데이터를 받을 때까지 응용 프로그램이 대기한다.

recv() 함수는 다음 두 종류의 성공적인 리턴을 할 수 있다.  
- 수신 버퍼에 데이터가 도달한 경우  
recv() 함수의 세 번째 인자인 len보다 크지 않은 범위에서 가능하면 많은 데이터를 응용 프로그램 버퍼에 복사한 후 실제 복사한 바이트 수를 리턴한다.  
이 경우 recv() 함수의 리턴 값은 최소 1, 최대 len이다.

- 접속이 정상 종료한 경우  
상대편 응용 프로그램이 closesocket() 함수를 호출해 접속을 종료하면, TCP 프로토콜 수준에서 접속 종료를 위한 패킷 교환 절차가 일어난다.  
이 경우 recv() 함수는 0을 리턴한다.  
recv() 함수의 리턴 값이 0 인 경우를 정상 종료(normal close, graceful close)라 부른다.

recv() 함수 사용 시 주의할 점은 세 번째 인자인 len으로 지정한 크기보다 적은 데이터가 응용 프로그램 버퍼에 복사될 수 있다는 사실이다.  
이는 TCP가 데이터 경계를 구분하지 않는다는 특성에 기인한다.  
따라서 자신이 받을 데이터의 크기를 미리 알고 있다면 그만큼 받을때까지 recv() 함수를 여러번 호출해야 한다.
<hr>  

UDP 모델
---------
TCP 소켓과 달리 운영체제 내부에 송신 버퍼가 없다.  
UDP는 TCP와 달리 데이터 재전송과 흐름제어를 하지 않으므로 TCP와 같은 송신 버퍼(데이터 재전송용)는 존재하지 않는다.  

#### UDP 서버 동작 순서  
1. socket() 함수로 소켓을 생성함으로써 사용할 프로토콜을 결정한다.

2. bind() 함수로 지역 IP 주소와 지역 포트 번호를 결정한다.

3. 클라이언트가 보낸 데이터를 recvfrom() 함수로 받는다.  
이때 원격 IP 주소와 원격 포트 번호, 즉 클라이언트의 주소를 알 수 있다.

4. 받은 데이터를 처리한 결과를 sendto() 함수로 보낸다.

5. 모든 작업을 마치면 closesocket() 함수로 소켓을 닫는다.

#### UDP 클라이언트 동작 순서
1. socket() 함수로 소켓을 생성함으로써 사용할 프로토콜을 결정한다.
	- (Optional) UDP 소켓에 대해 connect() 함수를 호출 할 수 있다.  
	이 경우 통신할 상대의 주소 정보가 내부적으로 기억되므로, sendto(), recvfrom() 대신 send(), recv() 함수를 사용해 특정 UDP 서버와 통신할 수 있다.  
	이로인한 장점은 sendto() 함수를 사용한 경우보다 효율적이다.  
	서버 주소를 한 번만 설정하면 send() 함수가 이 정보를 재활용하기 때문이다.  
	또 데이터를 받은 후 송신자의 주소(IP, PORT)를 확인하지 않아도 된다.  
	recv() 함수는 connect() 함수로 설정한 대상을 제외한 다른 UDP 응용 프로그램이 보낸 데이터는 수신하지 않기 때문이다.

	- connect() 함수는 특정 IP 주소와 포트 번호로 나타낼 수 있는 고정된 대상과 통신함을 의미한다.  
	통신 대상을 변경하려면 언제든지 connect() 함수를 다시 호출하면 된다.

2. sendto() 함수로 서버에 데이터를 보낸다.  
이때 원격 IP 주소와 원격 포트 번호는 물론, 지역 IP 주소와 지역 포트 번호도 결정된다.  

3. 서버가 처리해 보낸 데이터를 recvfrom() 함수로 받는다.

4. 모든 작업을 마치면 closesocket() 함수로 소켓을 닫는다.

#### UDP 모델을 사용할때 주의점
- 블로킹 소켓 사용할 경우, 송수신 함수의 호출 순서가 맞지 않으면 교착상태가 발생할 수 있다.
- 클라이언트는 데이터를 받은 후 송신자의 주소(IP, PORT)를 확인해야 한다.  
recvfrom() 함수는 UDP 서버가 보낸 데이터는 물론, 전혀 다른 UDP 응용 프로그램이 보낸 데이터도 수신할 수 있기 때문이다.

#### sendto() 함수
응용 프로그램의 데이터를 운영체제의 송신 버퍼에 복사함으로써 데이터를 전송한다.  
UDP의 경우, sendto() 함수 호출시 소켓의 지역 IP 주소와 지역 포트 번호가 아직 결정되지 않은 상태라면 운영체제가 자동으로 결정해준다.  
즉, sendto() 함수가 TCP의 bind() 함수 역할을 대신한다.

```
int sendto(
	SOCKET s,
	const char *buf,
	int len,
	int flags,
	const struct sockaddr *to,
	int to len
) (return 성공 - 보낸 바이트 수, 실패 - SOCKET_ERROR)
```
- s  
통신에 사용할 소켓

- buf  
보낼 데이터를 담고 있는 응용 프로그램 버퍼의 주소

- len  
보낼 데이터 크기(바이트 단위)

- flags  
sendto() 함수의 동작을 바꾸는 옵션으로 대부분은 0을 사용한다.  
사용 가능한 값으로 MSG_DONTROUTE, MSB_OOB가 있다.

- to  
목적지 주소를 담고 있는 소켓 주소 구조체  
UDP의 경우 특정 호스트나 라우터 주소는 물론이고 브로드캐스트나 멀티캐스트 주소를 사용할 수도 있다.

- tolen  
목적지 주소를 담고 있는 소켓 주소 구조체의 크기(바이트 단위)

#### sendto() 함수 주의점
- sendto() 함수는 UDP는 물론 TCP 소켓에서도 사용가능하며, 이 경우 to와 tolen 인자는 무시된다.  
TCP 소켓에 사용할 때만 flags 인자에 MSG_OOB를 사용할 수 있다.

- sendto() 함수로 보낸 데이터는 독립적인 UDP 데이터 그램(패킷)으로 만들어져 전송되며, 수신 측에서는 recvfrom() 함수 호출 한 번으로 이 데이터를 읽을 수 있다.  
따라서 UDP를 사용할 경우에는 TCP와 달리 응용 프로그램 수준에서 메시지 경계를 구분하는 작업을 할 필요가 없다.

- UDP 소켓에 대해 sendto() 함수를 호출할 경우 한 번에 보낼 수 있는 데이터의 크기에 제한이 있다.  
최소값은 0, 최댓값은 65507(65537 - 20(IP 헤더 크기) - 8(UDP 헤더 크기)) 바이트이다.  
실제로는 최댓값보다 훨씬 작은 크기를 사용하는 것이 바람직하다.  
특히 UDP를 이용해 브로드캐스트 패킷을 보낼 경우 512 바이트보다 작은 크기를 사용할 것을 권고한다.

- sendto() 함수로 보낸 응용 프로그램 데이터는 커널(운영체제) 영역에 복사되어 전송된 후 곧바로 버려진다.  
sendto() 함수가 리턴했다고 실제 데이터 전송이 완료된 것은 아니며, 데이터 전송이 끝났어도 상대방이 받았는지 확인할 수 없다.

- 블로킹 소켓을 사용할 경우, 커널 영역에 데이터를 복사할 공간이 부족하다면 sendto() 함수는 호출시 블록된다.

#### recvfrom() 함수
운영체제의 수신 버퍼에 도착한 데이터를 응용 프로그램 버퍼에 복사한다.  
TCP의 recv() 함수와 다른점은, UDP 패킷 데이터를 한 번에 하나만 읽을 수 있다는 점이다.  
즉, 응용 프로그램 버퍼를 크게 잡는다고 많은 데이터를 한꺼번에 읽을 수 없다.

```
int recvfrom(
	SOCKET s,
	char *buf,
	int len,
	int flags,
	struct sockaddr *from,
	int *fromlen
) (return 성공 - 받은 바이트 수, 실패 - SOCKET_ERROR)
```
- s  
통신에 사용할 소켓  
sendto() 함수에 사용하는 소켓과 달리, 이 소켓은 반드시 지역 주소(IP, PORT)가 미리 결정되어 있어야 한다.

- buf  
받은 데이터를 저장할 응용 프로그램 버퍼의 주소

- len  
응용 프로그램 버퍼의 크기(바이트 단위)  
도착한 UDP 패킷 데이터가 len보다 크면 len만큼 복사하고 나머지는 버린다.  
이때 recvfrom() 함수는 SOCKET_ERROR를 리턴한다.  
따라서 예상되는 UDP 패킷 데이터의 최대 크기를 감안해 응용 프로그램 버퍼를 준비해야 한다.

- flags  
recv() 함수의 동작을 바꾸는 옵션으로, 대부분은 0을 사용한다.  
사용 가능한 값으로 MSG_PEEK, MSG_OOB가 있다.  
recvfrom() 함수의 기본 동작은 수신 버퍼의 데이터를 응용 프로그램 버퍼에 복사 후 해당 데이터를 수신 버퍼에서 삭제하는 것이지만 MSG_PEEK 옵션을 사용하면 수신 버퍼에 데이터가 계속 남는다.

- from  
소켓 주소 구조체를 전달하면 송신자의 주소 정보(IP, PORT)로 채워진다.

- fromlen  
정수형 변수를 from이 가리키는 소켓 주소 구조체의 크기로 초기화한 후 전달한다.  
recvfrom() 함수가 리턴하면 *fromlen 변수는 recvfrom() 함수가 채워넣은 주소 정보의 크기(바이트 단위)를 갖는다.

#### recvfrom() 함수 주의점  
- recvfrom() 함수는 UDP 소켓은 물론, TCP 소켓에도 사용 가능하며, 이 경우 from과 fromlen 인자는 무시된다.  
TCP 소켓에 사용할때 만 flags 인자에 MSG_OOB를 사용할 수 있다.

- sendto() 함수로 보낸 데이터는 독립적인 UDP 데이터그램(패킷)으로 만들어져 전송되며, 수신 측에서는 recvfrom() 함수 호출 한 번으로 이 데이터를 읽을 수 있다.  
따라서 UDP를 사용할 경우, TCP와 달리 응용 프로그램 수준에서 메시지 경계를 구분하는 작업을 할 필요가 없다.

- UDP 소켓에 대해 recvfrom() 함수를 호출 할 경우 리턴 값이 0이 될 수 있는데, 이는 상대방이 sendto() 함수 호출시 데이터 크기를 최솟값인 0으로 설정했다는 뜻이다.  
UDP 프로토콜에는 연결 설정과 종료 개념이 없으므로 recvfrom() 함수의 리턴 값이 0이라고 해서 특별한 의미가 있는 것은 아니다.  
반면, TCP 소켓에 대해 recvfrom() 함수를 호출할 경우 리턴 값이 0이면 정상 종료(normal close 또는 graceful close)를 의미한다.

- 블로킹 소켓을 사용할 경우, 소켓 수신 버퍼에 도착한 데이터가 없으면 recvfrom() 함수는 호출시 블록된다.
<hr>

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
LRESULT CALLBACK WndProc(
	HWND hWnd, 
	UNIT uMsg, 
	WPARAM wParam, 
	LPARAM lParam
) {...}
```
- hWnd  
메시지가 발생한 윈도우의 핸들

- uMsg  
WSAAsyncSelect() 함수 호출시 등록했던 사용자 정의 메시지

- wParam  
네트워크 이벤트가 발생한 소켓.  
SOCKET으로 형변환하여 소켓 함수 호출에 그대로 사용

- lParam  
하위 16비트는 발생한 네트워크 이벤트, 상위 16비트는 오류 코드를 담고 있다.  
항상 오류 코드를 먼저 확인 후 네트워크 이벤트를 처리해야 한다.  
각각을 위한 정의 된 매크로가 있다. (WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam))

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
```
```
typedef struct _WSANETWORKEVENTS {
	long lNetworkEvents;					- 상수값이 조합된 형태로 저장되어 발생한 이벤트를 알려준다.
	int iErrorCode[FD_MAX_EVENTS];				- 네트워크 이벤트와 연관된 오류 정보가 저장된다. 오류 정보 참조시 배열 인덱스 값을 사용해야 한다.
}WSANETWORKEVENTS, *LPWSANETWORKEVENES;
```	
	네트워크 이벤트 - FD_ACCEPT, FD_READ, FD_WRITE, FD_CLOSE, FD_CONNECT, FD_OOB
	배열 인덱스 - FD_ACCEPT_BIT, FD_READ_BIT, FD_WRITE_BIT, FD_CLOSE_BIT, FD_CONNECT_BIT, FD_OOB_BIT
<hr>

Overlapped Model
----------------  
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
Overlapped Model을 사용하는 주된 이유는 데이터를 보내고 받는 작업을 효율적으로 처리하기 위해서다.

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
```
```
int WSARecv( 
	SOCKET s,
	LPWSABUF lpBuffers,
	DWORD dwBufferCount,
	LPDWORD lpNumberOfBytesRecvd,
	LPDWORD lpFlags,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)	(return 성공 - 0, 실패 - SOCKET_ERROR)
```
```
typedef struct __WSABUF {
	u_long len;		- 길이(바이트 단위)
	char *buf;		- 버퍼 시작 주소
}WSABUF, *LPWSABUF;
```
```
비동기 입출력을 위한 정보를 운영체제에 전달하거나, 운영체제가 비동기 입출력 결과를 응용 프로그램에 알려줄때 사용한다.
typedef struct _WSAOVERLAPPED {
	DWORD Internal;
	DWORD InternalHigh;
	DWORD Offset;
	DWORD OffsetHigh;
	WSAEVENT hEvent;
}WSAOVERLAPPED, *LPWSAOVERLAPPED;
```
- Internal, InternalHigh, Offset, OffsetHigh  
운영체제가 내부적으로만 사용하는 변수

- hEvent  
이벤트 객체의 핸들 값, Overlapped Model 1에서만 사용, 입출력 작업이 완료되면 hEvent가 가리키는 이벤트 객체는 신호 상태가 된다.

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
	SOCKET s,
	LPWSAOVERLAPPED lpOverlapped,
	LPDWORD lpcbTransfer,
	BOOL fWait,
	LPDWORD lpdwFlags
) (return 성공 - TRUE, 실패 - FALSE)
```

- s  
비동기 입출력 함수 호출에 사용했던 소켓을 넣는다.

- lpOverlapped  
비동기 입출력 함수 호출에 사용했던 WSAOVERLAPPED 구조체를 다시 넣는다.

- lpcbTransfer  
전송된 바이트 수가 여기에 저장된다.

- fWait  
비동기 입출력 작업이 끝날 때까지 대기하려면 TRUE, 그렇지 않으면 FALSE를 사용한다.  
WSAWaitForMultipleEvents() 함수를 이전에 호출해서 리턴했다면(3단계) 비동기 입출력 작업이 끝난다는 뜻이므로 FALSE를 사용하면 된다.

- lpdwFlags  
비동기 입출력 작업과 관련된 부가적인 정보가 여기에 저장된다.  
이 값은 거의 사용하지 않으므로 무시해도 좋다.

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
	DWORD dwError,
	DWORD cbTransferred,
	LPWSAOVERLAPPED lpOverlapped,
	DWORD dwFlags
)
```
- dwError  
비동기 입출력 결과, 오류가 발생하면 0이 아닌 값이 된다.

- cbTransferred  
전송 바이트 수, 통신 상대가 접속을 종료하면 이 값은 0이 된다.

- lpOverlapped  
비동기 입출력 함수 호출 시 넘겨준 WSAOVERLAPPED 구조체의 주소값이 이 인자를 통해 다시 응용 프로그램에 넘어온다.  
(Overlapped Model 2 에서는 이벤트 객체를 사용하지 않으므로 WSAOVERLAPPED 구조체를 완료 루틴 내부에서 직접 사용하는 일은 없다.)

- dwFlags  
항상 0이므로 적어도 현재까지는 사용하지 않는다.

<hr>

Completion Port Model
---------------------  
#### 동작원리
Completion Port Model의 핵심은 입출력 완료 포트라는 윈도우 운영체제가 제공하는 구조를 이해하고 활용하는 것이다.  
입출력 완료 포트(I/O completion port)는 비동기 입출력 결과와 이 결과를 처리할 스레드에 관한 정보를 담고 있는 구조로 Overlapped Model 2에서 소개한 APC 큐와 비슷한 개념이다.  

#### 입출력 완료 포트와 APC 큐의 차이점
- 생성과 파괴  
APC 큐는 각 스레드마다 자동으로 생성되고 파괴된다.  
입출력 완료 포트는 CreateIoCompletionPort() 함수를 호출하여 생성하고 CloseHandle() 함수를 호출하여 파괴한다.

- 접근 제약  
APC 큐에 저장된 결과는 APC 큐를 소유한 스레드만 확인할 수 있지만, 입출력 완료 포트에서는 이런 제약이 없다.  
대개 입출력 완료 포트에 접근하는 스레드를 별도로 두는데, 이를 작업자 스레드(Worker Thread)라 부른다.  
이상적인 작업자 스레드 생성 개수는 CPU 개수 * N개이다(N >= 1)

- 비동기 입출력 처리 방법  
APC 큐에 저장된 결과를 처리하려면 해당 스레드는 alertable wait 상태에 진입해야 한다.  
입출력 완료 포트에 저장된 결과를 처리하려면 작업자 스레드는 GetQueuedCompletionStatus() 함수를 호출해야 한다.

#### Completion Port Model을 이용한 입출력 과정
1. 응용 프로그램을 구성하는 임의의 스레드에서 비동기 입출력 함수를 호출함으로써 운영 체제에 입출력 작업을 요청한다.

2. 모든 작업자 스레드는 GetQueuedCompletionStatus() 함수를 호출해 입출력 완료 포트를 감시한다.  
완료한 비동기 입출력 작업이 아직 없다면 모든 작업자 스레드는 대기 상태가 된다.  
이때 대기 중인 작업자 스레드 목록은 입출력 완료 포트 내부에 저장된다.

3. 비동기 입출력 작업이 완료되면 운영쳊는 입출력 완료 포트에 결과를 저장한다.  
이때 저장되는 정보를 입출력 완료 패킷(I/O completion packet)이라 부른다.

4. 운영체제는 입출력 완료 포트에 저장된 작업자 스레드 목록에서 하나를 선택하여 깨운다.  
대기 상태에서 깨어난 작업자 스레드는 비동기 입출력 결과를 처리한다.  
이후 작업자 스레드는 필요에 따라 다시 비동기 입출력 함수를 호출할 수 있다.

#### Completion Port Model을 이용한 소켓 입출력 절차
1. CreateIoCompletionPort() 함수를 호출하여 입출력 완료 포트를 생성한다.

2. CPU 개수에 비례하여 작업자 스레드를 생성한다.  
모든 작업자 스레드는 GetQueuedCompletionStatus() 함수를 호출하여 대기 상태가 된다.

3. 비동기 입출력을 지원하는 소켓을 생성한다.  
이 소켓에 대한 비동기 입출력 결과가 저장되려면, CreateIoCompletionPort() 함수를 호출하여 소켓과 입출력 완료 포트를 연결해야 한다.

4. 비동기 입출력 함수를 호출한다.  
비동기 입출력 작업이 곧바로 완료되지 않으면, 소켓 함수는 SOCKET_ERROR를 리턴하고 오류 코드는 WSA_IO_PENDING으로 설정된다.

5. 비동기 입출력 작업이 완료되면, 운영체제는 입출력 완료 포트에 결과를 저장하고, 대기 중인 스레드 하나를 깨운다.  
대기 상태에서 깨어난 작업자 스레드는 비동기 입출력 결과를 처리한다.

6. 새로운 소켓을 생성하면 3 ~ 5 단계를, 그렇지 않으면 4 ~ 5 단계를 반복한다.

#### CreateIoCompletionPort()
이 함수는 두 가지 역할을 한다.  
1. 입출력 완료 포트를 생성한다.

2. 소켓과 입출력 완료 포트를 연결한다.  
소켓과 입출력 포트를 연결해두면 이 소켓에 대한 비동기 입출력 결과가 입출력 완료 포트에 저장된다.

```
HANDLE CreateIoCompletionPort(
	HANDLE FileHandle,
	HANDLE ExistingCompletionPort,
	ULONG CompeletionKey,
	DWORD NumberOfConcurrentThreads
)(return 성공 - 입출력 완료 포트 핸들, 실패 - NULL)
```

- FileHandle  
입출력 완료 포트와 연결할 파일 핸들이다.  
소켓 프로그래밍에서는 소켓 디스크립터를 넣어주면 된다.  
새로운 입출력 완료 포트를 생성할 때는 유효한 핸들 대신 INVALID_HANDLE_VALUE값을 사용해도 된다.

- ExistingCompletionPort  
파일 또는 소켓과 연결할 입출력 완료 포트 핸들이다.  
이 값이 NULL이면 새로운 입출력 완료 포트를 생성한다.

- CompletionKey  
입출력 완료 패킷(I/O completion packet)에 들어갈 부가 정보로 32비트 값을 줄 수 있다.  
입출력 완료 패킷은 비동기 입출력 작업이 완료될 때마다 생성되어 입출력 완료 포트에 저장되는 정보다.

- NumberOfConcurrentThreads  
동시에 실행할 수 있는 작업자 스레드의 개수다.  
0을 사용하면 자동으로 CPU 개수와 같은 수로 설정된다.  
운영체제는 실행 중인 작업자 스레드 개수가 여기서 설정한 값을 넘지 않도록 관리해준다.

##### 사용 예
- 입출력 완료 포트를 새로 생성
```
HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
if(hcp == NULL) return 1;
```
- 기존 소켓과 입출력 완료 포트를 연결
```
SOCKET sock;
...
HANDLE hResult = CreateIoCompletionPort((HANDLE)sock, hcp, (DWORD)sock, 0);
if(hResult == NULL) return 1;
```

#### GetQueuedCompletionStatus()
작업자 스레드는 GetQueuedCompletionStatus() 함수를 호출함으로써 입출력 완료포트에 입출력 완료 패킷이 들어올 때까지 대기한다.  
입출력 완료 패킷이 입출력 완료 포트에 들어오면 운영체제는 실행 중인 작업자 스레드의 개수를 체크한다.  
이 값이 CreateIoCompletionPort() 함수의 네 번째 인자로 설정한 값보다 작다면, 대기 상태인 작업자 스레드를 깨워서 입출력 완료 패킷을 처리하게 한다.

```
BOOL GetQueuedCompletionStatus(
	HANDLE CompletionPort,
	LPDWORD lpNumberOfBytes,
	LPDWORD lpCompletionKey,
	LPOVERLAPPED *lpOverlapped,
	DWORD dwMilliseconds
) (return 성공 - 0이 아닌 값, 실패 - 0)
```
- CompletionPort  
입출력 완료 포트 핸들

- lpNumberOfBytes  
비동기 입출력 작업으로 전송된 바이트 수가 여기에 저장된다.

- lpCompletionKey  
CreateIoCompletionPort() 함수 호출 시 전달한 세 번째 인자(32비트)가 여기에 저장된다.

- lpOverlapped  
비동기 입출력 함수 호출 시 전달한 OVERLAPPED 구조체의 주소 값이 여기에 저장된다.

- dwMiilliseconds  
작업자 스레드가 대기할 시간을 밀리초 단위로 지정한다.  
INFINITE 값을 넣으면 입출력 완료 패킷이 생성되어 운영체제가 깨울 때까지 무한히 대기한다.

응용 프로그램이 작업자 스레드에 특별한 사실을 알리기 위해 직접 입출력 완료 패킷을 생성할 수도 있다.  
이때 사용하는 함수는 PostQueuedCompletionStatus()이다.  
각 인자의 의미는 GetQueuedCompletionStatus와 유사하다.

소켓 입출력 모델 비교
--------------------
#### Select Model
- 장점  
모든 윈도우 운영체제는 물론, 유닉스/리눅스 운영체제에서도 사용할 수 있으므로 이식성이 높다.  
윈도우와 유닉스/리눅스에서 모두 실행할 서버가 필요하고 이식할 때 코드 수정을 최소화하고 싶다면 유일하게 선택할 수 있는 모델이다.

- 단점  
하위 호환성을 위해 존재하며, 성능은 여섯가지 모델 중 가장 떨어진다.  
스레드 당 처리할 수 있는 소켓 개수가 64개로 제한되어 있으므로, 그 이상의 소켓을 처리하려면 스레드를 여러 개 사용하는 것이 원칙이다.  
하지만 윈도우 운영체제에서는 FD_SETSIZE를 재정의함으로써 스레드당 처리할 수 있는 소켓의 개수를 늘릴 수 있다.

#### WSAAsyncSelect Model
- 장점  
소켓 이벤트를 윈도우 메시지 형태로 처리하므로 GUI 응용 프로그램과 잘 결합할 수 있다.  
MFC 소켓 클래스에서 내부적으로 사용하는 모델이므로, 학습해두면 MFC 소켓 클래스의 내부 동작을 이해하고자 할 때 도움이 된다.

- 단점  
단일 윈도우 프로시저에서 일반 윈도우 메시지와 소켓 메시지를 처리해야 하므로 성능 저하 요인이 된다.

#### WSAEventSelect Model  
- 장점  
Select 모델과 WSAAsyncSelect 모델의 특성을 혼합한 형태로, 비교적 뛰어난 성능을 제공하면서 윈도우를 필요로 하지 않는다.

- 단점  
스레드당 처리할 수 있는 소켓의 개수가 64개로 제한되어 있으므로, 그 이상의 소켓을 처리하려면 스레드를 여러 개 사용해야 한다.

#### Overlapped Model
- 장점  
비동기 입출력을 통해 뛰어난 성능을 제공한다.

- 단점  
Overlapped Model 1 : 스레드당 처리할 수 있는 소켓의 개수가 64개ㅐ로 제한되어 있으므로, 그 이상의 소켓을 처리하려면 스레드를 여러 개 사용해야 한다.
Overlapped Model 2 : 모든 비동기 소켓 함수에 대해 완료 루틴을 사용할 수 있는 것은 아니다.

#### Completion Port Model
- 장점  
비동기 입출력과 완료 포트를 통해 가장 뛰어난 성능을 제공한다.

- 단점  
가장 단순한 소켓 입출력 방식(블로킹 소켓 + 스레드)과 비교하면 코딩이 복잡하지만 성능 면에서 특별한 단점은 없다.

소켓 옵션
---------
처리 주체에 따라 크게 두 종류로 구분
1. 소켓 코드가 처리하는 옵션  
옵션을 적용하면 소켓 코드에서 해석하고 처리한다. 프로토콜 독립적인 성격이 있으나 옵션의 실제 적용 여부는 프로토콜 종류에 따라 달라진다.

2. 프로토콜 구현 코드가 처리하는 옵션  
옵션을 설정하면 프로토콜 구현 코드에서 해석하고 처리한다.  
프로토콜 의존적인 성격이 있으므로 프로토콜 종류에 따라 옵션 자체가 달라진다.

#### setsockopt()  
소켓 옵션을 설정할 때는 setsockopt() 함수를 사용한다.
```
int setsockopt(
	SOCKET s,
	int level,
	int optname,
	const char *optval,
	int optlen
) (return 성공 - 0, 실패 - SOCKET_ERROR)
```
- s  
옵션을 적용할 대상 소켓이다.

- level  
옵션을 해석하고 처리할 주체를 지시한다.  
예를 들어 소켓 코드가 처리하면 SOL_SOCKET, IPv4 프로토콜 코드가 처리하면 IPPROTO_IP, IPv6 프로토콜 코드가 처리하면 IPPROTO_IP6, TCP 프로토콜 코드가 처리하면 IPPROTO_TCP를 사용한다.  
옵션 이름이 정해지면 level 값은 자동으로 결정된다.

- optname  
설정할 옵션의 이름이다.

- optval  
설정할 옵션 값을 담고 있는 버퍼의 주소다.  
옵션 값은 대부분 정수형이지만 구조체형인 경우도 있다.

- optlen  
optval이 가리키는 버퍼의 크기(바이트 단위)다.

#### getsockopt()  
소켓 옵션 값을 얻을 때는 getsockopt() 함수를 사용한다.
```
int getsockopt(
	SOCKET s,
	int level,
	int optname,
	char *optval,
	int optlen
) (return 성공 - 0, 실패 - SOCKET_ERROR)
```
- s  
옵션 값을 얻을 대상 소켓이다.

- level  
setsockopt의 level과 의미가 같다.

- optname  
값을 얻을 옵션의 이름이다.

- optval  
얻은 옵션 값을 저장할 버퍼의 주소다.  
옵션 값은 대부분 정수형이지만 구조체형인 경우도 있다.

- optlen  
값-결과(value-result) 인자로 사용한다.  
함수 호출 전에는 optval이 가리키는 버퍼의 크기(바이트 단위)로 응용 프로그램에서 초기화한다.  
함수 호출 후에는 얻은 옵션 값의 크기로 운영체제가 값을 채운다.

#### level별 자주 사용하는 옵션

-  SOL_SOCKET  

	| optname |optval |get |set |설명 |
	|:-------|:------|:---|:---|:----|
	| SO_BROADCAST |BOOL |O |O |브로드캐스팅 데이터 전송 허용 여부 |
	| SO_KEEPALIVE|BOOL|O|O|주기적으로 연결 상태 확인 여부|
	| SO_LINGER |linger{} |O |O |소켓 송신 버퍼에 미전송 데이터가 있을 때 closesocket() 함수 리턴 지연 시간 설정|
	| SO_SNDBUF, SO_RCVBUF |int |O |O |소켓 송,수신 버퍼의 크기 설정|
	| SO_SNDTIMEO, SO_RCVTIMEO |int |O |O |데이터 송,수신 함수 호출 시 타임아웃 값 설정|
	| SO_REUSEADDR |BOOL |O |O |지역 주소(IP 주소, 포트 번호) 재사용 허용 여부|

- IPPROTO_IP

	| optname |optval |get |set |설명 |
	|:--------|:------|:---|:---|:----|
	| IP_HDRINCL |BOOL |O |O |데이터 송신 시 IP 헤더 직접 포함 여부|
	| IP_TTL |int |O |O |IP 패킷의 TTL값 설정|
	| IP_MULTICAST_IF |IN_ADDR{} |O |O |멀티캐스트 패킷을 보낼 인터페이스 선택|
	| IP_MULTICAST_LOOF |BOOL |O |O |멀티캐스트 패킷의 루프백 여부 (자신이 보낸 패킷을 자신도 받는지)|
	| IP_ADD_MEMBERSHIP|ip_mreq{} |X |O |멀티캐스트 그룹 가입|
	| IP_DROP_MEMBERSHIP |ip_mreq{} |X |O |멀티캐스트 그룹 탈퇴|

- IPPROTO_IPV6

	| optname |optval |get |set |설명 |
	|:--------|:------|:---|:---|:----|
	| IPV6_HDRINCL |BOOL |O |O |데이터 송신 시 IP 헤더 직접 포함 여부|
	| IPV6_UNICAST_HOPS |int |O |O |IP 패킷의 TTL 값 설정|
	| IPV6_MULTICAST_IF |DWORD |O |O |멀티캐스트 패킷을 보낼 인터페이스 선택|
	| IPV6_MULTICAST_HOPS |int |O |O |멀티캐스트 패킷의 TTL 값 설정|
	| IPV6_MULTICAST_LOOP |BOOL |O |O |멀티캐스트 패킷의 루프백 여부|
	| IPV6_ADD_MEMBERSHIP |ipv6_mreq{} |O |O |멀티캐스트 그룹 가입| 
	| IPV6_DROP_MEMBERSHIP |ipv6_mreq{} |O |O |멀티캐스트 그룹 탈퇴|

- IPPROTO_TCP

	| optname |optval |get |set |설명 |
	|:--------|:------|:---|:---|:----|
	|TCP_NODELAY |BOOL |O |O |Nagle 알고리즘 작동 여부|

 #### 주요 SOL_SOCKET Level Option 설명
1. SO_BROADCAST  
	해당 소켓을 이용해 브로드캐스트 데이터를 보낼 수 있다.  
	프로토콜 특성상 TCP 소켓에는 사용할 수 없고, UDP 소켓에만 사용할 수 있다.
	```
	사용예
	BOOL bEnable = TRUE;
	retval = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&bEnable, sizeof(bEnable));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	```

2. SO_KEEPALIVE  
	TCP 프로토콜 수준에서 연결 여부를 확인하려고 상대 TCP에 주기적으로(약 2시간) TCP 패킷을 보낸다.  
	이때 상대 TCP 반응에 따라
	
	- 정해진 시간 안에 응답하는 경우  
	TCP 연결에 문제가 없는 상태므로 정상적으로 동작
	
	- 정해진 사간 안에 응답하지 않는 경우  
	TCP 연결에 일시적 문제가 있는 상태로 간주할 수 있으므로 몇 번 더 TCP 패킷을 보내보고 그럼에도 응답이 없다면 자동으로 소켓을 닫아 시스템 자원 소모를 막는다.  
	나중에 응용 프로그램이 해당 소켓 사용시 오류가 발생한다.
	
	- RST(reset) 패킷으로 응답하는 경우  
	TCP는 예상치 못한 엉뚱한 패킷이 도달하면 비정상적인 상황으로 간주해 상대편에 RST 패킷을 보냄으로써 연결을 강제 종료한다는 특징이 있다.  
	RST 패킷이 도달하면 사실상 연결이 끊어진 것으로 자동으로 소켓을 닫아 시스템 자원 소모를 막는다.  
	나중에 응용 프로그램이 해당 소켓 사용시 오류가 발생한다.

	```
	사용예
	BOOL bEnable = TRUE;
	retval = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&bEnable, sizeof(bEnable));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	``` 

	SO_KEEPALIVE 옵션은 TCP 소켓에만 사용 가능하다.  
	일반적으로 TCP 서버의 연결 대기 소켓에 대해 설정하며, 이 경우 accept() 함수가 리턴하는 소켓도 자동으로 SO_KEEPALIVE 옵션이 설정된 상태로 만들어진다.  
	TCP 연결은 단순한 물리적 연결이 아닌 연결 설정 정보를 주고 받아 상태 정보를 일치시킨 상태로 통신하는 논리적 연결이다.  
	따라서 데이터 교환이 없다면 연결이 끊어진 경우를 감지 못하는 특성이 있다.  
	또한 접속 클라이언트 수에 비례하여 소켓을 생성하여 시스템 자원 소모가 늘어나므로 연결이 끊어진 소켓은 바로바로 닫아야 한다.

3. SO_LINGER  
	소켓 송신 버퍼에 미전송 데이터가 있을 때 closesocket() 함수의 리턴 지연 시간을 제어할 수 있다.  
	TCP 소켓 사용시, closesocket() 함수는 두 가지 기능을 한다.
	
	1. 소켓을 닫고 할당된 운영체제 자원을 반환한다.  
	따라서 closesocket() 함수 리턴 후 해당 소켓을 통신에 사용할 수 없다.
	2. TCP 프로토콜 수준에서 연결 종료 절차를 시작한다.  
	즉 TCP 연결 종료를 위한 최초 패킷(FIN)을 상대방에게 보낸다.  

	그런데 closesocket() 함수 호출 시점에 send() 함수를 보내려 했던 이전 데이터가 송신 버퍼에 아직 남아있다면 다음 방식으로 동작 옵션을 줄 수 있다.
	
	1. closesocket() 함수는 곧바로 리턴하고, 송신 버퍼의 데이터를 백그라운드로 모두 보내고 TCP 연결을 정상 종료한다.
	2. closesocket() 함수는 곧바로 리턴하고, 송신 버퍼의 데이터를 삭제하고 TCP 연결을 강제 종료한다.
	3. 송신 버퍼의 데이터를 모두 보내고 TCP 연결을 정상 종료한 후 closesocket() 함수가 리턴한다.  
	만약 일정 시간 안에 송신 버퍼의 데이터를 모두 보내지 못하면 TCP 연결을 강제 종료한 후 closesocket() 함수가 리턴한다.  
	이때 송신 버퍼에 남은 데이터는 삭제한다.

	SO_LINGER 옵션 값으로는 linger 구조체를 사용한다.
	```
	struct linger{
		u_short l_onoff;
		u_short l_linger;   
	}
	typedef struct linger LINGER;
	```
	- l_onoff  
	값이 0이면 closesocket() 함수는 곧바로 리턴한다.  
	0이 아니면 l_linger 멤버로 설정한 시간(초 단위)동안 리턴하지 않고 대기한다.

	- l_linger  
	closesocket() 함수가 리턴하지 않고 대기할 시간을 초 단위로 설정한다.  
	0 또는 양의 정수값을 사용한다.


	| LINGER |         |closesocket()|
	|:------|:---------|:------------|
	| l_onoff| l_linger|
	| 0      | 사용안함| 동작 (1) default|
	| 1      | 0       | 동작 (2)    |
	| 1      | 양의 정수| 동작 (3)   |

	```
	사용예
	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 10;
	retval = setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	```

	TCP 소켓에만 사용할 수 있다.  
	연결 대기 소켓에 대해 한 번만 설정해두면 accept() 함수가 리턴하는 소켓도 자동으로 SO_LINGER 옵션이 설정된 상태로 만들어진다.

4. SO_SNDBUF, SO_RCVBUF  
	운영체제가 소켓에 할당하는 송,수신 버퍼의 크기를 변경할 수 있다.  
	버퍼의 크기를 무한정 늘릴 수 없으므로 제대로 설정됐는지 확인이 필요하다. (getsockopt() 함수 사용)  
	권장 사항은, UDP 소켓은 소켓 버퍼 크기를 언제든 변경해도 되지만 TCP 소켓은 연결이 이루어지기 전에 변경하는 것이다.  
	즉, TCP 서버에서는 listen() 함수, TCP 클라이언트에서는 connect() 함수 호출 전에 옵션을 설정하는 것이 좋다.  
	서버에 옵션 적용시 연결 대기 소켓에 대해 한번만 설정해두면 accept() 함수가 리턴하는 소켓도 자동으로 같은 옵션이 설정된다.

	```
	사용예
	int optval, optlen;
	
	//현재 수신 버퍼값 확인
	optlen = sizeof(optval);
	retval = getsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &optlen);
	if(retval == SOCKET_ERROR) err_quit("getsockopt()");
	
	//수신 버퍼값 2배로 늘려 적용
	optval *= 2;
	retval = setsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(optval));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	
	//수신 버퍼값 2배로 늘려졌는지 확인
	optlen = sizeof(optval);
	retval = getsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &optlen);
	if(retval == SOCKET_ERROR) err_quit("getsockopt()");
	```

5. SO_SNDTIMEO, SO_RCVTIMEO  
	socket() 함수로 만든 소켓은 블로킹 소켓이므로, 데이터 전송 함수 호출 시 조건이 만족되지 않으면 무한정 블록되어 교착 상태가 발생할 수 있다.  
	이때 SO_SNDTIMEO, SO_RCVTIMEO 옵션으로 타임아웃을 설정해두면, 데이터 전송 함수(send, recv, sendto, recvfrom 등)가 작업 완료와 상관 없이 일정 시간 후에 리턴하게 된다.  
	옵션 값으로는 정수형을 사용하며, 밀리초단위로 타임아웃을 지정한다.

	```
	사용예
	int optval = 3000; //3초
	retval = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&optval, sizeof(optval));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	```

	TCP, UDP 소켓에 모두 사용 가능하다.  
	교착 상태 방지 목적외에 상대가 일정 시간안에 응답하는지 여부를 알고 싶을때도 사용할 수 있다. (ex : Ping)

6. SO_REUSEADDR  
	현재 사용 중인 IP 주소와 포트 번호를 재사용할 수 있다.  
	다시 말해 현재 사용 중인 IP 주소, 포트 번호를 이용해 bind() 함수를 (성공적으로) 호출할 수 있다.  
	TCP, UDP 소켓 모두 사용 가능하다.  

	SO_REUSEADDR 옵션의 목적
	1. TCP 서버 종료 후 재실행 시 bind() 함수에서 오류가 발생하는 일을 방지한다.  
	unix / linux에서 멀티 프로세스 기반으로 작성한 TCP 서버를 실행할 때 종종 발생하는 문제다.  
	새로운 접속이 이루어지면 fork() 함수로 자식 프로세스를 생성하여 요청을 처리하는데 TCP 서버 문제가 발생해 강제 종료되더라도 자식 프로세스가 남아 포트 번호를 계속 사용하므로 서버를 재실행시 bind() 오류가 발생한다.  
	단일 프로세스로 작성한 TCP 서버에서도 발생할 수 있는데 이는 서버의 비정상 종료로 인해 지역 주소(IP, PORT)의 TCP 상태가 TIME_WAIT에 머물러 있기 때문이다.  
	unix / linux에서는 바인딩 오류가 흔한 편이지만 windows에서는 상대적으로 드물다.  
	하지만 코드 이식성을 고려해 서버 작성시 옵션 설정을 하는것이 좋다.

	2. 여러 IP 주소를 보유한 호스트에서 같은 기능의 서버를 IP 주소별로 따로 운영할 수 있게한다.  
	unix / linux에서 서버 실행 시 IP 주소가 달라도 포트 번호만 같으면 바인딩 오류가 발생한다.  
	여러 IP 주소를 보유한 호스트에서 같은 기능의 서버를 IP 별로 따로 운영하고 싶다면 SO_REUSEADDR 옵션을 설정해야 한다.

	3. 멀티캐스팅 응용 프로그램이 같은 포트 번호를 사용할 수 있게 한다.  
	이는 UDP 서버에 한정해 활용한다. (TCP는 멀티캐스팅을 할 수 없다.)  

	```
	사용예
	BOOL optval = TRUE;
	retval = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	```

#### 주요 IPPROTO_IP, IPPROTO_IPV6 Level Option 설명
1. IP_MULTICAST_IF, IPV6_MULTICAST_IF  
	IP 주소를 둘 보유한 호스트에서 멀티캐스트 데이터를 보낼 네트워크 인터페이스를 선택할 때 사용한다.  
	설정하지 않으면 IP 라우팅 기능에 따라 각 패킷별로 적절한 네트워크 인터페이스가 선택되어 전송된다.
	```
	사용예
	//특정 IP 주소 (149.52.121.68)로 멀티캐스트 데이터를 보내도록 설정
	IN_ADDR local_addr;
	local_addr.s_addr = inet_addr("149.52.121.68");
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&local_addr, sizeof(local_addr));
	```
	
2. IP_MULTICAST_TTL, IPV6_MULTICAST_TTL  
	멀티캐스트 패킷이 생성될 때 IP 헤더의 TTL 필드는 기본값 1로 설정된다.  
	TTL은 라우터를 지날때마다 1씩 감소하는데 0이되면 패킷이 폐기된다.  
	따라서 TTL = 1은 라우트 경계를 넘어갈 수 없음을 뜻한다.  
	만약 라우터 경계를 넘어 멀티캐스트 패킷을 보내려면 이 옵션을 사용해야 한다.
	```
	사용예
	int ttl = 2;
	retval = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	```

3. IP_MULTICAST_LOOP, IPV6_MULTICAST_LOOP  
	멀티캐스트 그룹에 가입한 응용 프로그램이 자신의 그룹에 멀티캐스트 데이터를 보내면 자기 자신도 받는다.(default)  
	```
	사용예
	//자신이 보낸 멀티캐스트 데이터를 받지 않는다.
	BOOL optval = FALSE;
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&optval, sizeof(optval));
	```

4. IP_ADD_MEMBERSHIP / IP_DROP_MEMBERSHIP, IPV6_ADD_MEMBERSHIP / IPV6_ADD_MEMBERSHIP  
	멀티캐스트 데이터를 받으려면 반드시 멀티캐스트 그룹에 가입해야 한다.  
	IP*_ADD_MEMBERSHIP은 가입할때, IP*_DROP_MEMBERSHIP은 탈퇴할때 사용한다.  
	IPv4는 ip_merq 구조체를, IPv6는 ipv6_merq 구조체를 옵션 값으로 사용한다.  

	```
	#include <ws2tcpip.h>
	
	typedef struct ip_mreq {
		IN_ADDR imr_multiaddr;
		IN_ADDR imr_interface;
	} IP_MREQ;

	typedef struct ipv6_mreq {
		IN6_ADDR ipv6mr_multiaddr;
		ULONG ipv6mr_interface;
	} IPV6_MREQ;
	```
	- imr_multiaddr  
	가입하거나 탈퇴할 IPv4 멀티캐스트 주소

	- imr_interface  
	멀티캐스트 패킷을 받을 네트워크 인터페이스 IPv4 주소  
	INADDR_ANY(= 0)를 전달해 자동으로 선택되게 하는 것이 일반적

	- ipv6mr_multiaddr  
	가입하거나 탈퇴할 IPv6 멀티캐스트 주소

	- ipv6mr_interface  
	멀티캐스트 패킷을 받을 네트워크 인터페이스의 인덱스  
	0을 전달해 자동으로 선택되게 하는 것이 일반적

	```
	사용예 
	
	//IPv4
	//멀티캐스트 주소 구조체 생성
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(MULTICATIP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	
	//멀티캐스트 그룹 가입
	retval = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	
	//멀티캐스트 그룹 탈퇴
	retval = setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");

	//IPv6
	//주소 변환(문자열 -> IPv6)
	SOCKADDR_IN6 temp_addr;
	int addrLen = sizeof(temp_addr);
	WSAStringToAddress(MULTICASTIP, AF_INET6, NULL, (SOCKADDR*)&temp_addr, &addrLen);

	//멀티캐스트 주소 구조체 생성
	struct ipv6_mreq mreq;
	mreq.ipv6_multiaddr = temp_addr.sin6_addr;
	mreq.ipv6_interface = 0;

	//멀티캐스트 그룹 가입
	retval = setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");

	//멀티캐스트 그룹 탈퇴
	retval = setsockopt(sock, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
	if(retval == SOCKET_ERROR) err_quit("setsockopt()");
	```

#### 주요 IPPROTO_TCP Level Option 설명  
1. TCP_NODELAY  
	Nagle 알고리즘 작동을 중지하는 역할을 한다.  
	Nagle 알고리즘은 작은 패킷이 불필요하게 많이 생성되는 일을 방지해 네트워크 트래픽을 감소시키는 알고리즘이다.  
	TCP 전송 효율과 네트워크 활용도를 높인다는 장점 때문에 TCP 프로토콜에서는 기본 동작이다.

	TCP는 신뢰성 있는 데이터 전송 기법 중 하나로 ACK에 기반한 데이터 재전송을 한다.  
	즉, 데이터를 보낸 후 상대방이 잘 받았는지 ACK 패킷을 통해 확인하고, 그렇지 않을 경우 재전송한다.  
	그런데 데이터를 보낼 때마다 ACK가 도착할 때까지 대기하면 네트워크 대역폭 활용도가 낮아지는 문제가 있다.  
	TCP에서는 네트워크 자원을 효율적으로 활용하기위해 슬라이딩 윈도우 방식을 사용한다.  
	이 방식은 ACK를 받지 못해도 윈도우 크기만큼 데이터를 계속 보낼 수 있으므로 성능을 높일 수 있다.  
	여기에 추가적으로 각 데이터에 대해 상대방이 ACK로 응답할 것을 요구하면 성능이 떨어지기에 마지막으로 성공적으로 받은 데이터에 대해서만 ACK로 응답하는 방식을 사용한다.

	- Nagle 알고리즘 동작 방식
	1. 보낼 데이터가 MSS(Maximum Segment Size)로 정의된 크기만큼 쌓이면 상대편에 무조건 보낸다.  
	이 경우 슬라이딩 윈도우 방식으로 데이터를 계속 보낼 수 있다.

	2. 보낼 데이터가 MSS보다 작으면 이전에 보낸 데이터에 대한 ACK가 오기를 기다린다.  
	ACK가 도착하면 보낼 데이터가 MSS보다 작더라도 상대편에 보낸다.  
	이 경우 데이터를 전송할 때마다 ACK를 기다리고, ACK를 받으면 다음 데이터를 전송하는 동작을 반복한다.

	요약하자면 데이터가 충분히 크면 곧바로 보내고, 그렇지 않으면 데이터가 쌓일 때까지 대기한다.  
	단, 데이터가 충분히 쌓이지 않았더라도 이전에 보낸 데이터를 상대방이 받았다면 다음 데이터를 보낸다.  

	- Nagle 알고리즘 장점  
	작은 패킷이 불필요하게 많이 생성되는 일을 방지해 네트워크 트래픽을 감소시킨다.

	- Nagle 알고리즘 단점  
	데이터가 충분히 쌓일 때까지 또는 ACK가 도달할 때까지 대기하는 시간 때문에 응용 프로그램의 반응 시간이 길어질 수 있다.

	TCP_NODELAY 옵션은 Nagle 알고리즘의 장점을 포기하는 대신 응용 프로그램의 반응 속도를 빠르게 할 때 사용한다.

	```
	사용예
	//Nagle 알고리즘 사용안함
	BOOL optval = TRUE;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
	```