#include <windows.h>
#include <tchar.h>
#include "error_code.h"

static wchar_t g_path[MAX_PATH];
static HANDLE hFind;
WIN32_FIND_DATA FindFileData;

error_code_t goto_path(wchar_t* path)
{
    wcscpy(g_path, path);
    wcscat(g_path, L"\\*");
    return SUCCESS;
}

_TCHAR* find_file()
{

    static int first_in = 0;

    if(first_in == 0){
        hFind = FindFirstFile(g_path, &FindFileData);
        if(hFind == INVALID_HANDLE_VALUE){
            return NULL;
        }
        first_in = 1;
    }

    if(FindNextFile(hFind,&FindFileData)!=0 ){
        while((FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0 &&
            wcscmp(FindFileData.cFileName,L".")==0 ||
            wcscmp(FindFileData.cFileName,L"..")==0)        //判断是文件夹&&表示为"."||表示为"."
        {
            FindNextFile(hFind,&FindFileData);
        }

        if((FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0)    //如果不是文件夹
        {
            return FindFileData.cFileName;            //输出完整路径
        }
    }


    return NULL;
}


void stop_find_file()
{
    FindClose(hFind);
}

