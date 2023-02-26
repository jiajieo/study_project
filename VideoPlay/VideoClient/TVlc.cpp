#include "pch.h"
#include "TVlc.h"

TVlc::TVlc()
{
	m_instance = libvlc_new(0, NULL);
	m_media = NULL;
	m_player = NULL;
	m_hwnd = NULL;
}

TVlc::~TVlc()
{
	if (m_player != NULL) {
		libvlc_media_player_t* temp = m_player;
		m_player = NULL;
		libvlc_media_player_release(temp);
	}
	if (m_media != NULL) {
		libvlc_media_t* temp = m_media;
		m_media = NULL;
		libvlc_media_release(temp);
	}
	if (m_instance != NULL) {
		libvlc_instance_t* temp = m_instance;
		m_instance = NULL;
		libvlc_release(temp);
	}
}

int TVlc::SetMedia(const std::string& strUrl)
{
	if (m_instance == NULL||(m_hwnd==NULL)) return -1;
	if (strUrl == m_url)return 0;
	m_url = strUrl;
	if (m_media != NULL) {
		libvlc_media_release(m_media);
		m_media = NULL;
	}
	m_media = libvlc_media_new_path(m_instance, strUrl.c_str());
	if (!m_media) return -2;
	if (m_player != NULL) {
		libvlc_media_player_release(m_player);
		m_player = NULL;
	}
	m_player = libvlc_media_player_new_from_media(m_media);
	if (!m_player)return -3;
	CRect rect;
	GetWindowRect(m_hwnd, rect);
	std::string strRect = "";
	strRect.resize(32);
	sprintf((char*)strRect.c_str(), "%d:%d", rect.Width(), rect.Height());
	libvlc_video_set_aspect_ratio(m_player, strRect.c_str());
	libvlc_media_player_set_hwnd(m_player, m_hwnd);
	return 0;
}

#ifdef WIN32
int TVlc::SetHwnd(HWND hWnd)
{
	m_hwnd = hWnd;
	return 0;
}
#endif

int TVlc::Play()
{
	if (!m_player || !m_media || !m_player)return -1;
	return libvlc_media_player_play(m_player);
}

int TVlc::Pause()
{
	if (!m_player || !m_media || !m_player)return -1;
	libvlc_media_player_pause(m_player);
	return 0;
}

int TVlc::Stop()
{
	if (!m_player || !m_media || !m_player)return -1;
	libvlc_media_player_stop(m_player);
	return 0;
}

float TVlc::GetPosition()
{
	if (!m_player || !m_media || !m_player)return -1.0;
	return libvlc_media_player_get_position(m_player);
}

void TVlc::SetPosition(float pos)
{
	if (!m_player || !m_media || !m_player)return;
	libvlc_media_player_set_position(m_player, pos);
}

int TVlc::GetVolume()
{
	if (!m_player || !m_media || !m_player)return -1;
	return libvlc_audio_get_volume(m_player);
}

int TVlc::SetVolume(int Volume)
{
	if (!m_player || !m_media || !m_player)return -1;
	return libvlc_audio_set_volume(m_player, Volume);
}

VlcSize TVlc::GetMediaInfo()
{
	if (!m_player || !m_media || !m_player)return VlcSize(-1, -1);
	return VlcSize(libvlc_video_get_width(m_player), libvlc_video_get_height(m_player));
}

float TVlc::GetLength()
{
	if (!m_player || !m_instance || !m_media)return -1.0f;
	libvlc_time_t tm = libvlc_media_player_get_length(m_player);
	float ret = tm / 1000.0f;
	return ret;
}

std::string TVlc::Unicode2Utf8(const std::wstring& strIn)
{
	std::string str;
	int length = ::WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), strIn.size(), NULL, 0, NULL, NULL);
	str.resize(length + 1);
	::WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), strIn.size(), (LPSTR)str.c_str(), length, NULL, NULL);
	return str;
}