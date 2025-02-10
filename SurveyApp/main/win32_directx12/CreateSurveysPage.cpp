#include "Application.h"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <unordered_map>

#include "CreateSurveysPage.h"
#include "json.hpp"


#include <Windows.h>
#include <string>
#include <shobjidl.h> 

std::string sSelectedFile;
std::string sFilePath;
bool openFile()
{
    //  CREATE FILE OBJECT INSTANCE
    HRESULT f_SysHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(f_SysHr))
        return FALSE;

    // CREATE FileOpenDialog OBJECT
    IFileOpenDialog* f_FileSystem;
    f_SysHr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&f_FileSystem));
    if (FAILED(f_SysHr)) {
        CoUninitialize();
        return FALSE;
    }

    //  SHOW OPEN FILE DIALOG WINDOW
    f_SysHr = f_FileSystem->Show(NULL);
    if (FAILED(f_SysHr)) {
        f_FileSystem->Release();
        CoUninitialize();
        return FALSE;
    }

    //  RETRIEVE FILE NAME FROM THE SELECTED ITEM
    IShellItem* f_Files;
    f_SysHr = f_FileSystem->GetResult(&f_Files);
    if (FAILED(f_SysHr)) {
        f_FileSystem->Release();
        CoUninitialize();
        return FALSE;
    }

    //  STORE AND CONVERT THE FILE NAME
    PWSTR f_Path;
    f_SysHr = f_Files->GetDisplayName(SIGDN_FILESYSPATH, &f_Path);
    if (FAILED(f_SysHr)) {
        f_Files->Release();
        f_FileSystem->Release();
        CoUninitialize();
        return FALSE;
    }

    //  FORMAT AND STORE THE FILE PATH
    std::wstring path(f_Path);
    std::string c(path.begin(), path.end());
    sFilePath = c;

    //  FORMAT STRING FOR EXECUTABLE NAME
    const size_t slash = sFilePath.find_last_of("/\\");
    sSelectedFile = sFilePath.substr(slash + 1);

    //  SUCCESS, CLEAN UP
    CoTaskMemFree(f_Path);
    f_Files->Release();
    f_FileSystem->Release();
    CoUninitialize();
    return TRUE;
}

bool result = FALSE;


namespace fs = std::filesystem;

struct ResponseType {
    std::string id;
    int count;
    std::vector<std::string> labels;  // List of labels for the response options
};

std::vector<ResponseType> responseTypes;
std::vector<std::vector<std::string>> surveyData;


static std::string baseDirectory = "C:/LSD/AppFiles/";
static std::string surveysDirectory = baseDirectory + "TestPrograms/";
// static std::string CSVPath = baseDirectory + "SurveyQuestionsDemo.csv";
static std::string responseTypesPath = baseDirectory + "responsetypes.json";


static std::string CSVPath = "";

static char testProgramName[128] = {};

// Function to create directory if it doesn't exist
void EnsureDirectoryExists(const std::string& path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path); // No need to store the return value
        }
    }
    catch (const fs::filesystem_error&) {
        // Handle exception if needed
    }
}

// Function to load CSV data
void LoadDataFromFile(const std::string& filename, std::vector<std::vector<std::string>>& data) {
    data.clear();
    std::ifstream file(filename);
    if (!file.is_open()) {
        ImGui::OpenPopup("File Load Error");
        return;
    }

    std::string line;
    bool firstRow = true;
    while (std::getline(file, line)) {
        if (firstRow) {
            firstRow = false;
            continue;
        }

        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row);
    }

    file.close();
}

