#define UNICODE

#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <atomic>
#include <array>
#include <vector>
#include <string>
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
	std::cout << '\n';
}
template<typename T, typename... Args>
void LOG(const T& t, const Args&... args)
{
	std::cout << t << " ";
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
	struct MonitorInfo			// wrapper for PHYSICAL_MONITOR
	{
		PHYSICAL_MONITOR* Ptr;
		std::string Name		= "";
		HANDLE Handle			= 0;
		bool DccAvailable		= false;
		int MinimumBrightness	= 0;
		int MaximumBrightness	= 0;
		int CurrentBrightness	= 0;

		MonitorInfo()
		{
			Ptr = new PHYSICAL_MONITOR;
		}
		MonitorInfo(MonitorInfo&& other)
		{
			Ptr = other.Ptr;
			other.Ptr = new PHYSICAL_MONITOR;
			Name = std::move(other.Name);

			DccAvailable = other.DccAvailable;
			MinimumBrightness = other.MinimumBrightness;
			MaximumBrightness = other.MaximumBrightness;
			CurrentBrightness = other.CurrentBrightness;
			Handle = other.Handle;
		}
		~MonitorInfo()
		{
			delete Ptr;
		}
	};

	class ChangeBrightnessCommand
	{
		std::atomic<bool> m_working = false;
		std::chrono::milliseconds m_delay = std::chrono::milliseconds(400);
	public:
		int monitorIndex;
		int brightnessValue = 0;

	public:
		auto getDelay() {	return m_delay;	}
		void setDelay(int dt) { m_delay = std::chrono::milliseconds(dt); }
		bool isWorking() { return m_working; }

		void setWorking() { m_working = true; }
		void reset() { m_working = false; }
	};

	std::vector<MonitorInfo> PhysicalMonitorArray;
	size_t physicalMonitorCount = 0;

	bool MonitorWithNameExists(const std::string& name)
	{
		for (auto& monitor : PhysicalMonitorArray)
		{
			if (monitor.Name == name) return true;
		}
		return false;
	}

	void UpdateMonitorName(std::string& name)
	{
		name = name + std::to_string(physicalMonitorCount);
	}

	int CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{
		DWORD physicalMonitorsinCurrentHmonitor = 0;
		GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &physicalMonitorsinCurrentHmonitor);
		PHYSICAL_MONITOR* physicalMonitor = new PHYSICAL_MONITOR[physicalMonitorsinCurrentHmonitor];

		PhysicalMonitorArray.swap(std::vector<MonitorInfo>());
		if (GetPhysicalMonitorsFromHMONITOR(hMonitor, physicalMonitorsinCurrentHmonitor, physicalMonitor))
		{
			for (int i = 0; i < physicalMonitorsinCurrentHmonitor; i++)
			{
				MonitorInfo currentMonitor;
				// currentMonitor.Ptr = &physicalMonitor[i];

				memcpy(currentMonitor.Ptr, &physicalMonitor[i], sizeof(PHYSICAL_MONITOR));
				currentMonitor.Handle = currentMonitor.Ptr->hPhysicalMonitor;

				std::wstring _name = currentMonitor.Ptr->szPhysicalMonitorDescription;
				currentMonitor.Name = std::string(_name.begin(), _name.end());
				if (MonitorWithNameExists(currentMonitor.Name)) UpdateMonitorName(currentMonitor.Name);

				LOG("\tFound Monitor ", ++physicalMonitorCount, " : ", currentMonitor.Name,'\n');

				DWORD monitor_capabilities, colour_temperatures;
				GetMonitorCapabilities(currentMonitor.Handle, &monitor_capabilities, &colour_temperatures);

				DWORD minimum_brightness, maximum_brightness, current_brightness;
				while (!GetMonitorBrightness(currentMonitor.Handle, &minimum_brightness, &current_brightness, &maximum_brightness))
					std::this_thread::sleep_for(std::chrono::seconds(2));

				currentMonitor.DccAvailable = monitor_capabilities & MC_CAPS_BRIGHTNESS;
				currentMonitor.MaximumBrightness = maximum_brightness;
				currentMonitor.MinimumBrightness = minimum_brightness;
				currentMonitor.CurrentBrightness = current_brightness;

				PhysicalMonitorArray.emplace_back(std::move(currentMonitor));
			}
		}
		delete[] physicalMonitor;
		return 1;
	}

	void SetBrightnessDispatch(ChangeBrightnessCommand& cmd)
	{
		using namespace std::chrono_literals;
		using clock = std::chrono::high_resolution_clock;
		MonitorInfo& monitorInfo = PhysicalMonitorArray[cmd.monitorIndex];

		if (cmd.brightnessValue < monitorInfo.MinimumBrightness || cmd.brightnessValue > monitorInfo.MaximumBrightness)
		{
			LOG("BAD VALUE"); return;
		}

		clock::time_point tp = clock::now();
		while (true)
		{
			clock::time_point final = clock::now();
			if ((final - tp) > cmd.getDelay()) break;

			std::this_thread::sleep_for(30ms);
		}

		if (monitorInfo.CurrentBrightness != cmd.brightnessValue)
		{
			monitorInfo.CurrentBrightness = cmd.brightnessValue;
			LOG("set", monitorInfo.Name ," Brightness to ", cmd.brightnessValue);
			cmd.reset();
		}
		SetMonitorBrightness(monitorInfo.Handle, (DWORD)cmd.brightnessValue);
	}

	bool SetBrightness(PHYSICAL_MONITOR* physical_monitor, int brightness)
	{
		HANDLE physical_monitor_handle = physical_monitor->hPhysicalMonitor;
		return SetMonitorBrightness(physical_monitor_handle, (DWORD)brightness);
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

	EnumDisplayMonitors(NULL, NULL, Monitor::MonitorEnumProc, 0);

	Monitor::ChangeBrightnessCommand command;
	command.monitorIndex = 0;
	bool change_immediately = false;
	bool last_change_immediately = false;
	std::atomic_bool refreshing_monitor_list = false;

	while (!glfwWindowShouldClose(window))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000.0f / FPS) - 5));
		if (isMainWindowHidden) continue;


		glfwPollEvents();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::PushFont(font);
		ImGui::Begin("Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

		if (refreshing_monitor_list)
		{
			ImGui::Text("Refreshing");
		}
		else
		{
			Monitor::MonitorInfo& currentMonitor = Monitor::PhysicalMonitorArray[command.monitorIndex];
			if (!command.isWorking()) command.brightnessValue = currentMonitor.CurrentBrightness;

			ImGui::BeginTable("Table", 2, ImGuiTableFlags_Resizable);
			{
				ImGui::TableNextRow();
				{
					ImGui::TableNextColumn();
					{
						ImGui::PushItemWidth(30);
						ImGui::Text("Monitor: ");
						ImGui::PopItemWidth();
					}
					ImGui::TableNextColumn();
					{
						ImGui::PushItemWidth(-1);
						if (ImGui::BeginCombo("##combo", Monitor::PhysicalMonitorArray[command.monitorIndex].Name.c_str()))
						{
							for (int i = 0; i < Monitor::PhysicalMonitorArray.size(); i++)
							{
								bool is_selected = (command.monitorIndex == i);

								if (ImGui::Selectable(Monitor::PhysicalMonitorArray[i].Name.c_str(), is_selected))
									command.monitorIndex = i;
								if (is_selected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						ImGui::PopItemWidth();
					}
				}

				ImGui::TableNextRow();
				{
					ImGui::TableNextColumn();
					{
						ImGui::PushItemWidth(30);
						ImGui::Text("Brightness: ");
						ImGui::PopItemWidth();
					}
					ImGui::TableNextColumn();
					{
						ImGui::PushItemWidth(-1);
						ImGui::SliderInt("", &command.brightnessValue, currentMonitor.MinimumBrightness, currentMonitor.MaximumBrightness, "%d", ImGuiSliderFlags_AlwaysClamp);
						ImGui::PopItemWidth();
					}
				}
			} ImGui::EndTable();

			ImGui::Checkbox("Change Immediately(Not Recommended)", &change_immediately);

			ImGui::SameLine();
			if (ImGui::Button("Refresh"))
			{
				std::thread{
					[&refreshing_monitor_list]()
					{
						refreshing_monitor_list = true;
						EnumDisplayMonitors(NULL, NULL, Monitor::MonitorEnumProc, 0);
						refreshing_monitor_list = false;
					}
				}.detach();
			}

		}
		ImGui::End();
		ImGui::PopFont();

		if (!refreshing_monitor_list)
		{
			Monitor::MonitorInfo& currentMonitor = Monitor::PhysicalMonitorArray[command.monitorIndex];
			if (change_immediately != last_change_immediately)
			{
				if (change_immediately) command.setDelay(100);
				else command.setDelay(500);
				last_change_immediately = change_immediately;
			}

			if (command.brightnessValue != currentMonitor.CurrentBrightness && !command.isWorking())
			{
				command.setWorking();

				std::thread{
					[&command]()
					{
						Monitor::SetBrightnessDispatch(command);
					}
				}.detach();
			}
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

