#pragma once

#include <msctf.h>

#define CICERO_DISABLE 0
#define CICERO_ENABLE 1

// https://github.com/walbourn/directx-sdk-samples/blob/main/DXUT/Optional/ImeUi.cpp
// https://github.com/kassent/ChineseInput/blob/master/ChineseInput/Cicero.cpp
// Cicero UI-Less Mode
class Cicero :
	public Singleton<Cicero>,
	public ITfUIElementSink,
	public ITfInputProcessorProfileActivationSink,
	public ITfTextEditSink,
	public ITfThreadMgrEventSink
{
public:
	//ITfUIElementSink
	STDMETHODIMP BeginUIElement(DWORD dwUIElementId, BOOL* pbShow);
	STDMETHODIMP UpdateUIElement(DWORD dwUIElementId);
	STDMETHODIMP EndUIElement(DWORD dwUIElementId);

	//ITfTextEditSink
	STDMETHODIMP OnEndEdit(ITfContext* cxt, TfEditCookie ecReadOnly, ITfEditRecord* pEditRecord);

	//ITfInputProcessorProfileActivationSink
	STDMETHODIMP OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags);

	//ITfThreadMgrEventSink
	STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* pdim);
	STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* pdim);
	STDMETHODIMP OnSetFocus(ITfDocumentMgr* pdimFocus, ITfDocumentMgr* pdimPrevFocus);
	STDMETHODIMP OnPushContext(ITfContext* pic);
	STDMETHODIMP OnPopContext(ITfContext* pic);

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
	STDMETHODIMP_(ULONG)
	AddRef(void);
	STDMETHODIMP_(ULONG)
	Release(void);

	Cicero();
	~Cicero();

	HRESULT SetupSinks();
	void ReleaseSinks();

	ITfUIElement* GetUIElement(DWORD dwUIElementId);
	void UpdateCandidateList(ITfCandidateListUIElement* lpCandidate);

	bool bCiceroState;
	bool bCOMInitialized;

private:
	ITfThreadMgr* m_pThreadMgr;
	ITfThreadMgrEx* m_pThreadMgrEx;
	ITfInputProcessorProfiles* m_pProfiles;
	ITfInputProcessorProfileMgr* m_pProfileMgr;
	ITfContext* m_pBaseContext;

	TfClientId m_dwClientID;

	DWORD m_dwUIElementSinkCookie;
	DWORD m_dwInputProcessorProfileActivationSinkCookie;
	DWORD m_dwTextEditSinkCookie;
	DWORD m_dwThreadMgrEventSinkCookie;

	ULONG m_lRefCount;

	HRESULT UpdateCurrentInputMethodName();
	void UpdateCiceroState(DWORD dwProfileType, HKL hkl);
};