// desktop-capture.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <vector>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#pragma comment(lib, "opencv_world460d.lib")

using namespace std;
using namespace cv;

// alternative way timeout waiting console input, likes opencv highgui waitKey function
int cin_timeout(unsigned long timeout) {
#ifdef _WIN32
	auto cin_handle = GetStdHandle(STD_INPUT_HANDLE);
	// let console only accept keyboard input
	unsigned long mode;
	GetConsoleMode(cin_handle, &mode);
	mode = mode ^ ENABLE_MOUSE_INPUT ^ ENABLE_WINDOW_INPUT;
	SetConsoleMode(cin_handle, mode);
	// cleanup previous cin buffer
	FlushConsoleInputBuffer(cin_handle);
	//TODO: does not work fine via stdin handle
	auto wait = WaitForSingleObject(cin_handle, timeout);
	if (wait == WAIT_OBJECT_0)
		return std::cin.get();
	else
		return -1;
#else
	// unix should use FD_
	return -1;
#endif
}

Mat hwnd2mat(HWND hwnd)
{
	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER  bi;

	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom / 1;  //change this to whatever size you want to resize to
	width = windowsize.right / 1;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}

Mat image;
Point top_left_corner, bottom_right_corner;

void mouseEventHandle(int action, int x, int y, int flags, void* userData) {
	printf("mouse:x=%d, y=%d\n", x, y);

	// Mark the top left corner when left mouse button is pressed
	if (action == EVENT_LBUTTONDOWN) {
		top_left_corner = Point(x, y);
	}
	// When left mouse button is released, mark bottom right corner
	else if (action == EVENT_LBUTTONUP) {
		bottom_right_corner = Point(x, y);
		if (image.empty() == false)
			rectangle(image, top_left_corner, bottom_right_corner, Scalar(0, 255, 0), 2, 8);
	}
}

vector<Rect> rois;

