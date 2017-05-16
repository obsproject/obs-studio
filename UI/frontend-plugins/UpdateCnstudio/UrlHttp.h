#ifndef __HTTPCLIENT___
#define __HTTPCLIENT___
typedef	int (*PROGRESSFUN)(void *p,double dltotal, double dlnow,double ultotal, double ulnow);
typedef int (*PROGRESSTHREADFUN)( void* lpVoid );
#define DEFCONNECTTIME 120
#define DEFRECVETIME 120

#define NEWURLDESKEY "i2.0908o"

class CHttpClient  
{  
public: 
	struct WriteThis {
		const char *readptr;
		long sizeleft;
	};
     CHttpClient(void);  
    ~CHttpClient(void);
	 /** 
     * @brief   初始化http连接资源
	 */
	 bool		InitHttpConnect();
	 /** 
     * @brief   关闭http连接资源
	 */
	 void		 CloseHttpConnect();
	 CURL*		 GetLibCurl();
	/** 
     * @brief   设置附加的请求头
	 */
	void		SetUserHeader(std::vector<std::string>* inHeader);
	/** 
     * @brief   清除附加的请求头
	 */
	void		ClearUserHeader();
	/** 
     * @brief   设置cookie路径
	 */
	void			SetCookiePatch(const std::string& strFile);
	void			SetFollowLocation(bool bLocation);
	std::string		GetHttpResHeader();
	std::string		GetLastUrl();
	std::string		GetLastConnectIp();
	CURLcode		ExecuteHttp(CURL* http);
	__int64			DownUrlToFileEx(const std::string & strUrl,const std::string& localFileName);
	__int64			GetFileLen(const std::string& file);
	/** 
    * @brief 得到远程文件大小
	*/
	 __int64  GetRmoteFileSize(const std::string & strUrl);
	 /** 
    * @brief  下载指url数据到本地文件
	*/
	 FILE*    GetDownFilePtr();
	__int64	  DownUrlToFile(const std::string & strUrl,const std::string& localFileName,unsigned __int64 iBeg=0,PROGRESSFUN cb=NULL,void* userData=NULL);

	CURLcode  UpLoadFile(const std::string& strUrl,const std::string& localFileName,std::string &strResponse);
	//兼容旧版接口
	void UpAddForm(curl_httppost** phttppost,curl_httppost** plastpost,const std::string& name,const std::string& var,bool bFile=true);
	CURLcode UpLoadForm(const std::string& strUrl,curl_httppost* phttppost,std::string& strResponse);
	/** 
    * @brief
	*/
	void     SetHttpTimeOut(DWORD dwConnectTime=DEFCONNECTTIME,DWORD dwRequestTime=DEFRECVETIME);
	/**
	* @bries 设置接收数据是否只接收头
	*/
	void     SetResponseHeader(bool bHeader=false);
    /** 
    * @brief HTTP POST请求 
    * @param strUrl		 输入参数,请求的Url地址,如:http://www.xxxx.com 
    * @param strPost	 输入参数,数据内容
    * @param strResponse 输出参数,返回的内容
    * @return			 返回是否Post成功 
    */  
	int		Post(const std::string & strUrl,bool bPostBuff, const std::string & strPost, std::string & strResponse);  
  
    /** 
    * @brief HTTP		 GET请求 
    * @param strUrl		 输入参数,请求的Url地址,如:http://www.xxxx.com 
    * @param strResponse 输出参数,返回的内容
    * @return			 返回是否Post成功 
    */  
    int		Get(const std::string & strUrl, std::string & strResponse);  
  
    /** 
    * @brief HTTPS POST  请求,无证书版本 
    * @param strUrl		 输入参数,请求的Url地址,如:https://www.xxxxx.com 
    * @param strPost	 输入参数,数据内容
    * @param strResponse 输出参数,返回的内容
    * @param pCaPath	 输入参数,为CA证书的路径.如果输入为NULL,则不验证服务器端证书的有效性. 
    * @return			 返回是否Post成功 
    */  
    int		Posts(const std::string & strUrl,bool bPostBuff, const std::string & strPost, std::string & strResponse,const char * pCaPath = NULL);  
    /** 
    * @brief HTTPS       GET请求,无证书版本 
    * @param strUrl		 输入参数,请求的Url地址,如:https://www.xxxx.com 
    * @param strResponse 输出参数,返回的内容
    * @param pCaPath	 输入参数,为CA证书的路径.如果输入为NULL,则不验证服务器端证书的有效性. 
    * @return			 返回是否Post成功 
    */  
    int		Gets(const std::string & strUrl, std::string & strResponse, const char * pCaPath = NULL);
	//辅助方法
	/** 
    * @brief			 查找http返回头中值
    * @param strKey		 返回的串  
    */ 
	std::string	GetHttpHeaderVar(const std::string& strKey);
	std::string	UrlEncode(const std::string& strSrc);
	std::string	UrlDecode(const std::string& strSrc);
	std::string GetHttpHeaderDate();
	static size_t WriteFileCallback(void *contents, size_t size, size_t nmemb, void *userp);

public:  
    void		  SetDebug(bool bDebug);
	static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid);
	static size_t OnReadData(void *ptr, size_t size, size_t nmemb, void *userp);
	static int	  OnDebug(CURL *, curl_infotype itype, char * pData, size_t size, void *);
	static size_t writeFunc( void* buffer, size_t size, size_t nmemb, void* lpVoid );
	
	CURLcode						 m_curlretcode;
	int							     m_httpretcode;
	bool					         m_bStop;
	static	CRITICAL_SECTION				 *lockarray;  
	
public:
	void  SetDefHeader(CURL* curl);
	void* ApplyUserHeader();
	void SetDecode( BOOL decode );
 private:
    bool							 m_bDebug;
	bool							 m_bLocation;
	std::string							 m_strHeader;
	void*							 m_mUserData;
	std::string							 m_mCookieFile;
	PROGRESSFUN						 m_progressCB;
	bool							 m_bRecvHeader;
	DWORD							 m_dwRequestTime;
	DWORD							 m_dwConnectTime;
	std::vector<const std::string>	 m_mVheader;
	public:
	CURL*							 m_curl;
	
	std::string						 m_qurl;
	FILE*							 m_FilePtr;
};  
#endif