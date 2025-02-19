#include "Application.h"
#include "imgui.h"
#include "implot.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <set>
#include <algorithm>
#include <regex>
#include "json.hpp"

namespace fs = std::filesystem;



static std::string responsesPath = "C:/LSD/AppFiles/Responses";
static std::string responseTypesPath = "C:/LSD/AppFiles/responsetypes.json";
static std::string testProgramsPath = "C:/LSD/AppFiles/TestPrograms";

// Data to show correct survey
static int selectedTestProgramIndex = 0;
static int lastSelectedProgramIndex = 0;

static int lastCOIIndex = 0;
static int lastMOEIndex = 0;
static int lastDSPIndex = 0;

static std::string selectedTestProgram = "";
static std::string selectedTestEvent = "";

static std::vector<std::string> dropdownLabels = { "COI","Measure of Effectiveness", "Design Point" };

struct ResponseTypeA {
    std::string id;
    int count;
    std::vector<std::string> labels;  // List of labels for the response options
};

std::vector<ResponseTypeA> responseTypes;

// Load response types from a JSON file
void GetResponseTypes(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to load file: " << filename << std::endl;
        return;
    }

    nlohmann::json jsonData;
    file >> jsonData;

    responseTypes.clear();  // Clear the global responseTypes vector
    for (const auto& item : jsonData["responseTypes"]) {
        ResponseTypeA rt;
        rt.id = item["id"];  // Correctly using the `id` from the JSON
        rt.count = item["count"];  // Correctly using the `count`
        rt.labels = item["labels"].get<std::vector<std::string>>();  // Correctly using the `labels`
        responseTypes.push_back(rt);
    }
}

// Get all folder names from selected directory
std::vector<std::string> GetSurveyFolders(const std::string& directoryPath) {
    std::vector<std::string> folderNames;
    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (entry.is_directory()) {
                folderNames.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
    return folderNames;
}

// Display Test Program Combo Box (Dropdown) after getting local folder names
int RenderDropdown(const std::vector<std::string>& foldersSet, const std::string& identifier, int currentSelection) {
    ImGui::SameLine();
    std::string dropdownLabel = "## Dropdown for " + identifier;

    // Ensure that dropdownLabel is a C-style string using .c_str()
    if (ImGui::BeginCombo(dropdownLabel.c_str(), currentSelection == 0 ? "Select" : foldersSet[currentSelection].c_str())) {
        for (int i = 0; i < foldersSet.size(); ++i) {
            bool isSelected = (currentSelection == i);
            if (ImGui::Selectable(foldersSet[i].c_str(), isSelected)) {
                if (currentSelection != i) {
                    currentSelection = i;  // Update the selected index
                    // refreshData = true;  // Uncomment if needed
                }
            }
        }
        ImGui::EndCombo();
    }
    return currentSelection;  // Return the updated selection
}

// Function to count the number of files in a folder
int CountFilesInFolder(const std::string& folderPath) {
    int count = 0;
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            ++count;
        }
    }
    return count;
}

// Load data from csv file
bool loadCSV2(const std::string& filename, std::vector<std::vector<std::string>>& data, std::vector<std::string>& comments) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::string cell;
        std::istringstream lineStream(line);

        while (std::getline(lineStream, cell, ',')) {
            row.push_back(cell);
        }

        if (!row.empty()) {
            // Check if this is the comment row (first column = "comments")
            if (row[0] == "comments" && row.size() > 1) {
                comments.push_back(row[1]);  // Store the comment in the comments vector
            }
            else {
                data.push_back(row);  // Otherwise, add the row to survey data
            }
        }
    }

    return true;
}

// Bar plot code
void RenderProgressBars(const std::vector<float>& endResultData, const std::vector<float>& currentProgressData,
    const char* title, const char* endResultName, const char* currentProgressName, const std::vector<float>& plotLims) {

    // Insert 0 at the beginning of both endResultData and currentProgressData
    std::vector<float> modifiedEndResultData = { 0.0f };
    modifiedEndResultData.insert(modifiedEndResultData.end(), endResultData.begin(), endResultData.end());

    std::vector<float> modifiedCurrentProgressData = { 0.0f };
    modifiedCurrentProgressData.insert(modifiedCurrentProgressData.end(), currentProgressData.begin(), currentProgressData.end());

    // Begin the ImPlot context (for plotting)
    ImPlot::BeginPlot(title);

    // Set the x and y axis limits, keeping the minimum as -0.01f
    ImPlot::SetupAxesLimits(0.5f, (plotLims[0]) +0.5f, -0.1f, plotLims[1] * 1.3f);
    // ImPlot::SetupAxesLimits(0.0f, 10.0f, -0.1f, 10.0f);

    // Plot the "end result" data as bars, with the modified data
    ImPlot::PlotBars(endResultName, modifiedEndResultData.data(), static_cast<int>(modifiedEndResultData.size()));

    // Plot the "current progress" data as bars, with the modified data
    ImPlot::PlotBars(currentProgressName, modifiedCurrentProgressData.data(), static_cast<int>(modifiedCurrentProgressData.size()));

    // End the plot and window
    ImPlot::EndPlot();
}