// Saves all survey data to individual json files
void SaveSurveyToJSON(
    const std::string& mops,
    const std::vector<std::string>& questions,
    const std::vector<std::string>& rTs,
    const std::string& directoryPath) {

    // Construct file path: directoryPath + MOPS + ".json"
    std::string fullFilePath = directoryPath + mops + ".json";

    // Prepare JSON object
    nlohmann::json jsonData;

    // Add questions
    jsonData["questions"] = questions;

    // Add response types
    jsonData["responseTypes"] = rTs;

    // Add responses (NULL for each question)
    nlohmann::json responses = nlohmann::json::array();
    for (size_t i = 0; i < questions.size(); ++i) {
        responses.push_back(nullptr); // Each response is NULL
    }
    jsonData["responses"] = responses;

    // Metadata questions
    std::string metadataPath = baseDirectory + "metadataquestions.json";

    // Add metadata questions loading
    nlohmann::json metadataJson;
    std::ifstream metadataFile(metadataPath);
    if (metadataFile.is_open()) {
        metadataFile >> metadataJson;

        // Check if the "metadata" field exists in the JSON
        if (metadataJson.contains("aircrew")) {
            // Create a JSON object for metadata
            nlohmann::json metadataObj;

            // Iterate over the keys of the "metadata" object and store them in the metadata object
            for (const auto& item : metadataJson["aircrew"].items()) {
                metadataObj[item.key()] = item.value();
            }

            // Add the metadata object to the jsonData
            jsonData["metadata"] = metadataObj;
        }
    }

    // Open the JSON file for writing
    std::ofstream jsonFile(fullFilePath);
    if (!jsonFile.is_open()) {
        ImGui::OpenPopup("File Save Error");
        return;
    }

    // Write the JSON data with pretty printing (indent 4 spaces)
    jsonFile << jsonData.dump(4);
    jsonFile.close();
}


// Function to load response types from a JSON file
void LoadResponseTypesFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to load file: " << filename << std::endl;
        return;
    }

    nlohmann::json jsonData;
    file >> jsonData;

    responseTypes.clear();  // Clear the global responseTypes vector
    for (const auto& item : jsonData["responseTypes"]) {
        ResponseType rt;
        rt.id = item["id"];  // Correctly using the `id` from the JSON
        rt.count = item["count"];  // Correctly using the `count`
        rt.labels = item["labels"].get<std::vector<std::string>>();  // Correctly using the `labels`
        responseTypes.push_back(rt);
    }
}

// Function to render the questions and response types
void RenderSurveyData(std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>>& editableData, bool& needsSave) {
    for (auto& [testEvent, mopsMap] : editableData) {
        ImGui::Text("Test Event: %s", testEvent.c_str());
        for (auto& [mops, questions] : mopsMap) {
            ImGui::NewLine();
            ImGui::Text("  MOP/S: %s", mops.c_str());

            for (size_t i = 0; i < questions.size(); ++i) {
                auto& [question, responseType] = questions[i];
                char questionBuffer[512] = {};  // Buffer for question text

                // Copy the current question into the buffer
                strncpy_s(questionBuffer, sizeof(questionBuffer), question.c_str(), _TRUNCATE);

                // Input for question text
                ImGui::InputTextMultiline(("##Question " + std::to_string(i + 1) + testEvent + "_" + mops).c_str(), questionBuffer, 512, ImVec2(0, ImGui::GetTextLineHeight() * 2));

                // Check if question text changed and update it
                if (question != questionBuffer) {
                    question = questionBuffer;  // Update the actual question text
                    needsSave = true; // Mark as needing save
                }

                // Response type dropdown: Uses the global responseTypes
                std::vector<std::string> responseTypeLabels;
                for (const auto& rt : responseTypes) {
                    responseTypeLabels.push_back(rt.id);  // Correctly using `id` from ResponseType
                }

                ImGui::SameLine();

                bool dropdownChanged = false;
                if (ImGui::BeginCombo(("##Response Type " + std::to_string(i + 1) + testEvent + "_" + mops).c_str(), responseType.c_str())) {
                    for (const auto& label : responseTypeLabels) {
                        bool isSelected = (label == responseType);
                        if (ImGui::Selectable(label.c_str(), isSelected)) {
                            responseType = label;
                            questions[i].second = responseType;
                            dropdownChanged = true;
                            needsSave = true; // Mark as needing save
                        }
                    }
                    ImGui::EndCombo();
                }

            }

            // Add/Remove question buttons
            ImGui::NewLine();
            if (ImGui::Button(("Add Question to " + testEvent + " " + mops).c_str())) {
                questions.push_back({ "", "Default Response Type" });
                needsSave = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(("Remove Last Question from " + testEvent + " " + mops).c_str())) {
                if (!questions.empty()) {
                    questions.pop_back();
                    needsSave = true;
                }
            }
            ImGui::NewLine();
        }
        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();
    }
}

