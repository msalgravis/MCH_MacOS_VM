param(
    [string]$Owner = "msalgravis",
    [string]$Repo = "MCH_MacOS_VM",
    [string]$WorkflowFile = "macos-build.yml",
    [string]$Ref = "main",
    [int]$PollSeconds = 300,
    [switch]$DownloadArtifacts,
    [string]$ArtifactNameLike = "macos-plugins-*"
)

$ErrorActionPreference = "Stop"

function Get-GhPath {
    $candidates = @(
        "gh.exe",
        "$env:ProgramFiles\\GitHub CLI\\gh.exe",
        "$env:ProgramFiles(x86)\\GitHub CLI\\gh.exe",
        "$env:LOCALAPPDATA\\Programs\\GitHub CLI\\gh.exe"
    )

    foreach ($candidate in $candidates) {
        try {
            $resolved = Get-Command $candidate -ErrorAction Stop
            if ($resolved.Path) { return $resolved.Path }
        } catch {
        }

        if (Test-Path $candidate) { return $candidate }
    }

    throw "GitHub CLI (gh) not found. Install with: winget install --id GitHub.cli -e"
}

$gh = Get-GhPath
Write-Host "Using gh executable: $gh"

# Verify authenticated session for github.com without printing token value.
$authOutput = & $gh auth status 2>&1
if ($LASTEXITCODE -ne 0) {
    throw "GitHub CLI is not authenticated. Run: gh auth login"
}

if ($authOutput -notmatch "Logged in to github.com") {
    throw "GitHub CLI auth status is not healthy. Run: gh auth login"
}

Write-Host "GitHub CLI auth is valid."

# Dispatch workflow.
$dispatchPath = "repos/$Owner/$Repo/actions/workflows/$WorkflowFile/dispatches"
& $gh api -X POST $dispatchPath -f ref=$Ref | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Failed to dispatch workflow $WorkflowFile on $Ref"
}

Write-Host "Dispatched workflow $WorkflowFile on ref $Ref"

# Find newest workflow_dispatch run for this workflow/ref.
$runsPath = "repos/$Owner/$Repo/actions/workflows/$WorkflowFile/runs?branch=$Ref&event=workflow_dispatch&per_page=1"
$runsJson = & $gh api $runsPath
if ($LASTEXITCODE -ne 0) {
    throw "Failed to fetch workflow runs"
}

$runs = $runsJson | ConvertFrom-Json
if (-not $runs.workflow_runs -or $runs.workflow_runs.Count -lt 1) {
    throw "No workflow_dispatch run found after dispatch"
}

$run = $runs.workflow_runs[0]
$runId = [int64]$run.id
Write-Host "Run id: $runId"
Write-Host "Run URL: $($run.html_url)"

while ($true) {
    $runJson = & $gh api "repos/$Owner/$Repo/actions/runs/$runId"
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to poll run $runId"
    }

    $runState = $runJson | ConvertFrom-Json
    $stamp = Get-Date -Format s
    Write-Host "[$stamp] status=$($runState.status) conclusion=$($runState.conclusion)"

    if ($runState.status -eq "completed") {
        if ($runState.conclusion -ne "success") {
            throw "Workflow run failed. conclusion=$($runState.conclusion), url=$($runState.html_url)"
        }

        Write-Host "Workflow run completed successfully."

        if ($DownloadArtifacts) {
            $artifactsJson = & $gh api "repos/$Owner/$Repo/actions/runs/$runId/artifacts"
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to fetch artifacts for run $runId"
            }

            $artifacts = ($artifactsJson | ConvertFrom-Json).artifacts
            $match = $artifacts | Where-Object { $_.name -like $ArtifactNameLike -and -not $_.expired } | Select-Object -First 1
            if (-not $match) {
                throw "No non-expired artifact matching '$ArtifactNameLike'"
            }

            $targetDir = Join-Path -Path (Get-Location) -ChildPath "tools/github-actions"
            if (-not (Test-Path $targetDir)) {
                New-Item -Path $targetDir -ItemType Directory | Out-Null
            }

            $zipPath = Join-Path $targetDir "$($match.name)-run$runId.zip"
            $extractDir = Join-Path $targetDir "$($match.name)-run$runId"

            if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
            if (Test-Path $extractDir) { Remove-Item $extractDir -Recurse -Force }

            $downloadUrl = "repos/$Owner/$Repo/actions/artifacts/$($match.id)/zip"
            & $gh api $downloadUrl --output $zipPath
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to download artifact zip"
            }

            Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force

            Write-Host "Artifact ZIP: $zipPath"
            Write-Host "Artifact Extracted: $extractDir"
        }

        break
    }

    Start-Sleep -Seconds $PollSeconds
}
