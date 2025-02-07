#include "Application.h"
#include "imgui.h"
#include "HomePage.h"
#include <filesystem>

namespace MyApp
{
    void RenderHomePage()
    {
        ImGui::Text("Welcome to the Survey App");
        ImGui::NewLine();

        // Check if the folder exists, and create it if it doesn't
        std::string folderPath = "C:/AppFilesJSON";
        if (!std::filesystem::exists(folderPath))
        {
            std::filesystem::create_directory(folderPath);
        }

        // Center the buttons
        float buttonWidth = 200.0f; // Set a width for the buttons
        float windowWidth = ImGui::GetContentRegionAvail().x; // Get available window width
        float posX = (windowWidth - buttonWidth) * 0.5f; // Calculate centered position

        ImGui::NewLine();
        ImGui::NewLine();
        // Create buttons
        ImGui::SetCursorPosX(posX); // Set cursor position for centering
        if (ImGui::Button("Create Surveys", ImVec2(buttonWidth, 0)))
        {
            currentPage = Page::CreateSurveys;
        }

        ImGui::SetCursorPosX(posX); // Reset cursor position for the next button
        if (ImGui::Button("Complete Surveys", ImVec2(buttonWidth, 0)))
        {
            currentPage = Page::CompleteSurveys;
        }

        ImGui::SetCursorPosX(posX); // Reset cursor position for the next button
        if (ImGui::Button("Analyze Surveys", ImVec2(buttonWidth, 0)))
        {
            currentPage = Page::AnalyzeSurveys;
        }
    }
}
