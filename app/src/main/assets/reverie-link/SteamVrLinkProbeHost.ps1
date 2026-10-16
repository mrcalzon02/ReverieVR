# RV-0246 / S1: one-shot, authenticated TLS PC-link transport test.
# NOT a SteamVR driver, video streamer, headset registration, or background service.
function Start-SteamVrLinkProbeHost {
    Write-Host ''
    Write-Host 'ReverieVR PC LINK TEST - encrypted/authenticated one-shot test'
    Write-Host 'No VR frames or device input are received by this test.'
    Write-Host 'Requires Windows 10/11 and Windows PowerShell with New-SelfSignedCertificate.'
    Write-Host 'A temporary certificate is created in CurrentUser/My and removed on exit.'
    Write-Host 'Never run this over public Wi-Fi.'
    $transport = (Read-Host 'Connection: U=USB/ADB reverse (local only), W=private Wi-Fi/USB tether IP, or Q=cancel').Trim().ToUpperInvariant()
    if ($transport -eq 'Q') { return }
    if ($transport -ne 'U' -and $transport -ne 'W') { Write-Warning 'Invalid mode.'; return }
    if ((Read-Host 'Create temporary user certificate and listen up to 3 min? Type YES') -cne 'YES') {
        Write-Host 'Cancelled. No changes made.'
        return
    }
    $port = 38726
    $certificate = $null
    $listener = $null
    $rng = $null
    $sha = $null
    try {
        $certificate = New-SelfSignedCertificate -Type SSLServerAuthentication -Subject 'CN=ReverieVR-SteamVR-Link-Test' -CertStoreLocation 'Cert:\CurrentUser\My' -KeyExportPolicy NonExportable -KeyUsage DigitalSignature -NotAfter (Get-Date).AddDays(1) -ErrorAction Stop
        $sha = [System.Security.Cryptography.SHA256]::Create()
        $fingerprint = [BitConverter]::ToString($sha.ComputeHash($certificate.RawData)).Replace('-','')
        $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
        $bytes = New-Object byte[] 16
        $rng.GetBytes($bytes)
        $secret = [BitConverter]::ToString($bytes).Replace('-','')
        $address = if ($transport -eq 'U') { [System.Net.IPAddress]::Loopback } else { [System.Net.IPAddress]::Any }
        $listener = New-Object System.Net.Sockets.TcpListener($address, $port)
        $listener.Start(2)
        Write-Host ('Listening on port ' + $port + ' for at most 180 seconds.')
        if ($transport -eq 'U') {
            Write-Host 'Connect USB, authorize debugging, then run on the PC:'
            Write-Host ('  adb reverse tcp:' + $port + ' tcp:' + $port)
            Write-Host 'Phone mode: USB / ADB reverse; host 127.0.0.1.'
        } else {
            Write-Host 'Phone mode: private Wi-Fi / USB tether; enter the PC LAN IPv4 address.'
            Write-Host 'View PC address using ipconfig. Permit this PORT on the trusted interface manually if blocked.'
            Write-Host 'No firewall rule is installed by this script.'
        }
        Write-Host ('CERT SHA-256: ' + $fingerprint)
        Write-Host ('PAIRING TOKEN: ' + $secret)
        Write-Host 'Enter those values on phone: Stage A -> Laptop Link -> Test SteamVR PC Link.'
        $deadline = [DateTime]::UtcNow.AddSeconds(180)
        $attempts = 0
        $accepted = $false
        while ([DateTime]::UtcNow -lt $deadline -and $attempts -lt 5 -and -not $accepted) {
            if (-not $listener.Pending()) { Start-Sleep -Milliseconds 100; continue }
            $attempts++
            $client = $null
            $tls = $null
            try {
                $client = $listener.AcceptTcpClient()
                $client.ReceiveTimeout = 7000
                $client.SendTimeout = 7000
                $tls = New-Object System.Net.Security.SslStream($client.GetStream(), $false)
                $tls.AuthenticateAsServer($certificate, $false, [System.Security.Authentication.SslProtocols]::Tls12, $false)
                $buffer = New-Object byte[] 1
                $inputBytes = New-Object System.Collections.Generic.List[byte]
                while ($inputBytes.Count -lt 120) {
                    $n = $tls.Read($buffer, 0, 1)
                    if ($n -eq 0 -or $buffer[0] -eq 10) { break }
                    $inputBytes.Add($buffer[0])
                }
                $message = [System.Text.Encoding]::ASCII.GetString($inputBytes.ToArray())
                if ($inputBytes.Count -ge 120 -or
                    -not [string]::Equals($message, ('RVLINK/1 PROBE ' + $secret), [StringComparison]::Ordinal)) {
                    Write-Warning 'Connection refused: unsupported protocol or incorrect pairing token.'
                    continue
                }
                $reply = [System.Text.Encoding]::ASCII.GetBytes("RVLINK/1 READY`n")
                $tls.Write($reply, 0, $reply.Length)
                $tls.Flush()
                $accepted = $true
                Write-Host 'SUCCESS: TLS authenticated, certificate-pinned phone link confirmed.'
                Write-Host 'NO VR rendering/tracking is active; link test completed.'
            } catch {
                Write-Warning ('Link attempt failed: ' + $_.Exception.GetType().Name)
            } finally {
                if ($null -ne $tls) { $tls.Dispose() }
                if ($null -ne $client) { $client.Close() }
            }
        }
        if (-not $accepted) { Write-Warning 'No authenticated client confirmed before timeout/attempt limit.' }
    } catch {
        Write-Warning ('Cannot launch secure PC link test: ' + $_.Exception.Message)
    } finally {
        if ($null -ne $listener) { $listener.Stop() }
        if ($null -ne $rng) { $rng.Dispose() }
        if ($null -ne $sha) { $sha.Dispose() }
        if ($null -ne $certificate) {
            # Remove ONLY our freshly generated temporary certificate.
            try {
                Remove-Item -LiteralPath ('Cert:\CurrentUser\My\' + $certificate.Thumbprint) -Force -ErrorAction Stop
                Write-Host 'Temporary test certificate removed.'
            } catch {
                Write-Warning 'Could not clean temporary certificate; remove this thumbprint from CurrentUser/My:'
                Write-Warning $certificate.Thumbprint
            }
        }
    }
}
