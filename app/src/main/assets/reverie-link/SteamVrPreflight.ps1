# Read-only SteamVR host prerequisite probe. Not a headset driver.
# Uses Valve's per-user OpenVR path registry; never edits or deletes it.
function Show-SteamVrPreflight {
    Write-Host ''
    Write-Host 'SteamVR PC headset link — host prerequisites (READ ONLY)'
    Write-Host 'This does not start or install a SteamVR driver or video stream.'
    if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) {
        Write-Warning 'LOCALAPPDATA is unavailable; cannot inspect OpenVR paths.'
        return
    }
    $directory = Join-Path $env:LOCALAPPDATA 'openvr'
    # Valve's modern name is openvr.vrpaths. Some older tools documented
    # openvrpaths.vrpath; read that only as an optional compatibility path.
    $registryFile = Join-Path $directory 'openvr.vrpaths'
    if (-not (Test-Path -LiteralPath $registryFile -PathType Leaf)) {
        $legacy = Join-Path $directory 'openvrpaths.vrpath'
        if (Test-Path -LiteralPath $legacy -PathType Leaf) {
            $registryFile = $legacy
        } else {
            Write-Warning 'OpenVR registry not found. Install and run SteamVR once.'
            Write-Host ('Expected: ' + $registryFile)
            Write-Host 'Install SteamVR using Steam (app 250820), then try again.'
            return
        }
    }
    Write-Host ('OpenVR registry: ' + $registryFile)
    try {
        $registry = Get-Content -LiteralPath $registryFile -Raw -ErrorAction Stop |
            ConvertFrom-Json -ErrorAction Stop
    } catch {
        Write-Warning ('Cannot parse OpenVR registry: ' + $_.Exception.Message)
        return
    }
    $found = $false
    foreach ($entry in @($registry.runtime)) {
        if ([string]::IsNullOrWhiteSpace([string]$entry)) { continue }
        $root = [string]$entry
        $tool = Join-Path $root 'bin\win64\vrpathreg.exe'
        $steamVrExe = Join-Path $root 'bin\win64\vrstartup.exe'
        if ((Test-Path -LiteralPath $tool -PathType Leaf) -and
            (Test-Path -LiteralPath $steamVrExe -PathType Leaf)) {
            Write-Host ('SteamVR runtime: ' + $root)
            Write-Host ('Driver registration tool: ' + $tool)
            $found = $true
            break
        }
    }
    if (-not $found) {
        Write-Warning 'SteamVR executable/vrpathreg.exe not found at registered paths.'
        Write-Host 'Verify SteamVR in your Steam library; this tool changes nothing.'
    }
    $drivers = @($registry.external_drivers |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    Write-Host ('Registered external driver paths: ' + $drivers.Count)
    foreach ($driver in $drivers) {
        Write-Host ('  ' + [string]$driver)
    }
    Write-Host ''
    if ($found) {
        Write-Host 'SteamVR host prerequisites: FOUND.'
    } else {
        Write-Host 'SteamVR host prerequisites: INCOMPLETE.'
    }
    Write-Host 'ReverieVR SteamVR headset driver: NOT INSTALLED BY THIS TOOL.'
    Write-Host 'PC-to-phone VR streaming/pose link: NOT IMPLEMENTED YET.'
    Write-Host 'No registry, SteamVR driver or Windows firewall settings changed.'
}
