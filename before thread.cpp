#include <d3d9.h>
#include <iostream>
#include <chrono> //used for time

using std::cin; //io
using std::cout;

#include <iomanip> // std::setprecision
#include <thread>

#pragma comment (lib, "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/d3d9.lib")

#define WIDTH 1920
#define HEIGHT 1080

int mode = 1; //0: black/white  1: human benchmark
int state = 0; //used in benchmark mode. 0 stats screen, 1 red waiting screen, 2 green
auto redToGreenTime = std::chrono::high_resolution_clock::now();
auto greenStartTime = redToGreenTime;
auto greenClickTime = redToGreenTime;
auto redStartTime = redToGreenTime;
int errors = 0;
int click = 0;
bool stop = FALSE;
long long lastClick, minClick = 999999999999, maxClick = 0, averageClick = 0, clickSum = 0, clickAmount = 0;

bool colorWhiteFlag = TRUE;
bool useViewport = FALSE;
float frameTimeMicro = 0; //desired frame time in microseconds

UINT sizeofRAWINPUTHEADER = sizeof(RAWINPUTHEADER);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INPUT: {
		RAWINPUT* raw_buf;
		UINT cb_size = 0;

		//get the size of the RAWINPUT structure returned
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &cb_size, sizeofRAWINPUTHEADER);

		//allocate memory RAWINPUT structure
		raw_buf = (PRAWINPUT)malloc(cb_size);
		if (!raw_buf) { stop = TRUE; return false; }

		//HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(lParam);

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw_buf, &cb_size, sizeofRAWINPUTHEADER)) {
			if (mode == 1 && raw_buf->header.dwType == RIM_TYPEMOUSE) {
				if (raw_buf->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN ||
					raw_buf->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN) //only triggers once per button press
				{
					if (state == 0) {
						system("CLS"); //clear console
						state = 1;
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
							errors++;
						}
					}
					else if (state == 2) {
						state = 0;
						greenClickTime = std::chrono::high_resolution_clock::now();
						auto timeDiff = greenClickTime - greenStartTime;
						lastClick = std::chrono::duration_cast<std::chrono::microseconds>(timeDiff).count();
						if (lastClick < minClick) minClick = lastClick;
						if (lastClick > maxClick) maxClick = lastClick;
						clickSum += lastClick;
						clickAmount++;
						averageClick = clickSum / clickAmount;

						cout << "\n";
						cout << "  last click: " << std::fixed << std::setprecision(1) << (double)lastClick / 1000 << " ms\n";
						cout << "  min click: " << (double)minClick / 1000 << " ms\n";
						cout << "  max click: " << (double)maxClick / 1000 << " ms\n";
						cout << "  average click: " << (double)averageClick / 1000 << " ms\n";
						cout << "  successful clicks: " << clickAmount << "\n";
						cout << "  early clicks: " << errors << "\n";
					}
				}
			}
			else if (raw_buf->header.dwType == RIM_TYPEKEYBOARD
				&& (raw_buf->data.keyboard.Message == WM_KEYDOWN || raw_buf->data.keyboard.Message == WM_SYSKEYDOWN))
				if (raw_buf->data.keyboard.VKey == 0x57)  // W key
					colorWhiteFlag = TRUE;
				else if (raw_buf->data.keyboard.VKey == 0x42)  // B key
					colorWhiteFlag = FALSE;
				else if (raw_buf->data.keyboard.VKey == 0x1B) {  // ESC key
					stop = TRUE;
					break;
				}
		}
		//if (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT) 
		//	DefWindowProc(hWnd, message, wParam, lParam); //The application must call DefWindowProc so the system can perform cleanup.
		//https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-input

		free(raw_buf);
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

void messageThread(HWND hWnd) {
	MSG msg;
	while (true) {
		GetMessage(&msg, hWnd, 0, 0); //, PM_REMOVE)) {
		DispatchMessage(&msg);
	}
}

