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
static std::string responsesDirectory = baseDirectory + "Responses/";
static std::string responseTypesPath = baseDirectory + "responsetypes_template.json";

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
        std::string cell;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); ++i) { // Allows for commas to be in the questions themselves, rather than parsing by each comma in the CSV
            char c = line[i];

            if (c == '"') {
                if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                    cell += '"';
                    ++i; // skip the escaped quote
                }
                else {
                    inQuotes = !inQuotes;
                }
            }
            else if (c == ',' && !inQuotes) {
                row.push_back(cell);
                cell.clear();
            }
            else {
                cell += c;
            }
        }
        row.push_back(cell);  
        data.push_back(row);
    }

    file.close();
}


// Saves all survey data to individual json files
void SaveSurveyToJSON(
    const std::string& mops,
    const std::vector<std::string>& questions,
    const std::vector<std::string>& rTs,
    const std::string& directoryPath,
    const nlohmann::json& editableMetadata,
    const std::string& testEventName
) {
    // Construct file path: directoryPath + MOPS + ".json"
    std::string fullFilePath = directoryPath + mops + ".json";

    // Prepare JSON object
    nlohmann::json jsonData;

    // Add questions and response types
    jsonData["questions"] = questions;
    jsonData["responseTypes"] = rTs;

    // Initialize responses based on response type - This allows both multiple choice and "fill in the blank" type responses
    nlohmann::json responses = nlohmann::json::array();
    for (size_t i = 0; i < questions.size(); ++i) {
        // Check if this is a string response type
        if (rTs[i] == "String") {
            responses.push_back("");  // Initialize string responses to empty string
        }
        else {
            responses.push_back(nullptr);  // Keep null for other types
        }
    }

    jsonData["responses"] = responses;
    if (editableMetadata.contains("aircrew")) {
        jsonData["metadata"] = editableMetadata["aircrew"];
    }
    else {
        std::cerr << "Missing 'aircrew' metadata!" << std::endl;
        jsonData["metadata"] = {};  // fallback
    };
    
    // Save the file
    std::ofstream jsonFile(fullFilePath);
    if (!jsonFile.is_open()) {
        ImGui::OpenPopup("File Save Error");
        return;
    }

    jsonFile << jsonData.dump(4); // Pretty print with indent
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
            if (ImGui::Button(("Remove Question from " + testEvent + " " + mops).c_str())) {
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
    ImGui::NewLine();
    for (const auto& responseType : responseTypes) {
        // Correcting to access 'id' as the response type
        ImGui::Text("%s", responseType.id.c_str());

        // Render the labels associated with the response type
        for (const auto& label : responseType.labels) {  // Use 'labels' instead of 'responseLabels'
            ImGui::Text("  - %s", label.c_str());
        }
        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();
    }
}

// Function to edit Metadata
void RenderMetadataPopup(bool& openPopup, nlohmann::json& editableMetadata, const std::string& filePath) {
    static bool addNewFieldRequested = false;  // NEW: flag for adding field immediately

    if (openPopup)
        ImGui::OpenPopup("Edit Metadata");

    if (ImGui::BeginPopupModal("Edit Metadata", nullptr, ImGuiWindowFlags_None)) {
        ImVec2 windowSize(600, 400);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
        ImVec2 minSize(400, 300);
        ImVec2 maxSize(800, 600);
        ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

        std::vector<std::string> sectionNames;
        for (auto& category : editableMetadata.items()) {
            sectionNames.push_back(category.key());
        }

        std::vector<const char*> sectionItems;
        for (const auto& sectionName : sectionNames) {
            sectionItems.push_back(sectionName.c_str());
        }

        static int selectedSection = 0;
        if (ImGui::Combo("Select Section", &selectedSection, sectionItems.data(), sectionItems.size())) {
            // Handle section change if needed
        }

        std::string selectedCategoryName = sectionNames[selectedSection];
        nlohmann::json& fields = editableMetadata[selectedCategoryName];

        ImGui::Text("%s", selectedCategoryName.c_str());
        ImGui::Separator();

        // Deferred removal and rename
        std::string fieldToRemove = "";
        std::unordered_map<std::string, std::string> renameMap;

        // Title buffer
        static std::unordered_map<std::string, std::string> keyEditBuffers;

        // --- IMMEDIATE NEW FIELD INSERTION ---
        if (addNewFieldRequested) {
            int suffix = 1;
            std::string baseName = "New Field";
            std::string newFieldName = baseName;

            while (fields.contains(newFieldName)) {
                newFieldName = baseName + " " + std::to_string(suffix++);
            }

            fields[newFieldName] = {
                {"inputType", "text"},
                {"preset", ""},
                {"response", ""}
            };

            keyEditBuffers[newFieldName] = newFieldName;
            addNewFieldRequested = false;
        }

        // --- MAIN FIELD LOOP ---
        for (auto& item : fields.items()) {
            std::string key = item.key();
            nlohmann::json& field = item.value();
            std::string uniqueKey = key + "##" + std::to_string(reinterpret_cast<uintptr_t>(&field));

            // Init buffer
            if (keyEditBuffers.find(key) == keyEditBuffers.end()) {
                keyEditBuffers[key] = key;
            }

            // Editable field title
            char titleBuffer[256];
            strncpy(titleBuffer, keyEditBuffers[key].c_str(), sizeof(titleBuffer));
            titleBuffer[sizeof(titleBuffer) - 1] = '\0';

            if (ImGui::InputText(("Field Title##" + uniqueKey).c_str(), titleBuffer, sizeof(titleBuffer))) {
                keyEditBuffers[key] = std::string(titleBuffer);
            }

            // Queue for rename
            if (keyEditBuffers[key] != key && !keyEditBuffers[key].empty()) {
                renameMap[key] = keyEditBuffers[key];
            }

            // Input Type
            const char* inputTypes[] = { "Text", "Time", "Dropdown" };
            int inputTypeIndex = 0;
            if (field["inputType"] == "time") inputTypeIndex = 1;
            else if (field["inputType"] == "dropdown") inputTypeIndex = 2;

            if (ImGui::Combo(("Input Type##" + uniqueKey).c_str(), &inputTypeIndex, inputTypes, IM_ARRAYSIZE(inputTypes))) {
                if (inputTypeIndex == 0) {
                    field["inputType"] = "text";
                    field["preset"] = "";
                    field["response"] = "";
                }
                else if (inputTypeIndex == 1) {
                    field["inputType"] = "time";
                    field["preset"] = "";
                    field["response"] = "";
                }
                else if (inputTypeIndex == 2) {
                    field["inputType"] = "dropdown";
                    field["preset"] = { "" };
                    field["response"] = 0;
                }
            }

            // Text response
            if (field["inputType"] == "text") {
                char buffer[256];
                strcpy(buffer, field["response"].get<std::string>().c_str());
                if (ImGui::InputText(("Response##" + uniqueKey).c_str(), buffer, sizeof(buffer))) {
                    field["response"] = std::string(buffer);
                }
            }

            // Dropdown response
            if (field["inputType"] == "dropdown") {
                ImGui::Text("Preset Options");
                for (size_t i = 0; i < field["preset"].size(); ++i) {
                    std::string preset = field["preset"][i];
                    char buffer[256];
                    strcpy(buffer, preset.c_str());

                    if (ImGui::InputText(("Preset " + std::to_string(i + 1) + "##" + uniqueKey).c_str(), buffer, sizeof(buffer))) {
                        field["preset"][i] = std::string(buffer);
                    }

                    if (i == field["preset"].size() - 1) {
                        if (ImGui::Button(("Remove Preset##" + uniqueKey).c_str())) {
                            field["preset"].erase(i);
                        }
                    }
                }

                if (ImGui::Button(("Add Preset##" + uniqueKey).c_str())) {
                    field["preset"].push_back("");
                }

                int response = field["response"].get<int>();
                field["response"] = response;
            }

            if (field["inputType"] != "dropdown") {
                ImGui::Text("Response field not editable for this type.");
            }

            if (key != "User ID") { // Prevent removal of "User ID"
                if (ImGui::Button(("Remove Field##" + uniqueKey).c_str())) {
                    fieldToRemove = key;
                }
            }
            else {
                ImGui::TextDisabled("Cannot remove 'User ID'");
            }


            ImGui::Separator();
        }

        // --- APPLY REMOVAL ---
        if (!fieldToRemove.empty()) {
            fields.erase(fieldToRemove);
        }

        ImGui::NewLine();

        // --- ADD FIELD BUTTON ---
        if (ImGui::Button("Add New Field")) {
            addNewFieldRequested = true;
        }

        // --- SAVE CHANGES ---
        if (ImGui::Button("Save")) {
            // Apply renames
            for (const auto& [oldKey, newKey] : renameMap) {
                if (oldKey != newKey && !newKey.empty() && fields.contains(oldKey)) {
                    fields[newKey] = fields[oldKey];
                    fields.erase(oldKey);
                    keyEditBuffers[newKey] = keyEditBuffers[oldKey];
                    keyEditBuffers.erase(oldKey);
                }
            }

            renameMap.clear();
            openPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}





void MyApp::RenderCreateSurveysPage() {
    static bool needsSave = false;
    static bool needsReload = true;
    static std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>> editableData;

    float availableWidth = ImGui::GetContentRegionAvail().x;
    float availableHeight = ImGui::GetContentRegionAvail().y;
    float childHeight = availableHeight * 0.9f - 15.0f;

    static bool isFirstSave = true; // Track if it's the first save attempt
    ImGui::GetStyle().FrameRounding = 5.0f;
    ImGui::NewLine();
    if (ImGui::Button("Select Surveys csv file")) {
        result = openFile();
        CSVPath = sFilePath.c_str();
        editableData.clear();
        needsReload = true;
    }

    ImGui::SameLine();
    ImGui::NewLine();
    ImGui::NewLine();
    if (ImGui::Button("Back to Home")) {
        currentPage = Page::Home;
    }
    ImGui::GetStyle().FrameRounding = 5.0f;
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

    auto nestedChildHeight = childHeight- 140.0f;

    ImGui::BeginChild("Survey Editor", ImVec2(availableWidth * 0.7f, nestedChildHeight), true);
    RenderSurveyData(editableData, needsSave);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("Response Types", ImVec2(availableWidth * 0.25f, nestedChildHeight), true);
    RenderResponseTypes();
    ImGui::EndChild();

    // Save and Back buttons
    ImGui::BeginChild("SaveAndBack", ImVec2(availableWidth, 110.0f), false);
    ImGui::Separator();

    // Button to edit the metadata
    static bool openMetadataPopup = false;
    static nlohmann::json editableMetadata;
    static std::string currentFilePath;

    ImGui::NewLine();

    if (ImGui::Button("Edit Metadata")) {
        std::string metadataPath = baseDirectory + "metadata_template.json";
        std::ifstream metadataFile(metadataPath);
        if (metadataFile.is_open()) {
            if (editableMetadata.empty()) metadataFile >> editableMetadata;
            metadataFile.close();
            currentFilePath = metadataPath;
            openMetadataPopup = true;
        }
    }
    RenderMetadataPopup(openMetadataPopup, editableMetadata, currentFilePath);

    ImGui::Text("Enter Test Program Name:");
    ImGui::InputText("Test Program Name", testProgramName, IM_ARRAYSIZE(testProgramName));

    if (ImGui::Button("Populate Surveys") && (needsSave || isFirstSave)) {
        if (isFirstSave) {
            isFirstSave = false; // Disable first save flag after first save
        }

        // Append the folder name to the surveysDirectory, replacing spaces with underscores
        std::string folder = (testProgramName[0] == '\0') ? "New Program" : testProgramName; // Use "program1" if no test program name is provided
        std::replace(folder.begin(), folder.end(), ' ', '_'); // Replace spaces with underscores
        std::string fullDirectory = surveysDirectory + folder + "/";  // Full directory path


        // Ensure the directory for the test event exists before saving MOPS JSON files
        EnsureDirectoryExists(fullDirectory);

        // Do the same for the responses directory
        EnsureDirectoryExists(responsesDirectory + folder + "/");

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
                SaveSurveyToJSON(mops, questionList, responseTypeList, testEventDirectory, editableMetadata, testEvent);
            }
        }

        // --- Save metadataquestions.json ---
        std::string metadataOutputPath = fullDirectory + "metadataquestions.json";
        std::ofstream metadataOut(metadataOutputPath);
        if (metadataOut.is_open()) {
            metadataOut << editableMetadata.dump(4); // Pretty print
            metadataOut.close();
        }
        else {
            std::cerr << "Failed to save metadataquestions.json\n";
            ImGui::OpenPopup("File Save Error");
        }

        // --- Save responsetypes.json ---
        std::string responseTypesOutputPath = fullDirectory + "responsetypes.json";
        nlohmann::json rtJson;
        for (const auto& rt : responseTypes) {
            nlohmann::json entry;
            entry["id"] = rt.id;
            entry["count"] = rt.count;
            entry["labels"] = rt.labels;
            rtJson["responseTypes"].push_back(entry);
        }
        std::ofstream rtOut(responseTypesOutputPath);
        if (rtOut.is_open()) {
            rtOut << rtJson.dump(4); // Pretty print
            rtOut.close();
        }
        else {
            std::cerr << "Failed to save responsetypes.json\n";
            ImGui::OpenPopup("File Save Error");
        }


        // Confirm save status
        needsSave = false;
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
