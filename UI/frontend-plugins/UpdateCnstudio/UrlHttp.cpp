#include <curl/curl.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <vector>
#include <algorithm>
#include "UrlHttp.h"
using namespace std;
#define URLDESKEY	"HBSMY4yFB"
CRITICAL_SECTION*  CHttpClient::lockarray=NULL;

CHttpClient::CHttpClient(void) :   
m_bDebug(false),
m_dwConnectTime(DEFCONNECTTIME)
,m_dwRequestTime(DEFRECVETIME)
,m_bRecvHeader(false)
,m_progressCB(NULL)
,m_mUserData(NULL)
,m_bLocation(true)
{  
  m_mCookieFile.clear();
  m_mVheader.clear();
  m_curl=NULL;
  m_curlretcode=CURLE_FAILED_INIT;
  m_httpretcode=0;
  m_FilePtr=NULL;
  m_bStop=false;
}  
  
CHttpClient::~CHttpClient(void)  
{  
	m_mCookieFile.clear();
	m_mVheader.clear();
	CloseHttpConnect();
}  

std::string CHttpClient::GetHttpHeaderDate()
{
	string strRet;
	string strlkey="Date:";
	string strTempHeader=m_strHeader;
	if(strTempHeader.empty()||strlkey.empty())
		return strRet;
	transform(strTempHeader.begin(),strTempHeader.end(),strTempHeader.begin(),tolower);
	transform(strlkey.begin(),strlkey.end(),strlkey.begin(),tolower);
	string::size_type pos=strTempHeader.find(strlkey.c_str());
	string::size_type endpos=pos;
	if(pos!=string::npos)
	{
		pos+=strlkey.length();
		endpos=strTempHeader.find("\r",pos);
	}
	if(endpos>pos)
	{
		strRet=m_strHeader.substr(pos,endpos-pos);
	}
	return	strRet;
}



int CHttpClient::Post(const std::string & strUrl,bool bPostBuff, const std::string & strPost, std::string & strResponse)  
{  
	CURLcode res;
	string strlowurl=strUrl;
	string strKey;
	transform(strUrl.begin(),strUrl.end(),strlowurl.begin(),tolower);
	if(strlowurl.find("https://")!=string::npos)
	{
		return Posts(strUrl,bPostBuff,strPost,strResponse);
	}
	if(NULL == m_curl)  
	{  
		return CURLE_FAILED_INIT;  
	}  
	if(m_bDebug)  
	{  
		curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);  
		curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);  
	}  
	
#ifdef URLPARAMDES
	curl_easy_setopt(m_curl, CURLOPT_URL,CryptUrlParam(strUrl).c_str()); 
#else
	curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str()); 
#endif
	m_qurl=strUrl;
	curl_easy_setopt(m_curl, CURLOPT_POST, 1);
	if(bPostBuff)
	{
		struct WriteThis pooh;
		pooh.readptr =strPost.c_str();
		pooh.sizeleft =strPost.length();
		curl_easy_setopt(m_curl, CURLOPT_READFUNCTION,OnReadData);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, pooh.sizeleft);
		curl_easy_setopt(m_curl, CURLOPT_READDATA, &pooh);
	}
	else
	{
#ifdef URLPARAMDES
		if(!strPost.empty())
		{

			string strPostParam=UrlEncode(Base64Des(strPost,URLDESKEY,true));
			strKey=formatstring("crypt=%s",strPostParam.c_str());
			curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS,strKey.c_str()); 
		}
		else
		{
			curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, strPost.c_str());
		}
#else
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, strPost.c_str()); 
#endif

		curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);  
	}
	if(m_bRecvHeader)
	{
		curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, 1);
	}
	else
	{
		curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L);
	}
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&strResponse);
	SetDefHeader(m_curl);
	struct curl_slist *slist=NULL;
	slist=(curl_slist *)ApplyUserHeader();
	if(slist)
	{
		curl_easy_setopt(m_curl,CURLOPT_HTTPHEADER,slist);
	}
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_dwRequestTime);
	res = ExecuteHttp(m_curl);
	if(slist)
	{
		curl_slist_free_all(slist);
	}
	m_curlretcode=res;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE , &m_httpretcode); 
	return res;  
}  
  
