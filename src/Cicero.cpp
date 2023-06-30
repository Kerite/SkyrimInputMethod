#include "Cirero.h"

#include "Helpers/DebugHelper.h"
#include "InputPanel.h"
#include "Utils.h"

#include "Config.h"

bool GetLayoutName(const WCHAR* kl, WCHAR* nm)
{
	long lRet;
	HKEY hKey;
	static WCHAR tchData[64];
	DWORD dwSize;
	WCHAR keyPath[200];
	wsprintfW(keyPath, L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s", kl);
	lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_QUERY_VALUE, &hKey);
	if (lRet == ERROR_SUCCESS) {
		dwSize = sizeof(tchData);
		lRet = RegQueryValueExW(hKey, L"Layout Text", NULL, NULL, (LPBYTE)tchData, &dwSize);
	}
	RegCloseKey(hKey);
	if (lRet == ERROR_SUCCESS && wcslen(nm) < 64) {
		wcscpy(nm, tchData);
		return true;
	}
	return false;
}

Cicero::Cicero() :
	bCOMInitialized(false),
	bCiceroState(CICERO_DISABLE),
	m_pProfileMgr(nullptr),
	m_pProfiles(nullptr),
	m_pThreadMgr(nullptr),
	m_pThreadMgrEx(nullptr),
	m_pBaseContext(nullptr),
	m_lRefCount(1),
	m_dwUIElementSinkCookie(TF_INVALID_COOKIE),
	m_dwInputProcessorProfileActivationSinkCookie(TF_INVALID_COOKIE),
	m_dwThreadMgrEventSinkCookie(TF_INVALID_COOKIE),
	m_dwTextEditSinkCookie(TF_INVALID_COOKIE) {}

Cicero::~Cicero()
{
	INFO("Releasing Cicero");
	ReleaseSinks();
}

STDAPI_(ULONG)
Cicero::AddRef()
{
	return ++m_lRefCount;
}

STDAPI_(ULONG)
Cicero::Release()
{
	ULONG result = --m_lRefCount;
	if (result == 0)
		delete this;
	return result;
}

STDAPI Cicero::QueryInterface(REFIID riid, void** ppvObj)
{
	if (ppvObj == nullptr)
		return E_INVALIDARG;
	*ppvObj = nullptr;
	if (IsEqualIID(riid, IID_IUnknown))
		*ppvObj = reinterpret_cast<IUnknown*>(this);
	else if (IsEqualIID(riid, IID_ITfUIElementSink))
		*ppvObj = (ITfUIElementSink*)this;
	else if (IsEqualIID(riid, IID_ITfInputProcessorProfileActivationSink))
		*ppvObj = (ITfInputProcessorProfileActivationSink*)this;
	else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
		*ppvObj = (ITfThreadMgrEventSink*)this;
	else if (IsEqualIID(riid, IID_ITfTextEditSink))
		*ppvObj = (ITfTextEditSink*)this;
	else
		return E_NOINTERFACE;
	AddRef();
	return S_OK;
}

HRESULT Cicero::SetupSinks()
{
	int processedCodeBlock = 0;
	HRESULT hr = S_OK;
	ITfSource* pSource = nullptr;
	INFO("[TSF] Initializing Cicero");

	if (!bCOMInitialized) {
		hr = CoInitializeEx(NULL, ::COINIT_APARTMENTTHREADED);
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		hr = CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pThreadMgrEx));
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		hr = m_pThreadMgrEx->ActivateEx(&m_dwClientID, TF_TMAE_UIELEMENTENABLEDONLY);
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		hr = m_pThreadMgrEx->QueryInterface(IID_ITfSource, (LPVOID*)&pSource);
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		hr = pSource->AdviseSink(IID_ITfUIElementSink, (ITfUIElementSink*)this, &m_dwUIElementSinkCookie);
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		hr = pSource->AdviseSink(IID_ITfInputProcessorProfileActivationSink, (ITfInputProcessorProfileActivationSink*)this, &m_dwInputProcessorProfileActivationSinkCookie);
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		hr = pSource->AdviseSink(IID_ITfThreadMgrEventSink, (ITfThreadMgrEventSink*)this, &m_dwThreadMgrEventSinkCookie);
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		pSource->Release();
		hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (LPVOID*)&m_pProfiles);
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		hr = m_pProfiles->QueryInterface(IID_ITfInputProcessorProfileMgr, (LPVOID*)&m_pProfileMgr);
	}
	if (SUCCEEDED(hr)) {
		processedCodeBlock++;
		hr = UpdateCurrentInputMethodName();
	}
	if (SUCCEEDED(hr)) {
		bCOMInitialized = true;
		INFO("[TSF] Set-up Successful");
	} else {
		INFO("!!![TSF] Set-up Failed, Result: 0x{:X}, processed code blocks: {}", (ULONG)hr, processedCodeBlock);
	}
	return S_OK;
}