// Function to render the response types
void RenderResponseTypes() {
    ImGui::Text("Available Response Types:");
    for (const auto& responseType : responseTypes) {
        // Correcting to access 'id' as the response type
        ImGui::Text("Type: %s", responseType.id.c_str());  // Use 'id' instead of 'type'

        // Render the labels associated with the response type
        for (const auto& label : responseType.labels) {  // Use 'labels' instead of 'responseLabels'
            ImGui::Text("  - %s", label.c_str());
        }

        ImGui::Separator();
    }
}


void MyApp::RenderCreateSurveysPage() {
    static bool needsSave = false;
    static bool needsReload = true;
    static std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>> editableData;

    float availableWidth = ImGui::GetContentRegionAvail().x;
    float availableHeight = ImGui::GetContentRegionAvail().y;
    float childHeight = availableHeight * 0.9f;

    static bool isFirstSave = true; // Track if it's the first save attempt

    ImGui::NewLine();
    if (ImGui::Button("Select Surveys csv file")) {
        result = openFile();
        CSVPath = sFilePath.c_str();
        editableData.clear();
        needsReload = true;
    }
    ImGui::NewLine();

    // Load data once
    if (needsReload && CSVPath.size() > 1) {
        LoadDataFromFile(CSVPath, surveyData);
        LoadResponseTypesFromFile(responseTypesPath);

        // Populate editable data from CSV
        for (const auto& row : surveyData) {
            if (row.size() < 4) continue;
            std::string testEvent = row[0];
            std::string mops = row[1];
            std::string question = row[2];
            std::string responseType = row[3];

            if (question.empty() || responseType.empty()) continue;

            editableData[testEvent][mops].emplace_back(question, responseType);
        }

        needsReload = false;
    }

    auto nestedChildHeight = childHeight- 100.0f;

    ImGui::BeginChild("Survey Editor", ImVec2(availableWidth * 0.7f, nestedChildHeight), true);
    RenderSurveyData(editableData, needsSave);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("Response Types", ImVec2(availableWidth * 0.25f, nestedChildHeight), true);
    RenderResponseTypes();
    ImGui::EndChild();

    // Save and Back buttons
    ImGui::BeginChild("SaveAndBack", ImVec2(availableWidth, 100.0f), false);
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Text("Enter Test Program Name:");
    ImGui::InputText("Test Program Name", testProgramName, IM_ARRAYSIZE(testProgramName));

    if (ImGui::Button("Populate Surveys") && (needsSave || isFirstSave)) {
        if (isFirstSave) {
            isFirstSave = false; // Disable first save flag after first save
        }

        // Append the folder name to the surveysDirectory, replacing spaces with underscores
        std::string folder = (testProgramName[0] == '\0') ? "program1" : testProgramName; // Use "program1" if no test program name is provided
        std::replace(folder.begin(), folder.end(), ' ', '_'); // Replace spaces with underscores
        std::string fullDirectory = surveysDirectory + folder + "/";  // Full directory path


        // Ensure the directory for the test event exists before saving MOPS JSON files
        EnsureDirectoryExists(fullDirectory);

        // Iterate over editableData to save MOPS JSON files for each Test Event and MOPS combination
        for (const auto& [testEvent, mopsMap] : editableData) {
            // Create the directory for each Test Event
            std::string testEventDirectory = fullDirectory + testEvent + "/";
            EnsureDirectoryExists(testEventDirectory);  // Ensure the Test Event directory exists

            for (const auto& [mops, questions] : mopsMap) {
                // Extract questions and response types
                std::vector<std::string> questionList;
                std::vector<std::string> responseTypeList;

                for (const auto& [question, responseType] : questions) {
                    questionList.push_back(question);
                    responseTypeList.push_back(responseType);
                }

                // Save survey data to JSON for each MOPS
                SaveSurveyToJSON(mops, questionList, responseTypeList, testEventDirectory);
            }
        }

        // Confirm save status
        needsSave = false;
        currentPage = Page::Home;
    }

    ImGui::SameLine();
    if (ImGui::Button("Back")) {
        currentPage = Page::Home;
    }
    ImGui::EndChild();

    // Handle error pop-ups
    if (ImGui::BeginPopup("File Load Error")) {
        ImGui::Text("Failed to load a file. Please check the file path.");
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("File Save Error")) {
        ImGui::Text("Failed to save the file. Please check file permissions.");
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
