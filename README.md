# POSApp

Legacy embedded C point-of-sale application for Verifone Verix/Vx680 terminals.

This repository preserves an older production-style embedded POS/ticketing application, including source code, resources, Visual Studio/Verix build files, deployment scripts, and test scaffolding.

## Project Status

This is a historical portfolio codebase, not a modernized rewrite.

The code targets Verifone Verix/VxEOS tooling and requires proprietary SDK components that are not included in this repository.

## What It Does

- Runs on Verifone Vx680-style terminals
- Communicates over CommEngine, TCP/IP, and GPRS
- Handles login and operator flows
- Downloads event and ticketing data
- Processes sales flows
- Prints tickets and receipts
- Uses local bitmap and font resources
- Includes legacy unit-test scaffolding

## Repository Layout

```text
Source/                 Application C source
Include/                Application headers
Resources/              Terminal bitmap/font resources
Projects/rv/Debug/      Legacy Visual Studio / Verix build project
Output/rv/Download/     Hand-maintained terminal download scripts/config
Test/                   Test projects and CuTest-based test code
Test/cutest-1.5/        Vendored CuTest test framework
http-tiny-1.2/          Vendored HTTP reference/source package
iniparser/              Vendored INI parser source
libbmp-0.1.3/           Vendored BMP helper library
docs/                   Historical notes and supporting documentation
```

## Build Requirements

The original build expects:

- Windows
- RVDS 4.0 compiler
- Verifone VRXSDK 3.1.0A
- VxEOS SDK/update files
- ACT2000 components
- Verifone terminal download tools
- A compatible Verifone Vx680/VxEOS environment

These proprietary tools and SDKs are not included.

The main legacy build files are:

```text
Projects/rv/Debug/CESmpl.sln
Projects/rv/Debug/CESmpl.vcxproj
Projects/rv/Debug/CESmpl.smk
Projects/rv/Debug/build.bat
```

## Building

From a correctly configured Verix SDK environment:

```bat
cd Projects\rv\Debug
build.bat
```

The makefile currently assumes SDK paths similar to:

```text
C:\VxAps\VRXSDK
C:\VxAps\EOSSDK
C:\VxAps\ACT2000
```

Adjust `Projects/rv/Debug/CESmpl.smk` or your local environment as needed.

## Deployment

Deployment/download helper files are preserved under:

```text
Output/rv/Download/
```

These scripts expect generated build outputs under ignored local directories such as:

```text
Output/rv/Files/debug/
Output/rv/obj/
```

Those generated outputs are intentionally not committed.

## SSL Certificates

Certificates and private keys are not included in this repository.

Developers must provide any required certificates locally. Do not commit:

```text
*.key
*.pem
*.pfx
*.p12
Output/rv/Files/Certs/
```

## Tests

The repository includes a legacy Visual Studio test project:

```text
Test/Ticplus/Ticplus.sln
```

Tests use CuTest from:

```text
Test/cutest-1.5/
```

The test project may require adjustment depending on local Visual Studio and SDK availability.

## Third-Party Components

This repository contains vendored third-party code:

| Component | Purpose | License status |
| --- | --- | --- |
| CuTest 1.5 | C unit test framework | zlib/libpng-style license included |
| http-tiny 1.2 | HTTP reference/source package | Artistic License included |
| libbmp 0.1.3 | BMP image handling | Source headers mention LGPL; full license file is not currently present |
| iniparser | INI parsing | License unclear in current tree |

## Not Included

The repository intentionally does not include:

- Verifone proprietary manuals
- Verifone SDKs/toolchains
- Generated build outputs
- Debugger artifacts
- Private keys or certificates
- IDE databases/user files

## Historical Notes

The original Verix/CommEngine build notes are preserved in:

```text
docs/legacy-verix-build-notes.txt
```

They are useful for understanding the original embedded build environment, but some instructions reference files that are now intentionally excluded from the public repository.
