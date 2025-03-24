#include "Application.h"
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
#include "CompleteSurveysPage.h"
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
static string responseDirectory = "C:/LSD/AppFiles/responses/";

struct ResponseTypeC {
    string id;
    int count;
    std::vector<std::string> labels;  // List of labels for the response options
};

std::vector<ResponseTypeC> responseTypes;

// Current dt
static std::string currentDateTime;

// Data to show correct survey
static int selectedTestProgramIndex = -1;
static int lastSelectedProgramIndex = -1;
static std::string selectedTestProgram = "";
static std::string selectedTestEvent = "";
static std::string lastSelectedTestEvent = "";

// Store survey comments
static std::unordered_map<std::string, std::string> comments;

// Used for logic checks for saving and subsequent error messages
static std::vector<std::string> unansweredQuestions;


static nlohmann::json MOPS;

// initialize metadata holder to compile from all MOPS (also hold responses)
static nlohmann::json metadata;

static bool showSurvey = false;
static bool refreshData = false;

// End static variable definitions

// Begin function definitions
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////


// Get folder names from directory
std::vector<std::string> GetFolderNames(const std::string& directoryPath) {
    std::vector<std::string> folderNames;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_directory()) {
                folderNames.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
    return folderNames;
}

// Load response types from a JSON file
void LoadResponseTypes(const std::string& filename) {
    std::ifstream file(filename);
    cout << filename << endl;
    if (!file.is_open()) {
        std::cerr << "Failed to load file: " << filename << std::endl;
        return;
    }

    nlohmann::json jsonData;
    file >> jsonData;

    responseTypes.clear();  // Clear the global responseTypes vector
    for (const auto& item : jsonData["responseTypes"]) {
        ResponseTypeC rt;
        rt.id = item["id"];  // Correctly using the `id` from the JSON
        rt.count = item["count"];  // Correctly using the `count`
        rt.labels = item["labels"].get<std::vector<std::string>>();  // Correctly using the `labels`
        responseTypes.push_back(rt);
    }
}

// Get current date time and store in currentDateTime global var
void getCurrentDateTime() {
    // Get the current time from the system clock
    const auto now = std::chrono::system_clock::now();

    // Convert the time_point to the local time zone (assuming C++20 or later)
    const auto local_time = std::chrono::current_zone()->to_local(now);

    // Convert to a time_point in days (truncating to whole days)
    const auto time_point = std::chrono::time_point_cast<std::chrono::days>(local_time);

    // Convert time_point to year_month_day
    const auto year_month_day = std::chrono::year_month_day{ time_point };

    // Extract date components
    auto year = int(year_month_day.year());
    auto month = unsigned(year_month_day.month());
    auto day = unsigned(year_month_day.day());

    // Extract the time part (hour, minute, second) using std::localtime_s
    auto time = std::chrono::system_clock::to_time_t(now);  // Use `now` here, not `time_point`
    struct tm tm;

    // Use localtime_s instead of localtime
    localtime_s(&tm, &time);

    // Format the date and time as per the desired pattern
    currentDateTime = std::format("{:04}-{:02}-{:02}_{:02}-{:02}-{:02}", year, month, day, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

// Clear cached data when new selection made
void RefreshData() {
    responseTypes.clear();
    selectedTestEvent.clear();
    metadata.clear();
    MOPS.clear();
    refreshData = false;
}

// Display Test Program Combo Box (Dropdown) after getting local folder names
void RenderTestProgramCombo(const std::vector<std::string>& testPrograms) {
    ImGui::Text("Test Program");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##Test Program", selectedTestProgramIndex == -1 ? "Select" : testPrograms[selectedTestProgramIndex].c_str())) {
        for (int i = 0; i < testPrograms.size(); ++i) {
            bool isSelected = (selectedTestProgramIndex == i);
            if (ImGui::Selectable(testPrograms[i].c_str(), isSelected)) {
                if (selectedTestProgramIndex != i) {
                    selectedTestProgramIndex = i;
                    selectedTestProgram = testPrograms[i];
                }
            }
        }
        ImGui::EndCombo();
    }
    if (selectedTestProgramIndex != lastSelectedProgramIndex) {
        RefreshData();
        lastSelectedProgramIndex = selectedTestProgramIndex;
    }
}

// Show test events after selecting test program
void RenderTestEventsTable(const std::vector<std::string>& testEventFolders, char eventFilter[10]) {
    if (ImGui::BeginTable("TestEventTable", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Test Event", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < testEventFolders.size(); ++i) {
            std::string testEvent = testEventFolders[i];
            std::string filter(eventFilter, strlen(eventFilter)); // Convert eventFilter to std::string

            // Convert testEvent to lowercase
            std::transform(testEvent.begin(), testEvent.end(), testEvent.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });

            // Convert filter to lowercase
            std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });


            // Skip the row if the testEvent doesn't match the filter
            if (testEvent.find(filter) == std::string::npos) {
                continue;  // Skip this row if there's no match
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            // Only print what comes before the last underscore
            std::string str = testEventFolders[i].c_str();
            size_t pos = str.rfind('_');  // Find the last occurrence of '_'
            if (pos != std::string::npos) {
                str = str.substr(0, pos);  // Get the substring before the last underscore
            }
            ImGui::Text("%s", str.c_str());

            ImGui::TableSetColumnIndex(1);
            std::string buttonID = "Take Survey##" + std::to_string(i);

            // Check if the current test event is selected (either because it was previously selected or clicked now)
            bool isSelected = (lastSelectedTestEvent == testEventFolders[i]);

            // If selected, show a different style or label
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.4f, 1.0f)); // Change button color
            }

            if (ImGui::Button(buttonID.c_str())) {
                selectedTestEvent = testEventFolders[i];
                showSurvey = true;
                if (lastSelectedTestEvent != selectedTestEvent) MOPS.clear();
                lastSelectedTestEvent = selectedTestEvent;  // Remember which button was clicked
            }
            else (refreshData = true);

            if (isSelected) {
                ImGui::PopStyleColor(); // Restore the button style
            }
        }

        ImGui::EndTable();
    }
}