int main()
{
	bool display_output = false;
	HWND hwndDesktop = GetDesktopWindow();
	if (display_output) {
		namedWindow("output", WINDOW_NORMAL);
		resizeWindow("output", Size(1280, 720));
		setMouseCallback("output", mouseEventHandle);
	}

	int key = 0;

	int padx = 0;
	int pady = 0;
	int sizx = 0;
	int sizy = 0;
	bool isSaveCrop = false;
	bool isTrigger = true;

	rois.push_back(Rect(880, 783, 60, 60)); //1
	rois.push_back(Rect(1091, 783, 60, 60)); //2
	rois.push_back(Rect(1302, 783, 60, 60)); //3
	rois.push_back(Rect(774, 940, 60, 60)); //4 diff,x=211
	rois.push_back(Rect(986, 940, 60, 60)); //5
	rois.push_back(Rect(1197, 940, 60, 60)); //6
	rois.push_back(Rect(1408, 940, 60, 60)); //7
	rois.push_back(Rect(880, 1097, 60, 60)); //8
	rois.push_back(Rect(1091, 1097, 60, 60)); //9
	rois.push_back(Rect(1302, 1097, 60, 60)); //0

	Mat gray;
	namedWindow("gray");
	Rect target = Rect(720, 720, 800, 500);
	Mat tpl_isabera = imread("isabera-crop3.png", 0);
	Mat tpl_gwendol = imread("gwendol-crop5.png", 0);
	Mat tpl_mira = imread("mira-crop0.png", 0);
	Mat tpl_belteran = imread("belteran-crop7.png", 0);
	//imshow("isabera", temp);
	bool has_isabera = false;
	bool has_gwendol = false;
	bool has_mira = false;
	bool has_belteran = false;

	double temp_weight = 0.85f;

	double cnt_isabera = 0;// 2330;
	double cnt_gwendol = 0;// 367;
	double cnt_mira = 0;// 379;
	double cnt_draw = 0;// 12600;
	double cnt_isagwe = 0;// 31;
	double cnt_isamir = 0;// 25;
	double cnt_gwemir = 0;// 5;
	double cnt_combo = 0;// 5468;

	int trigger_timeout = 15;
	int trigger_elapsed = trigger_timeout;

	while (key != 27)
	{
		// reset every frames prevent false detect during drawing animation
		has_isabera = false;
		has_gwendol = false;
		has_mira = false;
		has_belteran = false;

		image = hwnd2mat(hwndDesktop);
		gray = image(target);
		cvtColor(gray, gray, COLOR_BGR2GRAY);
		resizeWindow("gray", Size(gray.cols, gray.rows));

		Mat res_isabera;
		matchTemplate(gray, tpl_isabera, res_isabera, TM_CCOEFF_NORMED);
		imshow("res_isabera", res_isabera);
		threshold(res_isabera, res_isabera, temp_weight, 1.0f, THRESH_TOZERO);
		vector<Point> maxima;
		findNonZero(res_isabera, maxima);
		for (auto it = maxima.begin(); it != maxima.end(); it++) {
			rectangle(gray, Rect(it->x, it->y, 60, 60), Scalar(0), 1);
			has_isabera = true;
		}

		Mat res_gwendol;
		matchTemplate(gray, tpl_gwendol, res_gwendol, TM_CCOEFF_NORMED);
		imshow("res_gwendol", res_gwendol);
		threshold(res_gwendol, res_gwendol, temp_weight, 1.0f, THRESH_TOZERO);
		maxima.clear();
		findNonZero(res_gwendol, maxima);
		for (auto it = maxima.begin(); it != maxima.end(); it++) {
			rectangle(gray, Rect(it->x, it->y, 60, 60), Scalar(0), 1);
			has_gwendol = true;
		}

		Mat res_mira;
		matchTemplate(gray, tpl_mira, res_mira, TM_CCOEFF_NORMED);
		imshow("res_mira", res_mira);
		threshold(res_mira, res_mira, temp_weight, 1.0f, THRESH_TOZERO);
		maxima.clear();
		findNonZero(res_mira, maxima);
		for (auto it = maxima.begin(); it != maxima.end(); it++) {
			rectangle(gray, Rect(it->x, it->y, 60, 60), Scalar(0), 1);
			has_mira = true;
		}
		
		Mat res_belteran;
		matchTemplate(gray, tpl_belteran, res_belteran, TM_CCOEFF_NORMED);
		//imshow("res_belteran", res_belteran);
		threshold(res_belteran, res_belteran, temp_weight, 1.0f, THRESH_TOZERO);
		maxima.clear();
		findNonZero(res_belteran, maxima);
		for (auto it = maxima.begin(); it != maxima.end(); it++) {
			rectangle(gray, Rect(it->x, it->y, 60, 60), Scalar(0), 1);
			has_belteran = true;
		}

		imshow("gray", gray);

		if (isSaveCrop || display_output) {
			int i = 0;
			char name[256] = { 0 };
			for (auto it = rois.begin(); it != rois.end(); it++) {
				Mat crop = image(*it);
				sprintf_s(name, "crop%d.png", i);
				if (isSaveCrop)
					imwrite(name, crop);
				i++;
				rectangle(image,
					Point(it->x, it->y),
					Point(it->x + it->width, it->y + it->height),
					Scalar(0, 255, 0), 2, 8);

			}
		}
		
		if (display_output)
			imshow("output", image);

		key = waitKey(200); // you can change wait time
		//printf("trigger:%d\n", trigger_elapsed);
		trigger_elapsed -= 1;
		if (trigger_elapsed <= 0) {
			cnt_draw += 1;
			cnt_combo += 1;
			if (has_isabera) {
				cnt_isabera += 1;
				if (has_gwendol)
					cnt_isagwe += 1;
				if (has_mira)
					cnt_isamir += 1;
			}
			if (has_gwendol) {
				cnt_gwendol += 1;
				if (has_mira)
					cnt_gwemir += 1;
			}
			if (has_mira)
				cnt_mira += 1;
			if (has_isabera || has_gwendol || has_mira) {
				printf("isab:%.2f[%d] gwen:%.2f[%d] mira:%.2f[%d] draw:%d, \nisagwe:%.2f[%d] isamir:%.2f[%d] gwemir:%.2f[%d] comb:%d\n",
					(cnt_isabera / cnt_draw * 100), (int)cnt_isabera,
					(cnt_gwendol / cnt_draw * 100), (int)cnt_gwendol,
					(cnt_mira / cnt_draw * 100), (int)cnt_mira, (int)cnt_draw,
					(cnt_isagwe / cnt_combo * 100), (int)cnt_isagwe,
					(cnt_isamir / cnt_combo * 100), (int)cnt_isamir,
					(cnt_gwemir / cnt_combo * 100), (int)cnt_gwemir, (int)cnt_combo);
			}
			if ((has_isabera && has_gwendol && has_mira) //||
				//(has_isabera && has_belteran) ||
				//(has_isabera && has_belteran && has_gwendol) ||
				/*(has_isabera && has_belteran && has_mira)*/) {
				trigger_elapsed = INT_MAX; // always keep value will never trigger key
				printf("found skipp drawing\n");
				continue;
			}
			if (isTrigger) {
				INPUT inputs[2] = {};
				ZeroMemory(inputs, sizeof(inputs));

				inputs[0].type = INPUT_KEYBOARD;
				inputs[0].ki.wVk = VK_UP;

				inputs[1].type = INPUT_KEYBOARD;
				inputs[1].ki.wVk = VK_UP;
				inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

				UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
			}
			trigger_elapsed = trigger_timeout;
		}
	}
}