int CHttpClient::Get(const std::string & strUrl, std::string & strResponse)  
{  
	CURLcode res;
	string strlowurl=strUrl;
	transform(strUrl.begin(),strUrl.end(),strlowurl.begin(),tolower);
	if(strlowurl.find("https://")!=string::npos)
	{
		return Gets(strUrl,strResponse);
	}
	if(NULL == m_curl)  
	{  
		return CURLE_FAILED_INIT;  
	}  
	if(m_bDebug)  
	{  
		curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);  
		curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);  
	}  
	curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str()); 
	m_qurl=strUrl;
	curl_easy_setopt(m_curl,CURLOPT_HTTPGET,1);
	curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);
	if(m_bRecvHeader)
	{
		curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, 1);
	}
	else
	{
		curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L);
	}
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&strResponse);
	SetDefHeader(m_curl);

	struct curl_slist *slist=NULL;
	slist=(curl_slist *)ApplyUserHeader();
	if(slist)
	{
		curl_easy_setopt(m_curl,CURLOPT_HTTPHEADER,slist);
	}
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_dwRequestTime);
	res = ExecuteHttp(m_curl);
	if(slist)
	{
		curl_slist_free_all(slist);
	}
	m_curlretcode=res;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE , &m_httpretcode); 
	return res;  
}  
  
int CHttpClient::Posts(const std::string & strUrl,bool bPostBuff,const std::string & strPost, std::string & strResponse, const char * pCaPath)  
{  
    CURLcode res;  
	if(NULL == m_curl)  
	{  
		return CURLE_FAILED_INIT;  
	}  
    if(m_bDebug)  
    {  
        curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);  
        curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);  
    }  
    curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str()); 
		m_qurl=strUrl;
    curl_easy_setopt(m_curl, CURLOPT_POST, 1); 
	if(bPostBuff)
	{
		struct WriteThis pooh;
		pooh.readptr  =strPost.c_str();
		pooh.sizeleft =strPost.length();
		curl_easy_setopt(m_curl, CURLOPT_READFUNCTION,OnReadData);
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, pooh.sizeleft);
		 curl_easy_setopt(m_curl, CURLOPT_READDATA, &pooh);
	}
	else
	{
		curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, strPost.c_str());  
		curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);  
	}
    
	if(m_bRecvHeader)
	{
		curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, 1);
	}
	else
	{
		curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L);
	}
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    SetDefHeader(m_curl);
    if(NULL == pCaPath)  
    {  
        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);  
        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, false);  
    }  
    else  
    {  
        //缺省情况就是PEM，所以无需设置，另外支持DER  
        //curl_easy_setopt(curl,CURLOPT_SSLCERTTYPE,"PEM");  
        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, true);  
        curl_easy_setopt(m_curl, CURLOPT_CAINFO, pCaPath);  
    }  
	struct curl_slist *slist=NULL;
	slist=(curl_slist *)ApplyUserHeader();
	if(slist)
	{
		curl_easy_setopt(m_curl,CURLOPT_HTTPHEADER,slist);
	}
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_dwRequestTime);
    res = ExecuteHttp(m_curl);
	if(slist)
	{
		curl_slist_free_all(slist);
	}
	
	m_curlretcode=res;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE , &m_httpretcode); 
    return res;  
}  
  
