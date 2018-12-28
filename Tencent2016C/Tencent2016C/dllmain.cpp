// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "TencentAPI.h"
#include <tlhelp32.h>
#include <shlwapi.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

extern "C" BOOL _declspec(dllexport) CheckVMWare1();
extern "C" BOOL _declspec(dllexport) CheckVMWare2();
extern "C" BOOL _declspec(dllexport) CheckVMWare3();
extern "C" BOOL _declspec(dllexport) CheckVMWare4();
extern "C" BOOL _declspec(dllexport) CheckVMWare5();
extern "C" BOOL _declspec(dllexport) CheckVMWare6();
extern "C" BOOL _declspec(dllexport) CheckVMWare7();
extern "C" BOOL _declspec(dllexport) CheckVMWare8();


//1.查询I/O通信端口
BOOL CheckVMWare1()
{
	BOOL bResult = TRUE;
	__try
	{
		__asm
		{
			push   edx
			push   ecx
			push   ebx				//保存环境
			mov    eax, 'VMXh'
			mov    ebx, 0			//将ebx清零
			mov    ecx, 10			//指定功能号，用于获取VMWare版本，为0x14时获取VM内存大小
			mov    edx, 'VX'		//端口号
			in     eax, dx			//从端口edx 读取VMware到eax
			cmp    ebx, 'VMXh'		//判断ebx中是否包含VMware版本’VMXh’，若是则在虚拟机中
			setz [bResult]			//设置返回值
			pop    ebx				//恢复环境
			pop    ecx
			pop    edx
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)	//如果未处于VMware中，则触发此异常
	{
		bResult = FALSE;
	}
	return bResult;
}

//2.通过MAC地址检测
BOOL CheckVMWare2()
{
	string strMac;
	getMacAddr(strMac);
	if (strMac == "00-05-69" || strMac == "00-0c-29" || strMac == "00-50-56")
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//3.CPUID检测
BOOL CheckVMWare3()
{
	DWORD dwECX = 0;
	bool b_IsVM = true;
	_asm
	{
		pushad;
		pushfd;
		mov eax, 1;
		cpuid;
		mov dwECX, ecx;
		and ecx, 0x80000000;//取最高位
		test ecx, ecx;		//检测ecx是否为0
		setz[b_IsVM];		//为零 (ZF=1) 时设置字节
		popfd;
		popad;
	}
	if (b_IsVM)				//宿主机
	{
		return FALSE;
	}
	else					//虚拟机
	{
		return TRUE;
	}
}

//4.通过主板序列号、型号、系统盘所在磁盘名称等其他硬件信息
BOOL CheckVMWare4()
{
	string table = "Win32_DiskDrive";
	wstring wcol = L"Caption";
	string ret;
	ManageWMIInfo(ret, table, wcol);
	if (ret.find("VMware") != string::npos)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//5.搜索特定进程检测
BOOL CheckVMWare5()
{
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);//拍摄快照
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	BOOL bMore = Process32First(hProcessSnap, &pe32);					  //遍历第一个进程
	while (bMore)
	{
		if (wcscmp(pe32.szExeFile, L"vmtoolsd.exe") == 0)				  //注意此处要用wcscmp（pe32.szExeFile是 WCHAR*）
		{
			return TRUE;
		}

		bMore = Process32Next(hProcessSnap, &pe32);					      //遍历下一个进程
	}
	CloseHandle(hProcessSnap);
	return FALSE;
}



//6.通过注册表检测
BOOL CheckVMWare6()
{
	HKEY hkey;
	if (RegOpenKey(HKEY_CLASSES_ROOT, L"\\Applications\\VMwareHostOpen.exe", &hkey) == ERROR_SUCCESS) 
	{
		return TRUE;	//RegOpenKey函数打开给定键,如果存在该键返回ERROR_SUCCESS
	}
	else
	{
		return FALSE;
	}
}




//7.通过服务检测
BOOL CheckVMWare7()
{
	int menu = 0;
	SC_HANDLE SCMan = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);	//打开服务控制管理器
	if (SCMan == NULL)
	{
		cout << GetLastError() << endl;
		printf("OpenSCManager Eorror/n");
		return -1;
	}
	LPENUM_SERVICE_STATUSA service_status;
	DWORD cbBytesNeeded = NULL;
	DWORD ServicesReturned = NULL;
	DWORD ResumeHandle = NULL;
	service_status = (LPENUM_SERVICE_STATUSA)LocalAlloc(LPTR, 1024 * 64);
	bool ESS = EnumServicesStatusA(SCMan,										//遍历服务
		SERVICE_WIN32,
		SERVICE_STATE_ALL,
		(LPENUM_SERVICE_STATUSA)service_status,
		1024 * 64,
		&cbBytesNeeded,
		&ServicesReturned,
		&ResumeHandle);
	if (ESS == NULL)
	{
		printf("EnumServicesStatus Eorror/n");
		return -1;
	}
	for (int i = 0; i < ServicesReturned; i++)
	{
		if (strstr(service_status[i].lpDisplayName, "Virtual Disk") != NULL || strstr(service_status[i].lpDisplayName, "VMware Tools") != NULL)
		{
			return TRUE;
		}
	}
	CloseServiceHandle(SCMan);
	return FALSE;
}

//文件路径检测
BOOL CheckVMWare8()
{
	if (PathIsDirectory(L"C:\\Program Files\\VMware\\") == 0)	
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}



//通过时间差检测
BOOL CheckVMWareTmp()
{
	__asm
	{
		rdtsc			//RDTSC指令将计算机启动以来的CPU运行周期数存放到EDX：EAX里面，其中EDX是高位，而EAX是低位。
		xchg ebx, eax	//测试此条指令运行时间
		rdtsc
		sub eax, ebx	//时间差
		cmp eax, 0xFF
		jg detected
	}
	return FALSE;
detected:
	return TRUE;
}