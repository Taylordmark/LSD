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
        float buttonWidth = 400.0f;  // Make buttons twice as wide as the original width (200.0f)
        float buttonHeight = 60.0f;  // Make buttons twice as tall as the default (30.0f)
        float windowWidth = ImGui::GetContentRegionAvail().x; // Get available window width
        float posX = (windowWidth - buttonWidth) * 0.5f; // Calculate centered position

        ImGui::NewLine();
        ImGui::NewLine();

        // Set button style for rounded edges (optional)
        ImGui::GetStyle().FrameRounding = 12.0f;  // Set rounding for the edges (increase for more rounded)

        // Create buttons
        ImGui::SetCursorPosX(posX); // Set cursor position for centering
        if (ImGui::Button("Create Surveys", ImVec2(buttonWidth, buttonHeight))) // Set custom size
        {
            currentPage = Page::CreateSurveys;
        }
        ImGui::NewLine();
        ImGui::NewLine();

        ImGui::SetCursorPosX(posX); // Reset cursor position for the next button
        if (ImGui::Button("Complete Surveys", ImVec2(buttonWidth, buttonHeight))) // Set custom size
        {
            currentPage = Page::CompleteSurveys;
        }
        ImGui::NewLine();
        ImGui::NewLine();

        ImGui::SetCursorPosX(posX); // Reset cursor position for the next button
        if (ImGui::Button("Analyze Surveys", ImVec2(buttonWidth, buttonHeight))) // Set custom size
        {
            currentPage = Page::AnalyzeSurveys;
        }
        ImGui::NewLine();
        ImGui::NewLine();

        // Optionally reset frame rounding to default
        ImGui::GetStyle().FrameRounding = 0.0f;  // Reset rounding to default if needed for other UI elements
    }
}