int CHttpClient::Gets(const std::string & strUrl, std::string & strResponse, const char * pCaPath)  
{  
    CURLcode res;  
	if(NULL == m_curl)  
	{  
		return CURLE_FAILED_INIT;  
	}  
    if(m_bDebug)  
    {  
        curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);  
        curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);  
    }  
    curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str());
		m_qurl=strUrl;
	curl_easy_setopt(m_curl,CURLOPT_HTTPGET,1);
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);  
	if(m_bRecvHeader)
	{
		curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, 1);
	}
	else
	{
		curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L);
	}
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&strResponse);
	SetDefHeader(m_curl);
    if(NULL == pCaPath)  
    {  
        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);  
        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, false);  
    }  
    else  
    {  
        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, true);  
        curl_easy_setopt(m_curl, CURLOPT_CAINFO, pCaPath);  
    }

	
	struct curl_slist *slist=NULL;
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_dwRequestTime);
	slist=(curl_slist *)ApplyUserHeader();
	if(slist)
	{
		curl_easy_setopt(m_curl,CURLOPT_HTTPHEADER,slist);
	}
	res = ExecuteHttp(m_curl);
	if(slist)
	{
		curl_slist_free_all(slist);
	}
	m_curlretcode=res;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE , &m_httpretcode); 
    return res;  
}  
  
///////////////////////////////////////////////////////////////////////////////////////////////  
  
void CHttpClient::SetDebug(bool bDebug)  
{  
    m_bDebug = bDebug;  
}  


void CHttpClient::SetHttpTimeOut( DWORD dwConnectTime,DWORD dwRequestTime )
{
	m_dwConnectTime=dwConnectTime;
	m_dwRequestTime=dwRequestTime;
}

void CHttpClient::SetResponseHeader( bool bHeader/*=false*/ )
{
	m_bRecvHeader=bHeader;
}

__int64 CHttpClient::GetRmoteFileSize( const std::string & strUrl )
{

	string strlowurl=strUrl;
	string strOut;
	double iret=-1;
	CURLcode res;
	transform(strUrl.begin(),strUrl.end(),strlowurl.begin(),tolower);
	if(m_curl==NULL)  
	{  
		return -1;  
	}
	curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str());
	m_qurl=strUrl;
	curl_easy_setopt(m_curl, CURLOPT_HTTPGET, strUrl.c_str());  
	if(m_bDebug)  
	{  
		curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);  
		curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);  
	}
	curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(m_curl, CURLOPT_HEADER, 1);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&strOut);
	SetDefHeader(m_curl);
	if(strlowurl.find("https://")!=string::npos)
	{
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);  
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, false);
	}
	struct curl_slist *slist=NULL;
	slist=(curl_slist *)ApplyUserHeader();
	if(slist)
	{
		curl_easy_setopt(m_curl,CURLOPT_HTTPHEADER,slist);
	}
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_dwRequestTime);
	res = ExecuteHttp(m_curl);
	m_curlretcode=res;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE , &m_httpretcode); 
	if(res==CURLE_OK&&m_httpretcode==200)
	{
		curl_easy_getinfo(m_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &iret);
		
	}
	if(slist)
	{
		curl_slist_free_all(slist);
	}
	return (__int64)iret;
}

__int64 CHttpClient::DownUrlToFile( const std::string & strUrl,const std::string& localFileName,unsigned	__int64 iBeg,PROGRESSFUN cb,void* userData)
{
	m_FilePtr=NULL;
	string strlowurl=strUrl;
	double iret=-1;
	transform(strUrl.begin(),strUrl.end(),strlowurl.begin(),tolower);
	if(NULL ==m_curl)  
	{  
		return -1;  
	}
	curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str());
	m_qurl=strUrl;
	curl_easy_setopt(m_curl,CURLOPT_HTTPGET,1);
	SetDefHeader(m_curl);
	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS,cb?0:1);
	curl_easy_setopt(m_curl, CURLOPT_PROGRESSFUNCTION,cb);
	curl_easy_setopt(m_curl, CURLOPT_PROGRESSDATA,userData);
	curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L);
	curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION,NULL);  
	if(strlowurl.find("https://")!=string::npos)
	{
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);  
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, false);
	}
	m_FilePtr=fopen(localFileName.c_str(), "ab+");
	if(!m_FilePtr)
		return -1;
	unsigned	__int64 begPos = GetFileLen(localFileName);
	curl_easy_setopt(m_curl, CURLOPT_FILE,m_FilePtr);
	struct curl_slist *slist=NULL;
	slist=(curl_slist *)ApplyUserHeader();
	if(slist)
	{
		curl_easy_setopt(m_curl,CURLOPT_HTTPHEADER,slist);
	}
	curl_easy_setopt(m_curl, CURLOPT_RESUME_FROM_LARGE,iBeg);
	curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L);
	curl_easy_setopt(m_curl, CURLOPT_FAILONERROR, 1L);
	m_bStop=false;
	m_curlretcode=curl_easy_perform(m_curl);
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE , &m_httpretcode);
	unsigned	__int64 endpos = GetFileLen(localFileName);
	fclose(m_FilePtr);
	m_FilePtr=NULL;
	if(m_curlretcode==CURLE_OK||m_curlretcode==CURLE_ABORTED_BY_CALLBACK)
	{
		iret=endpos-begPos;
	}
	if(slist)
	{
		curl_slist_free_all(slist);
	}
	return (__int64)iret;
}

