#include "Application.h"
#include "imgui.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib> // For std::rand() and std::srand()
#include <ctime>   // For std::time()
#include <filesystem> // For filesystem operations
#include <iostream>   // For standard I/O
#include "CreateSurveysPage.h"

void SaveMatrixToCSV(const std::vector<std::vector<std::string>>& matrix, const std::string& filePath) {
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

void MyApp::RenderCreateSurveysPage()
{
    ImGui::Text("Create Surveys\n");
    ImGui::NewLine();


    // Input fields for filtering
    ImGui::Text("Filter Test Point Matrix for Current Test Points:");

    ImGui::PushItemWidth(100); // Set the width of the input text boxes to 100 pixels

    ImGui::Separator();
    ImGui::Text("Current Test Points");
    ImGui::NewLine();

    // Create table columns
    ImGui::Columns(4, "CTP_Table", true); // Change to 4 columns
    ImGui::SetColumnWidth(0, 100); // Adjust column width as needed
    ImGui::Text("COI");
    ImGui::NextColumn();
    ImGui::SetColumnWidth(1, 150); // Adjust column width as needed
    ImGui::Text("Operation");
    ImGui::NextColumn();
    ImGui::SetColumnWidth(2, 150); // Adjust column width as needed
    ImGui::Text("Design Point");
    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::Text("Actions");
    ImGui::NextColumn();
    ImGui::Separator();

    // Render survey entries with index and remove button

    for (std::size_t i = 0; i < testPoints.size(); ++i)
    {

        ImGui::Text("%s", testPoints[i].coi.c_str());
        ImGui::NextColumn();
        ImGui::Text("%s", testPoints[i].operation.c_str());
        ImGui::NextColumn();
        ImGui::Text("%s", testPoints[i].designPoint.c_str());
        ImGui::NextColumn();

        // Remove button
        std::string buttonLabel = "Remove Point##" + std::to_string(i);
        if (ImGui::Button(buttonLabel.c_str()))
        {
            // Remove the entry from the vector
            testPoints.erase(testPoints.begin() + i);

            // Note: Do not increment index as we are already removing the entry
            --i; // Adjust index since we removed an item
        }
        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    // Input fields for adding new tes points
    ImGui::PushItemWidth(75); // Set the width of the input text boxes

    static char newCOI[128] = "";
    ImGui::InputText("COI", newCOI, IM_ARRAYSIZE(newCOI));
    ImGui::SameLine();

    static char newOperation[128] = "";
    ImGui::InputText("Operation", newOperation, IM_ARRAYSIZE(newOperation));
    ImGui::SameLine();

    static char newDesignPoint[128] = "";
    ImGui::InputText("Design Point", newDesignPoint, IM_ARRAYSIZE(newDesignPoint));
    ImGui::SameLine();

    if (ImGui::Button("Add Test Point"))
    {
        if (newCOI[0] != '\0' && newOperation[0] != '\0' && newDesignPoint[0] != '\0')
        {
            testPoints.push_back({ newCOI, newOperation, newDesignPoint });
        }
    }
    ImGui::PopItemWidth(); // Restore the default width
    ImGui::NewLine();



    // 2. Test Point Matrix Path
    ImGui::PushItemWidth(500);
    ImGui::Text("Please paste the file location of the Test Point Matrix Here");
    static char surveyMatrix[256] = ""; // Buffer for the input text
    ImGui::InputText("##SurveyMatrix", surveyMatrix, IM_ARRAYSIZE(surveyMatrix));
    ImGui::NewLine();

    // 3. MOP Surveys Folder Path
    ImGui::Text("Please paste the folder location of the MOP Surveys Here");
    static char surveysFolder[256] = ""; // Buffer for the input text
    ImGui::InputText("##SurveysFolder", surveysFolder, IM_ARRAYSIZE(surveysFolder));
    ImGui::NewLine();

    ImGui::PopItemWidth();

    // 4. Load and filter test point matrix
    // When clicking the submit button check for file load success and apply filtering
    if (ImGui::Button("Submit"))
    {
        std::ifstream file(surveyMatrix);
        if (file.is_open())
        {
            fileLoadStatus = "File loaded successfully!";
            testPointMatrix.clear(); // Clear previous data
            std::string line;

            // Read the file content
            while (std::getline(file, line))
            {
                std::vector<std::string> row;
                std::stringstream ss(line);
                std::string cell;
                while (std::getline(ss, cell,
                    ',')) // Assuming CSV format
                {
                    row.push_back(cell);
                }
                testPointMatrix.push_back(row);
            }

            file.close();


            // Filter the matrix based on COI, Operation, and Design Point inputs
            if (!testPointMatrix.empty())
            {
                // Get the headers
                const auto& headers = testPointMatrix[0];
                int coiIndex, operationIndex, designPointIndex;

                // Find indices of relevant columns
                for (int i = 0; i < headers.size(); ++i)
                {
                    if (headers[i] == "﻿COI") coiIndex = i;
                    else if (headers[i] == "Operation") operationIndex = i;
                    else if (headers[i] == "Design Point") designPointIndex = i;
                }

                // Create empty matrix
                std::vector<std::vector<std::string>> filteredMatrix;
                filteredMatrix.push_back(headers); // Keep the headers


                //First iterate through rows of the included test points, then test point matrix to get matches
                for (const auto& point : testPoints) {
                    std::string coiFilter = point.coi;
                    std::string operationFilter = point.operation;
                    std::string designPointFilter = point.designPoint;

                    // Iterate through rows of the test point matrix and get match
                    for (size_t i = 1; i < testPointMatrix.size(); ++i) // Skip the header row when reading data
                    {
                        const auto& row = testPointMatrix[i];
                        bool matches = true;

                        auto coi_ = row[coiIndex];
                        if (coi_ != coiFilter) matches = false;
                        auto opf_ = row[operationIndex];
                        if (opf_ != operationFilter) matches = false;
                        auto dpi_ = row[designPointIndex];
                        if (dpi_ != designPointFilter) matches = false;

                        // If all conditions match, add the row to the filtered matrix
                        if (matches) filteredMatrix.push_back(row);
                    }
                }
                testPointMatrix = filteredMatrix;
            }
        }
        else
        {
            fileLoadStatus = "Failed to load the file.";
        }
    }

    // 5. Test Point Matrix Window
    if (ImGui::Button("Show Test Point Matrix"))
    {
        // Open a new ImGui window
        ImGui::OpenPopup("TestPointMatrixPopup");
    }

    // Handle the popup window
    if (ImGui::BeginPopupModal("TestPointMatrixPopup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Test Point Matrix");

        // Define the height and width of the scrollable region
        const float tableHeight = 600.0f; // Increased height (original was 300.0f)
        const float tableWidth = 1200.0f; // Increased width

        if (!testPointMatrix.empty())
        {
            ImGui::BeginChild("TestPointMatrixScroll", ImVec2(tableWidth, tableHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

            ImGui::BeginTable("TestPointMatrixTable", testPointMatrix[0].size());

            // Set column names, including "User ID" if necessary
            for (int col = 0; col < testPointMatrix[0].size(); ++col)
            {
                ImGui::TableSetupColumn(testPointMatrix[0][col].c_str(), ImGuiTableColumnFlags_WidthStretch);
            }

            ImGui::TableHeadersRow();

            // Populate the table
            for (size_t row = 1; row < testPointMatrix.size(); ++row)
            {
                ImGui::TableNextRow();
                for (int col = 0; col < testPointMatrix[row].size(); ++col)
                {
                    ImGui::TableSetColumnIndex(col);
                    ImGui::Text("%s", testPointMatrix[row][col].c_str());
                }
            }

            ImGui::EndTable();
            ImGui::EndChild();
        }
        else
        {
            ImGui::Text("No data available.");
        }

        // Save button
        if (ImGui::Button("Save to CSV"))
        {
            // Determine the Downloads folder path based on surveyMatrix parent directory
            std::string surveyMatrixPath(surveyMatrix);
            std::filesystem::path downloadsPath = std::filesystem::path(surveyMatrixPath).parent_path();
            std::string filePath = (downloadsPath / "TestPointsAssigned.csv").string();

            // Save the matrix to CSV
            SaveMatrixToCSV(testPointMatrix, filePath);

            ImGui::Text("Saved to: %s", filePath.c_str());
        }

        // Close button
        if (ImGui::Button("Close"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ImGui::NewLine();

    // Display file load status
    ImGui::Text("%s", fileLoadStatus.c_str());

    // 4. Create New Users
    ImGui::Separator();
    ImGui::Text("Survey Group Table");
    ImGui::NewLine();

    // Create table columns
    ImGui::Columns(4, "SurveyTable", true); // Change to 4 columns
    ImGui::SetColumnWidth(0, 100); // Adjust column width as needed
    ImGui::Text("Index");
    ImGui::NextColumn();
    ImGui::SetColumnWidth(1, 150); // Adjust column width as needed
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::SetColumnWidth(2, 150); // Adjust column width as needed
    ImGui::Text("Rank");
    ImGui::NextColumn();
    ImGui::SetColumnWidth(3, 100); // Adjust column width as needed
    ImGui::Text("Actions");
    ImGui::NextColumn();
    ImGui::Separator();

    // Render survey entries with index and remove button
    int survey_index = 1;
    for (std::size_t i = 0; i < surveyEntries.size(); ++i)
    {
        ImGui::Text("%d", survey_index++);
        ImGui::NextColumn();
        ImGui::Text("%s", surveyEntries[i].name.c_str());
        ImGui::NextColumn();
        ImGui::Text("%s", surveyEntries[i].rank.c_str());
        ImGui::NextColumn();

        // Remove button
        std::string buttonLabel = "Remove##" + std::to_string(i);
        if (ImGui::Button(buttonLabel.c_str()))
        {
            // Remove the entry from the vector
            surveyEntries.erase(surveyEntries.begin() + i);

            // Note: Do not increment index as we are already removing the entry
            --i; // Adjust index since we removed an item
        }
        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    // Input fields for adding new users

    ImGui::PushItemWidth(200); // Set the width of the input text boxes to 100 pixels

    static char newName[128] = "";
    static char newRank[128] = "";

    ImGui::InputText("New Name", newName, IM_ARRAYSIZE(newName));
    ImGui::SameLine();
    ImGui::InputText("New Rank", newRank, IM_ARRAYSIZE(newRank));

    if (ImGui::Button("Add Entry"))
    {
        if (newName[0] != '\0' && newRank[0] != '\0')
        {
            surveyEntries.push_back({ newName, newRank });
            newName[0] = '\0'; // Clear input fields after adding
            newRank[0] = '\0';
        }
    }
    ImGui::PopItemWidth(); // Restore the default width
    ImGui::NewLine();

    // 5. Assign Random User IDs
    ImGui::Separator();
    if (ImGui::Button("Assign Random User IDs"))
    {
        bool hasUserIdColumn = false;

        if (surveyEntries.empty()) {
            ImGui::Text("No users to assign.");
        }

        if (testPointMatrix.empty()) {
            ImGui::Text("The test point matrix is empty.");
        }

        else {
            std::srand(static_cast<unsigned>(std::time(0))); // Seed the random number generator
            size_t numUsers = surveyEntries.size();

            if (std::all_of(testPointMatrix[0].back().begin(), testPointMatrix[0].back().end(), [](char c) { return std::isdigit(c); })) {
                hasUserIdColumn = true;
            }

            // Check if User ID column already exists
            std::string lastColumnLabel = testPointMatrix[0][testPointMatrix[0].size() - 1];
            if (lastColumnLabel == "User ID") {
                hasUserIdColumn = true;
            }

            // Add a new column if there isn't already a User ID column
            if (!hasUserIdColumn) {
                for (auto& row : testPointMatrix) {
                    if (row.size() > 0) {
                        int randomUserID = std::rand() % static_cast<int>(numUsers);
                        row.push_back(std::to_string(randomUserID));
                    }
                }
            }

            // If User ID column exists, replace it
            if (hasUserIdColumn) {
                for (auto& row : testPointMatrix) {
                    if (row.size() > 0) {
                        int randomUserID = std::rand() % static_cast<int>(numUsers);
                        row.back() = std::to_string(randomUserID); // Replace the last element
                    }
                }
            }

            // Set the label of the new column to "User ID"
            testPointMatrix[0][testPointMatrix[0].size() - 1] = "User ID";

        }
    }
    ImGui::NewLine();


    // 7. Download Users Matrix
    // Download button for surveyEntries
    if (ImGui::Button("Download Users Table")) {
        std::vector<std::vector<std::string>> surveyEntriesMatrix;
        int usridx = 0;
        for (const auto& entry : surveyEntries) {
            std::vector<std::string> row = { std::to_string(usridx++), entry.name, entry.rank };
            surveyEntriesMatrix.push_back(row);
        }

        // Determine the Downloads folder path based on surveyMatrix parent directory
        std::string surveyMatrixPath(surveyMatrix);
        std::filesystem::path downloadsPath = std::filesystem::path(surveyMatrixPath).parent_path();
        std::string filePath = (downloadsPath / "SurveyUsers.csv").string();

        // Save the survey entries to CSV
        SaveMatrixToCSV(surveyEntriesMatrix, filePath);

        ImGui::Text("Saved to: %s", filePath.c_str());
    }
    ImGui::NewLine();
    ImGui::AlignTextToFramePadding();
    if (ImGui::Button("Back to Home")) {
        currentPage = Page::Home;
    }
}








