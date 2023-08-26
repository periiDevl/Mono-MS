#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")
#include <vector>
#include"imgui.h"
#include"imgui_impl_glfw.h"
#include"imgui_impl_opengl3.h"

#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include <iostream>
#include <filesystem>
#include <thread> 
#include <fstream>
#include <string>
#include <sstream>
#include "Console.h"
#include "Guitheme.h"
#include<stb/stb_image.h>
Console con;

struct MemoryInfo {
    void* address;
    int intValue;
    int ID;
};

bool IsAppWindow(HWND hwnd) {
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (style & WS_VISIBLE) {
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (!(exStyle & WS_EX_TOOLWINDOW)) {
            return true;
        }
    }
    return false;
}


void GetTaskbarProcesses(std::vector<std::string>& processNames) {
    EnumWindows([](HWND hwnd, LPARAM lParam) {
        if (IsWindowVisible(hwnd) && IsAppWindow(hwnd)) {
            DWORD processId;
            GetWindowThreadProcessId(hwnd, &processId);

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
            if (hProcess != nullptr) {
                char processPath[MAX_PATH];
                if (GetModuleFileNameExA(hProcess, nullptr, processPath, MAX_PATH)) {
                    // Extract only the executable name from the path
                    std::string processName = processPath;
                    size_t lastBackslash = processName.find_last_of("\\/");
                    if (lastBackslash != std::string::npos) {
                        processName = processName.substr(lastBackslash + 1);
                    }
                    reinterpret_cast<std::vector<std::string>*>(lParam)->emplace_back(processName);
                }
                CloseHandle(hProcess);
            }
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&processNames));
}

DWORD GetProcessIdByName(const std::wstring& processName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(snapshot, &entry)) {
        do {
            if (wcscmp(entry.szExeFile, processName.c_str()) == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

bool WriteToProcessMemory(HANDLE hProcess, void* address, BYTE value) {
    return WriteProcessMemory(hProcess, address, &value, sizeof(value), nullptr) != 0;
}
HANDLE hProcess;
DWORD processID;
std::vector<MemoryInfo> memoryList;
std::vector<MemoryInfo> memoryListSearched;

std::vector<MemoryInfo> SearchedVal;
std::vector<MemoryInfo> ResetSearchedVal;
std::string tmp = "";
std::string& selectedProcessName = tmp;

int min = 0, max = 10000000;
int mbusage = 62;

void Run() {



    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, selectedProcessName.c_str(), -1, nullptr, 0);
    std::wstring wSelectedProcessName(bufferSize - 1, L'\0'); 
    MultiByteToWideChar(CP_UTF8, 0, selectedProcessName.c_str(), -1, &wSelectedProcessName[0], bufferSize);

    processID = GetProcessIdByName(wSelectedProcessName);

    memoryList.clear();

    if (processID != 0) {
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, processID);

        if (hProcess != NULL) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);

            MEMORY_BASIC_INFORMATION memInfo;
            SIZE_T bufferSize = mbusage * 1024 * 1024;
            BYTE* buffer = new BYTE[bufferSize];

            BYTE* address = reinterpret_cast<BYTE*>(sysInfo.lpMinimumApplicationAddress);

            while (address < sysInfo.lpMaximumApplicationAddress) {
                if (VirtualQueryEx(hProcess, address, &memInfo, sizeof(memInfo)) != 0) {
                    if (memInfo.State == MEM_COMMIT && memInfo.Protect != PAGE_NOACCESS) {
                        SIZE_T bytesRead = 0;
                        while (bytesRead < memInfo.RegionSize) {
                            SIZE_T bytesToRead = min(bufferSize, memInfo.RegionSize - bytesRead);
                            if (ReadProcessMemory(hProcess, address + bytesRead, buffer, bytesToRead, nullptr)) {
                                for (SIZE_T i = 0; i < bytesToRead - sizeof(int) + 1; ++i) {
                                    int intValue = *reinterpret_cast<int*>(buffer + i);
                                    MemoryInfo info;
                                    info.address = address + bytesRead + i;
                                    info.intValue = intValue;
                                    info.ID = i;
                                    if (intValue >= min && intValue <= max) {
                                        memoryList.push_back(info);
                                    }
                                }
                            }
                            else {
                                std::cerr << "Failed to read memory. Error code: " << GetLastError() << std::endl;
                            }
                            bytesRead += bytesToRead;
                        }
                    }
                    address += memInfo.RegionSize;
                }
                else {
                    std::cerr << "Failed to query memory. Error code: " << GetLastError() << std::endl;
                    break;
                }

                
            }

            

            

            delete[] buffer;
            CloseHandle(hProcess);
        }
        else {
            std::cerr << "Failed to open process. Error code: " << GetLastError() << std::endl;
        }
    }
    else {
        std::cerr << "Process not found." << std::endl;
    }
    /*
    for (const MemoryInfo& info : memoryList) {
        
        std::cout << "Memory at address " << reinterpret_cast<void*>(info.address)
            << " has integer value " << info.intValue << std::endl;

    }
    */
}