// Helper function to split a string by a delimiter
std::vector<std::string> SplitStringByDelimiter(const std::string& str, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(str);
    std::string part;

    while (std::getline(ss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}





// Main code

void MyApp::RenderAnalyzeSurveysPage() {

    // initialize variable to ask for a full page refresh
    static bool pageRefresh = false;
    ImGui::GetStyle().FrameRounding = 5.0f;

    // Ensure ImPlot context is available before proceeding
    ImPlot::CreateContext();

    // Show Title
    ImGui::Separator();
    ImGui::Text("Survey Analysis");
    ImGui::NewLine();

    // Test Programs variable for dropdown values, from Test Programs folder
    static std::vector<std::string> testPrograms;

    // If first time loading, load Test Program options
    if (testPrograms.empty()) {
        testPrograms = GetSurveyFolders(testProgramsPath);
        testPrograms.insert(testPrograms.begin(), "No Selection");
    }

    // Render the Test Program combo box for selection
    ImGui::Text("Select Test Program");
    ImGui::SetNextItemWidth(200.0f);

    // Get integer value for selected test program index
    selectedTestProgramIndex = RenderDropdown(testPrograms, "Test Program", selectedTestProgramIndex);
    
    ImGui::NewLine();

    // Display "Back to Home" button
    if (ImGui::Button("Back to Home")) {
        currentPage = Page::Home;  // Switch to Home page
    }

    ImGui::NewLine();
    ImGui::Separator();

    // Main Section

    // If a test program is selected, change selectedTestProgram to match (for path retrieval)
    if (selectedTestProgramIndex > 0) { selectedTestProgram = testPrograms[selectedTestProgramIndex]; }

    // After selection, fill test events, dataMap, responded events
    static std::vector<std::string> allTestEvents;
    static std::map<std::string, std::map<std::string, std::vector<std::string>>> dataMap;
    static std::vector<std::string> respondedEvents;

    // Set up aggregate COI variables for plot
    static std::map<int, int> COISurveyCounts;
    static std::map<int, int> COIResponseCounts;

    // Values for counts after filtering
    static std::vector<float> filteredSurveyCounts;
    static std::vector<float> filteredResponseCounts;
    static std::vector<float> filteredSurveysRemaining;

    
    // Set test program path using Dropdown value
    std::string testProgramPath = testProgramsPath + "/" + selectedTestProgram;
    std::string testResponsesPath = responsesPath + "/" + selectedTestProgram;

    // If test events have not been loaded and a test program is selected, load test events from TestProgram/Program path
    if ((allTestEvents.empty() or pageRefresh or respondedEvents.empty()) && selectedTestProgramIndex > 0) {
        allTestEvents = GetSurveyFolders(testProgramPath);        

        // Clear dataMap and COISurveyCounts
        dataMap.clear();
        COISurveyCounts.clear();

        // for each test event, tokenize it based on underscores and add to dataMap
        for (const auto& event : allTestEvents) {

            std::vector<std::string> parts = SplitStringByDelimiter(event, '_');

            // Ensure we have at least 4 parts (A, B, C, D, N)
            if (parts.size() >= 5) {
                try {
                    // Store parts B, C, D (i.e., parts[1], parts[2], parts[3]) as strings
                    std::string coiValue = parts[1];
                    std::string mopsValue = parts[2];
                    std::string dspsValue = parts[3];
                    std::string minCount = parts[4];

                    // Use the nested map structure to store COIS, MOES, DSPS
                    dataMap[coiValue][mopsValue].push_back(dspsValue);
                }

                catch (const std::invalid_argument&) {
                    std::cerr << "Invalid number format in event: " << event << std::endl;
                }
                catch (const std::out_of_range&) {
                    std::cerr << "Number out of range in event: " << event << std::endl;
                }

                // Extract the number after the last underscore
                size_t pos = event.rfind('_');
                if (pos != std::string::npos) {
                    std::string numberStr = event.substr(pos + 1);  // Get the part after the last underscore

                    // Count the number of files in the event's folder
                    std::string eventFolderPath = testProgramsPath + "/" + selectedTestProgram + "/" + event;
                    int fileCount = CountFilesInFolder(eventFolderPath);

                    // Multiply the number by the file count
                    try {
                        int number = std::stoi(numberStr);  // Convert to integer
                        int result = number * fileCount;

                        // Get the second character of the event value (for COISurveyCounts key)
                        if (event.length() > 1) {
                            char coiIndexChar = event[1];
                            int coiIndex = coiIndexChar - '1';

                            if (COISurveyCounts.contains(coiIndex)) {
                                COISurveyCounts[coiIndex] += result;
                            }
                            else {
                                COISurveyCounts[coiIndex] = result;
                            }
                        }
                    }
                    catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid number format after last underscore in event: " << event << std::endl;
                    }
                    catch (const std::out_of_range& e) {
                        std::cerr << "Number out of range after last underscore in event: " << event << std::endl;
                    }
                }
            }
        }

        respondedEvents = GetSurveyFolders(testResponsesPath);

        COIResponseCounts.clear();

        for (const auto& event : respondedEvents) {

            // Count the number of files in the event's folder
            std::string eventFolderPath = testResponsesPath + "/" + event;
            int fileCount = CountFilesInFolder(eventFolderPath);

            // Get the second character of the event value
            if (event.length() > 1) {
                char coiIndexChar = event[1];
                int coiIndex = coiIndexChar - '1';

                if (COIResponseCounts.contains(coiIndex)) {
                    COIResponseCounts[coiIndex] += fileCount;
                }
                else { COIResponseCounts[coiIndex] = fileCount; }
            }
        }
    }

    // If both are filled, do math for percent completion by COI and totals
    if (!allTestEvents.empty() && !respondedEvents.empty()) {

        static int totalSurveys;
        static int totalResponses;

        // Calculate the completion percentages for each COI
        std::vector<float> coiCompletionPercentages;
        float totalCompletionPercentage = 0.0f;

        // Calculate individual COI completion percentages and total completion
        for (int i = 0; i < COISurveyCounts.size(); ++i) {
            // Avoid division by zero and calculate completion percentage
            if (COIResponseCounts.contains(i) && COIResponseCounts[i] > 0) {
                float completionPercentage = static_cast<float>(COIResponseCounts[i]) / static_cast<float>(COISurveyCounts[i]) * 100.0f;
                coiCompletionPercentages.push_back(completionPercentage);
                totalResponses += COIResponseCounts[i];
            }
            else {
                coiCompletionPercentages.push_back(0.0f);
            }
            totalSurveys += COISurveyCounts[i];
        }

        // Calculate the total completion percentage for all COIs
        float totalCompletion = (totalResponses / totalSurveys) * 100.0f;
    }

    // Get response types key, value data
    if (responseTypes.empty()) GetResponseTypes(responseTypesPath);
    
    pageRefresh = false;

    // Filtering of data for plots

    // Dropdown variables
    std::vector<std::string> COIS;
    std::vector<std::string> MOES;
    std::vector<std::string> DSPS;

    // Dropdown status monitor
    static int selectedCOI = 0;
    static int selectedMOE = 0;
    static int selectedDSP = 0;

    ImGui::Columns(2, NULL, true);

    ImGui::Text("Survey Completion Filters");
    ImGui::NewLine();

    // Reset data if new test program selected
    if (lastSelectedProgramIndex != selectedTestProgramIndex) {
        selectedCOI = 0; selectedMOE = 0; selectedDSP = 0;
        lastSelectedProgramIndex = selectedTestProgramIndex;

        COIS.clear();
        MOES.clear();
        DSPS.clear();

        allTestEvents.clear();
        respondedEvents.clear();

        filteredSurveyCounts.clear();
        filteredResponseCounts.clear();


        pageRefresh = true;

        std::cout << "Clearing stuff" << std::endl;
    }

    // First check COI. If something selected, render next, etc. Otherwise set all filters to 0
    if (selectedTestProgramIndex > 0) {
        lastSelectedProgramIndex = selectedTestProgramIndex;
        ImGui::Text("COI"); ImGui::NewLine();

        COIS.push_back("No Selection");
        for (const auto& pair : dataMap) COIS.push_back(pair.first);
        ImGui::SetNextItemWidth(200.0f);
        selectedCOI = RenderDropdown(COIS, "COIS", selectedCOI);
        if (selectedCOI != lastCOIIndex) selectedMOE = 0; selectedDSP = 0;

    }
    
    // If COI is selected show MOES dropdown, otherwise lower filters set to 0
    if (selectedCOI > 0) {
        ImGui::Text("Measure of Effectiveness"); ImGui::NewLine();
        MOES.push_back("No Selection");
        for (const auto& pair : dataMap[std::to_string(selectedCOI)]) MOES.push_back(pair.first);
        ImGui::SetNextItemWidth(200.0f);
        selectedMOE = RenderDropdown(MOES, "MOES", selectedMOE);
        if (selectedMOE != lastMOEIndex) selectedDSP = 0;
    }
    else selectedMOE = 0;

    // If MOES is selected show DSP dropdown, otherwise set DSP filter to 0
    if (selectedMOE > 0) {
        ImGui::Text("Design Point"); ImGui::NewLine();
        DSPS = dataMap[std::to_string(selectedCOI)][std::to_string(selectedMOE)];
        DSPS.insert(DSPS.begin(), "No Selection");
        ImGui::SetNextItemWidth(200.0f);
        selectedDSP = RenderDropdown(DSPS, "DSPS", selectedDSP);
    }
    else selectedDSP = 0;

    // If any dropdown value has changed, reset plot data and set "last selected" values
    if ((selectedCOI != lastCOIIndex) or (selectedMOE != lastMOEIndex) or (selectedDSP != lastDSPIndex)) {
        filteredSurveyCounts.clear();
        filteredResponseCounts.clear();
        lastCOIIndex = selectedCOI;
        lastMOEIndex = selectedMOE;
        lastDSPIndex = selectedDSP;
    }

    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::NewLine();

    ImGui::BeginChild("Survey Statistics");
    // If test events is filled and Test Program is selected, run plot logic
    if (!allTestEvents.empty() && selectedTestProgramIndex != 0) {

        // if no COI is selected - get all data by COI and plot
        if (selectedCOI == 0) {
            if (filteredSurveyCounts.empty()) {
                // Fill filteredSurveyCounts with data by COI
                const int dataMapSize = static_cast<int>(dataMap.size());
                for (const auto& COIpair : dataMap) {
                    auto COI = COIpair.first;

                    // Fill filteredSurveyCounts
                    for (const auto& event : allTestEvents) {
                        // Split the event string by underscores
                        std::vector<std::string> parts = SplitStringByDelimiter(event, '_');

                        // If event is a part of COI add last part of folder name to counter for COI index
                        if (parts.size() > 1 && parts[1] == COI) {
                            int index = (std::stoi(parts[1]) - 1); // Get the index from the last part of the string
                            float valueToAdd = std::stof(parts.back()); // Convert the last part to integer (for the value)

                            // Ensure filteredSurveyCounts has enough space
                            if (index >= filteredSurveyCounts.size()) {
                                filteredSurveyCounts.resize(index + 1, 0); // Resize and initialize new elements to 0
                            }

                            filteredSurveyCounts[index] += valueToAdd;  // Add the value to the appropriate index
                        }
                    }

                    // Fill filteredResponseCounts
                    for (const auto& event : respondedEvents) {
                        // Split the event string by underscores
                        std::vector<std::string> parts = SplitStringByDelimiter(event, '_');

                        // If response is a part of COI, add number of responses
                        if (parts.size() > 1 && parts[1] == COI) {
                            int index = (std::stoi(parts[1]) - 1);  // Get the index from the second part of the string (assuming index is at parts[1])

                            // Value to add = # files in event folder / highest MOP value in the file titles
                            // testResponsesPath + event is the folder containing JSON files with titles like MPX_ABC_...json

                            // Iterate through files in the directory and get unique MOP values
                            std::set<float> mopSet;

                            // Fill mopSet with unique mop values from the specific test event folder
                            for (const auto& entry : fs::directory_iterator(testResponsesPath)) {
                                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                    // Extract the MOP value from the filename using a regex
                                    std::string filename = entry.path().filename().string();
                                    std::regex mopRegex("MOP(\\d+)");  // Regex to find MOPX where X is a number

                                    std::smatch match;
                                    if (std::regex_search(filename, match, mopRegex) && match.size() > 1) {
                                        // Convert the extracted MOP value to an integer
                                        int mopValue = std::stof(match.str(1));  // match.str(1) is the number part after "MOP"
                                        mopSet.insert(mopValue);
                                    }
                                }
                            }

                            //Get set size
                            float mopSetSize = std::max(static_cast<float>(mopSet.size()), 1.0f);

                            // Now calculate the value to add: # files / highest MOP value
                            int numFiles = std::distance(fs::directory_iterator(testResponsesPath), fs::directory_iterator{});
                            float valueToAdd = static_cast<float>(numFiles) / mopSetSize;

                            // Ensure filteredResponseCounts has enough space
                            if (index >= filteredResponseCounts.size()) {
                                filteredResponseCounts.resize(index + 1, 0);  // Resize and initialize new elements to 0
                            }

                            filteredResponseCounts[index] += valueToAdd;  // Add the value to the appropriate index
                        }
                    }
                }

                // Subtract responses from total to get remaining -> filteredSurveysRemaining
                size_t minLength = std::min(filteredSurveyCounts.size(), filteredResponseCounts.size());
                filteredSurveysRemaining = filteredSurveyCounts;

                for (size_t i = 0; i < minLength; ++i) {
                    filteredSurveysRemaining[i] -= filteredResponseCounts[i];
                }
            }

            if (!filteredSurveyCounts.empty()) {
                // Get y lim
                float max_survey_value = *std::max_element(filteredSurveyCounts.begin(), filteredSurveyCounts.end());

                // Set plotLims with max values
                std::vector<float> plotLims = { static_cast<float>(filteredSurveyCounts.size()), max_survey_value };

                /*
                std::cout << "Full dataset in filteredSurveyCounts: " << std::endl;
                for (size_t i = 0; i < filteredSurveyCounts.size(); ++i) {
                    std::cout << "Index " << i << ": " << filteredSurveyCounts[i] << std::endl;
                }
                std::cout << "Full dataset in filteredResponseCounts: " << std::endl;
                for (size_t i = 0; i < filteredResponseCounts.size(); ++i) {
                    std::cout << "Index " << i << ": " << filteredResponseCounts[i] << std::endl;
                }
                */

                // Changed back to filtered survey counts
                RenderProgressBars(filteredSurveyCounts, filteredResponseCounts, "Survey Completion by \n Critical Operational Issue",
                    "Total Surveys to Complete", "Surveys Administered", plotLims);
            }

        }

        // If no MOE selected - get all data by MOE and plot
        if (selectedCOI != 0 && selectedMOE == 0) {

            if (filteredSurveyCounts.empty()) {

                // Fill filteredSurveyCounts with data by MOE
                std::map<std::string, std::vector<std::string>> COIMAP = dataMap[std::to_string(selectedCOI)];
                const int COIMapSize = static_cast<int>(COIMAP.size());
                for (const auto& MOEpair : COIMAP) {
                    auto MOE = MOEpair.first;

                    // Add survey counts to the survey counter vector
                    for (const auto& event : allTestEvents) {
                        // Split the event string by underscores
                        std::vector<std::string> parts = SplitStringByDelimiter(event, '_');

                        // If event is a part of selected COI and current MOE (of loop) add last part of folder name to counter for COI index
                        if (parts.size() > 2 && parts[1] == std::to_string(selectedCOI) && parts[2] == MOE) {
                            int index = (std::stoi(parts[2]) - 1); // Get the MOE index
                            float valueToAdd = std::stoi(parts.back()); // Convert the last part to integer (for the value)

                            // Ensure filteredSurveyCounts has enough space
                            if (index >= filteredSurveyCounts.size()) {
                                filteredSurveyCounts.resize(index + 1, 0); // Resize and initialize new elements to 0
                            }

                            filteredSurveyCounts[index] += valueToAdd;  // Add the value to the appropriate index                               
                        }
                    }

                    // Add response counts to the responses counter vector
                    for (const auto& event : respondedEvents) {
                        // Split the event string by underscores
                        std::vector<std::string> parts = SplitStringByDelimiter(event, '_');

                        // If response is a part of MOE, add number of responses
                        if (parts.size() > 2 && parts[1] == std::to_string(selectedCOI) && parts[2] == MOE) {
                            // MOE index from second part
                            int index = (std::stoi(parts[2]) - 1);

                            // Value to add = # files in event folder / highest MOP value in the file titles
                            // testResponsesPath + event is the folder containing JSON files with titles like MOPX_ABC_...json

                            // Iterate through files in the directory and get unique MOP values
                            std::set<float> mopSet;

                            // Fill mopSet with unique mop values from the specific test event folder
                            for (const auto& entry : fs::directory_iterator(testResponsesPath)) {
                                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                    // Extract the MOP value from the filename using a regex
                                    std::string filename = entry.path().filename().string();
                                    std::regex mopRegex("MOP(\\d+)");  // Regex to find MOPX where X is a number

                                    std::smatch match;
                                    if (std::regex_search(filename, match, mopRegex) && match.size() > 1) {
                                        // Convert the extracted MOP value to an integer
                                        float mopValue = std::stof(match.str(1));  // match.str(1) is the number part after "MOP"
                                        mopSet.insert(mopValue);
                                    }
                                }
                            }

                            //Get set size
                            float mopSetSize = std::max(static_cast<float>(mopSet.size()), 1.0f);

                            // Now calculate the value to add: # files / highest MOP value
                            __int64 numFiles = std::distance(fs::directory_iterator(testResponsesPath), fs::directory_iterator{});
                            float valueToAdd = static_cast<float>(numFiles) / mopSetSize;

                            // Ensure filteredResponseCounts has enough space
                            if (index >= filteredResponseCounts.size()) {
                                filteredResponseCounts.resize(index + 1, 0);  // Resize and initialize new elements to 0
                            }

                            filteredResponseCounts[index] += valueToAdd;  // Add the value to the appropriate index
                        }
                    }
                }

                // Subtract responses from total to get remaining -> filteredSurveysRemaining
                size_t minLength = std::min(filteredSurveyCounts.size(), filteredResponseCounts.size());
                filteredSurveysRemaining = filteredSurveyCounts;
                for (size_t i = 0; i < minLength; ++i) {
                    filteredSurveysRemaining[i] -= filteredResponseCounts[i];
                }
            }

            if (!filteredSurveyCounts.empty()) {
                // Get y lim
                float max_survey_value = *std::max_element(filteredSurveyCounts.begin(), filteredSurveyCounts.end());

                // Set plotLims with max values
                std::vector<float> plotLims = { static_cast<float>(filteredSurveyCounts.size()), max_survey_value };

                /*
                // Print the full dataset of filteredResponseCounts
                std::cout << "Full dataset in filteredSurveyCounts: " << std::endl;
                for (size_t i = 0; i < filteredSurveyCounts.size(); ++i) {
                    std::cout << "Index " << i << ": " << filteredSurveyCounts[i] << std::endl;
                }

                // Print the full dataset of filteredResponseCounts
                std::cout << "Full dataset in filteredResponseCounts: " << std::endl;
                for (size_t i = 0; i < filteredResponseCounts.size(); ++i) {
                    std::cout << "Index " << i << ": " << filteredResponseCounts[i] << std::endl;
                }
                */


                RenderProgressBars(filteredSurveyCounts, filteredResponseCounts, "Selected COI Completion by \n Measure of Effectiveness",
                    "Total Surveys to Complete", "Surveys Administered", plotLims);
            }
        }

        // If no DSP is selected - get all data by Design point and plot response counts with labels from response types
        if (selectedMOE != 0 && selectedDSP == 0) {

            if (filteredSurveyCounts.empty()) {

                // Fill filteredSurveyCounts with data by MOP
                std::vector<std::string> MOPMap = dataMap[std::to_string(selectedCOI)][std::to_string(selectedMOE)];
                const int MOPMapSize = MOPMap.size();
                for (const auto& DSP : MOPMap) {

                    // Add survey counts to the survey counter vector
                    for (const auto& event : allTestEvents) {
                        // Split the event string by underscores
                        std::vector<std::string> parts = SplitStringByDelimiter(event, '_');

                        // If event is a part of selected COI and selected MOP and DSP (of loop) add last part of folder name to counter for DSP index
                        if (parts.size() > 3 && parts[1] == std::to_string(selectedCOI) && parts[2] == std::to_string(selectedMOE) && parts[3] == DSP) {
                            int index = (std::stoi(parts[3]) - 1); // Get the DSP index
                            float valueToAdd = std::stoi(parts.back()); // Convert the last part to integer (for the value)

                            // Ensure filteredSurveyCounts has enough space
                            if (index >= filteredSurveyCounts.size()) {
                                filteredSurveyCounts.resize(index + 1, 0); // Resize and initialize new elements to 0
                            }

                            filteredSurveyCounts[index] += valueToAdd;  // Add the value to the appropriate index                               
                        }
                    }

                    // Add response counts to the responses counter vector
                    for (const auto& event : respondedEvents) {
                        // Split the event string by underscores
                        std::vector<std::string> parts = SplitStringByDelimiter(event, '_');

                        // If response is a part of MOP, add number of responses
                        if (parts.size() > 3 && parts[1] == std::to_string(selectedCOI) && parts[2] == std::to_string(selectedMOE) && parts[3] == DSP) {
                            // DSP index from third part
                            int index = (std::stoi(parts[3]) - 1);

                            // Value to add = # files in event folder / highest MOP value in the file titles
                            // testResponsesPath + event is the folder containing JSON files with titles like MOPX_ABC_...json

                            // Iterate through files in the directory and get unique MOP values
                            std::set<float> mopSet;

                            // Fill mopSet with unique mop values from the specific test event folder
                            for (const auto& entry : fs::directory_iterator(testResponsesPath)) {
                                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                    // Extract the MOP value from the filename using a regex
                                    std::string filename = entry.path().filename().string();
                                    std::regex mopRegex("MOP(\\d+)");  // Regex to find MOPX where X is a number

                                    std::smatch match;
                                    if (std::regex_search(filename, match, mopRegex) && match.size() > 1) {
                                        // Convert the extracted MOP value to an integer
                                        int mopValue = std::stoi(match.str(1));  // match.str(1) is the number part after "MOP"
                                        mopSet.insert(mopValue);
                                    }
                                }
                            }

                            //Get set size
                            float mopSetSize = std::max(static_cast<float>(mopSet.size()), 1.0f);

                            // Now calculate the value to add: # files / highest MOP value
                            int numFiles = std::distance(fs::directory_iterator(testResponsesPath), fs::directory_iterator{});
                            float valueToAdd = static_cast<float>(numFiles) / mopSetSize;

                            // Ensure filteredResponseCounts has enough space
                            if (index >= filteredResponseCounts.size()) {
                                filteredResponseCounts.resize(index + 1, 0);  // Resize and initialize new elements to 0
                            }

                            filteredResponseCounts[index] += valueToAdd;  // Add the value to the appropriate index
                        }
                    }
                }

                // Subtract responses from total to get remaining -> filteredSurveysRemaining
                size_t minLength = std::min(filteredSurveyCounts.size(), filteredResponseCounts.size());
                filteredSurveysRemaining = filteredSurveyCounts;
                for (size_t i = 0; i < minLength; ++i) {
                    filteredSurveysRemaining[i] -= filteredResponseCounts[i];
                }
            }

            if (!filteredSurveyCounts.empty()) {
                // Get y lim
                float max_survey_value = *std::max_element(filteredSurveyCounts.begin(), filteredSurveyCounts.end());

                // Set plotLims with max values
                std::vector<float> plotLims = { static_cast<float>(filteredSurveyCounts.size()), max_survey_value };

                /*
                // Print the full dataset of filteredResponseCounts
                std::cout << "Full dataset in filteredSurveyCounts: " << std::endl;
                for (size_t i = 0; i < filteredSurveyCounts.size(); ++i) {
                    std::cout << "Index " << i << ": " << filteredSurveyCounts[i] << std::endl;
                }

                // Print the full dataset of filteredResponseCounts
                std::cout << "Full dataset in filteredResponseCounts: " << std::endl;
                for (size_t i = 0; i < filteredResponseCounts.size(); ++i) {
                    std::cout << "Index " << i << ": " << filteredResponseCounts[i] << std::endl;
                }
                */

                RenderProgressBars(filteredSurveyCounts, filteredResponseCounts, "Selected Measure of Effectiveness \n Completion by Design Point",
                    "Total Surveys to Complete", "Surveys Administered", plotLims);
            }
        }

        // If DSP is selected - get design point data and plot responses only
        if (selectedDSP != 0) {

            if (filteredSurveyCounts.empty()) {
                // Fill filteredSurveyCounts with data by DSP
                std::vector<std::string> MOEMap = dataMap[std::to_string(selectedCOI)][std::to_string(selectedMOE)];
                const int MOEMapSize = MOEMap.size();
                for (const auto& DSP : MOEMap) {

                    // Add response counts to the responses counter vector
                    for (const auto& event : respondedEvents) {
                        // Split the event string by underscores
                        std::vector<std::string> parts = SplitStringByDelimiter(event, '_');

                        // If response is a part of MOE, add number of responses
                        if (parts.size() > 3 && parts[1] == std::to_string(selectedCOI) && parts[2] == std::to_string(selectedMOE) && parts[3] == DSP) {
                            // MOE index from second part
                            int index = (std::stoi(parts[3]) - 1);

                            // Value to add = # files in event folder / highest MOP value in the file titles
                            // testResponsesPath + event is the folder containing JSON files with titles like MOPX_ABC_...json

                            // Iterate through files in the directory and extract the highest MOP value
                            float highestMOP = 0.0f;
                            for (const auto& entry : fs::directory_iterator(testResponsesPath)) {
                                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                    // Extract the MOP value from the filename using a regex
                                    std::string filename = entry.path().filename().string();
                                    std::regex mopRegex("MOP(\\d+)");  // Regex to find MOPX where X is a number

                                    std::smatch match;
                                    if (std::regex_search(filename, match, mopRegex) && match.size() > 1) {
                                        // Convert the extracted MOP value to an integer
                                        int mopValue = std::stoi(match.str(1));  // match.str(1) is the number part after "MOP"
                                        highestMOP = std::max(highestMOP, static_cast<float>(mopValue));  // Keep the highest MOP value
                                    }
                                }
                            }

                            // If no MOP value is found, handle it appropriately (maybe set to 1 or print a warning)
                            if (highestMOP == 0.0f) {
                                highestMOP = 1.0f;  // You can adjust this based on your needs
                            }

                            // Now calculate the value to add: # files / highest MOP value
                            int numFiles = std::distance(fs::directory_iterator(testResponsesPath), fs::directory_iterator{});
                            float valueToAdd = static_cast<float>(numFiles) / highestMOP;

                            // Ensure filteredResponseCounts has enough space
                            if (index >= filteredResponseCounts.size()) {
                                filteredResponseCounts.resize(index + 1, 0);  // Resize and initialize new elements to 0
                            }

                            filteredResponseCounts[index] += valueToAdd;  // Add the value to the appropriate index
                        }
                    }
                }

                // Subtract responses from total to get remaining -> filteredSurveysRemaining
                size_t minLength = std::min(filteredSurveyCounts.size(), filteredResponseCounts.size());
                filteredSurveysRemaining = filteredSurveyCounts;
                for (size_t i = 0; i < minLength; ++i) {
                    filteredSurveysRemaining[i] -= filteredResponseCounts[i];
                }
            }

            if (!filteredSurveyCounts.empty()) {
                // NOTHING HERE... CHANGE TO...?
            }
        }

    }
    ImGui::EndChild();



    
    ImGui::NextColumn();
    // Now all the metadata filters


    static nlohmann::json metadata;
    ImGui::Text("Individual Response Filters");
    // Set up vars
    static bool displayResponses = false;
    
    ImGui::NewLine();
    // Show all metadata stuff to filter it
    // Get metadata options from test program folder
    std::string metadataPath = testProgramPath + + "/" + "metadataquestions.json";

    // Add metadata questions loading
    static nlohmann::json metadataFilters;
    nlohmann::json metadataJson;
    std::ifstream metadataFile(metadataPath);

    if (metadataFile.is_open() && metadataFilters.empty()) {
        metadataFile >> metadataJson;

        // Check if the "metadata" field exists in the JSON
        if (metadataJson.contains("aircrew")) {

            // Iterate over the keys of the "metadata" object and store them in the metadata object
            for (const auto& item : metadataJson["aircrew"].items()) {
                metadataFilters[item.key()] = item.value();
            }            
        }
    }

    // Display metadata filters
    for (auto& [metaID, metaValues] : metadataFilters.items()) {  // We now use the global 'metadata'

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

                // Add "No selection" as the first option in the dropdown list
                options.insert(options.begin(), "No selection");

                // Ensure the response exists and is valid for the selection
                if (metaValues.contains("response")) {
                    currentSelection = metaValues["response"].get<int>();
                    if (currentSelection == -1) {
                        // If the current selection is -1, we treat it as "No selection"
                        currentSelection = 0;  // This corresponds to the "No selection" option
                    }
                }

                // Create the dropdown (combo) for the current array
                if (ImGui::BeginCombo(label.c_str(), options[currentSelection].c_str())) {  // Use the current selection for label

                    // Loop through the options
                    for (int i = 0; i < options.size(); ++i) {
                        bool isSelected = (currentSelection == i);

                        // Render the selectable item in the combo box
                        if (ImGui::Selectable(options[i].c_str(), isSelected)) {
                            // If "No selection" is chosen (i == 0), set response to -1
                            if (i == 0) {
                                metaValues["response"] = -1;  // No selection
                            }
                            else {
                                // For other options, map the index correctly: subtract 1 to account for "No selection"
                                metaValues["response"] = i;
                            }
                        }

                        // If the item is selected, set it as the default focus
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            // If input type is a string and show filled text box, if applicable
            if (inputTypeValue == "text") {
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

            // If input type is "time" show two input boxes, one for min and one for max
            if (inputTypeValue == "time") {
                std::string currentValue = metaValues["response"];

                // Split the currentValue into min and max if they exist, otherwise use empty strings
                std::string minValue, maxValue;
                size_t commaPos = currentValue.find(",");
                if (commaPos != std::string::npos) {
                    minValue = currentValue.substr(0, commaPos);
                    maxValue = currentValue.substr(commaPos + 1);
                }
                else {
                    minValue = currentValue;  // If no comma, the whole value is for "Min"
                }

                // Create buffers for both Min and Max input
                char minBuf[256];
                char maxBuf[256];

                // Use strncpy_s for safer string copy
                strncpy_s(minBuf, sizeof(minBuf), minValue.c_str(), _TRUNCATE);  // _TRUNCATE ensures the string fits in the buffer
                strncpy_s(maxBuf, sizeof(maxBuf), maxValue.c_str(), _TRUNCATE);  // _TRUNCATE ensures the string fits in the buffer

                // Create Min and Max labels with the original label (metaID)
                std::string minLabel = "Min##" + metaID;  // "Min##(original label)"
                std::string maxLabel = "Max##" + metaID;  // "Max##(original label)"

                // Set the width for the input boxes
                float inputWidth = 100.0f;  // Change this value to adjust the width of the input boxes

                // Set the width of the input boxes
                ImGui::PushItemWidth(inputWidth);

                // Display the Min input box
                if (ImGui::InputText(minLabel.c_str(), minBuf, sizeof(minBuf))) {
                    // When the user types something in the Min box, update the metadata map
                    minValue = std::string(minBuf);  // Update Min value
                }
                ImGui::SameLine();
                ImGui::Text("   ");
                ImGui::SameLine();
                // Display the Max input box
                if (ImGui::InputText(maxLabel.c_str(), maxBuf, sizeof(maxBuf))) {
                    // When the user types something in the Max box, update the metadata map
                    maxValue = std::string(maxBuf);  // Update Max value
                }

                // Reset the width back to the default
                ImGui::PopItemWidth();

                // Combine the Min and Max values into one string, separated by a comma
                metaValues["response"] = minValue + "," + maxValue;  // Store the result in the response field
            }
        }
    }    

    ImGui::NewLine();
    ImGui::NewLine();

    static bool displayError = false;

    if (selectedTestProgramIndex > 0) {
        if (ImGui::Button("Display All")) {
            // Check if COI and MOE are selected properly
            if (selectedCOI > 0 && selectedMOE > 0) displayResponses = true;            
            else displayError = true;
        }
    }

    if (displayError) {
        ImGui::NewLine();
        ImGui::Text("Looks like you haven't set all of the COI/MOE filters. \nAre you sure you want to create a bunch of plots?");

        // Buttons for handling user decision
        if (ImGui::Button("Yes, I'm sure")) { displayResponses = true; displayError = false; }
        ImGui::SameLine();
        if (ImGui::Button("Nope, sounds like a bad idea")) { displayResponses = false; displayError = false; }
    }


    ImGui::Columns(1);

    // string to store response filename, bool to store whether the response meets the metadata requirements
    static std::vector<std::pair<std::string, bool>> surveyResponses;

    // If empty, fill it
    if (surveyResponses.empty()) {
        // Iterate through all of the folders in responsesPath / selectedTestProgram
        fs::path programFolder = fs::path(responsesPath) / selectedTestProgram;

        // Make sure the directory exists
        if (fs::exists(programFolder) && fs::is_directory(programFolder)) {
            // Iterate through all subdirectories (e.g., V_2_2_02_4)
            for (const auto& dirEntry : fs::directory_iterator(programFolder)) {
                if (fs::is_directory(dirEntry)) {
                    fs::path subFolder = dirEntry.path();

                    // Iterate through all of the files in the subfolder (e.g., .json files)
                    for (const auto& fileEntry : fs::directory_iterator(subFolder)) {
                        if (fs::is_regular_file(fileEntry) && fileEntry.path().extension() == ".json") {
                            // Get the relative folder/filename string
                            std::string relativePath = subFolder.filename().string() + "/" + fileEntry.path().filename().string();

                            // Check metadata or any condition, here I'm using a placeholder boolean check
                            bool meetsMetadata = false;  // Add your metadata-check logic here

                            // Add the relative folder/filename string to the surveyResponses vector with a bool value (either true or false)
                            surveyResponses.push_back(std::make_pair(relativePath, meetsMetadata));

                        }
                    }
                }
            }
        }
        else {
            std::cerr << "Directory does not exist: " << programFolder << std::endl;
        }
    }

    // If displayResponses == true then plot all of the responses filtered by COI, MOE, all metadata filters, grouped by Design Point
    

    // first get the file paths of all responses

    // for each COI, MOE, design point combination (hopefully coi and moe will be filtered)

        // for each file path, check if conditions are met

        // for each where conditions are met, plot each MOPs results in a new plot




    ImGui::NewLine();
    
        
    ImPlot::DestroyContext();

}

















