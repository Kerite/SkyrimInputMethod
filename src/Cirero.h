#pragma once

#include <msctf.h>

#define CICERO_DISABLED 0
#define CICERO_ENABLED 1

class Cicero :
	public Singleton<Cicero>,
	public ITfUIElementSink,
	public ITfInputProcessorProfileActivationSink,
	public ITfTextEditSink,
	public ITfThreadMgrEventSink
{
public:
	// ITfUIElementSink
	STDMETHODIMP BeginUIElement(DWORD dwUIElementId, BOOL* pbShow);
	STDMETHODIMP UpdateUIElement(DWORD dwUIElementId);
	STDMETHODIMP EndUIElement(DWORD dwUIElementId);

	// ITfTextEditSink
	STDMETHODIMP OnEndEdit(ITfContext* cxt, TfEditCookie ecReadOnly, ITfEditRecord* pEditRecord);

	// ITfInputProcessorProfileActivationSink
	STDMETHODIMP OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags);

	// ITfThreadMgrEventSink
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

	int ciceroState;

private:
	TfClientId m_clientID;
	ITfThreadMgr* m_pThreadMgr;
	ITfThreadMgrEx* m_pThreadMgrEx;
	ITfInputProcessorProfiles* m_pProfiles;
	ITfInputProcessorProfileMgr* m_pProfileMgr;
	ITfContext* m_pBaseContext;

	DWORD m_uiElementSinkCookie;
	DWORD m_inputProcessorProfileActivationSinkCookie;
	DWORD m_textEditSinkCookie;
	DWORD m_threadMgrEventSinkCookie;

	ULONG m_refCount;

	void UpdateCurrentInputMethodName();
};