#define UNICODE

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <cassert>

#include <Windows.h>
#include <Shellapi.h>
#include <highlevelmonitorconfigurationapi.h>


#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"


#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifdef _DEBUG
void LOG()
{
	std::wcout << '\n';
}
template<typename T, typename... Args>
void LOG(const T& t, const Args&... args)
{
	std::wcout << t << " ";
	LOG(args...);
}
#else
template<typename T, typename... Args>
void LOG(const T& t, const Args&... args)
{
}
#endif

#define ID_TRAY_APP_ICON 5000
#define WM_TRAYICON (WM_USER + 1)
#define IDS_TOOLTIP 111
#define FPS 33.0f

const int main_window_width = 500;
const int main_window_height = 100;

std::string ExecutablePath = "";

std::atomic_bool isMainWindowHidden = false;

std::string GetExecutablePath()
{
	if (ExecutablePath.size() != 0) return ExecutablePath;

	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);

	std::string executablePath(exePath);
	size_t pos = executablePath.find_last_of("\\/");
	assert(pos != std::string::npos);

	return ExecutablePath = executablePath.substr(0, pos + 1);
}

std::string GetImGuiIniPath()
{
	std::string imguiIniDir(GetExecutablePath() + "imgui.ini");
	LOG("IMGUI INI FILE: ", imguiIniDir.c_str());
	return imguiIniDir;
}

std::string GetTTFFontFile()
{
	std::string fontPath(GetExecutablePath() + "NotoSansDisplay_Regular.ttf");
	LOG("USING FONT: ", fontPath.c_str());
	return fontPath;
}

void MoveMainWindowToBottomRight(GLFWwindow* window)
{
	struct
	{
		int xPos, yPos, width, height;
	} monitorWorkspace{};

	glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &monitorWorkspace.xPos, &monitorWorkspace.yPos, &monitorWorkspace.width, &monitorWorkspace.height);
	glfwSetWindowPos(window, monitorWorkspace.width - main_window_width, monitorWorkspace.height - main_window_height);
}

HWND mainWindowHandle;
GLFWwindow* mainWindow;
namespace TrayIcon
{
	HWND handle;
	NOTIFYICONDATA nidApp;
	HICON hIcon;
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void InitializeThread()
	{
		WNDCLASS wc = { 0 };
		MSG msg;
		HINSTANCE hInstance = GetModuleHandle(NULL);

		wc.lpfnWndProc = ::TrayIcon::WindowProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = L"BRIGHTNESS_SLIDER";
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

		RegisterClass(&wc);

		::TrayIcon::handle = CreateWindow(wc.lpszClassName, L"BRIGHTNESS_SLIDER", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL, hInstance, NULL);

		ShowWindow(::TrayIcon::handle, SW_HIDE);

		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (mainWindow)
				if (glfwWindowShouldClose(mainWindow)) return;
			std::this_thread::sleep_for(std::chrono::milliseconds(int(1000.0f / FPS)));
		}
	}

	void AddTrayIcon(HWND hwnd) {
		ZeroMemory(&nidApp, sizeof(NOTIFYICONDATA));
		nidApp.cbSize = sizeof(NOTIFYICONDATA);
		nidApp.hWnd = hwnd;
		nidApp.uID = ID_TRAY_APP_ICON;
		nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nidApp.uCallbackMessage = WM_TRAYICON;
		nidApp.hIcon = hIcon;
		LoadString(GetModuleHandle(NULL), IDS_TOOLTIP, nidApp.szTip, sizeof(nidApp.szTip));
		Shell_NotifyIcon(NIM_ADD, &nidApp);
	}

	void RemoveTrayIcon() {
		Shell_NotifyIcon(NIM_DELETE, &nidApp);
		DestroyIcon(hIcon);
	}

#define IDM_EXIT_COMMAND 1001

	void ShowContextMenu(HWND hwnd)
	{
		HMENU hmenu = CreatePopupMenu();
		if (hmenu)
		{
			AppendMenu(hmenu, MF_STRING, IDM_EXIT_COMMAND, L"Exit");
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hwnd);
			TrackPopupMenu(hmenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
			PostMessage(hwnd, WM_NULL, 0, 0);
			DestroyMenu(hmenu);
		}
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
		case WM_CREATE:
			hIcon = (HICON)LoadImage(NULL, L"logo.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
			if (hIcon == NULL) {
				MessageBox(NULL, L"Failed to load icon!", L"Error", MB_OK | MB_ICONERROR);
				PostQuitMessage(0);
			}
			AddTrayIcon(hwnd);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				// Handle menu item events here
			case IDM_EXIT_COMMAND: {
				glfwSetWindowShouldClose(mainWindow, true);
				break;
			}
			}
			break;

		case WM_TRAYICON:
			switch (lParam) {
			case WM_RBUTTONDOWN: {
				ShowContextMenu(hwnd);
				break;
			}
			case WM_LBUTTONUP:
				isMainWindowHidden = false;
				ShowWindow(mainWindowHandle, SW_SHOW);
				glfwFocusWindow(mainWindow);
				break;
			}
			break;

		case WM_CLOSE:
			isMainWindowHidden = true;
			ShowWindow(hwnd, SW_HIDE);
			break;

		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		return 0;
	}

}

