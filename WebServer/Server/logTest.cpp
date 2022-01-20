#include "log.h"
#include <iostream>
using namespace std;

int main() {
    Log::Instance()->Init(1, "./log", ".log", 2);
    cout << "OK" << endl;
    LOG_ERROR("MySql init error");
    LOG_INFO("This is log info");
    LOG_DEBUG("This is log for debug");
    LOG_WARN("This is log WARNING");
}
