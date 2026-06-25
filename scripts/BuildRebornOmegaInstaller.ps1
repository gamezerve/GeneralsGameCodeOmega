param(
    [string]$DisplayName,
    [int]$BuildRank
)

$Version = $DisplayName -replace '^Reborn Omega ', ''
$Target = "D:\TEMP\Zero Hour Reborn Omega v$Version"
$InstallerName = "Zero Hour $DisplayName Setup"
$InstallerOutput = "$Target\$InstallerName.exe"

$ISCC = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
$ISS = "$PSScriptRoot\Installer\RebornOmega.iss"

powershell -NoProfile -ExecutionPolicy Bypass `
    -File "$PSScriptRoot\PrepareRebornOmegaData.ps1" `
    -DisplayName $DisplayName

if ($LASTEXITCODE -ne 0)
{
    throw "PrepareRebornOmegaData failed with exit code $LASTEXITCODE"
}

$InstallerName = "Zero Hour $DisplayName Setup"

& $ISCC `
    "/DRebornDisplayName=$DisplayName" `
    "/DRebornVersion=$Version" `
    "/DRebornBuildRank=$BuildRank" `
    "/DRebornOutputDir=$Target" `
    "/DRebornSourceDir=$Target" `
    "/DRebornInstallerName=$InstallerName" `
    $ISS

if ($LASTEXITCODE -ne 0)
{
    throw "Inno Setup failed with exit code $LASTEXITCODE"
}