void CHttpClient::SetDefHeader(CURL* curl)
{
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING,"gzip,deflate");
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_dwConnectTime);  
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, m_bLocation);
	curl_easy_setopt(curl, CURLOPT_USERAGENT,"Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)");
	curl_easy_setopt(curl,CURLOPT_HEADERFUNCTION,OnWriteData);
	m_strHeader.clear();
	curl_easy_setopt(curl,CURLOPT_HEADERDATA,&m_strHeader);
	if (m_mCookieFile.empty())
	{
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE,"");
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, m_mCookieFile.c_str());
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR, m_mCookieFile.c_str());
	}
	//10秒内的速度小于1字节判定超时，避免卡住
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
	/*if(!m_mCookieFile.empty())
	{
		curl_easy_setopt(curl, CURLOPT_COOKIEJAR,m_mCookieFile.c_str());
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE,m_mCookieFile.c_str());
	}*/
	m_httpretcode=0;
	m_curlretcode=CURLE_UNSUPPORTED_PROTOCOL;
}

void CHttpClient::SetUserHeader(std::vector<std::string>* inHeader )
{
	ClearUserHeader();
	for(int i=0;i<inHeader->size();i++)
	{
		m_mVheader.push_back(inHeader->at(i));
	}
}

void CHttpClient::ClearUserHeader()
{
	if ( m_mVheader.size() > 0 )
	{
		m_mVheader.clear();
	}
}

void* CHttpClient::ApplyUserHeader()
{
	struct curl_slist *slist=NULL;
	for(int i=0;i<m_mVheader.size();i++)
	{
		if(slist==NULL)
		{
			slist=curl_slist_append(slist,m_mVheader.at(i).c_str());
		}
		else
		{
			curl_slist_append(slist,m_mVheader.at(i).c_str());
		}
	}
	return (void*)slist;
}

void CHttpClient::SetCookiePatch( const string& strFile )
{
	m_mCookieFile=strFile;
}

void CHttpClient::SetFollowLocation(bool bLocation)
{
	m_bLocation=bLocation;
}

std::string CHttpClient::GetHttpResHeader()
{
	return m_strHeader;
}

std::string CHttpClient::GetHttpHeaderVar( const string& strKey )
{
	string strRet;
	string strlkey=strKey;
	string strTempHeader=m_strHeader;
	if(strTempHeader.empty()||strKey.empty())
		return strRet;
	transform(strTempHeader.begin(),strTempHeader.end(),strTempHeader.begin(),tolower);
	transform(strKey.begin(),strKey.end(),strlkey.begin(),tolower);
	string::size_type pos=strTempHeader.find(strlkey.c_str());
	string::size_type endpos=pos;
	if(pos!=string::npos)
	{
		pos+=strlkey.length();
		endpos=strTempHeader.find("\r",pos);
	}
	if(endpos>pos)
	{
		strRet=m_strHeader.substr(pos,endpos-pos);
		strRet.erase(0,strRet.find_first_not_of(" "));
		strRet.erase(strRet.find_last_not_of(" ") + 1);
	}
	return	strRet;
}

