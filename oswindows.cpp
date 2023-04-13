#include "oswindows.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

// List of builtin commands, followed by their corresponding functions.
const char* builtin_names[] = { "list",  "kill", "cd",   "echo",
							   "clear", "pwd",  "help", "exit" };

int (*builtin_functions[])(char**) = { &ush_list, &ush_kill,  &ush_cd,
									   &ush_echo, &ush_clear, &ush_pwd,
									   &ush_help, &ush_exit };

int ush_builtins_number() { return sizeof(builtin_names) / sizeof(char*); }

void loop(void) {
	bool running;
	do {
		ush_pwd(nullptr); printf("> ");
		//Take input from user
		string userInput; cin >> userInput;
		//Split input string into tokens
		istringstream iss(userInput);
		vector<string> arguments;
		copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(arguments));
		//Execute the command
		running = execute(arguments);
	} while (running);
}

int execute(vector<string> args) {
	if (args.empty()) {
		return 1;
	}

	std::vector<char*> result;

	result.reserve(args.size() + 1);

	std::transform(args.begin(), args.end(),
		std::back_inserter(result),
		[](std::string& s) { return s.data(); });
	result.push_back(nullptr);

	for (int i = 0; i < ush_builtins_number(); i++) {
		if (strcmp(result[0], builtin_names[i]) == 0) {
			return (*builtin_functions[i])(result.data());
		}
	}

	return ush_launch(result.data());
}

int ush_echo(char** args) {
	int flag;

	/* This utility may NOT do getopt(3) option parsing. */
	if (*++args && !strcmp(*args, "-n")) {
		++args;
		flag = 1;
	}
	else {
		flag = 0;
	}

	while (*args) {
		(void)fputs(*args, stdout);
		if (*++args) {
			putchar(' ');
		}
	}

	if (!flag) {
		putchar('\n');
	}
	return 1;
}

int ush_exit(char** args) { return 0; }

int ush_help(char** args) {
	printf("*** KASH ***\nBuilt-in commands:\n");
	for (int i = 0; i < ush_builtins_number(); i++) {
		printf("%d: %s\n", i + 1, builtin_names[i]);
	}
	return 1;
}

void PrintProcessNameAndID(DWORD processID) {
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

	// Get a handle to the process.

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE, processID);

	// Get the process name.

	if (hProcess != NULL) {
		HMODULE hMod;
		DWORD cbNeeded;

		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
			GetModuleBaseName(hProcess, hMod, szProcessName,
				sizeof(szProcessName) / sizeof(TCHAR));
		}
	}

	// Print the process name and identifier.

	_tprintf(TEXT("%s  (PID: %u)\n"), szProcessName, processID);

	// Release the handle to the process.
	if (hProcess) {
		CloseHandle(hProcess);
	}
}

int ush_list(char** args) {
	// Get the list of process identifiers.

	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
		return 1;
	}

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.

	for (i = 0; i < cProcesses; i++) {
		if (aProcesses[i] != 0) {
			PrintProcessNameAndID(aProcesses[i]);
		}
	}
}

int ush_launch(char** args) {
	// define something for Windows (32-bit and 64-bit, this part is common
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL, (LPTSTR)args[0], NULL, NULL, FALSE, 0, NULL, NULL,
		&si, &pi)) {
		switch (GetLastError()) {
		case ERROR_INVALID_PARAMETER:
			printf("CreateProcess failed (%d).\n", GetLastError());
			break;
		case ERROR_FILE_NOT_FOUND:
			printf("Unknown internal or external command.\n");
			break;
		}
		return -1;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 1;
}

void killProcessByID(int process_id) {
	const auto explorer = OpenProcess(PROCESS_TERMINATE, false, process_id);
	if (explorer == NULL) {
		std::cout << "Invalid PID" << std::endl;
		return;
	}
	TerminateProcess(explorer, 1);
	CloseHandle(explorer);
}

int ush_kill(char** args) {
	try {
		int process_id = std::stoi(args[1]);
		killProcessByID(process_id);
	}
	catch (const std::exception&) {
		std::cout << "Invalid PID" << std::endl;
	}
	return 1;
}

int ush_cd(char** args) {
	if (args[1] == NULL) {
		fprintf(stderr, "ush: expected argument to \"cd\"\n");
	}
	else {
		if (!SetCurrentDirectory((LPWSTR)args[1])) {
			printf("cd failed (%d)\n", GetLastError());
		}
	}
	return 1;
}

void clear(HANDLE hConsole) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	SMALL_RECT scrollRect;
	COORD scrollTarget;
	CHAR_INFO fill;

	// Get the number of character cells in the current buffer.
	if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
		return;
	}

	// Scroll the rectangle of the entire buffer.
	scrollRect.Left = 0;
	scrollRect.Top = 0;
	scrollRect.Right = csbi.dwSize.X;
	scrollRect.Bottom = csbi.dwSize.Y;

	// Scroll it upwards off the top of the buffer with a magnitude of the entire
	// height.
	scrollTarget.X = 0;
	scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

	// Fill with empty spaces with the buffer's default text attribute.
	fill.Char.UnicodeChar = TEXT(' ');
	fill.Attributes = csbi.wAttributes;

	// Do the scroll
	ScrollConsoleScreenBuffer(hConsole, &scrollRect, NULL, scrollTarget, &fill);

	// Move the cursor to the top left corner too.
	csbi.dwCursorPosition.X = 0;
	csbi.dwCursorPosition.Y = 0;

	SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
}

int ush_clear(char** args) {
	HANDLE hStdout;
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	clear(hStdout);
	return 1;
}

int ush_pwd(char** args) {
	wchar_t path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);
	wcout << path;
	if (args != NULL) {
		cout << endl;
	}
	return 1;
}

#endif