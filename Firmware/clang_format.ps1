#   .\clang_format.ps1          -> formats all whitelisted files
#   .\clang_format.ps1 -dryrun  -> checks without modifying

param(
    [switch]$dryrun
)

$Whitelist = @("Framework", "Application")
$Blacklist = @("Framework/Libs", ".pio", "ThirdParty")

$files = Get-ChildItem -Recurse -Include *.c, *.h | Where-Object {
    $dir = $_.DirectoryName
    $inWhitelist = $Whitelist | Where-Object { $dir -match $_ }
    $inBlacklist = $Blacklist | Where-Object { $dir -match $_ }
    $inWhitelist -and -not $inBlacklist
}

Write-Host "Found $($files.Count) files to format"

foreach ($file in $files) {
    if ($dryrun) {
        & clang-format --style=file --dry-run --Werror $file.FullName
    } else {
        & clang-format --style=file -i $file.FullName
        Write-Host "Formatted: $($file.FullName)"
    }
}

Write-Host "Formatting done."