int main() //for console purposes. can make these into a config file.
{
	float FPS = 4000;
	char chr;
	cout << "Enter FPS: ";
	cin >> FPS;
	if (FPS > 0) { frameTimeMicro = 1000000 / FPS; }
	cout << "mode (0 for black-white, 1 for human benchmark): ";
	cin >> mode;
	if (mode == 0) {
		cout << "narrow stripe? (y/n)";
		cin >> chr;
		if (chr == 'y') useViewport = TRUE;
	}
	return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;
	MSG msg;
	HRESULT res;
	UINT width=WIDTH, height=HEIGHT;
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
		0, 0, width, height,
		NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	LPDIRECT3D9 d3d;
	LPDIRECT3DDEVICE9 d3ddev;

	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = FALSE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = (UINT)GetSystemMetrics(SM_CXSCREEN);
	d3dpp.BackBufferHeight = (UINT)GetSystemMetrics(SM_CYSCREEN);
	//d3dpp.FullScreen_RefreshRateInHz = 0;

	d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);

	if (useViewport) { //make a viewport with a narrow stripe. useful for input lag?
		D3DVIEWPORT9 pViewport;
		int nScreenWidth, nScreenHeight;
		nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
		//pViewport = { 0, 0, (DWORD)nScreenWidth, (DWORD)nScreenHeight,0.0,1.0 };
		pViewport = { 0, 0, 30, 1080,0.0,1.0 };
		res = d3ddev->SetViewport(&pViewport);
		if (res != S_OK) return -1;
	}

	if (d3ddev == NULL) return -1;

	RAWINPUTDEVICE Rid[2];
	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x06; //keyboard
	Rid[0].dwFlags = RIDEV_NOLEGACY;
	Rid[0].hwndTarget = 0;
	Rid[1].usUsagePage = 0x01;
	Rid[1].usUsage = 0x02; //mouse
	Rid[1].dwFlags = RIDEV_NOLEGACY;
	Rid[1].hwndTarget = 0;
	if (!RegisterRawInputDevices(Rid, 2, sizeof(RAWINPUTDEVICE))) return -1;

	//SetCursor(NULL);
	//d3ddev->ShowCursor(FALSE);
	auto start = std::chrono::high_resolution_clock::now();
	long long microseconds = 1;
	D3DCOLOR black = D3DCOLOR(0x00000000);
	D3DCOLOR white = D3DCOLOR(0x00FFFFFF);
	D3DCOLOR red   = D3DCOLOR(0x00FF0000);
	D3DCOLOR green = D3DCOLOR(0x0000FF00);
	std::thread msgThread(messageThread, hWnd);
	
	while (TRUE) {
		//while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
			//if (msg.message == WM_INPUT) {
				
			//}
			//else {
				//TranslateMessage(&msg); //virtual key codes, unneeded as raw input is used
			//	DispatchMessage(&msg);
			//}
		//}
		
		if (stop) break;

		if (frameTimeMicro > 0) { //only check time if fps capped or benchmark mode
			auto elapsed = std::chrono::high_resolution_clock::now() - start;
			microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		}
		if (mode > 0 && state == 1) {
			if (std::chrono::high_resolution_clock::now() >= redToGreenTime) {
				state = 2;
				greenStartTime = std::chrono::high_resolution_clock::now(); //keep actual green start timer
			}
		}

		if (frameTimeMicro == 0 || microseconds >= frameTimeMicro) { //draw frame
			if (frameTimeMicro > 0) start = std::chrono::high_resolution_clock::now();
			if (mode == 0) {
				if (colorWhiteFlag)  d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, white, 0.0f, 0);
				else				 d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, black, 0.0f, 0);
			}
			else if (mode == 1) {
				if (state == 0)		 d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, black, 0.0f, 0);
				else if (state == 1) d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, red,   0.0f, 0);
				else				 d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, green, 0.0f, 0);
			}
			d3ddev->Present(NULL, NULL, NULL, NULL);
		}
	}

	d3ddev->Release();
	d3d->Release();

	return 0;
}