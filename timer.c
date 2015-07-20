#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <tlhelp32.h>

// http://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c

HANDLE GetProcessByName(TCHAR* name)
{
    DWORD pid = 0;

    // Create toolhelp snapshot.
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(process));
    process.dwSize = sizeof(process);

    // Walkthrough all processes.
    if (Process32First(snapshot, &process))
    {
        do
        {
            // Compare process.szExeFile based on format of name, i.e., trim file path
            // trim .exe if necessary, etc.
            if (stricmp(process.szExeFile, name) == 0)
            {
                pid = process.th32ProcessID;
                break;
            }
        } while(Process32Next(snapshot, &process));
    }

    CloseHandle(snapshot);

    if (pid != 0)
    {
        return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }

    // Not found


    return NULL;
}

typedef struct {
    BYTE s;
    BYTE unk1;
    BYTE m;
    BYTE h;
    BYTE unk2; // day?
    BYTE unk3;
    BYTE unk4;
    BYTE unk5;
    DWORD t_ms;
} data;

DWORD ts;
BYTE state;

char o_t[11] = "XX:XX:XX.X";

HWND hwnd;

void write_time() {
    static DWORD pt;
    char n_t[11];
    if (state == 0) strcpy(n_t, "          ");
    else if (state == 1) strcpy(n_t, "00:00:00.0");
    else if (state == 2 || state == 3) {
        DWORD t = GetTickCount();
        if (state == 3) ts += t - pt;
        pt = t;
        DWORD d = t - ts;
        BYTE s = (d / 1000) % 60;
        BYTE m = (d / 1000 / 60) % 60;
        BYTE h = d / 1000 / 60 / 60;
        BYTE tens = d % 1000 / 100;
        sprintf(n_t, "%02d:%02d:%02d.%01d", h, m, s, tens);
    }
    if (strcmp(o_t, n_t) != 0) {
        strcpy(o_t, n_t);
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

BYTE pause_state = 0;
BYTE reset_state = 0;
const WORD pause_key = 268; // PAUSE
const WORD reset_key = 267; // SCROLL LOCK
volatile WPARAM hotkey = 0;

DWORD WINAPI timer_thread(LPVOID lpParameter) {
    state = 0;
    write_time();

    HANDLE proc;
    do {
        Sleep(40);
        if (hotkey) {
            if (hotkey == 2) state = 1;
            if (hotkey == 1) {
                if (state == 3) state = 2;
                else if (state ==2) state = 3;
                else {
                    state = 2;
                    ts = GetTickCount();
                }
            }
            hotkey = 0;
        }
        write_time();

        WORD keytab[288];
        data d;
        while(
            (proc = GetProcessByName("gta_sa.exe")) &&
            ReadProcessMemory(proc, (LPCVOID)0xBE5284, &d, sizeof(data), NULL) &&
            ReadProcessMemory(proc, (LPCVOID)0xBE8300, &keytab, sizeof(keytab), NULL)
        ) {
            CloseHandle(proc);
            if (keytab[reset_key] && !reset_state) {
                state = 1;
            }
            if (keytab[pause_key] && !pause_state) {
                if (state == 3) state = 2;
                else if (state ==2) state = 3;
                else {
                    state = 2;
                    ts = GetTickCount();
                }
            }
            if (state == 0) state = 1;
            if (d.h == 6 && d.m == 30 && d.s == 0 && d.unk2 == 1 && state == 1) {
                state = 2; // start timer
                ts = GetTickCount();
            } else if (d.t_ms == 0 && d.h == 8 && d.m == 0 && d.s == 0 && d.unk2 == 1) {
                state = 1; // reset timer
            }
            write_time();
            pause_state = keytab[pause_key];
            reset_state = keytab[reset_key];
            Sleep(40);
            hotkey = 0;
        }
    } while(1);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HFONT hf;
    if (!hf) {
        AddFontResourceEx("digital-7 (mono).ttf",
                          FR_PRIVATE,
                          NULL);
        hf = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Digital-7");
    }
    switch(message)
    {
    case WM_CHAR:
    {
        if(wParam==VK_ESCAPE)
            SendMessage(hwnd,WM_CLOSE,0,0);
        return 0;
    }
    case WM_PAINT:
    {
        COLORREF g_white = RGB(255, 255, 255);
        COLORREF g_black = RGB(0, 0, 0);
        PAINTSTRUCT ps;
        HDC hDC;
        hDC=BeginPaint(hwnd,&ps);
        HFONT hfOld = SelectObject(hDC, hf);
        SetBkMode(hDC, TRANSPARENT);
        int o[]= {-2, -1, 1, 2, 0};
        for(int x=0; x<5; x++) {
            for(int y=0; y<5; y++) {
                SetTextColor(hDC, o[x]==0&&o[y]==0?g_white:g_black);
                TextOut(hDC,5+o[x],5+o[y],o_t,sizeof(o_t)-1);
            }
        }
        SelectObject(hDC, hfOld);
        EndPaint(hwnd,&ps);
        return 0;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_HOTKEY:
    {
        hotkey = wParam;
        return 0;
    }
    }

    return DefWindowProc (hwnd, message, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hIns, HINSTANCE hPrev, LPSTR lpszArgument, int nCmdShow)
{
    char szClassName[]="GTA SA Timer";
    WNDCLASSEX wc;
    MSG messages;

    wc.hInstance=hIns;
    wc.lpszClassName=szClassName;
    wc.lpfnWndProc = WndProc;
    wc.style = CS_DBLCLKS;
    wc.cbSize = sizeof (WNDCLASSEX);
    wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground=CreateSolidBrush(RGB(1, 1, 1));
    RegisterClassEx(&wc);
    hwnd=CreateWindow(szClassName,szClassName,WS_OVERLAPPEDWINDOW,0,0,148,65,HWND_DESKTOP,NULL,hIns,NULL);
    ShowWindow(hwnd, nCmdShow);
    RegisterHotKey(hwnd, 1, 0x4000, VK_PAUSE);
    RegisterHotKey(hwnd, 2, 0x4000, VK_SCROLL);
    HANDLE thread = CreateThread(0, 0, timer_thread, NULL, 0, NULL);
    while(GetMessage(&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
    CloseHandle(thread);
    return messages.wParam;
}
