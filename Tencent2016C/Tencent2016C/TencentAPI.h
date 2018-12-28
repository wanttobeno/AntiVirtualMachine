#pragma once
#include <string>
#include <iostream>
#include <windows.h>

using namespace std;

//获取MAC地址
extern void getMacAddr(string &strMac);

//通过WMI获取主机信息
extern BOOL ManageWMIInfo(string &result, string table, wstring wcol);


