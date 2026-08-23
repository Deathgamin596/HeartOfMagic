param(
    [string]$preset = "Release-2022",
    [int]$threads = 32,
    [switch]$fresh,
    [string]$Version = "",
    [switch]$NoVersionPrompt
)
$vsDevShellPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
# Load in template default variables
. .\Build_Config_Template.ps1
# Load in local variable overrides
if (Test-Path .\Build_Config_Local.ps1) {
    . .\Build_Config_Local.ps1
}

# =============================================================================
# SEMANTIC VERSION PROMPT
# Updates the VERSION file (single source of truth for CMake / fomod info.xml)
# before the build runs. Supports auto-bump (major/minor/patch), custom entry,
# an explicit -Version override, or -NoVersionPrompt to skip entirely.
# =============================================================================

function Read-SemanticVersion {
    param([string]$Prompt)
    while ($true) {
        $answer = Read-Host $Prompt
        if ($answer -match '^\d+\.\d+\.\d+$') { return $answer }
        if ($answer -eq "") { return $null }
        Write-Warning "Invalid version. Use semantic format, e.g. 1.2.3 (or blank to cancel)."
    }
}

function Update-ProjectVersion {
    $versionFile = Join-Path $PSScriptRoot "VERSION"
    if (-not (Test-Path $versionFile)) {
        Write-Error "VERSION file not found at '$versionFile'"
        exit 1
    }

    $current = (Get-Content $versionFile -Raw).Trim()
    Write-Host "Current version: $current"

    # Explicit override via -Version
    if ($Version -ne "") {
        if ($Version -match '^\d+\.\d+\.\d+$') {
            if ($Version -ne $current) {
                Set-Content -Path $versionFile -Value $Version
                Write-Host "Version set to $Version"
            } else {
                Write-Host "Version already $Version"
            }
            return
        }
        Write-Warning "Provided -Version '$Version' is not a valid semantic version; falling back to prompt."
    }

    if ($NoVersionPrompt) { return }

    # If current isn't semantic, can't auto-bump - ask for manual entry
    if ($current -notmatch '^(\d+)\.(\d+)\.(\d+)$') {
        Write-Warning "Current version '$current' is not semantic; cannot auto-bump."
        $custom = Read-SemanticVersion "Enter new version (blank to keep '$current')"
        if ($custom -and $custom -ne $current) {
            Set-Content -Path $versionFile -Value $custom
            Write-Host "Version set to $custom"
        }
        return
    }

    $maj = [int]$Matches[1]
    $min = [int]$Matches[2]
    $pat = [int]$Matches[3]

    Write-Host "Select version update:"
    Write-Host "  1) Keep    ($current)"
    Write-Host ("  2) Patch   ({0}.{1}.{2})" -f $maj, $min, ($pat + 1))
    Write-Host ("  3) Minor   ({0}.{1}.0)" -f $maj, ($min + 1))
    Write-Host ("  4) Major   ({0}.0.0)" -f ($maj + 1))
    Write-Host "  5) Custom"
    $sel = Read-Host "Choice [1]"

    $new = $null
    switch ($sel) {
        ""  { }
        "1" { }
        "2" { $new = ("{0}.{1}.{2}" -f $maj, $min, ($pat + 1)) }
        "3" { $new = ("{0}.{1}.0" -f $maj, ($min + 1)) }
        "4" { $new = ("{0}.0.0" -f ($maj + 1)) }
        "5" { $new = Read-SemanticVersion "Enter custom version" }
        default { Write-Host "Invalid choice - keeping $current" }
    }

    if ($new -and $new -ne $current) {
        Set-Content -Path $versionFile -Value $new
        Write-Host ("Version updated: {0} -> {1}" -f $current, $new)
    } else {
        Write-Host "Version unchanged ($current)"
    }
}

Update-ProjectVersion

Write-Host "Running preset $preset"

# Set up Visual Studio 2022 x64 environment
# Override with env var if set
if ($env:VS_DEV_SHELL_PATH) {
    if (Test-Path $env:VS_DEV_SHELL_PATH) {
        $vsDevShellPath = $env:VS_DEV_SHELL_PATH
        Write-Host "Using VS Dev Shell path from environment: $vsDevShellPath"
    }
}
# Verify the path exists
if (-not (Test-Path $vsDevShellPath)) {
    Write-Error "Visual Studio Dev Shell script not found at '$vsDevShellPath'. Please check the path."
    exit 1
}
# Save current directory, launch VS dev shell, and return to original directory
$currentDirectory = $PWD.Path
& $vsDevShellPath -Arch amd64; Set-Location -Path "${currentDirectory}"


# Build cmake configure arguments
$cmakeArgs = @("-S", ".", "--preset=$preset", "-DCMAKE_COMPILE_JOBS=$threads", "-Wno-dev")
if ($fresh) {
    $cmakeArgs += "--fresh"
}

& cmake $cmakeArgs
if ($LASTEXITCODE -ne 0) { exit 1 }

& cmake --build --preset=$preset --parallel $threads
if ($LASTEXITCODE -ne 0) { exit 1 }
