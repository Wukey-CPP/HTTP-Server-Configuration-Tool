#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <random>
#include <sstream>
#include <iomanip>
#include "Drawing.h"

#pragma comment(lib, "ws2_32.lib")

struct ServerInfo {
    std::string ip;
    std::string hostSystem;
    std::chrono::steady_clock::time_point startTime;
};

LPCSTR Drawing::lpWindowName = "HTTP Server Setup";
ImVec2 Drawing::vWindowSize = { 600, 400 };
ImGuiWindowFlags Drawing::WindowFlags = 0;
bool Drawing::bDraw = true;

std::string serverAddress = "127.0.0.1";
std::string serverPort = "8080";
bool serverRunning = false;
bool htmlModified = false;
std::string statusMessage;
ImVec4 statusColor = ImVec4(0, 1, 0, 1);
std::chrono::steady_clock::time_point statusTimestamp;

const int HTML_BUFFER_SIZE = 7000;
char htmlBuffer[HTML_BUFFER_SIZE] = "<html><body><h1>Hello, this is a HTTP server maker Developed by Wukey!</h1></body></html>";

const int API_KEY_SIZE = 256;
char searchAPIKey[API_KEY_SIZE] = "";

std::unordered_map<std::string, ServerInfo> servers;
std::string currentAPIKey;

void showStatusMessage(const std::string& message, ImVec4 color) {
    statusMessage = message;
    statusColor = color;
    statusTimestamp = std::chrono::steady_clock::now();
}

std::string generateAPIKey() {
    const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string apiKey;
    std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    for (int i = 0; i < 16; ++i) apiKey += characters[distribution(generator)];

    return apiKey;
}

void startServer(const std::string& address, const std::string& port) {
    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL, hints;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        showStatusMessage("WSAStartup failed", ImVec4(1, 0, 0, 1));
        return;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(address.c_str(), port.c_str(), &hints, &result) != 0) {
        showStatusMessage("getaddrinfo failed", ImVec4(1, 0, 0, 1));
        WSACleanup();
        return;
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        showStatusMessage("socket failed", ImVec4(1, 0, 0, 1));
        freeaddrinfo(result);
        WSACleanup();
        return;
    }

    if (bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        showStatusMessage("bind failed", ImVec4(1, 0, 0, 1));
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    freeaddrinfo(result);

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        showStatusMessage("listen failed", ImVec4(1, 0, 0, 1));
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    ServerInfo info = { address, "Windows", std::chrono::steady_clock::now() };
    currentAPIKey = generateAPIKey();
    servers[currentAPIKey] = info;

    showStatusMessage("[+] Server started at " + address + ":" + port + " with API Key: " + currentAPIKey, ImVec4(0, 1, 0, 1));

    SOCKET ClientSocket = INVALID_SOCKET;
    while (serverRunning) {
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            showStatusMessage("accept failed", ImVec4(1, 0, 0, 1));
            closesocket(ListenSocket);
            WSACleanup();
            return;
        }

        char recvbuf[512];
        int iResult = recv(ClientSocket, recvbuf, sizeof(recvbuf), 0);
        if (iResult > 0) {
            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " + std::to_string(strlen(htmlBuffer)) + "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                std::string(htmlBuffer);
            send(ClientSocket, response.c_str(), (int)response.length(), 0);
        }
        else if (iResult == 0) {
            showStatusMessage("Connection closed", ImVec4(1, 1, 0, 1));
        }
        else {
            showStatusMessage("recv failed", ImVec4(1, 0, 0, 1));
        }

        closesocket(ClientSocket);
    }

    closesocket(ListenSocket);
    WSACleanup();
    showStatusMessage("[-] Server stopped.", ImVec4(1, 0, 0, 1));
}

std::string getServerInfo(const std::string& apiKey) {
    if (servers.find(apiKey) != servers.end()) {
        auto& info = servers[apiKey];
        auto uptime = std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now() - info.startTime).count();

        std::ostringstream oss;
        oss << "IP Address: " << info.ip << "\n"
            << "Host System: " << info.hostSystem << "\n"
            << "Uptime: " << uptime << " minutes\n";

        return oss.str();
    }
    else {
        return "Server not found.";
    }
}

bool isStatusMessageVisible() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - statusTimestamp).count() < 5;
}

void stopServer() {
    serverRunning = false;
}

void Drawing::Active() {
    bDraw = true;
}

bool Drawing::isActive() {
    return bDraw;
}

void Drawing::Draw() {
    if (isActive()) {
        ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::Begin(lpWindowName, &bDraw, WindowFlags);

        ImGui::Text("HTTP Server Configuration");

        ImGui::InputTextMultiline("HTML Content", htmlBuffer, HTML_BUFFER_SIZE, ImVec2(-FLT_MIN, 200), ImGuiInputTextFlags_CharsNoBlank);
        if (ImGui::IsItemDeactivatedAfterEdit()) htmlModified = true;

        ImGui::InputText("Server Address", &serverAddress[0], serverAddress.size() + 1);
        ImGui::InputText("Server Port", &serverPort[0], serverPort.size() + 1);

        if (ImGui::Button("Start Server")) {
            if (!serverRunning) {
                serverRunning = true;
                std::thread(startServer, serverAddress, serverPort).detach();
            }
            else {
                showStatusMessage("[!] Server is already running.", ImVec4(1, 1, 0, 1));
            }
        }

        ImGui::BeginDisabled(!htmlModified);
        if (ImGui::Button("Update Server [+]")) {
            if (serverRunning) {
                htmlModified = false;
                showStatusMessage("[+] Server HTML updated.", ImVec4(0, 1, 0, 1));
            }
            else {
                showStatusMessage("[!] Server is not running.", ImVec4(1, 1, 0, 1));
            }
        }
        ImGui::EndDisabled();

        ImGui::Text("Current API Key: %s", currentAPIKey.c_str());
        ImGui::InputText("Search API Key", searchAPIKey, API_KEY_SIZE);

        if (ImGui::Button("Generate New API Key")) {
            if (serverRunning) {
                currentAPIKey = generateAPIKey();
                servers[currentAPIKey] = { serverAddress, "Windows", std::chrono::steady_clock::now() };
                showStatusMessage("New API Key generated: " + currentAPIKey, ImVec4(0, 1, 0, 1));
            }
            else {
                showStatusMessage("[!] Server is not running.", ImVec4(1, 1, 0, 1));
            }
        }

        ImGui::BeginDisabled(searchAPIKey[0] == '\0');
        if (ImGui::Button("Get Server Info")) {
            showStatusMessage(getServerInfo(searchAPIKey), ImVec4(0, 1, 0, 1));
        }
        ImGui::EndDisabled();

        if (isStatusMessageVisible()) {
            ImGui::TextColored(statusColor, statusMessage.c_str());
        }

        ImGui::End();
    }
}
