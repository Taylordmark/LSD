# LSD
Local Survey Dialogue

Save files to C:/LSD/...

**AppFiles** contains Test Programs, Responses, teamplate data for response typed, metadata

**SurveyApp** contains app files

## To open in editor mode:
1. Open Visual Studio 2022 (ensure c++ development tools are installed)
2. Select "Continue without code ->"
3. Ctrl+Shift+O OR File->Open->Project/Solution
    "C:\LSD\SurveyApp\main\imgui_examples.sln"

## To open and run the app
1. Open Source Files/Application.cpp
2. Start without debugging with play button OR (Ctrl+F5)

## About the app
4 main pages
- **Home Page**\n
  Navigate to different functional pages
- **Create Surveys**\n
  Currently pulls data from "C:\LSD\AppFiles\SurveyQuestionsDemo.csv"\n
  Users can edit survey questions, response types\n
  Submittal creates folder in Test Program folder with all survey data\n
- **Complete Surveys**\n
  Pulls survey from associated test program folder\n
  Displays survey\n
  Submittal creates completed survey file\n
- **Analyze Surveys**\n
  Shows survey completion breakdown
  
  


To Do:
  - **Create Surveys** data pull must use a file selector, not default location\n
  - **Create surveys** submittal should make corresponding subfolder in Responses\n
  - Add metadata filters to **Analyze Surveys** (cockpit position etc)\n
  - Add sentiment analysis of surveys using hugging face pretrained model to **Analyze Surveys**\n
  - Make app look nicer with better fonts, color themes
  - Add data connection to Envision to get flight data for **Complete Surveys** page
  
  
