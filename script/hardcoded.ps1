param (
    # Specifies a path to one or more locations.
    [Parameter(Mandatory=$true,
            Position=0,
            ParameterSetName="InputFiles",
            ValueFromPipeline=$true,
            ValueFromPipelineByPropertyName=$true,
            HelpMessage="Path to one or more locations.")]
    [Alias("InPath")]
    [ValidateNotNullOrEmpty()]
    [string[]]
    $InputFiles,
    
    # Specifies a path to one or more locations.
    #[Parameter(Mandatory=$true,
    #        Position=1,
    #        ParameterSetName="OutputFile",
    #        HelpMessage="Output file path")]
    [Alias("OutPath")]
    [string]
    $OutputFile,
    [Alias("Context")]
    [string]
    $ContextPath
)

if (!$ContextPath) {
    $ContextPath = Get-Location
}

if ($OutputFile) {
    Out-File -FilePath $OutputFile -Encoding UTF8 -InputObject ""
    function Out-Default {
        param (
            [Parameter(ValueFromPipeline=$true)]
            [object[]]$InputObjects
        )
        Process {
            $_ | Out-File -FilePath $OutputFile -Append -Encoding UTF8
        }
    }
    #function Out-Default([Parameter(ValueFromPipeline=$true)][object[]]$InputObjects) { $InputObjects | Out-File -FilePath $OutputFile -Append -Encoding UTF8 }
} else {
    Set-Alias Out-Default Out-Host
}

function Out-CPPString {
    param (
        [Parameter(ValueFromPipeline=$true)]
        [string[]]$InputObjects
    )
    Process {
        "R`"###($_)###`" `"\n`""
    }
    #foreach ($line in $InputObjects) {
    #    "R`"###($line)###`" `"\n`""
    #}
}

foreach ($file in $InputFiles) {
    foreach ($line in Get-Content $file -Encoding UTF8) {
        $i = $line.IndexOf("`${")
        if ($i -eq -1) {
            $line | Out-Default
        } else {
            $j = $line.IndexOf('}', $i)
            $path = [IO.Path]::GetFullPath((Join-Path $ContextPath $line.SubString($i + 2, $j - $i - 2)))
            if (Test-Path $path) {
                "$($line.SubString(0, $i))`"" | Out-Default
                Get-Content $path | Out-CPPString | Out-Default
                "`"$($line.SubString($j + 1))" | Out-Default
            } else {
                $line | Out-Default
            }
        }
    }
}
