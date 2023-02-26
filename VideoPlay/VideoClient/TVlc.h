#pragma once
#include "vlc.h"
#include <string>
class VlcSize {
public:
	int nWidth;
	int nHeight;
	VlcSize(int width=0,int height=0) {
		nWidth = width;
		nHeight = height;
	}
	VlcSize(const VlcSize& size) {
		nWidth = size.nWidth;
		nHeight = size.nHeight;
	}
	VlcSize& operator=(const VlcSize& size) {
		if (this != &size) {
			nWidth = size.nWidth;
			nHeight = size.nHeight;
		}
		return *this;
	}
};
class TVlc
{
public:
	TVlc();
	~TVlc();
	int SetMedia(const std::string& strUrl);//strUrl如果包含中文，请传入utf-8的编码格式字符串
#ifdef WIN32
	int SetHwnd(HWND hWnd);
#endif
	int Play();
	int Pause();
	int Stop();
	float GetPosition();
	void SetPosition(float pos);
	int GetVolume();
	int SetVolume(int Volume);
	VlcSize GetMediaInfo();
	float GetLength();
	std::string Unicode2Utf8(const std::wstring& strIn);
protected:
	libvlc_instance_t* m_instance;
	libvlc_media_t* m_media;
	libvlc_media_player_t* m_player;
	std::string m_url;
#ifdef WIN32
	HWND m_hwnd;
#endif
};

