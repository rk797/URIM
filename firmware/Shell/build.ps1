Set-Location -Path (Split-Path -Path $MyInvocation.MyCommand.Definition -Parent)

$envFilePath = ".\env.txt"
if (-not (Test-Path -Path $envFilePath)) {
    Write-Host "[+] env.txt not found. Please create the file with the account_id."
    exit 1
}

$envFileContent = Get-Content $envFilePath | ForEach-Object {
    $line = $_.Trim()
    if ($line -match "^account_id=(.*)$") {
        return $matches[1]
    }
}

if (-not $envFileContent) {
    Write-Host "[+] Could not find 'account_id' in env.txt. Please ensure it is defined as 'account_id=<value>'."
    exit 1
}

$account_id = $envFileContent

function Check-AWSCLIConfig {
    $awsConfigPath = "$env:USERPROFILE\.aws\credentials"
    if (Test-Path $awsConfigPath) {
        $awsConfigContent = Get-Content $awsConfigPath
        if ($awsConfigContent -match "aws_access_key_id" -and $awsConfigContent -match "aws_secret_access_key") {
            return $true
        }
    }
    return $false
}

$awsVersion = aws --version 2>$null

if (-not $awsVersion) {
    Write-Host "[+] AWS CLI not found. Opening browser to install AWS CLI..."
    Start-Process "https://aws.amazon.com/cli/"
    exit 1
}

if (-not (Check-AWSCLIConfig)) {
    Write-Host "[+] AWS CLI is not configured. Please run 'aws configure' to set up your credentials."
    Start-Process "cmd.exe" -ArgumentList "/c aws configure"
    exit 1
}

$buildDir = "../Builds"
$firmwarePath = "../firmware.ino"
$boardFQBN = "arduino:avr:USBDEVICE"
$cliPath = ".\cli.exe" 

$r2Bucket = "firmware"
$r2Subfolder = "bin/hex"
$r2BaseUrl = "https://$account_id.r2.cloudflarestorage.com/"

if (-not (Test-Path -Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir
}


$compileCommand = "$cliPath compile --fqbn $boardFQBN --output-dir $buildDir $firmwarePath"
Invoke-Expression $compileCommand


Start-Sleep -Seconds 2

$filesToRename = @{
    "firmware.ino.hex" = "Firmware.hex";
    "firmware.ino.with_bootloader.hex" = "FirmwareCaterina.hex"
}

foreach ($originalName in $filesToRename.Keys) {
    $originalFilePath = Join-Path $buildDir $originalName
    $newFilePath = Join-Path $buildDir $filesToRename[$originalName]

    if (Test-Path -Path $originalFilePath) {
        if (Test-Path -Path $newFilePath) {
            Remove-Item -Path $newFilePath -Force
            Write-Host "[+] Deleted existing file: $newFilePath"
        }

        Rename-Item -Path $originalFilePath -NewName $filesToRename[$originalName]
        Write-Host "[+] Renamed $originalName to $($filesToRename[$originalName])"

        $uploadCommand = "aws s3 cp `"$newFilePath`" s3://$r2Bucket/$r2Subfolder/ --endpoint-url $r2BaseUrl"
        Invoke-Expression $uploadCommand
        Write-Host "[+] Uploaded $($filesToRename[$originalName]) to Cloudflare R2 ($r2Subfolder folder)"
    } else {
        Write-Host "[+] File $originalName not found, skipping..."
    }
}

Get-ChildItem -Path $buildDir | Where-Object { $_.Name -notin $filesToRename.Values } | Remove-Item -Force

Write-Host "[+] Build complete, unnecessary files deleted, and firmware renamed and uploaded successfully."
Read-Host
