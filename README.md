# LSD
**Local Survey Dialogue**

Save files to C:/LSD/...

**AppFiles** contains Test Programs, Responses, teamplate data for response typed, metadata

**SurveyApp** contains app code

## To open in editor mode:
1. Open Visual Studio 2022 (ensure c++ development tools are installed)
2. Select "Continue without code ->"
3. Ctrl+Shift+O OR File->Open->Project/Solution
    "C:\LSD\SurveyApp\main\imgui_examples.sln"

## To open and run the app
1. Open Source Files/Application.cpp in Visual Studio 2022
2. Start without debugging with play button OR (Ctrl+F5)

Updated version of app is available as an .exe file after running at ""C:\LSD\SurveyApp\main\win32_directx12\Debug\win32_directx12.exe""

## About the app

- **Home Page**
  -  Navigate to different functional pages
- **Create Surveys**
  -  Currently pulls data from "C:\LSD\AppFiles\SurveyQuestionsDemo.csv"
  -  Users can edit survey questions, response types
  -  Submittal creates folder in Test Program folder with all survey data
- **Complete Surveys**
  -  Pulls survey from associated test program folder
  -  Displays survey
  -  Submittal creates completed survey file
- **Analyze Surveys**
  -  Shows survey completion breakdown
  
  


## To Do:
### Primary
  - Add sentiment analysis of surveys using hugging face pretrained model to **Analyze Surveys**
  - Add data connection to Envision to get flight data for **Complete Surveys** page
  - Add ability to take in quantitative data in addition to typical survey results
   - This can be doene using a "fill in the blank" option in the responseTypes.json / dropdown options
### Secondary
  - Make app look nicer with better fonts, color themes
  - Add an "Export to CSV" button in the **Analyze Surveys** page that outputs all data in a similar format to the input csv in addition to responses
  - Add all factors and levels to **Create Surveys** page - from .csv file upload
    - .csv file should have all of this data and when file is uploaded, fill test program with Factors and Levels for filtering in analysis page
  
  
