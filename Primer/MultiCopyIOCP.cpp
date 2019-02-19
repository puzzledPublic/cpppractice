#include <iostream>
#include <memory>
#include <thread>
#include <Windows.h>
#include <string>
#include <atomic>
#include <mutex>

std::mutex mut;
const int BUF_SIZE = 65536;
const int BEGIN_KEY = 0;
const int READ_KEY = 1;
const int WRITE_KEY = 2;

struct COPY_CHUNK : OVERLAPPED {
	HANDLE _hfSrc, _hfDst;
	BYTE _arBuf[BUF_SIZE];
	COPY_CHUNK(HANDLE hfSrc, HANDLE hfDst) {
		memset(this, 0, sizeof(*this));
		_hfSrc = hfSrc;
		_hfDst = hfDst;
	}
};

struct COPY_ENV {
	HANDLE _hIocp;
	std::atomic<int> _nCpCnt;
	HANDLE _hevEnd;
};

void IOCPCopyProc(COPY_ENV& env);

int main() {
	std::string srcs[] = { "./src.txt", "./src2.txt", "./src3.txt", "./src4.txt", "./src5.txt", "./src6.txt", "./src7.txt", "./src8.txt" };
	std::string dsts[] = { "./dst.txt", "./dst2.txt", "./dst3.txt", "./dst4.txt", "./dst5.txt", "./dst6.txt", "./dst7.txt", "./dst8.txt" };
	COPY_CHUNK* arChunk[10];
	memset(arChunk, 0, sizeof(COPY_CHUNK*) * 10);

	COPY_ENV env;
	env._nCpCnt = 0;
	env._hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
	env._hevEnd = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (int i = 0; i < 8; i++) {
		HANDLE hSrcFile = CreateFile(srcs[i].c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (hSrcFile == INVALID_HANDLE_VALUE) {
			std::cout << srcs[i] << " open failed, code : " << GetLastError() << std::endl;
			return 0;
		}

		HANDLE hDstFile = CreateFile(dsts[i].c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);

		if (hDstFile == INVALID_HANDLE_VALUE) {
			std::cout << dsts[i] << " open failed, code : " << GetLastError() << std::endl;
			return 0;
		}

		CreateIoCompletionPort(hSrcFile, env._hIocp, READ_KEY, 0);
		CreateIoCompletionPort(hDstFile, env._hIocp, WRITE_KEY, 0);

		arChunk[i] = new COPY_CHUNK(hSrcFile, hDstFile);
		env._nCpCnt++;
	}

	LONG lChnCnt = env._nCpCnt;
	std::thread harWorks[2];
	for (int i = 0; i < 2; i++) {
		harWorks[i] = std::thread(IOCPCopyProc, std::ref<COPY_ENV>(env));
	}

	for (int i = 0; i < lChnCnt; i++) {
		/*BOOL bIsOK = ReadFile(arChunk[i]->_hfSrc, arChunk[i]->_arBuf, BUF_SIZE, NULL, arChunk[i]);
		if (!bIsOK) {
			DWORD dwErrCode = GetLastError();
			if (dwErrCode != ERROR_IO_PENDING) {
				break;
			}
		}*/
		PostQueuedCompletionStatus(env._hIocp, 0, BEGIN_KEY, arChunk[i]);	//PostQueuedCompletionStatus를 사용한 워커스레드 동작유도.
	}

	WaitForSingleObject(env._hevEnd, INFINITE);

	CloseHandle(env._hIocp);
	for (int i = 0; i < lChnCnt; i++) {
		delete arChunk[i];
	}
	for (int i = 0; i < 2; i++) {
		harWorks[i].join();
	}
	CloseHandle(env._hevEnd);

	return 0;
}

void IOCPCopyProc(COPY_ENV& env) {
	
	while (true) {
		DWORD dwErrCode = 0;
		COPY_CHUNK* pcc;
		DWORD dwTrBytes = 0;	//IO 실행한 바이트 수
		ULONG_PTR ulkey;

		BOOL bIsOk = GetQueuedCompletionStatus(env._hIocp, &dwTrBytes, &ulkey, (LPOVERLAPPED*)&pcc, INFINITE);

		if (!bIsOk) {
			if (pcc == nullptr) {
				break;
			}
			dwErrCode = GetLastError();
			goto $LABEL_CLOSE;
		}
		if (ulkey == READ_KEY) {
			{
				std::lock_guard<std::mutex> lk(mut);
				std::cout << " => Thr " << std::this_thread::get_id() << " Read bytes : " << dwTrBytes << "\n";
			}
			bIsOk = WriteFile(pcc->_hfDst, pcc->_arBuf, dwTrBytes, NULL, pcc);
		}
		else {
			if (ulkey != BEGIN_KEY) {
				pcc->Offset += dwTrBytes; //다음 읽을 위치 갱신.
				{
					std::lock_guard<std::mutex> lk(mut);
					std::cout << " <= Thr " << std::this_thread::get_id() << " Wrote bytes : " << pcc->Offset << "\n";	//offset -> 파일의 오프셋
				}
			}
			bIsOk = ReadFile(pcc->_hfSrc, pcc->_arBuf, BUF_SIZE, NULL, pcc);
		}

		if (!bIsOk) {
			dwErrCode = GetLastError();
			if (dwErrCode != ERROR_IO_PENDING) {
				goto $LABEL_CLOSE;
			}
		}
		continue;

	$LABEL_CLOSE:
		if (dwErrCode == ERROR_HANDLE_EOF) {
			std::lock_guard<std::mutex> lk(mut);
			std::cout << " ****** Thr " << std::this_thread::get_id() << " copy successfully completed..." << "\n";
		}
		else {
			std::lock_guard<std::mutex> lk(mut);
			std::cout << " ##### Thr " << std::this_thread::get_id() << " copy failed, code : " << dwErrCode << "\n";
		}

		CloseHandle(pcc->_hfSrc);
		CloseHandle(pcc->_hfDst);

		if (--env._nCpCnt == 0) {
			SetEvent(env._hevEnd);
		}
	}
}