int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(700 - 100, 400 - 100, "MMS ~ Jonathan peri", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    //load Icon
    int wid, hei;
    int channels;
    unsigned char* pixels = stbi_load("Assets/mmsicon.png", &wid, &hei, &channels, 4);

    GLFWimage images[1];
    images[0].width = wid;
    images[0].height = hei;
    images[0].pixels = pixels;
    glfwSetWindowIcon(window, 1, images);

    glfwMakeContextCurrent(window);
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);
    gladLoadGL();
    glViewport(0, 0, 800, 800);



	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
    DefaultTheme();
    int search = -1;

    int selectedProcessIndex = -1;
    std::vector<std::string> processNames;

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();






        ImGui::Begin("List", 0, (true ? ImGuiWindowFlags_NoResize : 0) | (true ? ImGuiWindowFlags_NoMove : 0));
        
        ImGui::InputInt("Num-Search", &search);

        if (ImGui::Button("Search")) {
            memoryListSearched.clear();
            for (int i = 0; i < memoryList.size(); ++i) {
                if (true) {
                    if (memoryList[i].intValue == search) {
                        memoryListSearched.push_back(memoryList[i]);
                    }
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Changed?")) { 
            ResetSearchedVal.clear();
            //memoryListSearched.clear();
            
            for (int i = 0; i < memoryList.size(); ++i) {
                for (int j = 0; j < memoryListSearched.size(); ++j) {
                    if (memoryList[i].intValue != memoryListSearched[j].intValue && memoryList[i].address == memoryListSearched[j].address) {
                        ResetSearchedVal.push_back(memoryList[i]);
                    }
                }
            }
            
            memoryListSearched = ResetSearchedVal;
        }
        ImGui::SameLine();
        if (ImGui::Button("Re-Inject"))
        {
            std::thread ijectThread(Run);
            ijectThread.detach();

            SearchedVal = memoryListSearched;
        }
        ImGui::SameLine();

        if (ImGui::Button("Modify List")) {
            for (int i = 0; i < memoryListSearched.size(); ++i) {
                BOOL writeResult = WriteProcessMemory(hProcess, memoryListSearched[i].address, &memoryListSearched[i].intValue, sizeof(memoryListSearched[i].intValue), NULL);
            }
        }
        ImGui::Separator();
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
        for (int i = 0; i < memoryListSearched.size(); ++i) {

            std::stringstream addressStream;
            addressStream << memoryListSearched[i].address;
            std::string addressString = addressStream.str();
            if (ImGui::Button(addressString.c_str())) {
            }
            ImGui::SameLine();
            ImGui::InputInt(("Input" + addressStream.str()).c_str(), &memoryListSearched[i].intValue);

            ImGui::SameLine();
            ImGui::InputInt(("InputAfter" + addressStream.str()).c_str(), &memoryListSearched[i].intValue);



        }


        ImGui::End();
        ImGui::Begin("Moderate", 0, (true ? ImGuiWindowFlags_NoResize : 0) | (true ? ImGuiWindowFlags_NoMove : 0));
        
        ImGui::Text(std::to_string(memoryList.size()).c_str());
        if (ImGui::Button("Start Injection"))
        {
            //Run();

            std::thread ijectThread(Run);
            ijectThread.detach();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        
        if (ImGui::Button("Select Process")) {
            processNames.clear();
            GetTaskbarProcesses(processNames);
        }

        if (!processNames.empty()) {
            const char* comboPreviewText = processNames[selectedProcessIndex].c_str();
            if (ImGui::BeginCombo("Processes", comboPreviewText)) {
                for (int i = 0; i < processNames.size(); ++i) {
                    bool isSelected = (selectedProcessIndex == i);
                    if (ImGui::Selectable(processNames[i].c_str(), isSelected))
                        selectedProcessIndex = i;

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (selectedProcessIndex >= 0 && selectedProcessIndex < processNames.size()) {
                selectedProcessName = processNames[selectedProcessIndex];
            }

            ImGui::Text("Selected Process: %s", selectedProcessName.c_str());
        }

        ImGui::Text("Usage (MB):");
        ImGui::SameLine();
        ImGui::InputInt("## 1", &mbusage);

        ImGui::Text("Min:");
        ImGui::SameLine();
        ImGui::InputInt("## 2", &min);

        ImGui::Text("Max:");
        ImGui::SameLine();
        ImGui::InputInt("## 3", &max);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();


    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