void Cicero::ReleaseSinks()
{
	HRESULT hr = S_OK;
	ITfSource* pSource = nullptr;
	if (m_dwTextEditSinkCookie != TF_INVALID_COOKIE) {
		if (m_pBaseContext && SUCCEEDED(m_pBaseContext->QueryInterface(&pSource))) {
			hr = pSource->UnadviseSink(m_dwTextEditSinkCookie);
			SafeRelease(&pSource);
			SafeRelease(&m_pBaseContext);
			m_dwTextEditSinkCookie = TF_INVALID_COOKIE;
		}
	}
	if (m_pThreadMgrEx && SUCCEEDED(m_pThreadMgrEx->QueryInterface(IID_PPV_ARGS(&pSource)))) {
		pSource->UnadviseSink(m_dwUIElementSinkCookie);
		pSource->UnadviseSink(m_dwInputProcessorProfileActivationSinkCookie);
		pSource->UnadviseSink(m_dwThreadMgrEventSinkCookie);
		m_pThreadMgrEx->Deactivate();
		SafeRelease(&pSource);
		SafeRelease(&m_pThreadMgr);
		SafeRelease(&m_pProfileMgr);
		SafeRelease(&m_pProfiles);
		SafeRelease(&m_pThreadMgrEx);
	}
	CoUninitialize();
	bCOMInitialized = false;
	INFO("[Cicero] Released Sinks");
}

// ITfUIElementSink::BeginUIElement
STDAPI Cicero::BeginUIElement(DWORD dwUIElementId, BOOL* pbShow)
{
	int token = rand();
	DEBUG("[TSF BeginUIElement#{}] == Start ==", token);
	HRESULT hr = S_OK;

	IMEPanel* pIMEPanel = IMEPanel::GetSingleton();
	ITfUIElement* pElement = GetUIElement(dwUIElementId);

	pIMEPanel->bEnabled = IME_UI_ENABLED;
	if (!pElement) {
		return E_INVALIDARG;
	}
	*pbShow = FALSE;
	ITfCandidateListUIElement* lpCandidate = nullptr;
	hr = pElement->QueryInterface(IID_PPV_ARGS(&lpCandidate));
	if (SUCCEEDED(hr)) {
		this->UpdateCandidateList(lpCandidate);
	}
	pElement->Release();
	DEBUG("[TSF BeginUIElement#{}] == Finish ==\n", token);
	return S_OK;
}

// ITfUIElementSink::UpdateUIElement
STDAPI Cicero::UpdateUIElement(DWORD dwUIElementId)
{
	int token = rand();
	DEBUG("[TSF UpdateUIElement#{}] == Start ==", token);
	HRESULT hr = S_OK;
	ITfUIElement* pElement = GetUIElement(dwUIElementId);
	if (!pElement) {
		return E_INVALIDARG;
	}
	ITfCandidateListUIElement* lpCandidate = nullptr;
	hr = pElement->QueryInterface(IID_PPV_ARGS(&lpCandidate));
	if (SUCCEEDED(hr)) {
		this->UpdateCandidateList(lpCandidate);
		pElement->Release();
	}
	DEBUG("[TSF UpdateUIElement#{}] == Finish ==\n", token);
	return S_OK;
}

