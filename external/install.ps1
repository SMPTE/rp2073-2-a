# PowerShell script to install and build external libraries used on Windows

#TODO: Add mxml dynamic library to path if it is not already listed

#TODO: Modify script to accept the Visual Studio build tool as a command-line argument

[CmdletBinding()]
param (
	# Command-line switch for removing packages
	[switch]$Clean = $false
)

# Locations of the packages used on Windows
$argparse_url = "https://github.com/cofyc/argparse"
$expat_url = "https://github.com/libexpat/libexpat"
$mxml_url = "https://github.com/michaelrsweet/mxml"
#$iotivity_url = "https://github.com/iotivity/iotivity"
$ya_getopt_url = "https://github.com/kubo/ya_getopt.git"
$uthash_url = "https://github.com/troydhanson/uthash"

$packages = @($argparse_url, $expat_url, $mxml_url, $ya_getopt_url, $uthash_url)

# Build tool for configuring CMake
$visual_studio = "Visual Studio 17 2022"

If ($Clean)
{
	Foreach ($url in $packages)
	{
		# Remove the repository directory
		$directory = [System.IO.Path]::GetFileNameWithoutExtension($(Split-Path -Leaf $url))
		If (Test-Path -Type Container $directory)
		{
			Write-Output "Removing $directory"
			Remove-Item -Force -Recurse -Confirm:$false $directory
		}
	}
	
	Exit
}

Foreach ($url in $packages)
{
	# Clone the repository if it does not exist in the current directory
	If (-Not (Test-Path -Type Container $(Split-Path -Leaf $url)))
	{
		#Write-Output "git clone $url"
		& git clone $url
	}
}

# Build the packages that use CMake
Foreach ($directory in @("argparse", "libexpat"))
{
	# Has the package been cloned from its repository into the current directory?
	If (Test-Path -Type Container $directory)
	{
		Write-Host ""
		Write-Host -ForegroundColor "Yellow" "Building external software: $directory"
		Write-Host ""

		# Enter the subdirectory that contains the CMakeLists.txt file
		$cmake_directory = $(If ($directory -eq "libexpat") {$(Join-Path -Path $directory -ChildPath "expat")} Else {$directory})
		Push-Location $cmake_directory

		# Configure the CMake build (specify the generator and architecture)
		#& cmake -G $visual_studio -A Win32 -B build
		& cmake -G "$visual_studio" -A x64 -B build

		# Build the all configurations using the generator and architecture specified during configuration
		& cmake --build build --config Debug
		& cmake --build build --config Release

		Pop-Location
	}
}

# The Mini-XML library does not have a CMakeLists.txt file
$project = ".\mxml\vcnet\mxml1.vcxproj"

# Can the Mini-XML library be built using MSBuild?
If ([bool](Get-Command -ErrorAction Ignore -Type Application MSBuild))
{
	# Build the Mini-XML library
	Write-Host ""
	Write-Host -ForegroundColor "Yellow" "Building external software: mxml"
	#MSBuild $project -property:Configuration=Debug
	& MSBuild $project -property:Configuration=Debug -property:Platform=x64 -property:OutDir=x64\Debug\
} else {
	Write-Host ""
	Write-Host -ForegroundColor "Yellow" "MSBuild not installed: Use Visual Studio to build $project"
}

# Done
Write-Host ""
