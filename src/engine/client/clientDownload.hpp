#ifndef __CLIENTDOWNLOAD_HPP__
#define __CLIENTDOWNLOAD_HPP__

//
// idClientDownloadSystemLocal
//
class idClientDownloadSystemLocal : public idClientDownloadSystemAPI {
public:
    idClientDownloadSystemLocal();
    ~idClientDownloadSystemLocal();

    virtual bool WWWBadChecksum(pointer pakname);

    static void ClearStaticDownload(void);
    static void DownloadsComplete(void);
    static void BeginDownload(pointer localName, pointer remoteName);
    static void NextDownload(void);
    static void InitDownloads(void);
    static void WWWDownload(void);
};

extern idClientDownloadSystemLocal clientDownloadLocal;

#endif //__CLIENTDOWNLOAD_HPP__