// ITfUIElementSink::EndUIElement
STDAPI Cicero::EndUIElement(DWORD dwUIElementId)
{
	int token = rand();
	DEBUG("[TSF EndUIElement#{}] == Start ==", token);
	HRESULT hr = S_OK;
	ITfUIElement* pElement = GetUIElement(dwUIElementId);
	if (!pElement) {
		return E_INVALIDARG;
	}
	ITfCandidateListUIElement* lpCandidate = nullptr;
	hr = pElement->QueryInterface(IID_PPV_ARGS(&lpCandidate));
	if (SUCCEEDED(hr)) {
		this->UpdateCandidateList(lpCandidate);
		pElement->Release();
	}
	DEBUG("[TSF EndUIElement#{}] == Finish ==\n", token);
	return S_OK;
}

// ITfTextEditSink
STDAPI Cicero::OnEndEdit(ITfContext* cxt, TfEditCookie ecReadOnly, ITfEditRecord* pEditRecord)
{
	HRESULT hr = S_OK;
	int token = rand();
	ITfContextComposition* pContextComposition;
	IEnumITfCompositionView* pEnumCompositionView;

	DEBUG("[TSF OnEndEdit#{}] == Start ==", token);
	if (SUCCEEDED(hr = cxt->QueryInterface(&pContextComposition))) {
		hr = pContextComposition->EnumCompositions(&pEnumCompositionView);
	}
	if (SUCCEEDED(hr)) {
		ITfCompositionView* pCompositionView;
		ULONG remainCount = 0;
		while (SUCCEEDED(pEnumCompositionView->Next(1, &pCompositionView, &remainCount)) && remainCount > 0) {
			ITfRange* pRange;
			WCHAR endEditBuffer[MAX_PATH];
			ULONG bufferSize = 0;
			if (SUCCEEDED(pCompositionView->GetRange(&pRange))) {
				pRange->GetText(ecReadOnly, TF_TF_MOVESTART, endEditBuffer, MAX_PATH, &bufferSize);
				endEditBuffer[bufferSize] = '\0';

				std::wstring result(endEditBuffer);

				IMEPanel* pIMEPanel = IMEPanel::GetSingleton();
				pIMEPanel->csImeInformation.Enter();
				DEBUG("(TSF) Set Composition String: {}", Utils::WideStringToString(result));
				pIMEPanel->wstrComposition = result;
				pIMEPanel->csImeInformation.Leave();

				pRange->Release();
			}
			pCompositionView->Release();
		}
	}
	if (pEnumCompositionView) {
		pEnumCompositionView->Release();
	}
	if (pContextComposition) {
		pContextComposition->Release();
	}
	DEBUG("[TSF OnEndEdit#{}] == Finish ==\n", token);
	return S_OK;
}

// ITfInputProcessorProfileActivationSink
STDAPI Cicero::OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags)
{
	int token = rand();
	DEBUG("[TSF OnActivated#{}] == Start ==", token);
	if (!(dwFlags & TF_IPSINK_FLAG_ACTIVE))
		return S_OK;
	this->UpdateCurrentInputMethodName();
	DEBUG("[TSF OnActivated#{}] == Finish ==\n", token);
	return S_OK;
}

// ITfThreadMgrEventSink::OnInitDocumentMgr
STDAPI Cicero::OnInitDocumentMgr(ITfDocumentMgr* pdim)
{
	return S_OK;
}

// ITfThreadMgrEventSink::OnUninitDocumentMgr
STDAPI Cicero::OnUninitDocumentMgr(ITfDocumentMgr* pdim)
{
	return S_OK;
}

