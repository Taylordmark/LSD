#include "Application.h"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdlib> // For std::rand() and std::srand()
#include <ctime>   // For std::time()
#include <filesystem> // For filesystem operations
#include <iostream>   // For standard I/O
#include "CompleteSurveysPage.h"


// Static variables for managing popup state
static bool showSurveyPopup = false;
static size_t surveyRow = 0;
static std::string surveyMessage;

// Function to load CSV into a vector of vectors
bool loadCSV(const std::string& filename, std::vector<std::vector<std::string>>& data) {
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
        data.push_back(row);
    }
    file.close();
    return true;
}

// Save response to CSV
void SaveSurveyToCSV(const std::vector<std::vector<std::string>>& matrix, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return;
    }

    for (const auto& row : matrix) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i < row.size() - 1) file << ','; // Add comma between columns
        }
        file << '\n'; // Newline after each row
    }

    file.close();
}

// Survey function
void completeSurvey(size_t row, const std::vector<std::vector<std::string>>& data) {
    // Ensure the row is valid and the data contains enough columns
    if (row < data.size() && data[row].size() > 15) {
        std::string surveyValue = data[row][15]; // Column index 15
        std::string filenameToFind = surveyValue + ".csv";
        bool fileFound = false;

        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file() && entry.path().filename() == filenameToFind) {
                fileFound = true;
                break;
            }
        }

        // Set the flag and message to show in the popup
        surveyRow = row;
        surveyMessage = "File '" + filenameToFind + "' " + (fileFound ? "found." : "not found.");
        showSurveyPopup = true; // Show the popup
    }
    else {
        // Handle invalid row or insufficient data
        surveyRow = row;
        surveyMessage = "Invalid row or insufficient data.";
        showSurveyPopup = true; // Show the popup
    }
}

// Function to convert long format to wide format with aggregation based on provided CSV structure
std::vector<std::vector<std::string>> longToWideFormat(const std::vector<std::vector<std::string>>& longData) {
    // Map to hold the question and a pair of aggregated options and their corresponding input types
    std::unordered_map<std::string, std::pair<std::string, std::string>> wideMap;

    for (const auto& row : longData) {
        if (row.size() < 3) continue; // Ensure there are at least three columns
        std::string question = row[0]; // Column 1 for the question
        std::string option = row[1]; // Column 2 for the options
        std::string inputType = row[2]; // Column 3 for the input type

        // Aggregate the options, separating by commas or semicolons as needed
        if (!wideMap[question].first.empty()) {
            wideMap[question].first += ", "; // Add a separator for options
        }
        wideMap[question].first += option;

        // You can also choose to aggregate input types if needed
        if (!wideMap[question].second.empty()) {
            wideMap[question].second += ", "; // Add a separator for input types
        }
        wideMap[question].second += inputType;
    }

    // Prepare the output in wide format
    std::vector<std::vector<std::string>> wideData;
    for (const auto& [question, valuePair] : wideMap) {
        std::vector<std::string> newRow = { question, valuePair.first, valuePair.second }; // Create a new row
        wideData.push_back(newRow); // Add the constructed row to wide data
    }

    return wideData;
}






