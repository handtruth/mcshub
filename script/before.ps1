if (Test-Path "hardcoded-save.cpp") {

} else {
    Copy-Item -Path "hardcoded.cpp" -Destination "hardcoded-save.cpp"
}
& "..\script\hardcoded.ps1" -InputFiles "hardcoded-save.cpp" -OutputFile "hardcoded.cpp" -Context ".."