// ITfThreadMgrEventSink::OnSetFocus
STDAPI Cicero::OnSetFocus(ITfDocumentMgr* pdimFocus, ITfDocumentMgr* pdimPrevFocus)
{
	HRESULT hr = S_OK;
	if (!pdimFocus)
		return S_OK;
	ITfSource* pSource = nullptr;
	if (m_dwTextEditSinkCookie != TF_INVALID_COOKIE) {
		if (m_pBaseContext && SUCCEEDED(m_pBaseContext->QueryInterface(&pSource))) {
			hr = pSource->UnadviseSink(m_dwTextEditSinkCookie);
			SafeRelease(&pSource);
			if (FAILED(hr)) {
				return S_OK;
			}
			SafeRelease(&m_pBaseContext);
			m_dwTextEditSinkCookie = TF_INVALID_COOKIE;
		}
	}
	hr = pdimFocus->GetBase(&m_pBaseContext);
	if (SUCCEEDED(hr)) {
		hr = m_pBaseContext->QueryInterface(&pSource);
	}
	if (SUCCEEDED(hr)) {
		hr = pSource->AdviseSink(IID_ITfTextEditSink, static_cast<ITfTextEditSink*>(this), &m_dwTextEditSinkCookie);
	}
	SafeRelease(&pSource);
	return S_OK;
}

// ITfThreadMgrEventSink::OnPushContext
STDAPI Cicero::OnPushContext(ITfContext* pic)
{
	return S_OK;
}

// ITfThreadMgrEventSink::OnPopContext
STDAPI Cicero::OnPopContext(ITfContext* pic)
{
	return S_OK;
}

ITfUIElement* Cicero::GetUIElement(DWORD a_dwUIElementId)
{
	ITfUIElementMgr* pElementMgr = nullptr;
	ITfUIElement* pElement = nullptr;
	if (SUCCEEDED(m_pThreadMgrEx->QueryInterface(IID_PPV_ARGS(&pElementMgr)))) {
		pElementMgr->GetUIElement(a_dwUIElementId, &pElement);
		pElementMgr->Release();
	}
	return pElement;
}

/// 更新候选字列表
void Cicero::UpdateCandidateList(ITfCandidateListUIElement* a_pCandidate)
{
	IMEPanel* pIMEPanel = IMEPanel::GetSingleton();
	Configs* pConfigs = Configs::GetSingleton();

	if (pIMEPanel->bEnabled) {
		UINT uSelectedIndex = 0, uCount = 0, uCurrentPage = 0, uPageCount = 0;
		DWORD dwPageStart = 0, dwCurrentPageSize = 0, dwPageSelection = 0;

		BSTR result = nullptr;

		a_pCandidate->GetSelection(&uSelectedIndex);
		a_pCandidate->GetCount(&uCount);
		a_pCandidate->GetCurrentPage(&uCurrentPage);

		dwPageSelection = static_cast<DWORD>(uSelectedIndex);
		a_pCandidate->GetPageIndex(nullptr, 0, &uPageCount);
		DEBUG("(TSF) Updating CandidateList, Selection: {}, Count: {}, CurrentPage: {}, PageCount: {}", uSelectedIndex, uCount, uCurrentPage, uPageCount);
		if (uPageCount > 0) {
			std::unique_ptr<UINT[], void(__cdecl*)(void*)> indexList(
				reinterpret_cast<UINT*>(Utils::HeapAlloc(sizeof(UINT) * uPageCount)),
				Utils::HeapFree);
			a_pCandidate->GetPageIndex(indexList.get(), uPageCount, &uPageCount);
			dwPageStart = indexList[uCurrentPage];
			if (uCurrentPage == uPageCount - 1) {
				// Last page
				dwCurrentPageSize = uCount - dwPageStart;
			} else {
				dwCurrentPageSize = std::min(uCount, indexList[uCurrentPage + 1]) - dwPageStart;
			}
		}
		dwCurrentPageSize = std::min<DWORD>(dwCurrentPageSize, Configs::iCandidateSize);
		if (dwCurrentPageSize)  // If current page size > 0, disable special keys
			InterlockedExchange(&pIMEPanel->bDisableSpecialKey, TRUE);

		DEBUG("(TSF) SelectedIndex in current page: {}, PageStartIndex: {}", dwPageSelection, dwPageStart);

		// Update Candidate Page Information to IMEPanel
		InterlockedExchange(&pIMEPanel->ulSlectedIndex, dwPageSelection);
		InterlockedExchange(&pIMEPanel->ulPageStartIndex, dwPageStart);

		WCHAR candidateBuffer[MAX_PATH];

		pIMEPanel->csImeInformation.Enter();
		pIMEPanel->vwsCandidateList.clear();
		for (int i = 0; i <= dwCurrentPageSize; i++) {
			if (FAILED(a_pCandidate->GetString(i + dwPageStart, &result)) || !result) {
				continue;
			}
			wsprintf(candidateBuffer, L"%d. %s", i + 1, result);
			DEBUG("(TSF) {}", Utils::WideStringToString(candidateBuffer));
			std::wstring temp(candidateBuffer);

			pIMEPanel->vwsCandidateList.push_back(temp);
			SysFreeString(result);
		}
		pIMEPanel->csImeInformation.Leave();
	}
}

