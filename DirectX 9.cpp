//#define USETHREADS	//spawn a second renderer thread and use busy message reading for input
#define MODEZERO		//comment out for human benchmark mode
//#define FPSLIMIT 4000 //comment out for uncapped FPS

//set these for different viewport resolution, for example
//#define HEIGHT 480
//#define WIDTH  640

#define HEIGHT GetSystemMetrics(SM_CYSCREEN)
#define WIDTH  GetSystemMetrics(SM_CXSCREEN)

 


//<----------- no need to change below this point ----------->

#include <d3d9.h>

//if linker can't find DirectX SDK, you can use these
/*#ifdef _M_IX86 //32 bit compile
#pragma comment (lib, "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/d3d9.lib")
#else
#pragma comment (lib, "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x64/d3d9.lib")
#endif*/

D3DCOLOR black = D3DCOLOR(0x00000000);
D3DCOLOR white = D3DCOLOR(0x00FFFFFF);

#ifdef MODEZERO
D3DCOLOR currentColor = white;
#else
#include <iostream>
#include <iomanip> // std::setprecision
#include <chrono> //used for time
D3DCOLOR currentColor = black;
D3DCOLOR red = D3DCOLOR(0x00FF0000);
D3DCOLOR green = D3DCOLOR(0x0000FF00);
auto redToGreenTime = std::chrono::high_resolution_clock::now();
auto greenStartTime = redToGreenTime;
auto greenClickTime = redToGreenTime;
auto redStartTime = redToGreenTime;
int state = 0; //0 stats screen, 1 red waiting screen, 2 green
int errors = 0;
int click = 0;
long long lastClick, minClick = 999999999999, maxClick = 0, averageClick = 0, clickSum = 0, clickAmount = 0;
#endif

#ifdef USETHREADS
#include <thread>
#endif

#ifdef FPSLIMIT
#include <chrono> //used for time
float frameTimeMicro = FPSLIMIT == 0 ? 0 : 1000000 / FPSLIMIT;
auto start = std::chrono::high_resolution_clock::now();
#else
float frameTimeMicro = 0;
#endif

bool stop = false;
long long microseconds = 1;

UINT sizeofRAWINPUTHEADER = sizeof(RAWINPUTHEADER);
RAWINPUT* raw_buf = (PRAWINPUT)malloc(800); //this dirty hack probably saves nanoseconds. worth it.
UINT cb_size = 0;
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INPUT: {
		//get the size of the RAWINPUT structure returned
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &cb_size, sizeofRAWINPUTHEADER);

		//allocate memory RAWINPUT structure
		//raw_buf = (PRAWINPUT)malloc(cb_size);
		//if (!raw_buf) { stop = TRUE; return false; }

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw_buf, &cb_size, sizeofRAWINPUTHEADER);
#ifndef MODEZERO
			if (raw_buf->header.dwType == RIM_TYPEMOUSE &&
				(raw_buf->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN ||
					raw_buf->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN)) { //only triggers once per button press

				if (state == 0) {
					system("CLS"); //clear console
					state = 1;
					currentColor = red;
					//pick a time to transition screen from red to green
					int rnd = rand() % 1000 + 3000; //(between 3 and 4 seconds)
					auto now = std::chrono::high_resolution_clock::now();
					redToGreenTime = now + std::chrono::milliseconds(rnd);
					redStartTime = now;
				}
				else if (state == 1) { //too early click
					auto now = std::chrono::high_resolution_clock::now();
					auto timeDiff = now - redStartTime;
					if (std::chrono::duration_cast<std::chrono::microseconds>(timeDiff).count() >= 500000) { //grace period for double clicks
						state = 0;
						currentColor = black;
						errors++;
					}
				}
				else if (state == 2) {
					state = 0;
					currentColor = black;
					greenClickTime = std::chrono::high_resolution_clock::now();
					auto timeDiff = greenClickTime - greenStartTime;
					lastClick = std::chrono::duration_cast<std::chrono::microseconds>(timeDiff).count();
					if (lastClick < minClick) minClick = lastClick;
					if (lastClick > maxClick) maxClick = lastClick;
					clickSum += lastClick;
					clickAmount++;
					averageClick = clickSum / clickAmount;

					std::cout << "\n";
					std::cout << "  last click: " << std::fixed << std::setprecision(1) << (double)lastClick / 1000 << " ms\n";
					std::cout << "  min click: " << (double)minClick / 1000 << " ms\n";
					std::cout << "  max click: " << (double)maxClick / 1000 << " ms\n";
					std::cout << "  average click: " << (double)averageClick / 1000 << " ms\n";
					std::cout << "  successful clicks: " << clickAmount << "\n";
					std::cout << "  early clicks: " << errors << "\n";
				}
			}
