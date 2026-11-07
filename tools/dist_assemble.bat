@echo off
setlocal

REM dist_assemble.bat - Assembles the release distribution folder
REM Usage: dist_assemble.bat <BUILD_DIR> <DIST_VERSION_DIR> <DIST_DIR> <CMAKE_SOURCE_DIR> <PAPYRUS_OUTPUT_DIR> <DLL_SPELLLEARNING> <DLL_SL_BOOKXP> <DLL_DONT_EAT> <CMAKE_COMMAND>

set "BUILD_DIR=%~1"
set "DIST_VERSION_DIR=%~2"
set "DIST_DIR=%~3"
set "CMAKE_SOURCE_DIR=%~4"
set "PAPYRUS_OUTPUT_DIR=%~5"
set "DLL_SPELLLEARNING=%~6"
set "DLL_SL_BOOKXP=%~7"
set "DLL_DONT_EAT=%~8"
set "CMAKE_EXE=%~9"

REM Convert forward slashes to backslashes for Windows shell compatibility
set "BUILD_DIR=%BUILD_DIR:/=\%"
set "DIST_VERSION_DIR=%DIST_VERSION_DIR:/=\%"
set "DIST_DIR=%DIST_DIR:/=\%"
set "CMAKE_SOURCE_DIR=%CMAKE_SOURCE_DIR:/=\%"
set "PAPYRUS_OUTPUT_DIR=%PAPYRUS_OUTPUT_DIR:/=\%"
set "DLL_SPELLLEARNING=%DLL_SPELLLEARNING:/=\%"
set "DLL_SL_BOOKXP=%DLL_SL_BOOKXP:/=\%"
set "DLL_DONT_EAT=%DLL_DONT_EAT:/=\%"

set "DIST_ZIP=%DIST_DIR%\%~nx2.zip"

echo ============================================
echo Assembling release distribution
echo   Output: %DIST_VERSION_DIR%
echo   Archive: %DIST_ZIP%
echo ============================================

REM 1. Clean and create directory structure
echo [1/7] Creating directory structure...
if exist "%DIST_VERSION_DIR%" rmdir /s /q "%DIST_VERSION_DIR%"
if not exist "%DIST_VERSION_DIR%\fomod" mkdir "%DIST_VERSION_DIR%\fomod"
if not exist "%DIST_VERSION_DIR%\SKSE\Plugins" mkdir "%DIST_VERSION_DIR%\SKSE\Plugins"
if not exist "%DIST_VERSION_DIR%\Scripts\Source" mkdir "%DIST_VERSION_DIR%\Scripts\Source"
if not exist "%DIST_VERSION_DIR%\optional\ISLPatch\SKSE\Plugins" mkdir "%DIST_VERSION_DIR%\optional\ISLPatch\SKSE\Plugins"
if not exist "%DIST_VERSION_DIR%\optional\ISLPatch\Scripts\Source" mkdir "%DIST_VERSION_DIR%\optional\ISLPatch\Scripts\Source"

REM 2. Copy DLLs
echo [2/7] Copying DLLs...
copy /y "%DLL_SPELLLEARNING%" "%DIST_VERSION_DIR%\SKSE\Plugins\"
copy /y "%DLL_SL_BOOKXP%" "%DIST_VERSION_DIR%\SKSE\Plugins\"
copy /y "%DLL_DONT_EAT%" "%DIST_VERSION_DIR%\optional\ISLPatch\SKSE\Plugins\"

REM 3. Copy fomod files
echo [3/7] Copying fomod files...
copy /y "%CMAKE_SOURCE_DIR%\fomod\info.xml" "%DIST_VERSION_DIR%\fomod\"
copy /y "%CMAKE_SOURCE_DIR%\fomod\ModuleConfig.xml" "%DIST_VERSION_DIR%\fomod\"

REM 4. Copy SKSE runtime data
echo [4/7] Copying SKSE runtime data...
if exist "%CMAKE_SOURCE_DIR%\SKSE\Plugins\SpellLearning" (
    "%CMAKE_EXE%" -E copy_directory "%CMAKE_SOURCE_DIR%\SKSE\Plugins\SpellLearning" "%DIST_VERSION_DIR%\SKSE\Plugins\SpellLearning"
)

REM 5. Copy PrismaUI views
echo [5/7] Copying PrismaUI views...
"%CMAKE_EXE%" -E copy_directory "%CMAKE_SOURCE_DIR%\PrismaUI\views" "%DIST_VERSION_DIR%\PrismaUI\views"

REM 6. Copy Papyrus scripts
echo [6/7] Copying Papyrus scripts...
if exist "%CMAKE_SOURCE_DIR%\Scripts\Source\*.psc" copy /y "%CMAKE_SOURCE_DIR%\Scripts\Source\*.psc" "%DIST_VERSION_DIR%\Scripts\Source\"
if exist "%PAPYRUS_OUTPUT_DIR%\*.pex" copy /y "%PAPYRUS_OUTPUT_DIR%\*.pex" "%DIST_VERSION_DIR%\Scripts\"
if exist "%CMAKE_SOURCE_DIR%\Scripts\Source\*.psc" copy /y "%CMAKE_SOURCE_DIR%\Scripts\Source\*.psc" "%DIST_VERSION_DIR%\optional\ISLPatch\Scripts\Source\"
if exist "%PAPYRUS_OUTPUT_DIR%\*.pex" copy /y "%PAPYRUS_OUTPUT_DIR%\*.pex" "%DIST_VERSION_DIR%\optional\ISLPatch\Scripts\"

REM 7. Exclude unnecessary files and create zip
echo [7/7] Cleaning up and creating archive...
if exist "%DIST_VERSION_DIR%\SKSE\Plugins\SpellLearning.pdb" del /f /q "%DIST_VERSION_DIR%\SKSE\Plugins\SpellLearning.pdb"
if exist "%DIST_VERSION_DIR%\SKSE\Plugins\SL_BookXP.pdb" del /f /q "%DIST_VERSION_DIR%\SKSE\Plugins\SL_BookXP.pdb"
if exist "%DIST_VERSION_DIR%\optional\ISLPatch\SKSE\Plugins\DontEatSpellTomes.pdb" del /f /q "%DIST_VERSION_DIR%\optional\ISLPatch\SKSE\Plugins\DontEatSpellTomes.pdb"

REM Remove dev-only files from PrismaUI views
if exist "%DIST_VERSION_DIR%\launch-dev-harness.bat" del /f /q "%DIST_VERSION_DIR%\launch-dev-harness.bat"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\launch-dev-harness.bat" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\launch-dev-harness.bat"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\dev-harness.html" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\dev-harness.html"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\dev-harness-bridge.js" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\dev-harness-bridge.js"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\dev-harness-toolbar.js" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\dev-harness-toolbar.js"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\browser-test.html" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\browser-test.html"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\test-runner.html" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\test-runner.html"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\wheel-test.html" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\wheel-test.html"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\config-demo.html" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\config-demo.html"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\real-data-sim.html" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\real-data-sim.html"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\tree-preview.html" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\tree-preview.html"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\run-tests.js" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\run-tests.js"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\modules\autoTest.js" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\modules\autoTest.js"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\modules\unificationTest.js" del /f /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\modules\unificationTest.js"
if exist "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\test-data" rmdir /s /q "%DIST_VERSION_DIR%\PrismaUI\views\SpellLearning\SpellLearningPanel\test-data"

cd /d "%DIST_DIR%"
"%CMAKE_EXE%" -E tar cf "%DIST_ZIP%" --format=zip -- "%~nx2"

echo ============================================
echo Distribution assembly complete.
echo ============================================
endlocal
