param([switch]$Launch)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$root = Join-Path $repo ('build/blackbox-sprint5/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$source = 'C:/paseoprojects/smaragd3ds/.tools/azahar'
New-Item -ItemType Directory -Path $root -Force > $null
$baseline = @()
foreach ($role in @('host','guest')) {
    $from = Join-Path $source $role
    $to = Join-Path $root $role
    New-Item -ItemType Directory -Path $to -Force > $null
    Get-ChildItem -LiteralPath $from -File | Where-Object { $_.Extension -in @('.dll','.conf') -or $_.Name -in @('azahar.exe','azahar-room.exe') } | Copy-Item -Destination $to
    foreach ($folder in @('plugins','scripting')) { Copy-Item -LiteralPath (Join-Path $from $folder) -Destination $to -Recurse }
    $user = Join-Path $to 'user'
    New-Item -ItemType Directory -Path $user -Force > $null
    foreach ($folder in @('config','nand','sysdata')) { Copy-Item -LiteralPath (Join-Path $from "user/$folder") -Destination $user -Recurse }
    $game = Join-Path $user 'sdmc/3ds/emerald3ds'
    New-Item -ItemType Directory -Path $game -Force > $null
    $originalGame = Join-Path $from 'user/sdmc/3ds/emerald3ds'
    foreach ($name in @('emerald3ds.sav','coop_profile.bin','settings.txt','plates.bin')) {
        $path = Join-Path $originalGame $name
        if (Test-Path -LiteralPath $path) {
            $baseline += [pscustomobject]@{ path=$path; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
            Copy-Item -LiteralPath $path -Destination $game
        }
    }
    foreach ($name in @('Emerald3DS.3dsx','Emerald3DS.smdh')) { Copy-Item -LiteralPath (Join-Path $repo "build/upstream/3ds_port/dist/$name") -Destination $game }
    Copy-Item -LiteralPath (Join-Path $repo 'build/upstream/3ds_port/build/emerald3ds.pak') -Destination $game
    $settings = Join-Path $game 'settings.txt'
    $s = [IO.File]::ReadAllText($settings) -replace '(?m)^voxel=\d+', 'voxel=0' -replace '(?m)^voxel_battle=\d+', 'voxel_battle=0'
    [IO.File]::WriteAllText($settings, $s)
    $config = Join-Path $user 'config/qt-config.ini'
    $ini = [IO.File]::ReadAllText($config)
    foreach ($storage in @('nand','sdmc')) {
        $path = (Join-Path $user $storage).Replace('\','/') + '/'
        $ini = [regex]::Replace($ini, '(?m)^' + $storage + '_directory=.*$', $storage + '_directory=' + $path)
    }
    $ini = $ini -replace '(?m)^Multiplayer\\(room_port|port)\\default=.*$', 'Multiplayer\$1\default=false'
    $ini = $ini -replace '(?m)^Multiplayer\\(room_port|port)=.*$', 'Multiplayer\$1=24873'
    $ini = $ini -replace '(?m)^Paths\\recentFiles=.*$', 'Paths\recentFiles='
    [IO.File]::WriteAllText($config, $ini)
    $exe = Join-Path $to $(if ($role -eq 'host') {'BBHost.exe'} else {'BBGast.exe'})
    Copy-Item -LiteralPath (Join-Path $to 'azahar.exe') -Destination $exe
}
$baseline | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $root 'original-profile-hashes.json') -Encoding UTF8
@{ root=$root; port=24873; created=(Get-Date).ToString('o'); build='03ecd34074355974'; abi='ebf0f1f4' } | ConvertTo-Json | Set-Content (Join-Path $root 'run.json') -Encoding UTF8
Write-Output "Testprofile: $root"
if ($Launch) {
    if (Get-NetUDPEndpoint -LocalPort 24873 -ErrorAction SilentlyContinue) { throw 'Testport 24873 ist belegt.' }
    $server = Start-Process -FilePath (Join-Path $root 'host/azahar-room.exe') -WorkingDirectory $root -ArgumentList @('--room-name','Sprint5Blackbox','--port','24873','--max_members','2','--preferred-app','"Emerald 3DS"') -WindowStyle Hidden -RedirectStandardOutput (Join-Path $root 'room-stdout.log') -RedirectStandardError (Join-Path $root 'room-stderr.log') -PassThru
    Write-Output "Testserver PID $($server.Id)"
    foreach ($role in @('host','guest')) {
        $exeName = if ($role -eq 'host') {'BBHost.exe'} else {'BBGast.exe'}
        $folder = Join-Path $root $role
        $game = Join-Path $folder 'user/sdmc/3ds/emerald3ds/Emerald3DS.3dsx'
        $p = Start-Process -FilePath (Join-Path $folder $exeName) -WorkingDirectory $folder -ArgumentList @('-w', ('"' + $game + '"')) -PassThru
        Write-Output "$role PID $($p.Id)"
    }
}