void window_focus_callback(GLFWwindow* window, int focused)
{
	if (focused)
	{
		//	window gained focus
		glfwShowWindow(window);
		isMainWindowHidden = false;
	}
	else
	{
		//	window lost focus
		glfwHideWindow(window);
		isMainWindowHidden = true;
	}
}

namespace Monitor
{
	bool SetBrightness(PHYSICAL_MONITOR* physical_monitor, int brightness)
	{
		HANDLE physical_monitor_handle = physical_monitor->hPhysicalMonitor;
		return SetMonitorBrightness(physical_monitor_handle, (DWORD)brightness);
	}

	bool IsChangingBrightnessSupported()
	{
		return true;
	}
}

GLFWwindow* StartGLFW()
{
	glfwInit();
	glfwWindowHint(GLFW_DECORATED, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(700, 700, "Slider", NULL, NULL);
	mainWindow = window;
	mainWindowHandle = glfwGetWin32Window(window);

	glfwSetWindowFocusCallback(window, window_focus_callback);
	ShowWindow(mainWindowHandle, SW_HIDE);
	glfwMakeContextCurrent(window);
	MoveMainWindowToBottomRight(window);
	return window;
}

void MainWindowThread()
{
#ifdef _DEBUG
	AllocConsole();
	FILE* dummy;
	freopen_s(&dummy, "CONOUT$", "w", stdout);
#endif
	GLFWwindow* window = StartGLFW();
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	std::string imguiIniFilePath = GetImGuiIniPath();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = imguiIniFilePath.c_str();


	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	ImFont* font = io.Fonts->AddFontFromFileTTF(GetTTFFontFile().c_str(), 18.0f);
	assert(font != nullptr);

	HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
	DWORD number_of_physical_monitors;
	GetNumberOfPhysicalMonitorsFromHMONITOR(monitor, &number_of_physical_monitors);

	PHYSICAL_MONITOR* physical_monitor_array = (PHYSICAL_MONITOR*)malloc(sizeof(PHYSICAL_MONITOR) * number_of_physical_monitors);
	GetPhysicalMonitorsFromHMONITOR(monitor, number_of_physical_monitors, physical_monitor_array);

	DWORD monitor_capabilities, colour_temperatures;

	HANDLE physical_monitor_handle = physical_monitor_array[0].hPhysicalMonitor;
	GetMonitorCapabilities(physical_monitor_handle, &monitor_capabilities, &colour_temperatures);
	assert(monitor_capabilities & MC_CAPS_BRIGHTNESS && "your monitor doesnt support dcc/ci");

	DWORD minimum_brightness, maximum_brightness, current_brightness;
	while (!GetMonitorBrightness(physical_monitor_handle, &minimum_brightness, &current_brightness, &maximum_brightness))
	{
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}

	int brightness = current_brightness;
	int brightness_in_last_frame = brightness;
	int physical_monitor_brightness = brightness;
	bool change_immediately = false;

	std::vector<char> should_change_brightness;
	while (!glfwWindowShouldClose(window))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000.0f / FPS) - 5));
		if (isMainWindowHidden && (current_brightness == physical_monitor_brightness)) continue;

		glfwPollEvents();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::PushFont(font);
		ImGui::SetWindowSize({ main_window_width, main_window_height }, 0);

		ImGui::Begin("Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
		ImGui::Checkbox("Change Immediately(Not Recommended)", &change_immediately);
		ImGui::BeginTable("Table", 2, ImGuiTableFlags_Resizable);
		ImGui::PushItemWidth(30);
		ImGui::TableNextColumn();
		ImGui::Text("Brightness: ");
		ImGui::TableNextColumn();
		ImGui::PopItemWidth();
		ImGui::PushItemWidth(-1);
		brightness_in_last_frame = brightness;
		ImGui::SliderInt("", &brightness, minimum_brightness, maximum_brightness, "%d", ImGuiSliderFlags_AlwaysClamp);
		current_brightness = brightness;
		ImGui::PopItemWidth();
		ImGui::EndTable();
		ImGui::End();

		ImGui::PopFont();

		if (brightness_in_last_frame == brightness)
			should_change_brightness.push_back(1);				// fill vector if brightness value stays same
		else
			std::vector<char>().swap(should_change_brightness); // empty the vector

		if (should_change_brightness.size() > (2 + 18 * !change_immediately) && current_brightness != physical_monitor_brightness)
		{
			std::vector<char>().swap(should_change_brightness);
			physical_monitor_brightness = current_brightness;
			SetMonitorBrightness(physical_monitor_handle, (DWORD)physical_monitor_brightness);
			LOG("change");
		}

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	std::thread thr1(MainWindowThread);
	::TrayIcon::InitializeThread();
	thr1.join();
	::TrayIcon::RemoveTrayIcon();
	return 0;
}

