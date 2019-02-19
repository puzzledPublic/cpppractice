#pragma once

#include <thread>
#include <iostream>
#include <Windows.h>

typedef void(WINAPI * PFN_WICB)(PVOID pParma);

class IocpThreadPool {
	static DWORD WINAPI IocpWorkerProc(PVOID pParam);

	HANDLE m_hIocp;
	int m_nThrCnt;
	PHANDLE m_parhThrs;

public:
	void Init(int nMaxCnt = 0, int nConcurrentCnt = 0) {
		m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, (DWORD)nConcurrentCnt);
		m_nThrCnt = nMaxCnt;
		if (m_nThrCnt == 0) {
			if (nConcurrentCnt > 0) {
				m_nThrCnt = nConcurrentCnt * 2;
			}
			else {
				m_nThrCnt = std::thread::hardware_concurrency() * 2;
			}
		}
		m_parhThrs = new HANDLE[m_nThrCnt];
		for (int i = 0; i < m_nThrCnt; i++) {
			DWORD dwThreadId;
			m_parhThrs[i] = CreateThread(NULL, 0, IocpWorkerProc, this, 0, &dwThreadId);
		}
	}

	void Uninit() {
		for (int i = 0; i < m_nThrCnt; i++) {
			PostQueuedCompletionStatus(m_hIocp, 0, NULL, NULL);
		}
		WaitForMultipleObjects(m_nThrCnt, m_parhThrs, TRUE, INFINITE);
		for (int i = 0; i < m_nThrCnt; i++) {
			CloseHandle(m_parhThrs[i]);
		}
		delete[] m_parhThrs;

		if (m_hIocp != nullptr) {
			CloseHandle(m_hIocp);
			m_hIocp = nullptr;
		}
	}
	void Enqueue(PFN_WICB pfnCB, PVOID pParam) {
		PostQueuedCompletionStatus(m_hIocp, 0, (ULONG_PTR)pfnCB, (LPOVERLAPPED)pParam);
	}
};

DWORD WINAPI IocpThreadPool::IocpWorkerProc(PVOID pParam) {

	IocpThreadPool* pTP = (IocpThreadPool*)pParam;
	LPOVERLAPPED pov = nullptr;
	DWORD dwTrBytes = 0;
	ULONG_PTR ulKey = 0;
	while (true) {
		BOOL bIsOK = GetQueuedCompletionStatus(pTP->m_hIocp, &dwTrBytes, &ulKey, &pov, INFINITE);
		if (!bIsOK) {
			break;
		}
		if (ulKey == 0) {
			break;
		}

		PFN_WICB pfnCB = (PFN_WICB)ulKey;
		PVOID param = (PVOID)pov;
		try {
			pfnCB(param);
		}
		catch (std::exception e) {
			std::cout << "UnKnown exception occurred : " << e.what() << std::endl;
		}
	}

	return 0;
}