#endif
#ifndef MODEZERO
			if (raw_buf->header.dwType == RIM_TYPEKEYBOARD) {
#endif
				if (raw_buf->data.keyboard.Message == WM_KEYDOWN || raw_buf->data.keyboard.Message == WM_SYSKEYDOWN)
#ifdef MODEZERO
					if (raw_buf->data.keyboard.VKey == 0x57)  // W key
						currentColor = white;
					else if (raw_buf->data.keyboard.VKey == 0x42)  // B key
						currentColor = black;
					else 
#endif
						if (raw_buf->data.keyboard.VKey == 0x1B) {  // ESC key
						stop = TRUE;
						break;
					}
		
#ifndef MODEZERO
		}
#endif
		if (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT) 
			DefWindowProc(hWnd, msg, wParam, lParam); //The application must call DefWindowProc so the system can perform cleanup.
		break;
	}
	case WM_CLOSE:
		stop = TRUE;
		DestroyWindow(hWnd);
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_DESTROY:
		stop = TRUE;
		PostQuitMessage(0);
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_QUIT:
		stop = TRUE;
		return DefWindowProc(hWnd, msg, wParam, lParam);
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int main() //for console purposes.
{
	return WinMain(GetModuleHandle(NULL), NULL, NULL, SW_SHOWNORMAL);
}

inline void renderFunc(LPDIRECT3DDEVICE9 d3ddev) {
#ifdef USETHREADS
	do {
#endif

#ifdef FPSLIMIT
		auto elapsed = std::chrono::high_resolution_clock::now() - start;
		microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
#endif
#ifndef MODEZERO
		if (state == 1 && std::chrono::high_resolution_clock::now() >= redToGreenTime) {
			state = 2;
			currentColor = green;
			greenStartTime = std::chrono::high_resolution_clock::now(); //keep actual green start timer
		}
#endif

#ifdef FPSLIMIT
		if (microseconds >= frameTimeMicro) {
			start = std::chrono::high_resolution_clock::now();
#endif
			d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, currentColor, 0.0f, 0);
			d3ddev->Present(NULL, NULL, NULL, NULL);
#ifdef FPSLIMIT
		}
#endif
#ifdef USETHREADS
	} while (!stop);
#endif
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;
	MSG msg;
	HRESULT res;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"somewindowclass";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(0, L"somewindowclass", L"test",
		WS_EX_TOPMOST | WS_POPUP,
		0, 0, (UINT)GetSystemMetrics(SM_CXSCREEN), (UINT)GetSystemMetrics(SM_CYSCREEN),
		NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	LPDIRECT3D9 d3d;
	LPDIRECT3DDEVICE9 d3ddev;
	D3DPRESENT_PARAMETERS d3dpp;

	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = FALSE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = (UINT)GetSystemMetrics(SM_CXSCREEN);
	d3dpp.BackBufferHeight = (UINT)GetSystemMetrics(SM_CYSCREEN);
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; //disable vsync

	d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);

	D3DVIEWPORT9 pViewport = { 0, 0, (DWORD)WIDTH, (DWORD)HEIGHT,0.0,1.0 };
	res = d3ddev->SetViewport(&pViewport);
	if (res != S_OK) return -1;

	if (d3ddev == NULL) return -1;

	RAWINPUTDEVICE Keyboard;
	Keyboard.usUsagePage = 0x01;
	Keyboard.usUsage = 0x06; //keyboard
	Keyboard.dwFlags = RIDEV_NOLEGACY;
	Keyboard.hwndTarget = hWnd;
	if (!RegisterRawInputDevices(&Keyboard, 1, sizeof(RAWINPUTDEVICE))) return -1;

#ifndef MODEZERO
	RAWINPUTDEVICE Mouse;
	Mouse.usUsagePage = 0x01;
	Mouse.usUsage = 0x02; //mouse
	Mouse.dwFlags = RIDEV_NOLEGACY;
	Mouse.hwndTarget = hWnd;
	if (!RegisterRawInputDevices(&Mouse, 1, sizeof(RAWINPUTDEVICE))) return -1;
#endif

	ShowCursor(FALSE);
	SetCursor(NULL);
	d3ddev->ShowCursor(FALSE);
	
#ifdef USETHREADS
	std::thread renderThread; //declare a thread without launching it
	renderThread = std::thread(renderFunc, d3ddev); //this launches the thread
#endif

	while (!stop) {
#ifdef USETHREADS
		GetMessage(&msg, NULL, 0, 0);
		DispatchMessage(&msg);
#else
		while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) //unthreaded - one time check per loop
			DispatchMessage(&msg);
		renderFunc(d3ddev); //draw frame
#endif
	}

#ifdef USETHREADS
	renderThread.join(); //var 'stop' ends the thread
#endif
	d3ddev->Release();   //release d3d
	d3d->Release();

	free(raw_buf);
	return 0;			 //all done
}