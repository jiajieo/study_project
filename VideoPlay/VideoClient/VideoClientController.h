#pragma once
#include "TVlc.h"
#include "VideoClientDlg.h"
enum TVlcCommand {
	TVLC_PLAY,
	TVLC_PAUSE,
	TVLC_STOP,
	TVLC_GET_VOLUME,
	TVLC_GET_POSITION,
	TVLC_GET_LENGTH
};
class VideoClientController
{
public:
	VideoClientController();
	~VideoClientController();
	int Init(CWnd*& pWnd);
	int Invoke();
	//如果strUrl中包含中文符号或字符，则使用utf-8编码
	int SetMedia(const std::string& strUrl);
	//返回-1.0表示错误
	float VideoCtrl(TVlcCommand cmd);
	void SetPosition(float pos);
	int SetWnd(HWND hWnd);
	int SetVolume(int volume);
	VlcSize GetMediaInfo();
	std::string Unicode2Utf8(const std::wstring& strIn);
protected:
	TVlc m_vlc;
	CVideoClientDlg m_dlg;
};

