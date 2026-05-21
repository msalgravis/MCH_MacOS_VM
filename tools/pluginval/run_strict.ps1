$ErrorActionPreference = 'Stop'

$pluginval = Join-Path $PSScriptRoot 'pluginval.exe'
$plugin = Join-Path $PSScriptRoot '..\..\build\BinauralSpeakerRoom_artefacts\Release\VST3\Sonarworks VMPRO MCH.vst3'
$plugin = [System.IO.Path]::GetFullPath($plugin)

if (-not (Test-Path $pluginval)) {
    Write-Error "pluginval not found at: $pluginval"
}

if (-not (Test-Path $plugin)) {
    Write-Error "Plugin artifact not found at: $plugin"
}

& $pluginval --strictness-level 10 --repeat 1 --timeout-ms 120000 $plugin
$exitCode = $LASTEXITCODE
Write-Output "PLUGINVAL_EXIT:$exitCode"
exit $exitCode
