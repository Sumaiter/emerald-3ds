param(
    [string]$RepoRoot = (Get-Location).Path,
    [string]$AzaharRoot,
    [string]$BuildPort,
    [switch]$ServerOnly
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
if (!$AzaharRoot) {
    $commonGit = & git -C $RepoRoot rev-parse --path-format=absolute --git-common-dir
    if ($LASTEXITCODE -ne 0) { throw 'RepoRoot muss auf den gewuenschten Git-Arbeitsbaum zeigen.' }
    $AzaharRoot = Join-Path (Split-Path $commonGit.Trim() -Parent) '.tools/azahar'
}
$AzaharRoot = (Resolve-Path -LiteralPath $AzaharRoot).Path

function Start-CoopRoom {
    $roomPort = 24872
    $roomExecutable = Join-Path $AzaharRoot 'host/azahar-room.exe'
    if (!(Test-Path -LiteralPath $roomExecutable -PathType Leaf)) { throw "Raumserver fehlt: $roomExecutable" }
    $roomExecutable = (Resolve-Path -LiteralPath $roomExecutable).Path
    $listeners = @(Get-NetUDPEndpoint -ErrorAction Stop | Where-Object LocalPort -eq $roomPort)
    if ($listeners.Count) {
        foreach ($listener in $listeners) {
            $owner = Get-Process -Id $listener.OwningProcess -ErrorAction Stop
            if ($owner.Path -ne $roomExecutable) { throw "UDP-Port $roomPort ist durch einen anderen Prozess belegt (PID $($owner.Id))." }
        }
        Write-Output "Raumserver laeuft bereits auf 127.0.0.1:$roomPort (PID $($listeners[0].OwningProcess))."
        return
    }
    $roomDirectory = Join-Path $AzaharRoot 'room'
    New-Item -ItemType Directory -Path $roomDirectory -Force > $null
    $stdout = Join-Path $roomDirectory 'stdout.log'
    $stderr = Join-Path $roomDirectory 'stderr.log'
    $roomProcess = Start-Process -FilePath $roomExecutable -WorkingDirectory $roomDirectory `
        -ArgumentList @('--room-name', '"Emerald Koop"', '--room-description', '"Lokaler Test"',
            '--port', "$roomPort", '--max_members', '2', '--preferred-app', '"Emerald 3DS"') `
        -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru
    for ($attempt = 0; $attempt -lt 10; $attempt++) {
        $roomProcess.Refresh()
        if ($roomProcess.HasExited) {
            $details = (Get-Content -LiteralPath $stdout,$stderr -ErrorAction SilentlyContinue) -join "`n"
            throw "Raumserver wurde beendet (Exitcode $($roomProcess.ExitCode)).`n$details"
        }
        $ready = @(Get-NetUDPEndpoint -ErrorAction Stop |
            Where-Object { $_.LocalPort -eq $roomPort -and $_.OwningProcess -eq $roomProcess.Id })
        if ($ready.Count) {
            Write-Output "Raumserver gestartet auf 127.0.0.1:$roomPort (PID $($roomProcess.Id)); Raumpasswort leer."
            return
        }
        Start-Sleep -Milliseconds 200
    }
    Stop-Process -Id $roomProcess.Id -ErrorAction SilentlyContinue
    throw "Raumserver hat UDP-Port $roomPort nicht geoeffnet. Protokolle: $roomDirectory"
}

if ($ServerOnly) {
    Start-CoopRoom
    return
}

if (!$BuildPort) { $BuildPort = Join-Path $RepoRoot 'build/upstream/3ds_port' }
$payload = @{
    'Emerald3DS.3dsx' = Join-Path $BuildPort 'dist/Emerald3DS.3dsx'
    'Emerald3DS.smdh' = Join-Path $BuildPort 'dist/Emerald3DS.smdh'
    'emerald3ds.pak' = Join-Path $BuildPort 'build/emerald3ds.pak'
}
foreach ($path in $payload.Values) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Build-Datei fehlt: $path" }
}
$profiles = @(
    @{ Directory = 'host'; Name = 'Host' },
    @{ Directory = 'guest'; Name = 'Gast' }
)

