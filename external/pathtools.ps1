# PowerShell script to add a directory to the executable path
#
# The script is intended for adding dynamic libraries to the path


# Add the specified directory to the executable path
Function Add-Path {

    param([string] $Directory, [switch] $Append)

	# Get the absolute path to the directory
	$directory = $(Resolve-Path $directory)
	#Write-Output $directory

    # Get the current executable path list
    $pathlist = $env:Path.Split(";")

    # True if the specified directory was found on the path
    $found = $false

    # Look for the specified directory on the path
    Foreach ($path in $pathlist) {
	    If ($path -eq $directory) {
		    #Write-Output "Found: $path"
		    $found = $true
	    }
    }

    # Add the directory if it was not found on the path
    If (-Not $found) {
        If ($append) {
            #Write-Output "Append $directory to executable path"
    	    $env:Path += ";$directory"
        }
        Else {
            #Write-Output "Prepend $directory to executable path"
    	    $env:Path = "$directory;$env:Path"
        }
    }
}


# Remove the specified directory from the executable path
Function Remove-Path {

    param([string] $Directory)

    # Get the current executable path list
    $pathlist = $env:Path.Split(";")

    # Copy the current path excluding the directory to be removed
	$newpath = ""
	Foreach ($path in $pathlist) {
		If (-Not ($path -eq $directory)) {
			$separator = If ($newpath -eq "") {""} Else {";"}
			$newpath = $newpath + $separator + $path
		}
	}

	#Write-Output $newpath
	$env:Path = $newpath
}


# List the directories on the executable path
Function Show-Path {
	
	# Get the current executable path list
	$pathlist = $env:Path.Split(";")

	Foreach ($path in $pathlist) {
		Write-Output $path
	}
}


# Directory to add to the path (change script to use command-line argument)
$libdir = "$env:USERPROFILE\Software\Mini-XML\mxml\vcnet\Debug\Win32"
