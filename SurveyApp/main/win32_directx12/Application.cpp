#include "Application.h"
#include "imgui.h"
#include <iostream>
#include <cstdlib> // For std::rand() and std::srand()
#include <ctime>   // For std::time()


namespace MyApp {
    // Define the global variables
    Page currentPage = Page::Home;   // Initialize currentPage



    void LoadFonts() {
        // Get the ImGui IO object
        ImGuiIO& io = ImGui::GetIO();

        // Path to the Times New Roman font file
        const char* fontPath = "C:/Users/Work/Dev/ReorganizedApp/Times New Roman.ttf";
        if (io.Fonts->AddFontFromFileTTF(fontPath, 16.0f) == nullptr) {
            std::cerr << "Failed to load font: " << fontPath << std::endl;
        }

        // set this as the default font
        io.FontDefault = io.Fonts->Fonts.back();  // This is the last added font
    }

    void Initialize() {
        // Step 1: Initialize ImGui context
        ImGui::CreateContext();

        // Step 2: Load fonts
        LoadFonts();
    }

    // Function to render the UI
    void RenderUI() {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        ImGui::Begin("Survey App");

        // Page rendering
        switch (currentPage) {
        case Page::Home:
            RenderHomePage();
            break;
        case Page::CreateSurveys:
            RenderCreateSurveysPage();
            break;
        case Page::CompleteSurveys:
            RenderCompleteSurveysPage();
            break;
        case Page::AnalyzeSurveys:
            RenderAnalyzeSurveysPage();
            break;
        }        

        ImGui::End();
    }

    void Cleanup() {
        // Cleanup your application if necessary (e.g., destroying ImGui context, etc.)
        ImGui::DestroyContext();
    }
}
