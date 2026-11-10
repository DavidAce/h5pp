param([Parameter(Mandatory)][string]$Preset)
$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path
$build = Join-Path $root "build/$Preset"
$out = Join-Path $build 'hang-diagnostics'
New-Item -ItemType Directory -Force $out | Out-Null
Start-Transcript -Path "$out/diagnostics.txt"
$env:H5PP_DIAGNOSE_HANG = '1'
$exe = Join-Path $build 'tests/Debug/h5pp-test-appendToDataset.exe'
$toolsDir = Join-Path $env:RUNNER_TEMP 'procdump'
New-Item -ItemType Directory -Force $toolsDir | Out-Null
Invoke-WebRequest https://download.sysinternals.com/files/Procdump.zip -OutFile "$toolsDir/procdump.zip" -TimeoutSec 60
Expand-Archive "$toolsDir/procdump.zip" $toolsDir -Force
$procdump = Join-Path $toolsDir 'procdump64.exe'
$cdb = Get-ChildItem "${env:ProgramFiles(x86)}/Windows Kits/10/Debuggers/x64/cdb.exe" -ErrorAction SilentlyContinue | Select-Object -First 1

function Save-Hang($Process, [string]$Label) {
    # Record all processes: a child or Windows error dialog may outlive the test.
    Get-CimInstance Win32_Process | Select-Object ProcessId, ParentProcessId, Name, CommandLine |
        Export-Csv "$out/$Label-processes.csv" -NoTypeInformation
    $targets = @(Get-Process -Id $Process.Id -ErrorAction SilentlyContinue) +
               @(Get-Process -Name 'h5pp-test-appendToDataset' -ErrorAction SilentlyContinue)
    foreach ($target in ($targets | Sort-Object Id -Unique)) {
        try {
            $target.Modules | Select-Object ModuleName, FileName, FileVersionInfo |
                Format-List | Out-File "$out/$Label-$($target.Id)-modules.txt"
        } catch { Write-Warning "Cannot list modules: $_" }
        # Two samples distinguish an idle wait from a loop making no useful progress.
        foreach ($sample in 1..2) {
            if ($target.HasExited) { break }
            $target.Refresh()
            $target | Select-Object Id, ProcessName, CPU, Handles, WorkingSet64, MainWindowTitle | Format-List
            $dump = "$out/$Label-$($target.Id)-$sample.dmp"
            $dumper = Start-Process $procdump -ArgumentList "-accepteula -ma $($target.Id) `"$dump`"" -PassThru -NoNewWindow
            if (-not $dumper.WaitForExit(60000)) { $dumper.Kill(); Write-Warning 'ProcDump timed out' }
            if ($sample -eq 1) { Start-Sleep -Seconds 5 }
        }
    }
}

function Invoke-Probe([string]$Label, [string]$Command, [string]$Arguments, [string]$Directory) {
    Write-Host "Starting $Label : $Command $Arguments"
    $process = Start-Process $Command -ArgumentList $Arguments -WorkingDirectory $Directory -PassThru -NoNewWindow `
        -RedirectStandardOutput "$out/$Label-stdout.txt" -RedirectStandardError "$out/$Label-stderr.txt"
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        # This deadline applies to one small test, not configuration or compilation.
        while (-not $process.WaitForExit(10000)) {
            Get-Process -Id $process.Id -ErrorAction SilentlyContinue |
                Select-Object Id, ProcessName, CPU, Handles, WorkingSet64 | Format-Table | Out-Host
            Get-Process -Name 'h5pp-test-appendToDataset' -ErrorAction SilentlyContinue |
                Select-Object Id, ProcessName, CPU, Handles, WorkingSet64 | Format-Table | Out-Host
            Get-Content "$out/$Label-stderr.txt" -Tail 8 | Out-Host
            if ($watch.Elapsed.TotalSeconds -ge 180) {
                Save-Hang $process $Label | Out-Host
                return $false
            }
        }
        Write-Host "$Label exited with code $($process.ExitCode) after $($watch.Elapsed.TotalSeconds) seconds"
        return ($process.ExitCode -eq 0)
    } finally {
        if (-not $process.HasExited) { taskkill /PID $process.Id /T /F | Out-Host }
        Get-Content "$out/$Label-stdout.txt" -Tail 100 | Out-Host
        Get-Content "$out/$Label-stderr.txt" -Tail 100 | Out-Host
    }
}

try {
    # Keep CTest's pipe handling first, then compare with direct file output.
    $ctestOk = Invoke-Probe 'ctest' (Get-Command ctest).Source "--preset $Preset -R ^h5pp-test-appendToDataset$ -V --timeout 600" $root
    $listOk = Invoke-Probe 'list-tests' $exe '--list-tests' "$build/tests"
    $directOk = Invoke-Probe 'direct' $exe '--success' "$build/tests"

    Copy-Item "$build/Testing/Temporary/LastTest.log" $out -ErrorAction SilentlyContinue
    Copy-Item "$build/CMakeCache.txt" $out
    # Preserve matching symbols and dependency binaries for offline dump analysis.
    if (Get-ChildItem $out -Filter '*.dmp') {
        $files = Get-ChildItem "$build/pkg-build", "$root/install/$Preset" -Recurse -File |
            Where-Object { $_.Extension -in '.pdb', '.dll' -and $_.Name -notmatch '^vc\d+\.pdb$' }
        $files += Get-Item $exe, "$build/tests/Debug/h5pp-test-appendToDataset.pdb"
        foreach ($file in $files) {
            $dest = Join-Path "$out/symbols" ([IO.Path]::GetRelativePath($root, $file.FullName))
            New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
            Copy-Item $file.FullName $dest
        }
        if ($cdb) {
            foreach ($dump in (Get-ChildItem $out -Filter '*.dmp')) {
                $args = "-z `"$($dump.FullName)`" -y `"$build/tests/Debug;$root/install/$Preset/bin`" -c `".reload; ~* kv; !runaway; lm; q`""
                $debugger = Start-Process $cdb.FullName -ArgumentList $args -PassThru -NoNewWindow `
                    -RedirectStandardOutput "$($dump.FullName).stacks.txt" -RedirectStandardError "$($dump.FullName).debugger-errors.txt"
                if (-not $debugger.WaitForExit(60000)) { $debugger.Kill() }
                Get-Content "$($dump.FullName).stacks.txt" -Tail 150
            }
        } else { Write-Host 'CDB is unavailable. Dumps and symbols are saved for offline analysis.' }
    }
    if (-not ($ctestOk -and $listOk -and $directOk)) { throw 'Windows test diagnostics found a failure or hang. See the uploaded artifact.' }
} finally {
    Stop-Transcript
}
