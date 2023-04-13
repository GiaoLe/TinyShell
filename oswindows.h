#ifndef OSWRAPPER_H
#define OSWRAPPER_H

#pragma warning(push, 3)
// C header files
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <string>
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
// C++ header file
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;
#pragma warning(pop)

#pragma warning(disable: 4996)

#define USH_TOK_DELIM " \t\r\n\a"

void loop(void);
int execute(vector<string> args);
int ush_launch(char** args);
int ush_cd(char** args);
int ush_echo(char** args);
int ush_clear(char** args);
int ush_list(char** args);
int ush_pwd(char** args);
int ush_help(char** args);
int ush_kill(char** args);
int ush_exit(char** args);

#endif