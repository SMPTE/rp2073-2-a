# Script to perform simple tests on the encoder and decoder

# Root directory for the VC-5 software
$root = ".."

# All software is built for the x86-64 architecture on Windows
#$architecture = "x64"

# List of programs to test
$programs = @("encoder", "decoder")

# Visual Studio build configurations
$configurations = @("Debug", "Release")

# Log file for capturing output from programs invoked by the script
$logfile = Join-Path $(Get-Location) "testbuild.log"
#Write-Output $logfile

# Remove the old test build log
If (Test-Path -PathType Leaf "$logfile") {
	Remove-Item $logfile
}

# Open a new log file
[void](New-Item $logfile)

# Test the builds of all programs
Foreach ($p in $programs)
{
	#Write-Output "`n------ Testing $p ------`n"
	pushd $root/$p/build/cmake
	
	# Clean and configure the CMake build
	#Write-Output "`n------ Rebuilding the CMake files ------`n"
	Invoke-Expression "$HOME/Scripts/make.ps1 clean-all" | Out-File -Append $logfile
	Invoke-Expression "$HOME/Scripts/make.ps1 cmake" | Out-File -Append $logfile
	
	# Build and test each configuration
	Foreach ($c in $configurations)
	{
		#Write-Output "`n------ Building $p $($c.ToLower()) configuration ------`n"
		Invoke-Expression "$HOME/Scripts/make.ps1 build -Config $c" | Out-File -Append $logfile
		
		#Write-Output "`n------ Testing $p $($c.ToLower()) configuration ------"
		$result = "Passed"
		try {
			Invoke-Expression "./build/$c/$p.exe" 2>&1 | Out-File -Append $logfile
		} catch {
			$result = "Failed"
		}
		Write-Output "${result}: $p.exe ($c)"
	}
	
	popd
}

#Write-Output "`n------ Done ------`n"
