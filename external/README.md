# External Software

The VC-5 programs use software from other source code repositories.

Older releases of the VC-5 software included the software packages copied into this repository
but all packages that are currently in use are available on GitHub so only the URL to the
external repository is provided.

Scripts are included in this folder to clone the external packages into this folder.
The scripts will build the software if it can be built from the command-line.
On Windows, some packages must be build using Visual Studio.

[install.sh](./install.sh)
> Bash script for cloning and building the external software packages on macOS and Linux.

[install.ps1](./install.ps1)
> PowerShell script for cloning and building the external software packages on Windows.

[pathtools.ps1](./pathtools.ps1)
> PowerShell functions for managing the executable path environment variable.

[Archive/](./Archive)
> Old packages that were included in earlier releases of the VC-5 software.
These packages have been superseded by cloning software from GitHub repositories.
In some cases, older packages were replaced by packages with more favorable license agreements.

License agreements and an overview of the licenses for external software currently used in the
VC-5 codebase are provided in [licenses](../licenses).

