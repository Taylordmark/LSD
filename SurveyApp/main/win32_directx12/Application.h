#ifndef APPLICATION_H
#define APPLICATION_H

#include "imgui.h"
#include <vector>
#include <string>

// Enum to define pages in the application
enum class Page {
    Home,
    CreateSurveys,
    CompleteSurveys,
    AnalyzeSurveys
};

namespace MyApp {
    // Declare functions for UI rendering
    void RenderUI();
    void LoadFonts();
    void RenderHomePage();
    void RenderCreateSurveysPage();
    void RenderCompleteSurveysPage();
    void RenderAnalyzeSurveysPage();

    // Declare global variables used across different files
    extern Page currentPage;                   // Declaring currentPage as extern
}

#endif // APPLICATION_H