bool CHttpClient::InitHttpConnect()
{
	CloseHttpConnect();
	m_curl= curl_easy_init();
	return m_curl?true:false;
}

void CHttpClient::CloseHttpConnect()
{
	ClearUserHeader();
	
	curl_easy_cleanup(m_curl);

	m_curl=NULL;

}

CURL* CHttpClient::GetLibCurl()
{
	return m_curl;
}
size_t CHttpClient::OnWriteData( void* buffer, size_t size, size_t nmemb, void* lpVoid )
{
	std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);  
	if( NULL == str || NULL == buffer )  
	{  
		return -1;  
	}  
	char* pData = (char*)buffer;  
	
	str->append(pData, size * nmemb); 
	return nmemb;  
	
}

size_t CHttpClient::OnReadData( void *ptr, size_t size, size_t nmemb, void *userp )
{
	struct WriteThis *pooh =dynamic_cast<struct WriteThis *>((struct WriteThis *)userp);
	if(!pooh)
		return 0;
	if(size*nmemb < 1)
		return 0;
	if(pooh->sizeleft) {
		*(char *)ptr = pooh->readptr[0]; 
		pooh->readptr++;                
		pooh->sizeleft--;              
		return 1;                   
	}
	return 0;  
}
int CHttpClient::OnDebug( CURL *, curl_infotype itype, char * pData, size_t size, void * )
{
//#ifdef _CONSOLE
	if(itype == CURLINFO_TEXT)  
	{  
		 printf("[TEXT]%s\n", pData);  
	}  
	else if(itype == CURLINFO_HEADER_IN)  
	{  
		printf("[HEADER_IN]%s\n", pData);  
	}  
	else if(itype == CURLINFO_HEADER_OUT)  
	{  
		printf("[HEADER_OUT]%s\n", pData);  
	}  
	else if(itype == CURLINFO_DATA_IN)  
	{  
		printf("[DATA_IN]%s\n", pData);  
	}  
	else if(itype == CURLINFO_DATA_OUT)  
	{  
		printf("[DATA_OUT]%s\n", pData);  
	} 
//#endif
	return 0;  
}

std::string CHttpClient::UrlEncode( const string& strSrc )
{
	char* pOutSrc=NULL;
	string strRet;
	pOutSrc=curl_escape(strSrc.c_str(),strSrc.length());
	strRet.append(pOutSrc);
	if(pOutSrc)
	{
		curl_free(pOutSrc);
	}
	return strRet;
}
std::string CHttpClient::UrlDecode( const string& strSrc )
{
	char* pOutSrc=NULL;
	string strRet;
	pOutSrc=curl_unescape(strSrc.c_str(),strSrc.length());
	strRet.append(pOutSrc);
	if(pOutSrc)
	{
		curl_free(pOutSrc);
	}
	return strRet;
}

std::string CHttpClient::GetLastUrl()
{
	char* pUrl = "";
	if(m_curl)
	{
		curl_easy_getinfo(m_curl,CURLINFO_EFFECTIVE_URL,&pUrl);
	}
	return string(pUrl);
}

CURLcode CHttpClient::ExecuteHttp( CURL* http )
{
	CURLcode code=curl_easy_perform(http);
	curl_easy_getinfo(http, CURLINFO_RESPONSE_CODE , &m_httpretcode); 

	return code;
}
__int64	CHttpClient::GetFileLen( const string& file )
{
	
	namespace fs = std::tr2::sys;
	_ULonglong	fileSize = fs::file_size(fs::path(file));
	return fileSize;
}