# Check both profiles before changing files. An active game must be closed by its user.
foreach ($profile in $profiles) {
    $folder = Join-Path $AzaharRoot $profile.Directory
    if (!(Test-Path -LiteralPath (Join-Path $folder 'azahar.exe'))) { throw "Azahar fehlt: $folder" }
    $running = Get-Process -Name azahar,Host,Gast -ErrorAction SilentlyContinue |
        Where-Object { $_.Path -and (Split-Path $_.Path -Parent) -eq $folder }
    if ($running) { throw "$($profile.Name) laeuft bereits. Vor dem Aktualisieren das Fenster schliessen." }
}

Start-CoopRoom

function Set-ConsoleNickname([string]$UserDirectory, [string]$Nickname) {
    $configs = @(Get-ChildItem -LiteralPath (Join-Path $UserDirectory 'nand/data') -Filter config -Recurse -File |
        Where-Object { $_.FullName -match '[\\/]sysdata[\\/]00010017[\\/]00000000[\\/]config$' })
    if ($configs.Count -ne 1) { throw "Keine eindeutige 3DS-Konfiguration in $UserDirectory" }
    $path = $configs[0].FullName
    $bytes = [IO.File]::ReadAllBytes($path)
    if ($bytes.Length -lt 0x455C) { throw "Ungueltige 3DS-Konfiguration: $path" }
    $entries = [BitConverter]::ToUInt16($bytes, 0)
    if ($entries -gt 1479) { throw "Ungueltige Blockanzahl: $path" }
    for ($i = 0; $i -lt $entries; $i++) {
        $entry = 4 + 12 * $i
        if ([BitConverter]::ToUInt32($bytes, $entry) -ne 0x000A0000) { continue }
        $offset = [BitConverter]::ToUInt32($bytes, $entry + 4)
        $size = [BitConverter]::ToUInt16($bytes, $entry + 8)
        if ($size -ne 28 -or $offset -lt 0x455C -or $offset + $size -gt $bytes.Length) {
            throw "Ungueltiger Namensblock: $path"
        }
        $oldName = [Text.Encoding]::Unicode.GetString($bytes, $offset, 20).TrimEnd([char]0)
        if ($oldName -ceq $Nickname) { return }
        $backup = "$path.before-host-gast"
        if (!(Test-Path -LiteralPath $backup)) { Copy-Item -LiteralPath $path -Destination $backup }
        [Array]::Clear($bytes, $offset, 20)
        $nameBytes = [Text.Encoding]::Unicode.GetBytes($Nickname)
        [Array]::Copy($nameBytes, 0, $bytes, $offset, $nameBytes.Length)
        [IO.File]::WriteAllBytes($path, $bytes)
        return
    }
    throw "Namensblock fehlt: $path"
}

foreach ($profile in $profiles) {
    $folder = Join-Path $AzaharRoot $profile.Directory
    $userDirectory = Join-Path $folder 'user'
    $gameDirectory = Join-Path $userDirectory 'sdmc/3ds/emerald3ds'
    if (!(Test-Path -LiteralPath $gameDirectory)) { throw "Spielprofil fehlt: $gameDirectory" }
    Set-ConsoleNickname $userDirectory $profile.Name
    foreach ($name in $payload.Keys) {
        Copy-Item -LiteralPath $payload[$name] -Destination (Join-Path $gameDirectory $name) -Force
    }
    # A distinct executable name makes the two instances identifiable in Task Manager.
    $executable = Join-Path $folder "$($profile.Name).exe"
    Copy-Item -LiteralPath (Join-Path $folder 'azahar.exe') -Destination $executable -Force
}

foreach ($profile in $profiles) {
    $folder = Join-Path $AzaharRoot $profile.Directory
    $game = Join-Path $folder 'user/sdmc/3ds/emerald3ds/Emerald3DS.3dsx'
    $process = Start-Process -FilePath (Join-Path $folder "$($profile.Name).exe") `
        -ArgumentList @('-w', ('"' + $game + '"')) -WorkingDirectory $folder -PassThru
    Write-Output "$($profile.Name) gestartet (PID $($process.Id))."
}
