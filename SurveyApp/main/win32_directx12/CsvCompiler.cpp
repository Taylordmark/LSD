// #include "Application.h"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::milliseconds
#include "CsvCompiler.h"
#include "json.hpp"

using std::string;
using std::vector;
using std::unordered_map;
using std::ifstream;
using std::endl;
using std::map;
using std::pair;
using std::cout;

// Static variable definitions
static string baseDirectory = "C:/LSD/AppFiles/TestPrograms/";
static string responseDirectory = "C:/LSD/AppFiles/Responses/";
static string compilerDirectory = "C:/LSD/AppFiles/Compiler/";

// Create compiler directory if it does not exist
if (!std::filesystem::exists(compilerDirectory)) {
    std::filesystem::create_directories(compilerDirectory);
}