// Load metadata, questions, and response types from JSON files
bool LoadMetadataAndQuestions(std::string testEventPath) {
    bool success = false;
    try {

        // Iterate through files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(testEventPath)) {
            if (entry.path().extension() == ".json") {

                // Extract the file name without the extension
                std::string MOPSID = entry.path().stem().string();

                // If it's a JSON file, get data
                std::ifstream jsonFile(entry.path());
                if (jsonFile.is_open()) {
                    nlohmann::json jsonData;
                    jsonFile >> jsonData;

                    if (jsonData.contains("metadata")) {                        
                        // Update metadata global
                        for (const auto& [m, v] : jsonData["metadata"].items()) {
                            if (metadata.find(m) == metadata.end()) {
                                metadata[m] = v;
                            }
                        }
                    }

                    if (jsonData.contains("questions")) {
                        MOPS[MOPSID]["questions"] = jsonData["questions"];
                    }

                    if (jsonData.contains("responseTypes")) {
                        MOPS[MOPSID]["responseTypes"] = jsonData["responseTypes"];
                    }

                    if (jsonData.contains("responses")) {
                        MOPS[MOPSID]["responses"] = jsonData["responses"];
                    }

                    success = true;
                    refreshData = false;
                }
                else {
                    std::cerr << "Failed to open JSON file: " << entry.path() << std::endl;
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error loading JSON files: " << e.what() << std::endl;
    }
    return success;
}

// Display Metadata and Questions with Response Types
void RenderSurveys() {
    // Create a scrollable area for the survey questions and responses
    ImGui::BeginChild("MetaChild", ImVec2(0, 255), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    float availableWidth = ImGui::GetContentRegionAvail().x;

    ImGui::Text("Flight Information");
    ImGui::Text("------------------------------------------------------------------");
    ImGui::NewLine();

    // Iterate through metadata and display items with associated input boxes
    for (auto& [metaID, metaValues] : metadata.items()) {

        // Display the key above input
        ImGui::Text("%s", metaID.c_str());
        std::string label = "##" + metaID;

        // Check if 'metaValues' has the required keys
        if (metaValues.contains("inputType")) {
            std::string inputTypeValue = metaValues["inputType"].get<std::string>();

            // If input type is array (dropdown), handle the dropdown logic
            if (inputTypeValue == "dropdown") {

                std::vector<std::string> options = metaValues["preset"];
                int currentSelection = -1;

                // Ensure the response exists and is valid for the selection
                if (metaValues.contains("response")) {
                    currentSelection = metaValues["response"].get<int>();
                }

                // Create the dropdown (combo) for the current array
                if (ImGui::BeginCombo(label.c_str(), currentSelection >= 0 ? options[currentSelection].c_str() : "")) {  // Use the current selection for label

                    // Loop through the options
                    for (int i = 0; i < options.size(); ++i) {
                        bool isSelected = (currentSelection == i);

                        // Render the selectable item in the combo box
                        if (ImGui::Selectable(options[i].c_str(), isSelected)) {
                            metaValues["response"] = i;  // Update the response when a new option is selected
                        }

                        // If the item is selected, set it as the default focus
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            // Check if metaValue is a string and show filled text box, if applicable
            if (inputTypeValue == "text" or inputTypeValue == "time") {
                std::string currentValue = metaValues["response"];

                // Display the input box with the current value
                char buf[256];

                // Use strncpy_s for safer string copy
                strncpy_s(buf, sizeof(buf), currentValue.c_str(), _TRUNCATE);  // _TRUNCATE ensures the string fits in the buffer

                if (ImGui::InputText(label.c_str(), buf, sizeof(buf))) {
                    // When the user types something, update the metadata map
                    metaValues["response"] = std::string(buf);  // Update the response field with new text
                }
            }
        }
    }

    ImGui::EndChild();

    ImGui::Separator();
    ImGui::NewLine();



    // Create a scrollable area for the survey questions and responses
    ImGui::BeginChild("SurveyChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

    // Print unanswered questions
    for (const auto& message : unansweredQuestions) {
        ImGui::Text("%s", message.c_str());  // Print each unanswered question message
    }

    ImGui::NewLine();

    ImGui::Text("Surveys");
    ImGui::Text("------------------------------------------------------------------");

    // Iterate through MOPS (key-value pairs in JSON)
    for (const auto& [MOPID, MOPValues] : MOPS.items()) {

        // Display survey name
        ImGui::NewLine();
        ImGui::Text("Survey: %s", MOPID.c_str());

        // Iterate through questions
        for (size_t q = 0; q < MOPValues["questions"].size(); ++q) {
            ImGui::NewLine();

            // Get previously selected response
            auto selectedResponse = MOPS[MOPID]["responses"][q];

            // Display the question text
            ImGui::Text("%s", MOPValues["questions"][q].get<std::string>().c_str());

            // Now match response types based on question index
            if (q < MOPValues["responseTypes"].size()) {

                std::string responseType = MOPValues["responseTypes"][q].get<std::string>();

                // Find the corresponding response type in the global responseTypes vector
                auto it = std::find_if(responseTypes.begin(), responseTypes.end(), [&responseType](const ResponseTypeC& rt) {
                    return rt.id == responseType;
                    });

                ImGui::NewLine();

                if (it != responseTypes.end()) {
                    // We found the corresponding response type, now render the UI components (radio buttons, text inputs, etc.)
                    const auto& labels = it->labels;
                    int count = it->count;  // The number of radio buttons to display

                    // Only create radio buttons if there are valid labels and count is greater than 0
                    if (count > 0) {
                        std::vector<int> labelPositions = { 0, count - 1 };

                        // Calculate label positions for radio buttons
                        if (labels.size() > 2) {
                            for (int j = 1; j < labels.size() - 1; ++j) {
                                int pos = static_cast<int>((j * (count - 1)) / (labels.size() - 1));
                                labelPositions.push_back(pos);
                            }
                        }

                        std::sort(labelPositions.begin(), labelPositions.end());

                        // Loop through all radio buttons
                        for (int b = 0; b < count; ++b) {

                            // Create a unique ID for each radio button
                            std::string radioButtonID = "##" + MOPID + "Q_" + std::to_string(q) + "B_" + std::to_string(b);

                            // Set a minimum width for the radio button (200px in this case)
                            ImGui::PushItemWidth(200);

                            // Check if button is selected
                            bool isSelected = (selectedResponse == b);

                            // Display the radio button
                            if (ImGui::RadioButton(radioButtonID.c_str(), isSelected)) {
                                MOPS[MOPID]["responses"][q] = b;
                            }

                            ImGui::PopItemWidth();

                            // Check if this radio button should have a label
                            if (std::find(labelPositions.begin(), labelPositions.end(), b) != labelPositions.end()) {
                                ImGui::SameLine();

                                __int64 labelIndex = std::distance(labelPositions.begin(), std::find(labelPositions.begin(), labelPositions.end(), b));
                                ImGui::Text(labels[labelIndex].c_str());
                            }
                        }

                    }
                }
            }
        }

        char commentsBuff[1024];  // Declare the buffer once

        if (comments.find(MOPID) != comments.end()) {
            // If MOPID exists, copy the value into commentsBuff
            strncpy(commentsBuff, comments[MOPID].c_str(), sizeof(commentsBuff) - 1);
            commentsBuff[sizeof(commentsBuff) - 1] = '\0';
        }
        else {
            commentsBuff[0] = '\0';
        }


        ImGui::NewLine();
        ImGui::Text("Comments");
        if (ImGui::InputTextMultiline(("##Comments" + MOPID).c_str(), commentsBuff, sizeof(commentsBuff), ImVec2(availableWidth*.9f, 75))) {
            comments[MOPID] = commentsBuff;
        }

        ImGui::Separator();

    }
    ImGui::EndChild();  // End the scrollable area
}

// Save surveys function
int saveSurvey(const std::string& MOP, const nlohmann::json& surveyJson) {
    try {

        // Check if the surveyJson is a valid JSON object
        if (surveyJson.is_string()) {
            std::cerr << "Error: surveyJson is a string, but it should be a JSON object!" << std::endl;
            return 0;  // Return 0 on error
        }

        // Ensure current date and time are properly fetched
        getCurrentDateTime();

        // Get user ID from metadata (ensure metadata["User ID"] is valid)
        std::string userID = metadata["User ID"]["response"];
        if (userID.empty()) {
            std::cerr << "Error: User ID is empty." << std::endl;
            return 0;  // Return 0 if userID is empty
        }

        // Construct the directory and file name
        std::string directory = responseDirectory + selectedTestProgram + "/" + selectedTestEvent + "/";
        std::string fileName = MOP + "_" + userID + "_" + currentDateTime + ".json";

        // Ensure the directory exists
        if (!std::filesystem::exists(directory)) {
            // Directory does not exist, attempt to create it
            if (!std::filesystem::create_directories(directory)) {
                std::cerr << "Error: Could not create directories for " << directory << std::endl;
                return 0;  // Return 0 if directory creation fails
            }
        }

        // Combine the directory and filename to get the full path
        std::string fullPath = directory + fileName;

        // Open the file for saving
        std::ofstream outFile(fullPath);
        if (!outFile) {
            std::cerr << "Error: Could not open file for writing: " << fullPath << std::endl;
            return 0;  // Return 0 if file opening fails
        }

        // Write the JSON content to the file
        outFile << surveyJson;

        return 1;  // Return 1 on successful saving
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 0;  // Return 0 if an exception occurs
    }
}



// Main GUI code
void MyApp::RenderCompleteSurveysPage() {
    // Calculate dimensions
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float availableHeight = ImGui::GetContentRegionAvail().y;
    float childHeight = availableHeight * 0.9f;

    static std::vector<std::string> testPrograms = GetFolderNames(baseDirectory);
    static std::vector<std::string> testEvents;

    static std::string testProgramPath = baseDirectory;

    if (selectedTestProgramIndex != -1) {
        static string responseTypesPath = baseDirectory + selectedTestProgram + "/" + "responsetypes.json";

        if (responseTypes.empty()) {LoadResponseTypes(responseTypesPath);}
    }
    

    ImGui::BeginChild("Test Event Display", ImVec2(availableWidth * 0.3f, childHeight), true);

    // Render the Test Program combo box
    RenderTestProgramCombo(testPrograms);

    // If test events have not been loaded and a test program is selected, load test events
    if (selectedTestProgramIndex != -1) {
        testProgramPath = baseDirectory + selectedTestProgram;
        testEvents = GetFolderNames(testProgramPath);
    }

    ImGui::GetStyle().FrameRounding = 5.0f;

    char eventFilter[10] = "";
    ImGui::Text("Event Filter");
    ImGui::SameLine();
    if (ImGui::InputText("##Test Event Filter", eventFilter, sizeof(eventFilter))) {  }

    // Render the test events table
    RenderTestEventsTable(testEvents, eventFilter);

    // If a MOPS is empty and test event is selected, fill MOPS
    if (MOPS.empty() && !selectedTestEvent.empty()) {
        // Construct the path to the selected test event
        string  testEventPath = baseDirectory + selectedTestProgram + "/" + selectedTestEvent;

        // Load metadata and questions for the selected test event
        if (LoadMetadataAndQuestions(testEventPath)) {
            refreshData = false;
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Survey", ImVec2(availableWidth * 0.67f, childHeight), true);

    ImGui::Text("Survey");

    ImGui::BeginChild("SurveyContent", ImVec2(availableWidth * 0.67f, childHeight - 70), true);

    if (showSurvey) {        
        // Survey Display Code
        ImGui::Text("Test Event %s", selectedTestEvent.c_str());
        ImGui::NewLine();
        // Render the metadata and questions
        if (!MOPS.empty()) {
            RenderSurveys();
        }
    }

    ImGui::EndChild();

    // Submit Surveys Button
    if (ImGui::Button("Submit Surveys")) {
        unansweredQuestions.clear();

        // Loop through MOPS to check that all survey questions have been answered (all metadata filled too?)
        for (auto& [MOPID, MOPValues] : MOPS.items()) {
            // Loop through the responses and check each one
            int index = 0;  // Counter to keep track of the index in the responses array
            for (auto& r : MOPValues["responses"]) {
                if (!r.is_number_integer()) {  // If the response is not an integer
                    // Create a message string for unanswered questions
                    std::string message = "Did you answer " + MOPID + " - Question " + std::to_string(index + 1) + "?";
                    unansweredQuestions.push_back(message);  // Add to unansweredQuestions vector
                }                
                ++index;  // Increment the index for the next response
            }
        }       

        // Check if there are any unanswered questions by checking the vector size
        if (unansweredQuestions.empty()) {
            // Get current date and time
            getCurrentDateTime();

            bool allSurveysSavedSuccessfully = true; // Flag to track if all surveys are saved successfully

            // Before saving surveys, ensure the directory exists
            std::string directory = responseDirectory + selectedTestProgram + "/" + selectedTestEvent + "/";
            // Ensure the directory exists or create it
            if (!std::filesystem::exists(directory)) {
                if (!std::filesystem::create_directories(directory)) {
                    std::cerr << "Error: Could not create directories for " << directory << std::endl;
                }
            }

            // Iterate through MOPS and save the survey data
            for (const auto& [MOPID, MOPValues] : MOPS.items()) {
                // Add metadata to MOPS
                MOPValues["metadata"] = metadata;

                // Add comments to MOPS
                MOPValues["comment"] = comments[MOPID];

                // Save survey and check if it was successful
                if (saveSurvey(MOPID, MOPValues) == 0) {
                    allSurveysSavedSuccessfully = false; // If any survey fails to save, set flag to false
                }
            }

            // Only call RefreshData if all surveys were saved successfully
            if (allSurveysSavedSuccessfully) {
                // Clear responses
                RefreshData();
            }
            else {
                std::cerr << "Error: One or more surveys could not be saved." << std::endl;
            }
        }

    }

    ImGui::EndChild();

    ImGui::NewLine();
    ImGui::BeginChild("HeadBack", ImVec2(availableWidth, 50), true);

    if (ImGui::Button("Back to Home")) {
        selectedTestProgramIndex = -1;
        selectedTestProgram.clear();
        selectedTestEvent.clear();
        testEvents.clear();
        MOPS.clear();
        //dataLoaded = false;
        refreshData = true;
        currentPage = Page::Home;  // Set the current page to Home
    }
    ImGui::EndChild();
}
