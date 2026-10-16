# ReverieVR optional Windows companion, packaged as plain-text in the APK.
# Copyright ReverieVR contributors. No installation, service, administrator
# privilege, PC activity monitoring, screen scraping, or background startup.
. (Join-Path $PSScriptRoot 'SteamVrPreflight.ps1')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Find-Scrcpy {
    $command = Get-Command -Name 'scrcpy.exe' -ErrorAction SilentlyContinue
    if ($null -ne $command) { return $command.Source }
    $candidate = Read-Host 'Path to scrcpy.exe (leave blank to cancel)'
    if ([string]::IsNullOrWhiteSpace($candidate)) { return $null }
    $candidate = $candidate.Trim().Trim('"')
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        Write-Warning 'That file does not exist.'
        return $null
    }
    if ([System.IO.Path]::GetFileName($candidate) -ine 'scrcpy.exe') {
        Write-Warning 'Select scrcpy.exe from the official scrcpy distribution.'
        return $null
    }
    return (Resolve-Path -LiteralPath $candidate).Path
}

function Start-ScrcpyMode {
    param([string[]]$Arguments)
    $exe = Find-Scrcpy
    if ($null -eq $exe) {
        Write-Host 'Install official scrcpy if needed: https://github.com/Genymobile/scrcpy'
        return
    }
    Write-Host ('Launching: scrcpy ' + ($Arguments -join ' '))
    # External process owns its own exit state; no background Windows service.
    & $exe @Arguments
    if ($LASTEXITCODE -ne 0) {
        Write-Warning ('scrcpy returned exit code ' + $LASTEXITCODE +
            '. Inspect its output and verify USB/ADB/UHID compatibility.')
    }
}

function Show-OpticalDrives {
    Write-Host ''
    Write-Host 'Locally attached optical drives (read-only listing):'
    $drives = @([System.IO.DriveInfo]::GetDrives() |
        Where-Object { $_.DriveType -eq [System.IO.DriveType]::CDRom })
    if ($drives.Count -eq 0) {
        Write-Host 'No optical drives detected.'
    } else {
        foreach ($drive in $drives) {
            $state = if ($drive.IsReady) { 'media present' } else { 'no readable media' }
            Write-Host ('  ' + $drive.Name + ' - ' + $state)
        }
    }
    Write-Host ''
    Write-Host 'Windows SMB shares normal FILES, not raw optical-disc sectors.'
    Write-Host 'To use an image with ReverieVR, put a lawful ISO/ZIP in a'
    Write-Host 'read-only Windows shared folder, then use Laptop Files in the APK.'
    Write-Host 'PS1 multi-track BIN/CUE or protected discs need specialized'
    Write-Host 'imaging and compatible emulators; this utility does not bypass DRM.'
}

while ($true) {
    Write-Host ''
    Write-Host '=============================================='
    Write-Host ' ReverieVR Laptop Link - optional host utility'
    Write-Host '=============================================='
    Write-Host '1. Forward laptop controller only (no video/audio)'
    Write-Host '2. Mirror phone screen + forward laptop controller'
    Write-Host '3. Set up scrcpy Wi-Fi (USB authorization first)'
    Write-Host '4. Inspect local optical drives / file sharing'
    Write-Host '5. Check SteamVR installation and driver readiness (read only)'
    Write-Host 'Q. Quit'
    Write-Host 'No system settings or firewall rules are changed.'
    $choice = (Read-Host 'Choose').Trim().ToUpperInvariant()
    try {
        switch ($choice) {
            '1' { Start-ScrcpyMode -Arguments @('--no-video','--no-audio','--gamepad=uhid') }
            '2' { Start-ScrcpyMode -Arguments @('--stay-awake','--no-audio','--gamepad=uhid') }
            '3' { Start-ScrcpyMode -Arguments @('--tcpip','--no-audio','--gamepad=uhid') }
            '4' { Show-OpticalDrives }
            '5' { Show-SteamVrPreflight }
            'Q' { return }
            default { Write-Host 'Unknown choice.' }
        }
    } catch {
        Write-Warning ('Requested action failed: ' + $_.Exception.Message)
    }
}
