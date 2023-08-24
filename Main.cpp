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
struct MemoryInfo {
    void* address;
    int intValue;
};

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
std::vector<MemoryInfo> memoryList;
void Run() {
    std::wstring targetExeName = L"Ovlauncher.exe";
    DWORD processID = GetProcessIdByName(targetExeName);

    if (processID != 0) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, processID);

        if (hProcess != NULL) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);

            MEMORY_BASIC_INFORMATION memInfo;
            SIZE_T bufferSize = 62 * 1024 * 1024; // 62MB
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
                                    memoryList.push_back(info);
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

    for (const MemoryInfo& info : memoryList) {
        std::cout << "Memory at address " << reinterpret_cast<void*>(info.address)
            << " has integer value " << info.intValue << std::endl;
    }
}


int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(700, 400, "OV-Launcher ~ BoringInnovators", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
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

    int search = 696948796;
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.00f, 0.00f, 0.00f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();





        ImGui::Begin("UI");


        static char Name[128] = "";

        ImGui::InputText("ProcessName", Name, IM_ARRAYSIZE(Name));

        if (ImGui::Button("Start Injection"))
        {
            //Run();
            
            std::thread myThread(Run);
            myThread.detach();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }


        ImGui::End();

        ImGui::Begin("List");
        ImGui::Text(std::to_string(memoryList.size()).c_str());
        ImGui::InputInt("Search", &search);

        for (int i = 0; i < memoryList.size(); ++i) {
            if (true) {
                if (memoryList[i].intValue == search) {
                    std::stringstream addressStream;
                    addressStream << memoryList[i].address;
                    std::string addressString = "(" + std::to_string(memoryList[i].intValue) + ") at " + addressStream.str();
                    if (ImGui::Button(addressString.c_str())) {
                    }
                }
            }
        }

        

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
