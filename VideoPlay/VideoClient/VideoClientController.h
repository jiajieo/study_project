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
	//���strUrl�а������ķ��Ż��ַ�����ʹ��utf-8����
	int SetMedia(const std::string& strUrl);
	//����-1.0��ʾ����
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