void MyApp::RenderCompleteSurveysPage() {
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float availableHeight = ImGui::GetContentRegionAvail().y;
    float childHeight = availableHeight * 0.9f;

    static bool filterPointsCheck = false;

    // Declare indices here
    static int maneuverGroupIndex = -1;
    static int focusPositionIndex = -1;
    static int eventTypeIndex = -1;
    static int eventTimeIndex = -1;

    ImGui::BeginChild("Child1", ImVec2(availableWidth * 0.5f, childHeight), true);
    ImGui::Text("Create Surveys Interface\n");
    ImGui::NewLine();

    // Input fields for filtering
    ImGui::Text("Filter Test Point Matrix for Current Test Points:");
    ImGui::NewLine();

    static int selectedManeuverGroup = -1; // -1 for 'Select'
    static int selectedFocusPosition = -1; // -1 for 'Select'
    static int selectedEventType = -1; // -1 for 'Select'
    static int selectedTimeType = -1; // -1 for 'Select'
    static bool filtersChanged = false; // Track if filters have changed

    // Extract unique values for the dropdowns
    std::vector<std::string> uniqueManeuverGroups;
    std::vector<std::string> uniqueFocusPositions;
    std::vector<std::string> uniqueEventTypes;
    std::vector<std::string> uniqueTimeTypes = { "Day", "Night" };

    if (!testPointMatrix.empty()) {
        const auto& headers = testPointMatrix[0];

        
        // Find the indices for the relevant columns
        for (int i = 0; i < headers.size(); ++i) {
            std::string current_header = headers[i];
            if (headers[i] == "Maneuver Group") maneuverGroupIndex = i;
            else if (headers[i] == "Focus Position") focusPositionIndex = i;
            else if (headers[i] == "Event Type") eventTypeIndex = i;
            else if (headers[i] == "Time of Day") eventTimeIndex =i;
        }

        // Check for missing columns and prompt the user
        if (maneuverGroupIndex == -1) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Maneuver Group column not found!");
            if (ImGui::BeginCombo("Select Maneuver Group Column", "Select")) {
                for (int i = 0; i < headers.size(); ++i) {
                    if (ImGui::Selectable(headers[i].c_str(), false)) {
                        maneuverGroupIndex = i;
                    }
                }
                ImGui::EndCombo();
            }
        }
        else {
            // Populate unique values for Maneuver Group
            for (size_t i = 1; i < testPointMatrix.size(); ++i) {
                const auto& row = testPointMatrix[i];
                const std::string& value = row[maneuverGroupIndex];
                if (!value.empty() && std::find(uniqueManeuverGroups.begin(), uniqueManeuverGroups.end(), value) == uniqueManeuverGroups.end()) {
                    uniqueManeuverGroups.push_back(value);
                }
            }
        }

        if (focusPositionIndex == -1) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Focus Position column not found!");
            if (ImGui::BeginCombo("Select Focus Position Column", "Select")) {
                for (int i = 0; i < headers.size(); ++i) {
                    if (ImGui::Selectable(headers[i].c_str(), false)) {
                        focusPositionIndex = i;
                    }
                }
                ImGui::EndCombo();
            }
        }
        else {
            // Populate unique values for Focus Position
            for (size_t i = 1; i < testPointMatrix.size(); ++i) {
                const auto& row = testPointMatrix[i];
                const std::string& value = row[focusPositionIndex];
                if (!value.empty() && std::find(uniqueFocusPositions.begin(), uniqueFocusPositions.end(), value) == uniqueFocusPositions.end()) {
                    uniqueFocusPositions.push_back(value);
                }
            }
        }

        if (eventTypeIndex == -1) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Event Type column not found!");
            if (ImGui::BeginCombo("Select Event Type Column", "Select")) {
                for (int i = 0; i < headers.size(); ++i) {
                    if (ImGui::Selectable(headers[i].c_str(), false)) {
                        eventTypeIndex = i;
                    }
                }
                ImGui::EndCombo();
            }
        }
        else {
            // Populate unique values for Event Type
            for (size_t i = 1; i < testPointMatrix.size(); ++i) {
                const auto& row = testPointMatrix[i];
                const std::string& value = row[eventTypeIndex];
                if (!value.empty() && std::find(uniqueEventTypes.begin(), uniqueEventTypes.end(), value) == uniqueEventTypes.end()) {
                    uniqueEventTypes.push_back(value);
                }
            }
        }
    }

    // Create dropdowns with 'Select' option
    if (ImGui::BeginCombo("Maneuver Group", selectedManeuverGroup == -1 ? "Select" : uniqueManeuverGroups[selectedManeuverGroup].c_str())) {
        if (ImGui::Selectable("Select", selectedManeuverGroup == -1)) {
            selectedManeuverGroup = -1; // Reset filter
            filtersChanged = true; // Indicate that filters have changed
        }
        for (int i = 0; i < uniqueManeuverGroups.size(); ++i) {
            bool isSelected = (selectedManeuverGroup == i);
            if (ImGui::Selectable(uniqueManeuverGroups[i].c_str(), isSelected)) {
                selectedManeuverGroup = i;
                filtersChanged = true; // Indicate that filters have changed
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Focus Position", selectedFocusPosition == -1 ? "Select" : uniqueFocusPositions[selectedFocusPosition].c_str())) {
        if (ImGui::Selectable("Select", selectedFocusPosition == -1)) {
            selectedFocusPosition = -1; // Reset filter
            filtersChanged = true; // Indicate that filters have changed
        }
        for (int i = 0; i < uniqueFocusPositions.size(); ++i) {
            bool isSelected = (selectedFocusPosition == i);
            if (ImGui::Selectable(uniqueFocusPositions[i].c_str(), isSelected)) {
                selectedFocusPosition = i;
                filtersChanged = true; // Indicate that filters have changed
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Event Type", selectedEventType == -1 ? "Select" : uniqueEventTypes[selectedEventType].c_str())) {
        if (ImGui::Selectable("Select", selectedEventType == -1)) {
            selectedEventType = -1; // Reset filter
            filtersChanged = true; // Indicate that filters have changed
        }
        for (int i = 0; i < uniqueEventTypes.size(); ++i) {
            bool isSelected = (selectedEventType == i);
            if (ImGui::Selectable(uniqueEventTypes[i].c_str(), isSelected)) {
                selectedEventType = i;
                filtersChanged = true; // Indicate that filters have changed
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Time of Day", selectedTimeType == -1 ? "Select" : uniqueTimeTypes[selectedTimeType].c_str())) {
        if (ImGui::Selectable("Select", selectedTimeType == -1)) {
            selectedTimeType = -1; // Reset filter
            filtersChanged = true; // Indicate that filters have changed
        }
        for (int i = 0; i < uniqueTimeTypes.size(); ++i) {
            bool isSelected = (selectedTimeType == i);
            if (ImGui::Selectable(uniqueTimeTypes[i].c_str(), isSelected)) {
                selectedTimeType = i;
                filtersChanged = true; // Indicate that filters have changed
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::Columns(1);

    //##############################################################
    ImGui::EndChild();
    ImGui::SameLine(); // Position next to the first child
    ImGui::BeginChild("Child2", ImVec2(availableWidth * 0.5f, childHeight), true);
    //##############################################################


    // Test Point Matrix Path
    static char surveyMatrix[256] = "C:/Surveys/TestPointMatrix.csv";
    static bool fileLoaded = false;
    static std::vector<std::vector<std::string>> tempTestPointMatrix; // Store temp matrix

    // Attempt to load the file if it hasn't been loaded
    if (!fileLoaded) {
        std::ifstream file(surveyMatrix);
        if (file.is_open()) {
            fileLoaded = true; // Mark the file as loaded
            testPointMatrix.clear(); // Clear previous data
            tempTestPointMatrix.clear(); // Clear previous original data
            std::string line;

            // Read the file content
            while (std::getline(file, line)) {
                std::vector<std::string> row;
                std::stringstream ss(line);
                std::string cell;
                while (std::getline(ss, cell, ',')) { // Assuming CSV format
                    row.push_back(cell);
                }
                testPointMatrix.push_back(row);
            }
            tempTestPointMatrix = testPointMatrix; // Store original data
            file.close();
        }
        else {
            fileLoaded = false; // Reset loaded status
        }
    }

    // Filtering logic
    if (fileLoaded && filtersChanged) {
        // Reset tempTestPointMatrix to the original data before filtering
        std::vector<std::vector<std::string>> filteredMatrix;
        filteredMatrix.push_back(testPointMatrix[0]); // Keep the headers

        // Use testPointMatrix for filtering
        for (size_t i = 1; i < testPointMatrix.size(); ++i) { // Skip header row
            const auto& row = testPointMatrix[i];
            bool matches = true;

            if (selectedManeuverGroup >= 0 && maneuverGroupIndex >= 0) {
                if (row[maneuverGroupIndex] != uniqueManeuverGroups[selectedManeuverGroup]) matches = false;
            }
            if (selectedFocusPosition >= 0 && focusPositionIndex >= 0) {
                if (row[focusPositionIndex] != uniqueFocusPositions[selectedFocusPosition]) matches = false;
            }
            if (selectedEventType >= 0 && eventTypeIndex >= 0) {
                if (row[eventTypeIndex] != uniqueEventTypes[selectedEventType]) matches = false;
            }
            // Set as match if equal or undefined (ex. night means empty cells and night cells are matches, not day cells
            if (selectedTimeType >= 0 && eventTimeIndex >= 0) {
                // Check the selected time type
                std::string selectedTime = uniqueTimeTypes[selectedTimeType];

                // Set the matches based on the selected time type
                if (selectedTime == "Night") {
                    if (row[eventTimeIndex] == "Day") matches = false;
                }
                else if (selectedTime == "Day") {
                    if (row[eventTimeIndex] == "Night") matches = false;
                }
            }

            if (matches) filteredMatrix.push_back(row);
        }

        tempTestPointMatrix = filteredMatrix; // Update the temp matrix with filtered results
        filtersChanged = false; // Reset filtersChanged flag after applying filters
    }

    // Declare a variable to track the survey window state
    static bool showSurveyWindow = false;
    static bool refreshSurveyWindow = false;
    static int currentSurveyRow = -1;

    // Display the test point matrix (original or filtered)
    if (!tempTestPointMatrix.empty()) {
        // Define a fixed width for the child to enable horizontal scrolling
        float childWidth = availableWidth * 0.45f; // Adjust as needed
        float childHeight = 300.0f; // Adjust as needed

        // Create a child window for scrolling
        ImGui::BeginChild("TestPointMatrixScroll", ImVec2(childWidth, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

        // Begin the table with the appropriate flags for scrolling
        ImGui::BeginTable("TestPointMatrixTable",
            static_cast<int>(tempTestPointMatrix[0].size() + 1),
            ImGuiTableFlags_ScrollX | ImGuiTableFlags_Borders); // Extra column for the button

        // Set up the button column first
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 100.0f); // Fixed width for the button column

        // Set the remaining column names
        for (size_t col = 0; col < tempTestPointMatrix[0].size(); ++col) {
            ImGui::TableSetupColumn(tempTestPointMatrix[0][col].c_str(), ImGuiTableColumnFlags_WidthFixed);
        }

        // Create header rows
        ImGui::TableHeadersRow();

        // Populate the table
        for (size_t row = 1; row < tempTestPointMatrix.size(); ++row) {
            ImGui::TableNextRow();

            // Set the column for the button (first column)
            ImGui::TableSetColumnIndex(0); // First column for the button
            std::string buttonLabel = "Take Survey##" + std::to_string(row); // Unique label for each button
            if (ImGui::Button(buttonLabel.c_str())) {
                // Set the current row index and open the survey window
                currentSurveyRow = static_cast<int>(row);
                showSurveyWindow = true;
                refreshSurveyWindow = true;
            }

            // Populate remaining columns
            for (size_t col = 0; col < tempTestPointMatrix[row].size(); ++col) {
                ImGui::TableSetColumnIndex(static_cast<int>(col) + 1); // Adjust for the button column
                ImGui::Text("%s", tempTestPointMatrix[row][col].c_str());
            }
        }

        ImGui::EndTable();
        ImGui::EndChild(); // End the scrollable child
    }
    else {
        ImGui::Text("No data available.");
    }

    ImGui::EndChild();
    ImGui::NewLine();

    // New Survey Window
    if (showSurveyWindow) {
        ImGui::Begin("Survey", &showSurveyWindow);

        // Placeholder for parsed survey data
        static std::vector<std::string> questions1; // Store questions for survey 1
        static std::vector<std::string> options1; // Store options as single strings for survey 1
        static std::vector<std::string> questions2; // Store questions for survey 2
        static std::vector<std::string> options2; // Store options as single strings for survey 2

        static std::string userComment1; // Store user comments for survey 1
        static std::string userComment2; // Store user comments for survey 2

        // If the survey window needs to be refreshed
        if (refreshSurveyWindow) {
            // Clear previous data
            questions1.clear();
            options1.clear();
            userComment1.clear();

            questions2.clear();
            options2.clear();
            userComment2.clear();

            // Get matrix row data
            std::vector<std::string> row_data = tempTestPointMatrix[currentSurveyRow];

            // Get CSV filenames
            std::string survey1Path = "C:/Surveys/" + row_data[12] + ".csv";
            std::string survey2Path = "C:/Surveys/" + row_data[13] + ".csv";

            // Load survey data
            std::vector<std::vector<std::string>> survey1Data;
            std::vector<std::vector<std::string>> survey2Data;

            if (loadCSV(survey1Path, survey1Data) && loadCSV(survey2Path, survey2Data)) {
                // Convert to wide format
                auto survey1Wide = longToWideFormat(survey1Data);
                auto survey2Wide = longToWideFormat(survey2Data);

                // Extract questions and options for survey 1
                for (size_t i = 1; i < survey1Wide.size(); ++i) { // Start from 1 to skip header
                    questions1.push_back(survey1Wide[i][0]); // Column 0: Questions
                    options1.push_back(survey1Wide[i][1]); // Column 1: Options as single string
                }

                // Extract questions and options for survey 2
                for (size_t i = 1; i < survey2Wide.size(); ++i) { // Start from 1 to skip header
                    questions2.push_back(survey2Wide[i][0]);
                    options2.push_back(survey2Wide[i][1]);
                }
            }
            else {
                std::cerr << "Failed to load CSV data." << std::endl;
            }

            refreshSurveyWindow = false; // Reset the refresh flag after loading data
        }

        // Display Survey 1
        ImGui::Text("Survey 1");
        ImGui::Separator();
        for (size_t i = 0; i < questions1.size(); ++i) {
            ImGui::Text(questions1[i].c_str());

            // Split options string by ", "
            std::stringstream ss(options1[i]);
            std::string option;
            int optionIndex = 0;

            // Track selected option for this question
            static int selectedOption1[10] = { -1 }; // Assuming max 10 questions, adjust as necessary

            while (std::getline(ss, option, ',')) {
                option = option.substr(option.find_first_not_of(' ')); // Trim leading spaces
                if (ImGui::RadioButton((option).c_str(), selectedOption1[i] == optionIndex)) {
                    selectedOption1[i] = optionIndex; // Store selected option
                }
                ImGui::SameLine();
                optionIndex++;
            }
        }

        // Comment box for survey 1
        ImGui::InputText("Comment for Survey 1", &userComment1[0], userComment1.size() + 1); // Fixed size and char pointer access

        // Display Survey 2
        ImGui::Text("Survey 2");
        ImGui::Separator();
        for (size_t i = 0; i < questions2.size(); ++i) {
            ImGui::Text(questions2[i].c_str());

            // Split options string by ", "
            std::stringstream ss(options2[i]);
            std::string option;
            int optionIndex = 0;

            // Track selected option for this question
            static int selectedOption2[10] = { -1 }; // Assuming max 10 questions, adjust as necessary

            while (std::getline(ss, option, ',')) {
                option = option.substr(option.find_first_not_of(' ')); // Trim leading spaces
                if (ImGui::RadioButton((option).c_str(), selectedOption2[i] == optionIndex)) {
                    selectedOption2[i] = optionIndex; // Store selected option
                }
                ImGui::SameLine();
                optionIndex++;
            }
        }

        // Comment box for survey 2
        ImGui::InputText("Comment for Survey 2", &userComment2[0], userComment2.size() + 1); // Fixed size and char pointer access

        // Submit button for the survey
        if (ImGui::Button("Submit")) {
            std::ofstream outFile("survey_results.csv", std::ios::app);
            if (outFile.is_open()) {
                outFile << "Survey 1 Comment: " << userComment1 << std::endl;
                outFile << "Survey 2 Comment: " << userComment2 << std::endl;
                outFile.close();
                std::cout << "Survey comments saved successfully!" << std::endl;
            }
            else {
                std::cerr << "Failed to open survey results file for writing." << std::endl;
            }
        }

        ImGui::End(); // End the survey window
    }










}