__int64 CHttpClient::DownUrlToFileEx( const std::string & strUrl,const std::string& localFileName )
{
	__int64 ibeg=0;
	__int64 iRemoteSize=GetRmoteFileSize(strUrl);
	if(iRemoteSize<0)
			return -1;
	m_strHeader.clear();
	// 临时注释
	DownUrlToFile(strUrl,localFileName,GetFileLen(localFileName));
	//DownLoadThreadSingleFile( 5, strUrl, localFileName );
	if(m_curlretcode==CURLE_OK&&m_httpretcode==200)
		return 1;
	return -1;
	
}

std::string CHttpClient::GetLastConnectIp()
{
	char* ip = "";
	if(m_curl)
	{
		curl_easy_getinfo(m_curl,CURLINFO_PRIMARY_IP,&ip);
	}
	return string(ip);
}

FILE* CHttpClient::GetDownFilePtr()
{
	return m_FilePtr;
}

CURLcode CHttpClient::UpLoadFile( const std::string& strUrl,const std::string& localFileName,std::string &strResponse)
{
	CURLcode res;  

	if(NULL == m_curl)  
	{  
		return CURLE_FAILED_INIT;  
	}  
	if(m_bDebug)  
	{  
		curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);  
		curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);  
	}  
	curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str()); 
	m_qurl=strUrl;
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&strResponse);
	SetDefHeader(m_curl);
	struct curl_slist *slist=NULL;
	m_mVheader.push_back("Expect:");
	slist=(curl_slist *)ApplyUserHeader();
	if(slist)
	{
		curl_easy_setopt(m_curl,CURLOPT_HTTPHEADER,slist);
	}

	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	string strFileName=localFileName.substr(localFileName.rfind('\\')+1,localFileName.length()-localFileName.rfind('\\')-1);
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME,"file",
		CURLFORM_FILE, localFileName.c_str(),
		CURLFORM_END);
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME,"file",
		CURLFORM_COPYCONTENTS, strFileName.c_str(),
		CURLFORM_END);
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME,"submit",
		CURLFORM_COPYCONTENTS,"send",
		CURLFORM_END);
	curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, formpost);
	res = ExecuteHttp(m_curl);
	if(slist)
	{
		curl_slist_free_all(slist);
	}
	m_curlretcode=res;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE , &m_httpretcode); 
	curl_formfree(formpost);
	return res;  
}

CURLcode CHttpClient::UpLoadForm(const std::string& strUrl,curl_httppost* phttppost,std::string& strResponse)
{
	CURLcode res;  
	if(NULL == m_curl)  
	{  
		return CURLE_FAILED_INIT;  
	}  
	if(m_bDebug)  
	{  
		curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);  
		curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);  
	}  
	curl_easy_setopt(m_curl, CURLOPT_URL, strUrl.c_str()); 
	m_qurl=strUrl;
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&strResponse);
	SetDefHeader(m_curl);
	struct curl_slist *slist=NULL;
	m_mVheader.push_back("Expect:");
	slist=(curl_slist *)ApplyUserHeader();
	if(slist)
	{
		curl_easy_setopt(m_curl,CURLOPT_HTTPHEADER,slist);
	}
	curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, phttppost);
	res = ExecuteHttp(m_curl);
	if(slist)
	{
		curl_slist_free_all(slist);
	}
	m_curlretcode=res;
	curl_easy_getinfo(m_curl,CURLINFO_RESPONSE_CODE,&m_httpretcode); 
	//curl_formfree(mForm->formpost);
	return res;
}

void CHttpClient::UpAddForm(curl_httppost** phttppost,curl_httppost** plastpost,const std::string& name,const std::string& var,bool bFile)
{
	curl_formadd(phttppost,
		plastpost,
		CURLFORM_COPYNAME,name.c_str(),
		bFile?(CURLFORM_FILE):(CURLFORM_COPYCONTENTS),var.c_str(),
		CURLFORM_END);

}


void CHttpClient::SetDecode( BOOL decode )
{
	//curl_easy_setopt(m_curl,CURLOPT_HTTP_TRANSFER_DECODING,decode);
	curl_easy_setopt(m_curl,CURLOPT_HTTP_CONTENT_DECODING,decode);
}
