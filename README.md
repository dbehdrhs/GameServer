# GameServer

Windows IOCP 기반 멀티스레드 게임 서버입니다.  
네트워크 / 게임 로직 / DB 처리를 각각 독립된 스레드로 분리하고, Lock-free 큐로 스레드 간 통신합니다.

---

## 아키텍처

```
[Client]
   │  TCP (WinSock2)
   ▼
┌─────────────────────────────┐
│        IOCP Thread Pool      │  (CPU 코어 수 × 2)
│  AcceptEx / WSARecv / WSASend│
│         (CIOCP)              │
└────────────┬────────────────┘
             │ StreamData (wUserID + raw packet)
             ▼
        CBuffer (Queue)          ← CRITICAL_SECTION + Event
             │
             ▼
┌─────────────────────────────┐
│       GameServer Thread      │  (1개)
│  패킷 파싱 / 유저 상태 관리  │
│       (CGameServer)          │
└──────┬───────────────┬───────┘
       │               │
       │ DB 요청       │ DB 결과
       ▼               ▲
  CBuffer(DB요청큐)  CBuffer(DB결과큐)
       │               │
       ▼               │
┌─────────────────────────────┐
│          DB Thread           │  (1개)
│  MySQL Prepared Statement    │
│           (CDB)              │
└─────────────────────────────┘
```

---

## 핵심 구현

### IOCP (CIOCP)
- `AcceptEx`로 비동기 Accept 처리 (소켓 풀 사전 할당)
- `TransmitFile(TF_DISCONNECT | TF_REUSE_SOCKET)`로 소켓 재사용
- Completion Key로 클라이언트 ID를 직접 전달해 O(1) 조회
- Worker 스레드 수 = CPU 코어 × 2 (자동 감지)

### 스레드 간 통신 (CBuffer)
- `std::queue` + `CRITICAL_SECTION` + `CreateEvent`로 구현
- GameServer 스레드는 `WaitForMultipleObjects`로 패킷 큐·DB 결과 큐를 동시에 감시
- 동일 구조로 IOCP→Game, Game→DB, DB→Game 세 큐 운영

### 패킷 구조 (Packet.h)
- `#pragma pack(1)`로 패킷 정렬 없이 직렬화
- 모든 패킷은 `PACKET_HEADER(wPacketSize + packetID)` 상속
- 수신 측에서 헤더 ID로 핸들러 테이블 조회 → O(1) 디스패치

```
[PACKET_HEADER] wPacketSize(2) | packetID(2)
[CS_LOGIN]      HEADER + szID(32) + szPassword(32)
[SC_LOGIN]      HEADER + bSuccess(4)
[CS_TEST]       HEADER + nAccountID(4) + nCharacterID(4) + nItemID(4)
```

### 패킷 핸들러 테이블 (CGameServer_Packet.cpp)
- 멤버 함수 포인터 배열 `m_handlers[PACKET_MAX]`로 패킷 ID → 핸들러 매핑
- 새 패킷 추가 시 `RegisterHandlers()`에 한 줄만 추가하면 되고, 디스패치 로직은 불변

```cpp
// 새 패킷 추가 예시
m_handlers[PACKET_ROOM_JOIN] = &CGameServer::PacketProcessRoomJoin;
```

### 로그인 플로우
```
Client  →  CS_LOGIN(ID/PW)
         →  [IOCP Thread] RecvProcess → CBuffer(Game큐)
         →  [Game Thread] PacketProcessLogin → CBuffer(DB큐)
         →  [DB Thread]   QueryLogin (Prepared Statement)
                          → CBuffer(결과큐)
         →  [Game Thread] DBResultProcess → CIOCP::SendData
         →  SC_LOGIN(bSuccess) → Client
```

### DB (CDB)
- MySQL C API + Prepared Statement (SQL Injection 방지)
- 접속 정보는 `Info.ini`에서 `GetPrivateProfileString`으로 로드 (하드코딩 제거)
- DB 스레드는 게임 스레드와 완전히 분리되어 쿼리 지연이 게임 루프에 영향 없음

### 유저 상태 머신 (CUser)
```
READY → (접속) → CONNECT → (로그인 완료) → LOGIN_COMPLETE
                               ↓
                          (로그아웃/접속끊김)
                               ↓
                            READY
```
- `LOGIN_COMPLETE` 미만 유저의 패킷은 GameServer에서 즉시 드롭

---

## 기술 스택

| 항목 | 내용 |
|------|------|
| 언어 | C++ (Visual Studio) |
| 네트워크 | WinSock2, IOCP, AcceptEx, WSARecv/WSASend |
| DB | MySQL 8.x, C API (Prepared Statement) |
| 동기화 | CRITICAL_SECTION, CreateEvent, WaitForSingleObject |
| 빌드 | Visual Studio 2019+, Windows SDK |

---

## 빌드 및 실행

### 사전 준비
1. MySQL 8.x 설치 후 `accountdb` 스키마 생성
2. `Server/CreateDataTable.sql` 실행
3. `Server/GameServer/Info.ini`에 DB 접속 정보 입력

```ini
[Server]
Port=8989

[DB]
Host=127.0.0.1
Port=3306
User=root
Password=your_password
Schema=accountdb
```

### 빌드
```
Server/dbehdrhs.sln → Visual Studio에서 열기 → Build Solution (Ctrl+Shift+B)
```

### 실행
1. `GameServer.exe` 실행
2. `TestClient.exe` 실행 (Login → Test packets × 10 → Logout 자동 수행)

---

## 프로젝트 구조

```
Server/
├── GameServer/
│   ├── CIOCP.h/cpp              # IOCP 네트워크 레이어
│   ├── CGameServer.h            # GameServer 클래스 선언
│   ├── CGameServer.cpp          # 생성/소멸, Start/Stop, Worker, Connect
│   ├── CGameServer_Packet.cpp   # 패킷 핸들러 테이블 및 패킷 처리
│   ├── CGameServer_DB.cpp       # DB 요청/결과 처리
│   ├── CDB.h/cpp                # DB 처리 스레드
│   ├── CBuffer.h/cpp            # 스레드 간 메시지 큐
│   ├── CUser.h/cpp              # 유저 상태 관리
│   ├── SocketContext.h/cpp      # 소켓 컨텍스트 (AcceptEx/Recv/Send)
│   ├── Packet.h                 # 패킷 구조체 정의
│   ├── PacketEnum.h             # 패킷 ID 열거형
│   ├── ServiceManager.h/cpp     # 서버 시작/종료 관리
│   └── Info.ini                 # 서버/DB 설정
├── TestClient/
│   └── TestClient.cpp           # 더미 클라이언트 (Login~Logout 자동화)
└── CreateDataTable.sql          # DB 스키마
```