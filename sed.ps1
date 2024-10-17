# Define output file
$outputFile = "compile_commands.json"

# Get all .o.json files in the current directory
$oJsonFiles = Get-ChildItem -Path .\build\obj -Filter *.o.json

# Check if there are any files
if ($oJsonFiles.Count -eq 0) {
    Write-Host "No .o.json files found in the current directory."
    exit 1
}

# Start the JSON array with [
"[`n" | Out-File -FilePath $outputFile -Encoding utf8 -Force

# Loop through each file, adding its content to the output
foreach ($file in $oJsonFiles) {
    Get-Content -Path $file.FullName | Out-File -FilePath $outputFile -Append -Encoding utf8

    # Add a comma after each file's content except the last one
    if ($file -ne $oJsonFiles[-1]) {
        "," | Out-File -FilePath $outputFile -Append -Encoding utf8
    }
}

# Close the JSON array with ]
"`n]" | Out-File -FilePath $outputFile -Append -Encoding utf8