HRESULT Cicero::UpdateCurrentInputMethodName()
{
	DEBUG("[TSF] Updating Current Input Method");
	static WCHAR lastTipName[64];
	HRESULT hr = S_OK;
	TF_INPUTPROCESSORPROFILE tip;
	ZeroMemory(lastTipName, sizeof(lastTipName));  // Clear lastTipName
	if (!m_pProfileMgr) {
		ERROR("(UpdateCurrentInputMethodName) ProfileMgr is not initialized");
		return E_POINTER;
	}
	hr = m_pProfileMgr->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &tip);
	if (FAILED(hr)) {
		ERROR("(UpdateCurrentInputMethodName) GetActiveProfile failed");
	}
	DEBUG("Updating State, 0x{:X}", tip.dwProfileType);
	this->UpdateCiceroState(tip.dwProfileType, tip.hkl);
	if (tip.dwProfileType & TF_PROFILETYPE_INPUTPROCESSOR) {
		BSTR bstrImeName = nullptr;
		m_pProfiles->GetLanguageProfileDescription(tip.clsid, tip.langid, tip.guidProfile, &bstrImeName);
		if (bstrImeName != nullptr && wcslen(bstrImeName) < 64)
			wcscpy(lastTipName, bstrImeName);
		if (bstrImeName != nullptr)
			SysFreeString(bstrImeName);
	} else if (tip.dwProfileType & TF_PROFILETYPE_KEYBOARDLAYOUT) {
		static WCHAR klnm[KL_NAMELENGTH];
		if (GetKeyboardLayoutNameW(klnm)) {
			GetLayoutName(klnm, lastTipName);
		}
	}
	IMEPanel* pIMEPanel = IMEPanel::GetSingleton();

	pIMEPanel->csImeInformation.Enter();
	pIMEPanel->wstrInputMethodName = std::wstring(lastTipName);
	pIMEPanel->csImeInformation.Leave();

	DEBUG("[TSF] Current Input Method: {}", Utils::WideStringToString(lastTipName));
	return hr;
}

void Cicero::UpdateCiceroState(DWORD dwProfileType, HKL hkl)
{
	if (dwProfileType & TF_PROFILETYPE_INPUTPROCESSOR) {
		DEBUG("[TSF UpdateCiceroState] TIP InputMethod {}", (unsigned int)hkl);
		bCiceroState = true;
	} else if (dwProfileType & TF_PROFILETYPE_KEYBOARDLAYOUT) {
		DEBUG("[TSF UpdateCiceroState] HKL/IME {}", (unsigned int)hkl);
		bCiceroState = false;
	} else {
		DEBUG("[TSF UpdateCiceroState] Unknown Profile Type {}", (unsigned int)hkl);
		bCiceroState = false;
	}
}
