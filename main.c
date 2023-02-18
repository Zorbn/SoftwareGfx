#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <Windows.h>
#include <sys/timeb.h>
#include <stdio.h>

LRESULT CALLBACK window_process_message(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);

struct Frame {
	BITMAPINFO bitmap_info;
	HBITMAP bitmap;
	HDC device_context;
	HBRUSH background_brush;
	bool quit;

	int32_t width;
	int32_t height;
	int32_t window_width;
	int32_t window_height;
	int32_t min_window_width;
	int32_t min_window_height;
	uint32_t* pixels;
};

const int32_t default_window_width = 256;
const int32_t default_window_height = 240;

int WINAPI WinMain(_In_ HINSTANCE h_instance, _In_opt_ HINSTANCE h_prev_instance, _In_ PSTR p_cmd_line, _In_ int n_cmd_show) {
	const wchar_t window_class_name[] = L"Window Class";
	WNDCLASS window_class = { 0 };
	window_class.lpfnWndProc = window_process_message;
	window_class.hInstance = h_instance;
	window_class.lpszClassName = window_class_name;
	RegisterClass(&window_class);

	static struct Frame frame = { 0 };
	frame.width = default_window_width;
	frame.height = default_window_height;

	frame.bitmap_info.bmiHeader.biWidth = frame.width;
	frame.bitmap_info.bmiHeader.biHeight = frame.height;
	frame.bitmap_info.bmiHeader.biSize = sizeof(frame.bitmap_info.bmiHeader);
	frame.bitmap_info.bmiHeader.biPlanes = 1;
	frame.bitmap_info.bmiHeader.biBitCount = 32;
	frame.bitmap_info.bmiHeader.biCompression = BI_RGB;
	frame.device_context = CreateCompatibleDC(0);
	frame.background_brush = CreateSolidBrush(BLACK_BRUSH);

	frame.bitmap = CreateDIBSection(NULL, &frame.bitmap_info, DIB_RGB_COLORS, &frame.pixels, 0, 0);

	if (!frame.bitmap) {
		return -1;
	}

	SelectObject(frame.device_context, frame.bitmap);

	RECT window_size = { 0 };
	window_size.right = frame.width;
	window_size.bottom = frame.height;
	AdjustWindowRect(&window_size, WS_OVERLAPPEDWINDOW, false);
	frame.min_window_width = window_size.right - window_size.left;
	frame.min_window_height = window_size.bottom - window_size.top;

	HWND window_handle = CreateWindowEx(0, window_class_name, L"Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		window_size.right - window_size.left, window_size.bottom - window_size.top, NULL, NULL, h_instance, NULL);

	if (window_handle == NULL) {
		return -1;
	}

	SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)&frame);
	ShowWindow(window_handle, n_cmd_show);

	MSG message = { 0 };
	int32_t p = 0;

	struct timeb start, end;
	ftime(&start);

	char str[256];

	while (!frame.quit) {
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
			DispatchMessage(&message);
		}

		for (int32_t i = 0; i < frame.width / 5; i++) {
			if (frame.pixels) {
				frame.pixels[(p++) % (frame.width * frame.height)] = 0xff000000 | (0xff << ((p % 3) * 8));
				for (int32_t j = 0; j < 10; j++) frame.pixels[(rand() + (rand() << 16)) % (frame.width * frame.height)] = 0;
			}
		}

		InvalidateRect(window_handle, NULL, FALSE);
		UpdateWindow(window_handle);

		ftime(&end);
		sprintf_s(str, sizeof(str), "ms: %"PRId64"\n", (end.time - start.time) + (end.millitm - start.millitm));
		OutputDebugStringA(str);
		start = end;
	}

	DeleteObject(frame.background_brush);

	return 0;
}

LRESULT CALLBACK window_process_message(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
	static struct Frame* frame;

	if (!frame) {
		frame = (struct Frame*)GetWindowLongPtr(window_handle, GWLP_USERDATA);
	}

	switch (message) {

	case WM_CREATE: {

	} break;

	case WM_QUIT:
	case WM_DESTROY: {
		frame->quit = true;
	} break;

	case WM_PAINT: {
		static PAINTSTRUCT paint;
		static HDC device_context;

		device_context = BeginPaint(window_handle, &paint);

		RECT scaled_window = { 0 };

		SelectObject(device_context, frame->background_brush);

		if (frame->window_width < frame->window_height) {
			scaled_window.right = frame->window_width;
			scaled_window.bottom = (LONG)((frame->height / (float)frame->width) * scaled_window.right);
			scaled_window.top = (frame->window_height - scaled_window.bottom) / 2;
			scaled_window.bottom += scaled_window.top;

			Rectangle(device_context, paint.rcPaint.left, 0, paint.rcPaint.right, scaled_window.top);
			Rectangle(device_context, paint.rcPaint.left, scaled_window.bottom, paint.rcPaint.right, paint.rcPaint.bottom);
		} else {
			scaled_window.bottom = frame->window_height;
			scaled_window.right = (LONG)((frame->width / (float)frame->height) * scaled_window.bottom);
			scaled_window.left = (frame->window_width - scaled_window.right) / 2;
			scaled_window.right += scaled_window.left;

			Rectangle(device_context, 0, paint.rcPaint.top, scaled_window.left, paint.rcPaint.bottom);
			Rectangle(device_context, scaled_window.right, paint.rcPaint.top, paint.rcPaint.right, paint.rcPaint.bottom);
		}

		StretchBlt(device_context, scaled_window.left, scaled_window.top, scaled_window.right - scaled_window.left,
			scaled_window.bottom - scaled_window.top, frame->device_context, 0, 0, frame->width, frame->height, SRCCOPY);
		EndPaint(window_handle, &paint);

		ValidateRect(window_handle, NULL);
	} break;
	
	case WM_SIZE: {
		frame->window_width = LOWORD(l_param);
		frame->window_height = HIWORD(l_param);
	} break;
	
	case WM_GETMINMAXINFO: {
		LPMINMAXINFO lp_mmi = (LPMINMAXINFO)l_param;
		
		if (frame) {
			lp_mmi->ptMinTrackSize.x = frame->min_window_width;
			lp_mmi->ptMinTrackSize.y = frame->min_window_height;
		}
	} break;

	default: {
		return DefWindowProc(window_handle, message, w_param, l_param);
	} break;

	}

	return 0;
}