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



//////////////////////////////////////
// Imports and Variable Definitions
//////////////////////////////////////

namespace fs = std::filesystem;

namespace ImPlot {
    enum ImAxis_ {
        ImAxis_X, // Declared here
        ImAxis_Y,
        ImAxis_Z
    };
}

using std::string;
using std::vector;
using std::unordered_map;
using std::ifstream;
using std::endl;
using std::map;
using std::pair;
using std::cout;


static string responsesPath = "C:/LSD/AppFiles/Responses";
static string testProgramsPath = "C:/LSD/AppFiles/TestPrograms";

// Data to show correct survey
static int selectedTestProgramIndex = 0; 
static int lastSelectedProgramIndex = 0;

static int lastCOIIndex = 0;
static int lastMOEIndex = 0;
static int lastDSPIndex = 0;

static string selectedTestProgram = "";
static string selectedTestEvent = "";

static vector<string> dropdownLabels = { "COI","Measure of Effectiveness", "Design Point" };

struct ResponseTypeA {
    string id;
    int count;
    vector<string> labels;  // List of labels for the response options
};

struct QuestionData {
    vector<string> labels;
    vector<int> responses;  // Responses (numeric values)
    vector<string> comments;
};

vector<ResponseTypeA> responseTypes;


//////////////////////////////////////
// Functions
//////////////////////////////////////

// Get all folder names from selected directory
vector<string> GetSurveyFolders(const string& directoryPath) {
    vector<string> folderNames;
    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (entry.is_directory()) {
                folderNames.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error reading directory: " << e.what() << endl;
    }
    return folderNames;
}

// Display Test Program Combo Box (Dropdown) after getting local folder names
int RenderDropdown(const vector<string>& foldersSet, 
    const string& identifier, 
    int currentSelection) {
    ImGui::SameLine();
    string dropdownLabel = "## Dropdown for " + identifier;

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
int CountFilesInFolder(const string& folderPath) {
    int count = 0;
    
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            ++count;
        }
        else { std::cout << entry << " Not regular file" << std::endl; }
    }
    
    return count;
}

