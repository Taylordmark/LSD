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
   Test Event must have the proper format with each of the following in the proper order and separated by an underscore "_"
   - V for Vary or other single letter representing alternative type
   - COI #
   - MOE #
   - Design Point #
   - Survey Count / Nomenclature
   Response Type may be empty
6.  Make any required changes to the survey in the GUI
7.  Enter Test Program Name and the bottom (ex. T7A)
8.  Click the populate surveys button

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
  - Fix plots to display again
  - Check that time filters work properly for min / max values
  - Add filters for date
  - Add error handling for when MOE not selected but plots displayed (or just plots labelled with COI, MOE, etc if not selected)
  - Add a csv export option that combines all data for all filtered responses with metadata, response value, response value as text, etc.
  - Add sentiment analysis of surveys using hugging face pretrained model to **Analyze Surveys**
  - Add data connection to Envision to get flight data for **Complete Surveys** page
  - Add ability to take in quantitative data in addition to typical survey results
   - This can be doene using a "fill in the blank" option in the responseTypes.json / dropdown options
   - Make a "CSV compiler" that takes in all data from various csv exports and compiles them into a single csv for Qlik analysis - primary feature here is avoiding duplicates which should be pretty easy given the naming convention
   - Change survey structure such that they are divided by test event rather than MOP
       - Response files will probably not be divided by MOP anymore
   - Make response csv save responses as text rather than index number
   - Formulas for Mx data in **Analyze Surveys**

### Secondary
  - Make app look nicer with better fonts, color themes
  - Add an "Export to CSV" button in the **Analyze Surveys** page that outputs all data in a similar format to the input csv in addition to responses
  - Add all factors and levels to **Create Surveys** page - from .csv file upload
    - .csv file should have all of this data and when file is uploaded, fill test program with Factors and Levels for filtering in analysis page
  - With plot summaries, show which test events have been completed and which have not, ex test event 1 is at 2/4 complete
  - Add tracker for if responses have been QC'd for data type, validity, if comments match responses, etc
  - Allow for N/A responses to save as N/A rather than 0

## Recent Changes and Additions Made (April 2025 - June 2025):
  - Added function in CompleteSurveys.cpp to create CSV outputs with survey responses in same place as json responses
  -  Added response types, such as string and Likert 5 Point Satisfactory in responsetypes_template.json
  -  Functionality to allow for "fill in the blank" type responses to appear in the surveys
  -  Improved CSV parsing allowing survey questions to contain commas
  -  Standalone CSV compiler written in python. Contact Jason Medeiros (kaulanaopuuokalani@gmail.com) for more information
  -  Python script to populate survey questions. Contact Jason Medeiros for more informaton
