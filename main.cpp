// main.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "SaperaUse.h"
#include <filesystem>
#include <algorithm>

SaperaUse cam;

bool validateConfigFile(const std::string& inputPath, std::string& cleanedPath) {
    cleanedPath = inputPath;

    // 去除首尾空格
    cleanedPath.erase(0, cleanedPath.find_first_not_of(" \t"));
    cleanedPath.erase(cleanedPath.find_last_not_of(" \t") + 1);

    // 去除首尾双引号
    if (!cleanedPath.empty() && cleanedPath.front() == '"' && cleanedPath.back() == '"') {
        cleanedPath = cleanedPath.substr(1, cleanedPath.size() - 2);
    }

    return true;
}

int main()
{
    std::string configPath;

    std::cout << "请输入配置文件(.ini)路径：";
    std::getline(std::cin, configPath);  // 从命令行读取整行路径

    if (!ConfigManager::getInstance().loadConfig(configPath)) {
        std::cout << "ini配置文件加载失败\n";
        return -1;
    }

    std::cout << "正在初始化\n";
    cam.GrabbersInit();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "初始化完成，按任意键开始\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    while (1) {
        if (_kbhit() != 0) {  //如果键盘被敲击
            break;
        }
    }
    std::cout << "正在加载画面\n";


    cam.CreateDevice(CONFIG.getGrabberIndex(), CONFIG.getCameraIndex(), CONFIG.getGrabberConfigPath().c_str());  // grabberIndex - deviceIndex - configFilePath
}


// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