// Load data from csv file
bool loadCSV2(const string& filename, 
    vector<vector<string>>& data, 
    vector<string>& comments) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << filename << std::endl;
        return false;
    }

    string line;
    while (std::getline(file, line)) {
        vector<string> row;
        string cell;
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

// Helper function to split a string by a delimiter
vector<string> SplitStringByDelimiter(const string& str, 
    char delimiter) {
    vector<string> parts;
    std::stringstream ss(str);
    string part;

    while (std::getline(ss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}

// Load response types from a JSON file
void GetResponseTypes(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to load file: " << filename << endl;
        return;
    }

    nlohmann::json jsonData;
    file >> jsonData;

    responseTypes.clear();  // Clear the global responseTypes vector
    for (const auto& item : jsonData["responseTypes"]) {
        ResponseTypeA rt;
        rt.id = item["id"];  // Correctly using the `id` from the JSON
        rt.count = item["count"];  // Correctly using the `count`
        rt.labels = item["labels"].get<vector<string>>();  // Correctly using the `labels`
        responseTypes.push_back(rt);
    }
}

// Function to retrieve the response data for each file
std::map<std::string, QuestionData> ProcessResponseData(const std::string& testResponsePath, 
    const std::vector<std::string>& respondedEvents,
    const std::vector<ResponseTypeA>& responseTypes, 
    int selectedCOI, 
    int selectedMOE) {

    int responseCounter = 0;

    // Map to hold the responses grouped by questions
    std::map<std::string, QuestionData> questionResponses;

    // Iterate through the directories in the base folder
    std::cout << "Processing directory: " << testResponsePath << std::endl;
    for (const auto& entry : fs::directory_iterator(testResponsePath)) {
        if (entry.is_directory()) {
            std::string eventDir = entry.path().filename().string(); // Get the event directory name
            std::cout << "Found event directory: " << eventDir << std::endl;

            // Split the directory name to extract COI and MOE
            std::vector<std::string> parts = SplitStringByDelimiter(eventDir, '_');
            std::cout << "Split event directory: ";
            for (const auto& part : parts) {
                std::cout << part << " ";
            }
            std::cout << std::endl;

            if (parts.size() > 2 && parts[1] == std::to_string(selectedCOI) && parts[2] == std::to_string(selectedMOE)) {
                std::cout << "Matching COI and MOE found. COI: " << selectedCOI << ", MOE: " << selectedMOE << std::endl;

                // Iterate through the files inside this event directory
                for (const auto& fileEntry : fs::directory_iterator(entry.path())) {
                    if (fileEntry.is_regular_file()) {
                        std::string rPath = fileEntry.path().string(); // Get the full file path
                        std::cout << "Processing file: " << rPath << std::endl;

                        // Open the JSON file
                        std::ifstream file(rPath);
                        if (!file.is_open()) {
                            std::cout << "Failed to open file: " << rPath << std::endl;
                            continue;
                        }

                        nlohmann::json jsonData;
                        file >> jsonData;

                        auto questions = jsonData["questions"].get<std::vector<std::string>>();
                        auto responseTypeIDs = jsonData["responseTypes"].get<std::vector<std::string>>(); // response type IDs
                        auto responses = jsonData["responses"].get<std::vector<int>>(); // Numeric responses
                        auto comment = jsonData["comment"];

                        std::cout << "Processing JSON data..." << std::endl;
                        // For each question, responseTypeID, response set
                        for (size_t i = 0; i < questions.size(); ++i) {
                            auto question = questions[i];
                            auto responseTypeID = responseTypeIDs[i];
                            auto response = responses[i];

                            std::cout << "Processing question: " << question << std::endl;
                            std::cout << "ResponseTypeID: " << responseTypeID << ", Response: " << response << std::endl;

                            // If new question, set up the basics
                            if (questionResponses.find(question) == questionResponses.end()) {
                                std::cout << "New question encountered: " << question << std::endl;

                                // Create QuestionData object and fill it
                                int c = 0;
                                std::vector<std::string> l;

                                // Get data from responseTypes
                                for (const auto& rt : responseTypes) {
                                    std::string id = rt.id;

                                    if (id == responseTypeID) {
                                        c = rt.count;
                                        l = rt.labels;
                                        std::cout << "Found matching ResponseTypeID. Count: " << c << ", Labels: ";
                                        for (const auto& label : l) {
                                            std::cout << label << " ";
                                        }
                                        std::cout << std::endl;
                                        break;  // We found the matching responseTypeID, no need to continue looping
                                    }
                                }

                                // Initialize the responses vector with zeros based on the response count
                                std::vector<int> r(c, 0);

                                // Add the new question with an empty response count to the map
                                questionResponses[question] = { l, r};
                                std::cout << "Added new question to map: " << question << std::endl;
                            }

                            // Access the existing response vector and increment the corresponding response
                            auto& r = questionResponses[question].responses;
                            r[response - 1] += 1;
                            cout << "Response is " << response << "... subtracting 1 then adding to index" << endl << endl;
                            std::cout << "Incremented response for question: " << question << " to count: " << r[response - 1] << std::endl;
                            
                            questionResponses[question].comments.push_back(comment);
                        }   
                    }
                }
            }
        }
    }

    /*
    // Print out the final grouped responses for debugging
    std::cout << "Final grouped responses: " << std::endl;
    for (const auto& entry : questionResponses) {
        std::cout << "Question: " << entry.first << endl << "Labels: ";
        for (const auto& label : entry.second.labels) {
            std::cout << label << ", ";
        }
        std::cout << endl << "Responses: ";
        for (const auto& response : entry.second.responses) {
            std::cout << response << " ";
        }
        std::cout << endl << "Comments: ";
        for (const auto& response : entry.second.comments) {
            std::cout << response << "\n ";
        }
        std::cout << std::endl << endl;
    }
    */
    // Return the grouped responses (map of questions to responses)
    return questionResponses;
}

// Bar plot code
void RenderProgressBars(const std::map<std::string, int>& surveyPlotData,
    const std::map<std::string, int>& responsesPlotData,
    const char* title,
    const char* endResultName,
    const char* currentProgressName,
    const std::vector<float>& plotLims) {

    // Extract the keys and values from surveyPlotData and responsesPlotData
    std::vector<std::string> surveyKeys;
    std::vector<int> surveyValues;

    for (const auto& entry : surveyPlotData) {
        surveyKeys.push_back(entry.first);
        surveyValues.push_back(entry.second);
    }

    std::vector<std::string> responsesKeys;
    std::vector<int> responsesValues;

    for (const auto& entry : responsesPlotData) {
        responsesKeys.push_back(entry.first);
        responsesValues.push_back(entry.second);
    }    

    // Insert 0 at the beginning of both endResultData and currentProgressData (assuming these are defined elsewhere)
    std::vector<float> modifiedEndResultData = { 0.0f };
    modifiedEndResultData.insert(modifiedEndResultData.end(), surveyValues.begin(), surveyValues.end());

    std::vector<float> modifiedCurrentProgressData = { 0.0f };
    modifiedCurrentProgressData.insert(modifiedCurrentProgressData.end(), responsesValues.begin(), responsesValues.end());

    // Begin the ImPlot context (for plotting)
    ImPlot::BeginPlot(title);

    // Set the x and y axis limits, keeping the minimum as -0.01f
    ImPlot::SetupAxesLimits(0.5f, plotLims[0] + 0.5f, -0.1f, plotLims[1] * 1.3f);

    // Plot the "end result" data as bars, with the modified data
    ImPlot::PlotBars(endResultName, modifiedEndResultData.data(), static_cast<int>(modifiedEndResultData.size()));

    // Plot the "current progress" data as bars, with the modified data
    ImPlot::PlotBars(currentProgressName, modifiedCurrentProgressData.data(), static_cast<int>(modifiedCurrentProgressData.size()));

    // End the plot and window
    ImPlot::EndPlot();
}

// Response plots code
void RenderResponsePlots(const std::map<std::string, 
    QuestionData>& plotData) {
    static bool popupOpen = false;  // Control whether popup is open
    static int currentPlotIndex = 0; // For navigating between plots

    // Trigger popup opening based on a button press (or some other event)
    if (ImGui::Button("Open Plot Window")) {
        popupOpen = true;
    }

    // Ensure the popup window is open
    if (popupOpen) {
        ImGui::OpenPopup("Response Plot");
    }

    // Ensure the popup window is open and plotData is not empty
    if (ImGui::BeginPopupModal("Response Plot", NULL)) {
        // Convert the plot data into appropriate vectors
        std::vector<std::string> questions;
        std::vector<std::vector<int>> responses;
        std::vector<std::vector<std::string>> allLabels;
        std::vector<std::vector<std::string>> allComments;  // Add a vector for comments

        for (const auto& entry : plotData) {
            questions.push_back(entry.first);
            responses.push_back(entry.second.responses);
            allLabels.push_back(entry.second.labels);
            allComments.push_back(entry.second.comments);  // Retrieve the comments
        }

        // Ensure currentPlotIndex is within bounds
        currentPlotIndex = std::clamp(currentPlotIndex, 0, static_cast<int>(questions.size()) - 1);

        // Get the current question, responses, labels, and comments
        const std::string& currentQuestion = questions[currentPlotIndex];
        const std::vector<int>& currentResponses = responses[currentPlotIndex];
        const std::vector<std::string>& currentLabels = allLabels[currentPlotIndex];
        const std::vector<std::string>& currentComments = allComments[currentPlotIndex];  // Get comments

        // Loop through each label and replace spaces with newline characters
        std::vector<std::string> modifiedLabels;
        for (const auto& label : currentLabels) {
            std::string modifiedLabel = label; // Make a copy of the label
            std::replace(modifiedLabel.begin(), modifiedLabel.end(), ' ', '\n'); // Replace spaces with newlines
            modifiedLabels.push_back(modifiedLabel);
        }

        // Find the maximum response value for the y-axis limit
        float maxResponse = *std::max_element(currentResponses.begin(), currentResponses.end());

        // Begin plotting
        ImPlot::BeginPlot(("Responses: " + currentQuestion).c_str());

        // Set up the axes and labels for the X and Y axes
        ImPlot::SetupAxes("Response Labels", "Count", ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels);

        ImPlot::SetupAxis(ImAxis_X1, "", ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxis(ImAxis_Y1, "Response Count");

        // Set the x and y axis limits
        ImPlot::SetupAxesLimits(0.5f, static_cast<float>(currentResponses.size()) + 1.5f, maxResponse * -0.33f, maxResponse * 1.3f);

        // Plot the bar chart for the current question's responses
        ImPlot::PlotBars("Responses", currentResponses.data(), currentResponses.size(), 0.8f);  // 0.8f is the bar width

        // Display the labels on the x-axis (adjust placement if needed)
        for (size_t i = 0; i < modifiedLabels.size(); ++i) {
            ImPlot::PlotText(modifiedLabels[i].c_str(), i + 1, -0.1f);  // Adjust text placement as needed
        }

        ImPlot::EndPlot();

        ImGui::Text("Comments");
        

        std::set<std::string> uniqueComments;
        // Add each comment to the set to remove duplicates
        for (const auto& comment : currentComments) {
            uniqueComments.insert(comment);
        }
        // Display the unique comments in the popup
        for (const auto& comment : uniqueComments) {
            ImGui::Text("%s", comment.c_str());  // Display each unique comment as text
        }
        if (currentComments.empty()) {
            ImGui::Text("None");
        }
        ImGui::NewLine();

        // Display navigation buttons
        if (ImGui::Button("Previous")) {
            currentPlotIndex--;
        }
        ImGui::SameLine();
        if (ImGui::Button("Next")) {
            currentPlotIndex++;
        }

        // Close button to exit the popup
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
            popupOpen = false;
        }

        ImGui::EndPopup(); // End of popup modal
    }
}






//////////////////////////////////////
// MAIN CODE
//////////////////////////////////////

void MyApp::RenderAnalyzeSurveysPage() {

    //////////////////////////////////////
    // Variable Definitions
    //////////////////////////////////////
       
    static bool pageRefresh = false; // initialize variable to ask for a full page refresh
    static nlohmann::json metadataFilters; // Initialize metadata filters        
    static map<string, map<string, map<string, int>>> programMap; // List of all COIs, MOEs, DSPs that exist in the selected test program
    static map<string, map<string, map<string, int>>> responsesMap; // List of all COIs, MOEs, DSPs that exist in the selected test program responses
    static vector<string> allTestEvents; // List of test event folder names that appear in selected test program
    static vector<string> respondedEvents; // List of test event folder names that have responses in selected test program        
    static map<int, int> testProgramSurveyCounts; // Set up aggregate COI program variables for plot
    static map<int, int> testProgramResponseCounts; // Set up aggregate COI response variables for plot
    static map<string, int> filteredSurveyCounts; // Values for program counts after filtering
    static map<string, int> filteredResponseCounts; // Values for response counts after filtering
    static bool filterComplete = false; // Bool to track if values are filtered
    static bool displayError = false; // To store the error message state
    static bool showMatchingResponses = false; // To manage the matching responses window state
    static bool showSurveyDetailsWindow = false; // To track the survey response details window status
    static std::string selectedResponseFilename; // To store the currently selected response filename
    static bool openPlotWindow = false; // To display plots of actual responses

    // Make buttons look cute
    ImGui::GetStyle().FrameRounding = 5.0f;

    // Initialize ImPlot
    ImPlot::CreateContext();

    // Test Programs variable for dropdown values, from Test Programs folder
    static vector<string> testPrograms;

    // If first time loading, load Test Program options
    if (testPrograms.empty()) {
        testPrograms = GetSurveyFolders(testProgramsPath);
        testPrograms.insert(testPrograms.begin(), "No Selection");
    }

    //////////////////////////////////////
    // Primary GUI code
    //////////////////////////////////////
    
    // Window Title
    ImGui::Separator();
    ImGui::Text("Survey Analysis");
    ImGui::NewLine();

    // Render the Test Program combo box for selection
    ImGui::Text("Select Test Program");
        
    // Get integer value for selected test program index
    ImGui::SetNextItemWidth(200.0f);
    selectedTestProgramIndex = RenderDropdown(testPrograms, "Test Program", selectedTestProgramIndex);
    
    ImGui::NewLine();

    // Display "Back to Home" button
    if (ImGui::Button("Back to Home")) {
        currentPage = Page::Home;  // Switch to Home page
        selectedTestProgramIndex = 0;
        testPrograms.clear();
    }

    ImGui::NewLine();
    ImGui::Separator();

    // Main Section

    // If a test program is selected, change selectedTestProgram to match (for path retrieval)
    if (selectedTestProgramIndex > 0) { selectedTestProgram = testPrograms[selectedTestProgramIndex]; }

    // Set test program paths using Dropdown value
    string testProgramPath = testProgramsPath + "/" + selectedTestProgram;
    string testResponsesPath = responsesPath + "/" + selectedTestProgram;

    // If test program has been selected
    // On Refresh, fill the programMap and responsesMap 
    if ((allTestEvents.empty() or pageRefresh or respondedEvents.empty()) && selectedTestProgramIndex > 0) {

        // Set path for response types using test program path
        string responseTypesPath = testProgramPath + "/" + "responsetypes.json";

        // Get response types path
        if (responseTypes.empty()) GetResponseTypes(responseTypesPath);

        // Get test event folder names
        allTestEvents = GetSurveyFolders(testProgramPath);

        // Get responded events
        respondedEvents = GetSurveyFolders(testResponsesPath);

        // Reset maps of test program and responses
        programMap.clear();
        responsesMap.clear();

        // tokenize each test event based on underscores and add to maps
        for (const string& event : allTestEvents) {

            string eventSurveysPath = testProgramPath + "/" + event;
            string eventResponsesPath = testResponsesPath + "/" + event;

            vector<string> parts = SplitStringByDelimiter(event, '_');

            // If correct format, add to dataMap
            if (parts.size() >= 5) {
                try {
                    // Get COI, MOE, DSP, minCount from testEvent folder name
                    string coiValue = parts[1];
                    string mopsValue = parts[2];
                    string dspsValue = parts[3];
                    int minCount = std::stoi(parts[4]);

                    // Count MOP files in test event folder
                    int mopCount = CountFilesInFolder(eventSurveysPath);

                    // Use the nested map structure to store COIS, MOES, DSPS, surveysRequired in both maps
                    programMap[coiValue][mopsValue][dspsValue] = minCount * mopCount;

                    // If the event has responses, do the same for responsesMap
                    if (std::find(respondedEvents.begin(), respondedEvents.end(), event) != respondedEvents.end()) {

                        // Count response files in the responses folder
                        int responseCount = CountFilesInFolder(eventResponsesPath);
                        responsesMap[coiValue][mopsValue][dspsValue] = responseCount;
                    }
                }

                catch (const std::invalid_argument&) {
                    std::cerr << "Invalid number format in event: " << event << std::endl;
                }
                catch (const std::out_of_range&) {
                    std::cerr << "Number out of range in event: " << event << std::endl;
                }
            }
        }
    }

    // Get response types key, value data
    string responseTypesPath = testProgramPath + "/" + "responsetypes.json";
    
    
    pageRefresh = false;

    // Filtering of data for plots

    // Dropdown variables
    vector<string> COIS;
    vector<string> MOES;
    vector<string> DSPS;

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

        filterComplete = false;
        pageRefresh = true;

        metadataFilters.clear();

        std::cout << "Clearing stuff" << endl;
    }

    ///////////////////////////////
    // Dropdowns
    ///////////////////////////////
    
    // First check COI. If something selected, render next, etc. Otherwise set all filters to 0
    if (selectedTestProgramIndex > 0) {
        lastSelectedProgramIndex = selectedTestProgramIndex;
        ImGui::Text("COI"); ImGui::NewLine();

        COIS.push_back("No Selection");
        for (const auto& pair : programMap) COIS.push_back(pair.first);
        ImGui::SetNextItemWidth(200.0f);
        selectedCOI = RenderDropdown(COIS, "COIS", selectedCOI);
        if (selectedCOI != lastCOIIndex) { selectedMOE = 0; selectedDSP = 0; }

    }
    else selectedCOI = 0;

    // If COI is selected show MOES dropdown, otherwise lower filters set to 0
    if (selectedCOI > 0) {
        ImGui::Text("Measure of Effectiveness"); ImGui::NewLine();
        MOES.push_back("No Selection");
        for (const auto& pair : programMap[std::to_string(selectedCOI)]) MOES.push_back(pair.first);
        ImGui::SetNextItemWidth(200.0f);
        selectedMOE = RenderDropdown(MOES, "MOES", selectedMOE);
        if (selectedMOE != lastMOEIndex) selectedDSP = 0;
    }
    else selectedMOE = 0;

    // If MOES is selected show DSP dropdown, otherwise set DSP filter to 0
    if (selectedMOE > 0) {
        ImGui::Text("Design Point"); ImGui::NewLine();
        DSPS.push_back("No Selection");
        for (const auto& pair : programMap[std::to_string(selectedCOI)][std::to_string(selectedMOE)]) DSPS.push_back(pair.first);
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
        filterComplete = false;
        cout << "Clearing" << endl;
    }

    ImGui::NewLine();
    ImGui::NewLine();
    ImGui::NewLine();

    ///////////////////////////////
    // Plots
    ///////////////////////////////
    
    ImGui::BeginChild("Surveys Summary Statistics");
    // If test events is filled and Test Program is selected, run summary plot logic
    if (!allTestEvents.empty() && selectedTestProgramIndex != 0) {
        // If Program selected, plot data by COI
        if (selectedCOI == 0) {

            // Fill filteredSurveyCounts with last val of test event name * MOP files in folder
            if (filteredSurveyCounts.empty()) {

                // Iterate through COIs to add vals at index
                for (const auto& COIpair : programMap) {
                    auto COI = COIpair.first;

                    // Iterate through test events
                    for (const auto& event : allTestEvents) {
                        // Split the event string by underscores to get coi data etc
                        vector<string> parts = SplitStringByDelimiter(event, '_');

                        // If test event is part of current COI iteration
                        if (parts[1] == COI) {
                            auto MOE = parts[2];
                            auto DSP = parts[3];

                            // Ensure filteredSurveyCounts has the proper index (COI)
                            if (filteredSurveyCounts.find(COI) == filteredSurveyCounts.end()) {
                                // Initialize COI in filteredSurveyCounts if it doesn't exist
                                filteredSurveyCounts[COI] = 0;
                            }

                            // Add the value from dataMap to the filteredSurveyCounts map for the current COI
                            filteredSurveyCounts[COI] += programMap[COI][MOE][DSP];  // Add the value to the appropriate index
                        }
                    }
                }
            }

            // Fill filteredSurveyCounts with num responses in applicable test events
            if (filteredResponseCounts.empty()) {

                // Add keys from Survey Counts to Response Counts
                for (const auto& k : filteredSurveyCounts) {
                    const std::string& key = k.first;
                    if (filteredResponseCounts.find(key) == filteredResponseCounts.end()) {
                        // Add the key to filteredResponseCounts with a default value of 0
                        filteredResponseCounts[key] = 0;
                    }
                }                
               
                // Iterate through COIs
                for (const auto& COIpair : responsesMap) {
                    auto COI = COIpair.first;

                    // Iterate through responses
                    for (const auto& event : respondedEvents) {
                        // Split the event string by underscores
                        vector<string> parts = SplitStringByDelimiter(event, '_');

                        // If test event is part of current COI iteration
                        if (parts[1] == COI) {
                            auto MOE = parts[2];
                            auto DSP = parts[3];
                                                        
                            // Ensure filteredSurveyCounts has the proper index (COI)
                            if (filteredResponseCounts.find(COI) == filteredResponseCounts.end()) {
                                // Initialize COI in filteredSurveyCounts if it doesn't exist
                                filteredResponseCounts[COI] = 0;
                            }                            

                            // Add the value from dataMap to the filteredSurveyCounts map for the current COI
                            filteredResponseCounts[COI] += responsesMap[COI][MOE][DSP];  // Add the value to the appropriate index
                        }
                    }
                }
            }

            // Plot data
            if (!filteredSurveyCounts.empty()) {
                // Get max survey value (y lim)
                float max_survey_value = std::max_element(filteredSurveyCounts.begin(), filteredSurveyCounts.end(),
                    [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                        return a.second < b.second; // Compare the `second` value (survey count) of each pair
                    })->second;  // Extract the `second` (survey count) of the max element

                // Set plotLims with max values
                vector<float> plotLims = { static_cast<float>(filteredSurveyCounts.size()), max_survey_value };

                // Changed back to filtered survey counts
                RenderProgressBars(filteredSurveyCounts, filteredResponseCounts, "Survey Completion by \n Critical Operational Issue",
                    "Total Surveys to Complete", "Surveys Administered", plotLims);
            }

        }

        // If COI selected, plot data by MOE
        if (selectedCOI != 0 && selectedMOE == 0) {

            string COI = std::to_string(selectedCOI);

            // Fill filteredSurveyCounts with last val of test event name * MOP files in folder
            if (filteredSurveyCounts.empty()) {

                // Fill filteredSurveyCounts with data by MOE
                map<string, map<string, int>> currentCOIMAP = programMap[COI];

                for (const auto& MOEpair : currentCOIMAP) {
                    auto MOE = MOEpair.first;

                    // Add survey counts to the survey counter vector
                    for (const auto& event : allTestEvents) {
                        // Split the event string by underscores
                        vector<string> parts = SplitStringByDelimiter(event, '_');

                        // If test event is part of selected COI and current MOE iteration
                        if (parts.size() > 2 && parts[1] == COI && parts[2] == MOE) {
                            auto DSP = parts[3];
                            // Ensure filteredSurveyCounts has the proper index (COI)
                            if (filteredSurveyCounts.find(MOE) == filteredSurveyCounts.end()) {
                                // Initialize COI in filteredSurveyCounts if it doesn't exist
                                filteredSurveyCounts[MOE] = 0;

                                cout << "MOE " << MOE << " is in the keys" << endl;
                            }

                            // Add the value from dataMap to the filteredSurveyCounts map for the current COI
                            filteredSurveyCounts[MOE] += programMap[COI][MOE][DSP];  // Add the value to the appropriate index
                        }


                    }
                }
            }

            // Fill filteredSurveyCounts with num responses in applicable test events
            if (filteredResponseCounts.empty()) {

                // Add keys from Survey Counts to Response Counts
                for (const auto& k : filteredSurveyCounts) {
                    const std::string& key = k.first;
                    if (filteredResponseCounts.find(key) == filteredResponseCounts.end()) {
                        // Add the key to filteredResponseCounts with a default value of 0
                        filteredResponseCounts[key] = 0;
                    }
                }

                // Fill filteredSurveyCounts with data by MOE
                map<string, map<string, int>> currentCOIMAP = responsesMap[std::to_string(selectedCOI)];

                for (const auto& MOEpair : currentCOIMAP) {
                    auto MOE = MOEpair.first;

                    // Add survey counts to the survey counter vector
                    for (const auto& event : respondedEvents) {
                        // Split the event string by underscores
                        vector<string> parts = SplitStringByDelimiter(event, '_');

                        // If test event is part of selected COI and current MOE iteration
                        if (parts.size() > 2 && parts[1] == COI && parts[2] == MOE) {
                            auto DSP = parts[3];
                            // Ensure filteredSurveyCounts has the proper index (COI)
                            if (filteredResponseCounts.find(MOE) == filteredResponseCounts.end()) {
                                // Initialize COI in filteredResponseCounts if it doesn't exist
                                filteredResponseCounts[MOE] = 0;
                            }

                            // Add the value from dataMap to the filteredResponseCounts map for the current COI
                            filteredResponseCounts[MOE] += responsesMap[COI][MOE][DSP];  // Add the value to the appropriate index
                            cout << "Adding values to COI " << MOE << " of " << responsesMap[COI][MOE][DSP] << endl;
                        }
                    }
                }
            }

            // Make the plot
            if (!filteredSurveyCounts.empty()) {
                // Get max survey value (y lim)
                float max_survey_value = std::max_element(filteredSurveyCounts.begin(), filteredSurveyCounts.end(),
                    [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                        return a.second < b.second; // Compare the `second` value (survey count) of each pair
                    })->second;  // Extract the `second` (survey count) of the max element

                // Set plotLims with max values
                vector<float> plotLims = { static_cast<float>(filteredSurveyCounts.size()), max_survey_value };

                RenderProgressBars(filteredSurveyCounts, filteredResponseCounts, "Selected COI Completion by \n Measure of Effectiveness",
                    "Total Surveys to Complete", "Surveys Administered", plotLims);
            }
        }

        // If MOE selected, plota data by DSP
        if (selectedMOE != 0 && selectedDSP == 0) {
            
            string COI = std::to_string(selectedCOI);
            string MOE = std::to_string(selectedMOE);

            // Fill filteredSurveyCounts with last val of test event name * MOP files in folder
            if (filteredSurveyCounts.empty()) {

                // Fill filteredSurveyCounts with data by DSP
                map<string, int> currentMOEMap = programMap[COI][MOE];

                for (const auto& MOEpair : currentMOEMap) {
                    auto DSP = MOEpair.first;

                    // Add survey counts to the survey counter vector
                    for (const auto& event : allTestEvents) {
                        // Split the event string by underscores
                        vector<string> parts = SplitStringByDelimiter(event, '_');

                        // If event is a part of selected COI and selected MOP and DSP (of loop) add last part of folder name to counter for DSP index
                        if (parts.size() > 3 && parts[1] == COI && parts[2] == MOE && parts[3] == DSP) {
                            if (filteredSurveyCounts.find(DSP) == filteredSurveyCounts.end()) {
                                // Initialize COI in filteredSurveyCounts if it doesn't exist
                                filteredSurveyCounts[DSP] = 0;
                            }
                            // Add the value from dataMap to the filteredSurveyCounts map for the current COI
                            filteredSurveyCounts[DSP] += programMap[COI][MOE][DSP];  // Add the value to the appropriate index
                        }
                    }
                }
            }

            // Fill filteredSurveyCounts with num responses in applicable test events
            if (filteredResponseCounts.empty()) {

                // Add keys from Survey Counts to Response Counts
                for (const auto& k : filteredSurveyCounts) {
                    const std::string& key = k.first;
                    if (filteredResponseCounts.find(key) == filteredResponseCounts.end()) {
                        // Add the key to filteredResponseCounts with a default value of 0
                        filteredResponseCounts[key] = 0;
                    }
                }

                // Fill filteredSurveyCounts with data by DSP
                map<string, int> currentMOEMap = responsesMap[COI][MOE];

                for (const auto& MOEpair : currentMOEMap) {
                    auto DSP = MOEpair.first;

                    // Add survey counts to the survey counter vector
                    for (const auto& event : respondedEvents) {
                        // Split the event string by underscores
                        vector<string> parts = SplitStringByDelimiter(event, '_');

                        // If event is a part of selected COI and selected MOP and DSP (of loop) add last part of folder name to counter for DSP index
                        if (parts.size() > 3 && parts[1] == COI && parts[2] == MOE && parts[3] == DSP) {
                            if (filteredResponseCounts.find(DSP) == filteredResponseCounts.end()) {
                                // Initialize COI in filteredSurveyCounts if it doesn't exist
                                filteredResponseCounts[DSP] = 0;
                            }
                            // Add the value from dataMap to the filteredSurveyCounts map for the current COI
                            filteredResponseCounts[DSP] += responsesMap[COI][MOE][DSP];  // Add the value to the appropriate index
                        }
                    }
                }
            }

            // Make the plot
            if (!filteredSurveyCounts.empty()) {
                // Get max survey value (y lim)
                float max_survey_value = std::max_element(filteredSurveyCounts.begin(), filteredSurveyCounts.end(),
                    [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                        return a.second < b.second; // Compare the `second` value (survey count) of each pair
                    })->second;  // Extract the `second` (survey count) of the max element

                // Set plotLims with max values
                vector<float> plotLims = { static_cast<float>(filteredSurveyCounts.size()), max_survey_value };

                RenderProgressBars(filteredSurveyCounts, filteredResponseCounts, "Selected Measure of Effectiveness \n Completion by Design Point",
                    "Total Surveys to Complete", "Surveys Administered", plotLims);
            }
        }

        // If DSP selected, plot only the single DSP
        if (selectedDSP != 0) {

            string COI = std::to_string(selectedCOI);
            string MOE = std::to_string(selectedMOE);
            string DSP = std::to_string(selectedDSP);

            // Fill filteredSurveyCounts with last val of test event name * MOP files in folder
            if (filteredSurveyCounts.empty()) {

                // Fill filteredSurveyCounts with data by DSP
                map<string, int> currentMOEMap = programMap[COI][MOE];

                for (const auto& MOEpair : currentMOEMap) {

                    // Add survey counts to the survey counter vector
                    for (const auto& event : allTestEvents) {
                        // Split the event string by underscores
                        vector<string> parts = SplitStringByDelimiter(event, '_');

                        // If event is a part of selected COI and selected MOP and DSP (of loop) add last part of folder name to counter for DSP index
                        if (parts.size() > 3 && parts[1] == COI && parts[2] == MOE && parts[3] == DSP) {
                            if (filteredSurveyCounts.find(DSP) == filteredSurveyCounts.end()) {
                                // Initialize COI in filteredSurveyCounts if it doesn't exist
                                filteredSurveyCounts[DSP] = 0;
                            }
                            // Add the value from dataMap to the filteredSurveyCounts map for the current COI
                            filteredSurveyCounts[DSP] += programMap[COI][MOE][DSP];  // Add the value to the appropriate index
                        }
                    }
                }
            }

            // Fill filteredSurveyCounts with num responses in applicable test events
            if (filteredResponseCounts.empty()) {

                // Add keys from Survey Counts to Response Counts
                for (const auto& k : filteredSurveyCounts) {
                    const std::string& key = k.first;
                    if (filteredResponseCounts.find(key) == filteredResponseCounts.end()) {
                        // Add the key to filteredResponseCounts with a default value of 0
                        filteredResponseCounts[key] = 0;
                    }
                }

                // Fill filteredSurveyCounts with data by DSP
                map<string, int> currentMOEMap = responsesMap[COI][MOE];

                for (const auto& MOEpair : currentMOEMap) {

                    // Add survey counts to the survey counter vector
                    for (const auto& event : respondedEvents) {
                        // Split the event string by underscores
                        vector<string> parts = SplitStringByDelimiter(event, '_');

                        // If event is a part of selected COI and selected MOP and DSP (of loop) add last part of folder name to counter for DSP index
                        if (parts.size() > 3 && parts[1] == COI && parts[2] == MOE && parts[3] == DSP) {
                            if (filteredResponseCounts.find(DSP) == filteredResponseCounts.end()) {
                                // Initialize COI in filteredSurveyCounts if it doesn't exist
                                filteredResponseCounts[DSP] = 0;
                            }
                            // Add the value from dataMap to the filteredSurveyCounts map for the current COI
                            filteredResponseCounts[DSP] += responsesMap[COI][MOE][DSP];  // Add the value to the appropriate index
                        }
                    }
                }
            }

            // Make the plot
            if (!filteredSurveyCounts.empty()) {
                // Get max survey value (y lim)
                float max_survey_value = std::max_element(filteredSurveyCounts.begin(), filteredSurveyCounts.end(),
                    [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                        return a.second < b.second; // Compare the `second` value (survey count) of each pair
                    })->second;  // Extract the `second` (survey count) of the max element

                // Set plotLims with max values
                vector<float> plotLims = { static_cast<float>(filteredSurveyCounts.size()), max_survey_value };

                RenderProgressBars(filteredSurveyCounts, filteredResponseCounts, "Selected Measure of Effectiveness \n Completion by Design Point",
                    "Total Surveys to Complete", "Surveys Administered", plotLims);
            }
        }


        /*
        // SHOWS PLOT DATA FOR SANITY CHECK
        if (!filteredSurveyCounts.empty()) {
            // Print contents of filteredSurveyCounts for debugging
            std::cout << "filteredSurveyCounts contents:" << std::endl;
            for (const auto& entry : filteredSurveyCounts) {
                std::cout << "Key: " << entry.first << ", Value: " << entry.second << std::endl;
            }
        }
        
        if (!filteredResponseCounts.empty()) {
            // Print contents of filteredSurveyCounts for debugging
            std::cout << "filteredResponseCounts contents:" << std::endl;
            for (const auto& entry : filteredResponseCounts) {
                std::cout << "Key: " << entry.first << ", Value: " << entry.second << std::endl;
            }
        }
        cout << endl;
        */

    }
    ImGui::EndChild();

    ImGui::NextColumn();

    ///////////////////////////////
    // Metadata Filters GUI
    ///////////////////////////////
    
    // Set path based on selected program
    string metadataPath = testProgramPath + "/" + "metadataquestions.json";
    static string lastMetaPath = "";

    nlohmann::json metadataJson;
    std::ifstream metadataFile(metadataPath);

    static vector<pair<string, bool>> surveyResponses; // To store survey responses
    
    // If path changed, reset everything
    if (metadataPath != lastMetaPath) {
        metadataFilters.clear();
        lastMetaPath = metadataPath;
        surveyResponses.clear();
        cout << "Path changed, clearing" << endl;
    }

    ImGui::Text("Individual Response Filters");
    ImGui::NewLine();
    
    // Metadata questions loading
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

    // Display metadata filter input fields
    for (auto& [metaID, metaValues] : metadataFilters.items()) {  // We now use the global 'metadata'

        // Display the key above input
        ImGui::Text("%s", metaID.c_str());
        string label = "##" + metaID;

        // Check if 'metaValues' has the required keys
        if (metaValues.contains("inputType")) {
            string inputTypeValue = metaValues["inputType"].get<std::string>();

            // If input type is array (dropdown), handle the dropdown logic
            if (inputTypeValue == "dropdown") {

                vector<string> options = metaValues["preset"];
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
                string currentValue = metaValues["response"];

                // Display the input box with the current value
                char buf[256];

                // Use strncpy_s for safer string copy
                strncpy_s(buf, sizeof(buf), currentValue.c_str(), _TRUNCATE);  // _TRUNCATE ensures the string fits in the buffer

                if (ImGui::InputText(label.c_str(), buf, sizeof(buf))) {
                    // When the user types something, update the metadata map
                    metaValues["response"] = string(buf);  // Update the response field with new text
                }
            }

            // If input type is "time" show two input boxes, one for min and one for max
            if (inputTypeValue == "time") {
                string currentValue = metaValues["response"];

                // Split the currentValue into min and max if they exist, otherwise use empty strings
                string minValue, maxValue;
                size_t commaPos = currentValue.find(",");
                if (commaPos != string::npos) {
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
                string minLabel = "Min##" + metaID;  // "Min##(original label)"
                string maxLabel = "Max##" + metaID;  // "Max##(original label)"

                // Set the width for the input boxes
                float inputWidth = 100.0f;  // Change this value to adjust the width of the input boxes

                // Set the width of the input boxes
                ImGui::PushItemWidth(inputWidth);

                // Display the Min input box
                if (ImGui::InputText(minLabel.c_str(), minBuf, sizeof(minBuf))) {
                    // When the user types something in the Min box, update the metadata map
                    minValue = string(minBuf);  // Update Min value
                }
                ImGui::SameLine();
                ImGui::Text("   ");
                ImGui::SameLine();
                // Display the Max input box
                if (ImGui::InputText(maxLabel.c_str(), maxBuf, sizeof(maxBuf))) {
                    // When the user types something in the Max box, update the metadata map
                    maxValue = string(maxBuf);  // Update Max value
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

    // Buttons to apply filters, show responses, or show plots
    if (selectedTestProgramIndex > 0) {
        if (ImGui::Button("Apply Filters")) {
            filterComplete = false;
        }
        if (ImGui::Button("Show Matching Responses")) {
            showMatchingResponses = true; // Set flag to display responses window
        }
        if (ImGui::Button("Show Plots")) {
            static bool openPlotWindow = true;
        }

    }

    // Window to tell people not to plot without selecting COI & MOE
    if (displayError) {
        ImGui::Begin("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("COI and MOE must be selected!");

        // Add Close button to the error window
        if (ImGui::Button("Close")) {
            displayError = false; // Close the error window
        }

        ImGui::End();
        ImGui::End();
    }
    
    //////////////////////////////////////
    // Applying Filters
    //////////////////////////////////////

    // Filter data if not complete since last change
    if (!filterComplete && selectedTestProgramIndex > 0) {

        surveyResponses.clear();

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
                            string relativePath = subFolder.filename().string() + "/" + fileEntry.path().filename().string();

                            // Check metadata or any condition, here I'm using a placeholder boolean check
                            bool meetsMetadata = true;

                            // Add the relative folder/filename string to the surveyResponses vector with a bool value (either true or false)
                            surveyResponses.push_back(std::make_pair(relativePath, meetsMetadata));

                        }
                    }
                }
            }
        }
        else {
            std::cerr << "Directory does not exist: " << programFolder << endl;
        }

        // First filtering by COI, MOE, DSP, then by metadata. Not done simultaneously because it's faster (I think?)
        for (size_t i = 0; i < surveyResponses.size(); ++i) {

            // Extract the filename from the pair
            std::string filename = surveyResponses[i].first;

            // If something has been selected from the dropdowns, filter the data by COI, MOE, MOP
            if (selectedCOI > 0) {

                // Find the first underscore
                size_t pos = filename.find('_');
                if (pos != std::string::npos) {

                    // Extract the part before the first underscore (e.g., "V" for vary)
                    std::string prefix = filename.substr(0, pos);

                    // Extract the part between the first and third underscores for the version-like segment
                    size_t secondPos = filename.find('_', pos + 1);
                    size_t thirdPos = filename.find('_', secondPos + 1);
                    size_t fourthPos = filename.find('_', thirdPos + 1);  // Check for the fourth underscore

                    if (secondPos != std::string::npos && thirdPos != std::string::npos && fourthPos != std::string::npos) {
                        // Extract the full version segment (e.g., '1_1_02_4')
                        std::string versionSegment = filename.substr(pos + 1, fourthPos - pos - 1);

                        // Split the version segment by underscores
                        std::vector<std::string> parts = SplitStringByDelimiter(versionSegment, '_');

                        // Convert to ints
                        try {
                            int part1 = std::stoi(parts[0]);
                            int part2 = std::stoi(parts[1]);
                            int part3 = std::stoi(parts[2]);

                            // Set entry.second to false if conditions are not met
                            if (selectedCOI > 0 && part1 != selectedCOI) {
                                surveyResponses[i].second = false;
                            }
                            if (selectedMOE > 0 && part2 != selectedMOE) {
                                surveyResponses[i].second = false;
                            }
                            if (selectedDSP > 0 && part3 != selectedDSP) {
                                surveyResponses[i].second = false;
                            }
                        }
                        catch (const std::invalid_argument& e) {
                            // Handle invalid integer conversion (if the version segment is not valid)
                            surveyResponses[i].second = false; // Set to false on error
                        }
                        catch (const std::out_of_range& e) {
                            // Handle out of range exceptions for integer conversion
                            surveyResponses[i].second = false; // Set to false on error
                        }
                    }
                    else {
                        std::cerr << "Second, third, or fourth underscore not found in filename: " << filename << "\n";
                    }
                }
                else {
                    std::cerr << "First underscore not found in filename: " << filename << "\n";
                }
            }

            // Get a list of active metadata filters
            std::vector<std::string> activeFilterIDs;

            for (auto& [metaID, metaValues] : metadataFilters.items()) {
                // If dropdown, check if response > 0
                if (metaValues["inputType"] == "dropdown") {
                    if (metaValues["response"] > 0) {
                        activeFilterIDs.push_back(metaID);
                    }
                }

                // If text, check if len > 0
                if (metaValues["inputType"] == "text") {
                    if (metaValues["response"].get<std::string>().length() > 0) {
                        activeFilterIDs.push_back(metaID);
                    }
                }

                // If time, check if len > 1 (ignore comma)
                if (metaValues["inputType"] == "time") {
                    std::string timeResponse = metaValues["response"].get<std::string>();
                    if (timeResponse.length() > 1) {
                        activeFilterIDs.push_back(metaID);
                    }
                }
            }

            // If any filters are active, check that the surveyResponses match
            if (!activeFilterIDs.empty()) {
                fs::path programFolder = fs::path(responsesPath) / selectedTestProgram;

                for (size_t i = 0; i < surveyResponses.size(); ++i) {
                    // If COI, MOE, and DSP filter conditions were met, check metadata
                    if (surveyResponses[i].second == true) {
                        // Construct the full file path for the current survey response
                        fs::path filePath = programFolder / surveyResponses[i].first;

                        // Open the file using std::ifstream
                        std::ifstream file(filePath);

                        if (file.is_open()) {
                            // Parse the JSON data from the file
                            nlohmann::json currentMeta;
                            file >> currentMeta;
                            currentMeta = currentMeta["metadata"];

                            // Iterate through the activeFilterIDs to check that currentMeta values match metadataFilters values
                            for (auto& id : activeFilterIDs) {
                                
                                cout << currentMeta["User ID"].dump() << std::endl;

                                // If dropdown, check for exact match of response
                                if (metadataFilters[id]["inputType"] == "dropdown") {
                                    auto responseMetaValue = currentMeta[id]["response"];
                                    auto filterMetaValue = metadataFilters[id]["response"];
                                    cout << responseMetaValue << endl << filterMetaValue << endl;

                                    if (responseMetaValue != filterMetaValue) {
                                        surveyResponses[i].second = false;  // Set to false if no match
                                    }
                                }

                                // If text, check that metadataFilters string appears in currentMeta
                                if (metadataFilters[id]["inputType"] == "text") {
                                    std::string filterValue = metadataFilters[id]["response"];  // Assuming the response is a string
                                    std::string currentValue = currentMeta[id]["response"]; // Assuming the response is a string

                                    if (currentValue.find(filterValue) == std::string::npos) {
                                        surveyResponses[i].second = false;  // Set to false if string doesn't match
                                    }
                                }

                                // If time, separate the two values using the comma and check if currentMeta value is between the two
                                if (metadataFilters[id]["inputType"] == "time") {
                                    std::string timeRange = metadataFilters[id]["response"];  // Assuming the response is a string like "08:00,18:00"
                                    size_t commaPos = timeRange.find(',');

                                    if (commaPos != std::string::npos) {
                                        std::string startTime = timeRange.substr(0, commaPos);
                                        std::string endTime = timeRange.substr(commaPos + 1);

                                        // Assuming currentMeta[id]["response"] is a time string (e.g., "10:00")
                                        std::string currentTime = currentMeta[id]["response"];

                                        if (currentTime < startTime || currentTime > endTime) {
                                            surveyResponses[i].second = false;  // Set to false if current time is outside the range
                                        }
                                    }
                                }
                            }


                            // Process metadata or any other logic here
                        }
                        else {
                            std::cerr << "Failed to open file: " << filePath << std::endl;
                        }
                    }
                }                
            }
        }
        filterComplete = true;
        std::cout << "Finished processing survey responses.\n";
    }


    //////////////////////////////////////
    // Displaying Matching Responses, Individual Surveys
    ////////////////////////////////////// 
 
    // Display Matching Responses Window
    if (showMatchingResponses) {
        ImGui::Begin("Matching Survey Responses", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

        for (const auto& response : surveyResponses) {
            const string& responseFilename = response.first;
            bool meetsMetadata = response.second;

            // Only display responses that meet the metadata criteria
            if (meetsMetadata) {
                if (ImGui::Button(responseFilename.c_str())) {
                    // Store the selected response filename and show the details window
                    selectedResponseFilename = responseFilename;
                    showSurveyDetailsWindow = true;
                }
            }
        }
        ImGui::NewLine();
        ImGui::NewLine();
        // Add Close button to the Matching Responses window
        if (ImGui::Button("Close")) {
            showMatchingResponses = false; // Close the responses window
        }
        ImGui::NewLine();

        ImGui::End();
    }

    // Show the Survey Response Details Window if needed
    if (showSurveyDetailsWindow) {
        fs::path responsePath = fs::path(responsesPath) / selectedTestProgram / selectedResponseFilename;

        std::ifstream surveyFile(responsePath);
        if (surveyFile.is_open()) {
            nlohmann::json surveyData;
            surveyFile >> surveyData;

            // Set window size to 1/4 of screen width and auto height
            ImVec2 screenSize = ImGui::GetIO().DisplaySize;  // Get the screen size
            ImVec2 windowSize(screenSize.x / 4.0f, 0.0f);   // Set width to 1/4 of screen, height will auto-adjust
            ImGui::SetWindowSize(windowSize);

            // Create a new window to display the JSON data
            ImGui::Begin("Survey Response Details");

            // Add Close button to reset the window status
            if (ImGui::Button("Close")) {
                showSurveyDetailsWindow = false; // Close the survey details window by resetting the flag
            }

            // Display the comment
            ImGui::Text("Comment: %s", surveyData["comment"].get<std::string>().c_str());

            // Metadata label
            ImGui::Text("Metadata:");

            // Display metadata fields
            for (auto& [key, value] : surveyData["metadata"].items()) {

                if (value["inputType"] == "dropdown") {
                    // For dropdown, show the value at the index of "response"
                    int responseIndex = value["response"].get<int>();
                    const nlohmann::json& preset = value["preset"];
                    if (responseIndex >= 0 && responseIndex < preset.size()) {
                        ImGui::Text("  %s: %s", key.c_str(), preset[responseIndex].get<std::string>().c_str());
                    } else {
                        ImGui::Text("  %s: Invalid index", key.c_str());
                    }
                }
                else {
                    // Handle other input types if needed (e.g., text or number)
                    if (value.contains("response")) {
                        if (value["response"].is_string()) {
                            ImGui::Text("  %s: %s", key.c_str(), value["response"].get<std::string>().c_str());
                        } else if (value["response"].is_number()) {
                            ImGui::Text("  %s: %f", key.c_str(), value["response"].get<float>());
                        }
                    }
                }
            }

            // Display questions and responses
            ImGui::Text("Questions:");
            for (size_t i = 0; i < surveyData["questions"].size(); ++i) {
                // Display the question
                ImGui::Text("  Question %zu: %s", i + 1, surveyData["questions"][i].get<std::string>().c_str());

                // Display the response
                if (i < surveyData["responses"].size()) {
                    ImGui::Text("  Response: %d", surveyData["responses"][i].get<int>());
                }
                else {
                    ImGui::Text("  Response: Not provided");
                }

                // If there are response types, display them (assuming the size matches)
                if (i < surveyData["responseTypes"].size()) {
                    ImGui::Text("  Response Type: %s", surveyData["responseTypes"][i].get<std::string>().c_str());
                }
                else {
                    ImGui::Text("  Response Type: Not provided");
                }
            }

            ImGui::End();
        }
        else {
            ImGui::Text("Failed to open survey file: %s", selectedResponseFilename.c_str());
        }
    }    

    //////////////////////////////////////
    // Plots
    //////////////////////////////////////

    static std::map<std::string, QuestionData> plotData;

    if (selectedMOE > 0 && !filterComplete) {
        plotData = ProcessResponseData(testResponsesPath, respondedEvents, responseTypes, selectedCOI, selectedMOE);
        filterComplete = true;
    }
    
    if (!plotData.empty()){ //&& !plotGenerated) {
        // Generate plots if plotData not empty
        // cout << "Plotting" << endl;
        RenderResponsePlots(plotData);
        // plotGenerated = true;
    }

    ImGui::NewLine();
    ImGui::Columns(1);

    ImPlot::DestroyContext();        
}
















