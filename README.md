# LSD
**Local Survey Dialogue**


## To download and run the application
Save AppFiles folder to C:/LSD/
Full path should now be C:/LSD/AppFiles/

Save LSD.exe anywhere

Clicking on the LSD.exe file should open the app and all functionality should be available

## Creating a new Test Program
1. Open application
2. Select Create Surveys page
3. Click on Select Surveys csv file
4. Select the csv file which contains test questions
    Format of file must match exactly with four columns
       [Test Event, MOPS, Question, Response Type]
   Test Event must have the proper format
   (V for Vary or other single letter)_(COI #)_(MOE #)_(Design Point #)_(Survey Count / Nomenclature)
   Response Type may be empty
5.  Make any required changes to the survey in the GUI
6.  Enter Test Program Name and the bottom (ex. T7A)
7.  Click the populate surveys button

## Completing a Survey
1. Select Test Program
2. Select test event or use the test event filter no narrow down the list then select test event
3. Fill in metadata for user ID, flight times, etc
4. Complete actual survey
5. Click Submit button

## Analyze Surveys
Select Test Program
- Left side shows survey completion
- Selecting a COI, MOE shows completion of the specific selected COI by MOE or MOE by design point, respectively
- Right side shows survey filters - currently INOP
- Open plot window after selecting COI and MOE shows all surveys for said MOE
- Showing matching responses shows all individual surveys where filter criteria matches (filters currently INOP but matching responses display properly when no filters selected)
   
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
  
  
