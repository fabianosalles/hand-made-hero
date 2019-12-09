#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct Win32OffscreenBuffer{
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
	int bytesPerPixel;
};

//TODO: global for now
global_variable bool running;
global_variable Win32OffscreenBuffer globalBackBuffer;

struct Win32WindowDimension {
	int width;
	int height;
};

internal Win32WindowDimension Win32GetWindowDimension(HWND window) {
	Win32WindowDimension result;
	RECT clientRect;
	GetClientRect(window, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;
	return result;
}

internal void RenderWeirdGradient(Win32OffscreenBuffer buffer, int xOffset, int yOffset) {
	uint8 *row = (uint8 *)buffer.memory;
	for (int y = 0; y < buffer.height; ++y) {
		uint32 *pixel = (uint32 *)row;
		for (int x = 0; x < buffer.width; ++x) {
			uint8 r = 0;
			uint8 g = (x + xOffset);
			uint8 b = (y + yOffset);
			*pixel++ = ((g << 8) | b);
		}
		row += buffer.pitch;
	}
}

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *buffer, int width, int height) {

	if (buffer->memory) {
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}
	buffer->width = width;
	buffer->height = height;
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height; //top down image
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;
	buffer->bytesPerPixel = 4;
	int memorySize = buffer->bytesPerPixel * buffer->width * buffer->height;
	buffer->memory = VirtualAlloc(0, memorySize, MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = buffer->width * buffer->bytesPerPixel;	
}

internal void Win32DisplayBufferInWindow(HDC deviceContext, 
				  int windowWidth, int windowHeight,
				  Win32OffscreenBuffer buffer, 
				  int x, int y, int width, int height) {

	//correct the aspect ratio
	
	StretchDIBits(deviceContext, 
		0, 0, windowWidth, windowHeight,
		0, 0, buffer.width, buffer.height,
		buffer.memory, 
		&buffer.info,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;
	switch (message)
	{
	case WM_SIZE: {

	}break;

	case WM_CLOSE: {
		//TOOD: handle this witha message to the user?
		running = false;
	}break;

	case WM_DESTROY: {
		//TODO: handle this as an error and recreate the window?
		running = false;
	}break;

	case WM_ACTIVATEAPP: {
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	}break;

	case WM_PAINT: {
		PAINTSTRUCT paint;
		HDC deviceContext = BeginPaint(window, &paint);
		int x = paint.rcPaint.left;
		int y = paint.rcPaint.top;
		LONG height = paint.rcPaint.bottom -= paint.rcPaint.top;
		LONG width = paint.rcPaint.right -= paint.rcPaint.left;
		
		Win32WindowDimension dimension = Win32GetWindowDimension(window);
		Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, 
					globalBackBuffer, 
					x, y, width, height);
		EndPaint(window, &paint);
	}break;

	default: {
		OutputDebugStringA("default\n");
		result = DefWindowProc(window, message, wParam, lParam);
	}break;

	}
	return result;
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstace, LPSTR lpCmdLine, int nCmdShow) {

	WNDCLASS windowClass = { };
	Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

	windowClass.style = CS_HREDRAW|CS_VREDRAW;
	windowClass.lpfnWndProc = MainWindowCallback;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "HandmadeHeroWindowClass";
	if (RegisterClass(&windowClass)) {
		HWND window = CreateWindowExA(
			0, windowClass.lpszClassName, 
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			0, 0, hInstance, 0);
		if (window) {
			running = true;
			int xOffset = 0;
			int yOffset = 0;
			while(running) {
				MSG message;
				while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)){
					if (message.message == WM_QUIT) {
						running = false;
					}
					TranslateMessage(&message);
					DispatchMessageA(&message);				
				}

				RenderWeirdGradient(globalBackBuffer, xOffset, yOffset);
				
				HDC deviceContext = GetDC(window);
				Win32WindowDimension dimension = Win32GetWindowDimension(window);				
				Win32DisplayBufferInWindow( deviceContext, 
											dimension.width, dimension.height,
					                        globalBackBuffer, 
											xOffset, yOffset, 
											dimension.width, dimension.height);
				ReleaseDC(window, deviceContext);
				++xOffset;
				yOffset += 2;
			}
		}
		else {
			//TODO(fabiano): logging
		}
	}
	else {
		//TODO(fabiano): logging
	}

	return 0;
}

