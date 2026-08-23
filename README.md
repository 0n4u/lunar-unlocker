# LunarUnlocker

> **Windows x64 Lunar Client runtime customization and cosmetic research project.**

LunarUnlocker is a Windows x64 native loader, JVM bridge, and Java bytecode transformation project designed around Lunar Client.

It provides runtime cosmetic-related functionality for cosmetics, badges, emotes, sprays, jams, and Lunar+ features while keeping modifications in memory rather than permanently modifying Lunar Client files.

The project combines:

- Windows x64 native injection
- JVM/JNI integration
- Java bytecode transformation
- Runtime class and member mapping
- Lunar Client runtime hooks
- Cosmetic service integration
- Relogin/service-state handling
- In-game configuration
- Multi-version Minecraft mapping infrastructure

---

## Overview

LunarUnlocker operates against a running 64-bit Minecraft JVM.

At a high level, the native loader locates the selected Minecraft process, establishes the native/JVM bridge, loads the Java payload, and initializes the Lunar-specific runtime components.

The Java layer contains the bytecode transformation, mapping, reflection, runtime, GUI, and Lunar integration logic used by the project.

```text
┌─────────────────────────┐
│    LunarUnlocker.exe    │
│     Windows Loader      │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│ LunarUnlockerNative.dll │
│ Native JVM / JNI Bridge │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│     Minecraft JVM       │
│                         │
│ LunarUnlocker Payload   │
└────────────┬────────────┘
             │
             ├── Bytecode transformers
             ├── Runtime mappings
             ├── Lunar hooks
             ├── Cosmetic integration
             ├── Relogin handling
             └── In-game UI

```

---

# Features

## Lunar Client Runtime Integration

LunarUnlocker includes runtime hooks for Lunar Client functionality such as:

- Cosmetics
- Badges
- Emotes
- Sprays
- Jams
- Lunar+
- Cosmetic catalog handling
- Runtime service-state handling
- Relogin/session replay logic
- Client-side configuration

The implementation is performed at runtime and does not require permanently replacing Lunar Client files.

> Lunar Client is actively updated. Internal class names, service behavior, mappings, and runtime implementation details can change and may temporarily break compatibility.

---

## In-Memory Operation

The project is designed around runtime modification rather than static client patching.

The runtime architecture includes:

- Native process attachment
- JVM interaction
- Runtime Java payload loading
- Class discovery
- Bytecode transformation
- Runtime mapping
- Reflection helpers
- Native-to-Java communication
- Bytecode caching

This makes the project useful for studying JVM instrumentation and compatibility across changing Minecraft/Lunar environments.

---

## Native Windows Loader

`LunarUnlocker.exe` provides the Windows x64 loader interface.

The loader is responsible for functionality including:

- Minecraft process discovery
- Process selection
- Native injection
- Payload initialization
- Runtime bootstrap
- Configuration
- Injection status
- Lunar integration startup

The corresponding native payload is:

```text
LunarUnlockerNative.dll

```

which provides the low-level bridge between the loader and the target JVM.

---

## Configuration

The loader exposes runtime configuration for supported Lunar-related functionality.

Available options can include:

- Auto Inject
- Cosmetics
- Badges
- Emotes
- Sprays
- Jams
- Lunar+
- Debug mode
- Language

# Bytecode Transformation

The Java payload contains ASM-based bytecode transformation infrastructure.

This includes functionality for:

- Inspecting loaded classes
- Matching instructions
- Transforming runtime bytecode
- Maintaining transformed-class state
- Runtime method/class mapping
- Version-specific name remapping
- Native bridge registration

The build currently uses libraries including:

- ASM 9.7.1
- Javassist
- Gson
- Guava
- Netty
- LWJGL


---

# Runtime Mapping System

Minecraft and Lunar Client change internal names and structures between releases.

LunarUnlocker contains mapping infrastructure intended to isolate much of that version-specific behavior.

Mapping-related components include:

- Class name remapping
- Member name remapping
- Identity mapping tables
- Version-specific mapping tables
- Lunar mappings
- Minecraft mappings
- SRG-style mapping resources
- Runtime reflection helpers

Resources are organized under the project's mapping directories.

The repository contains mapping infrastructure intended for environments including:

- Vanilla
- Forge
- Fabric
- NeoForge
- Lunar Client

### Compatibility Notice

Mapping data being present does **not** guarantee that every LunarUnlocker feature works on every Minecraft release.

Features can depend on:

- Lunar Client internals
- Minecraft mappings
- JVM implementation
- Class-loader behavior
- Rendering implementation
- Networking implementation
- Loader-specific behavior

Version-specific maintenance may therefore be required.

---

# Repository Structure

```text
lunar-unlocker/
│
├── build.gradle
│   Main Gradle build and packaging configuration
│
├── settings.gradle
│   Dependency repositories and Gradle 8.8 requirement
│
├── gradle.properties
│   Project and Gradle configuration
│
├── gradlew
├── gradlew.bat
│   Bundled Gradle wrapper
│
├── build.bat
│   One-click Windows toolchain setup and build script
│
├── gradle/
│   └── wrapper/
│       Gradle Wrapper files
│
├── native/
│   Windows x64 native loader, injector,
│   JVM bootstrap, bridge, and GUI
│
├── src/main/
│   │
│   ├── java/
│   │   └── gg/lunarunlocker/
│   │       │
│   │       ├── asm/
│   │       │   Bytecode transformation and matching
│   │       │
│   │       ├── mapping/
│   │       │   Runtime mapping and transformation logic
│   │       │
│   │       ├── reflect/
│   │       │   Reflection and mapping helpers
│   │       │
│   │       ├── lunar/
│   │       │   Lunar-specific integration and runtime hooks
│   │       │
│   │       ├── runtime/
│   │       │   Native bridge, cache, and runtime support
│   │       │
│   │       ├── ui/
│   │       │   In-game interface components
│   │       │
│   │       ├── wrapper/
│   │       │   Client/class-loader wrappers
│   │       │
│   │       ├── event/
│   │       │   Runtime event infrastructure
│   │       │
│   │       └── ...
│   │
│   └── resources/
│       │
│       ├── mappings/
│       │   Mapping tables and version data
│       │
│       └── resources/
│           Fonts, shaders, UI resources,
│           localization, and other assets
│
├── tools/
│   Recovery, analysis, and build utilities
│
├── LICENSE
└── README.md

```

---

# Build Outputs

A complete native build generates:

| ArtifactDescription       |                                         |
| ------------------------- | --------------------------------------- |
| `LunarUnlocker.exe`       | Windows x64 loader and injector         |
| `LunarUnlockerNative.dll` | Native JVM bridge and payload bootstrap |
| Injection JAR             | Java runtime payload                    |

The intermediate native bundle is generated under:

```text
build/injection/

```

The one-click build script then copies the final executable and native DLL to the parent output directory.

---

# Requirements

## Running

- Windows x64
- 64-bit Minecraft JVM
- Lunar Client for Lunar-specific functionality
- A compatible Minecraft/Lunar Client version

## Building

The build system uses:

- JDK 17
- Git
- CMake
- Visual Studio C++ Build Tools
- Windows SDK
- Gradle 8.8
- Windows Package Manager (`winget`) for automatic dependency installation

The included build script can detect several missing development dependencies and install them automatically.

Administrator privileges may be requested when build dependencies need to be installed.

---

# Building

## One-Click Build

Open Command Prompt in the repository directory and run:

```bat
build.bat

```

The script checks the required toolchain and then builds the full Windows x64 package.

The full build performs the equivalent of:

```bat
gradlew.bat clean prepareInjectionBundle -PtargetRelease=8

```

along with the required native Java configuration.

---

## Check the Toolchain

To inspect the development environment without installing missing components:

```bat
build.bat check

```

The script checks components including:

- Git
- JDK 17
- CMake
- Visual Studio C++ Build Tools
- Project source

---

## Build Java Only

To compile and verify only the Java injection payload:

```bat
build.bat javaonly

```

This runs the Java build and payload verification without compiling the native Windows components.

---

# Advanced Gradle Tasks

Developers can also use the Gradle wrapper directly.

## Compile the Project

```bat
gradlew.bat clean build

```

---

## Build the Injection JAR

```bat
gradlew.bat injectionJar

```

---

## Verify the Injection Payload

```bat
gradlew.bat verifyInjectionPayload

```

The verification task checks that required LunarUnlocker runtime classes and bundled dependencies exist and verifies that the payload can be loaded by its intended Java target.

---

## Build Native Components

```bat
gradlew.bat buildNative

```

This builds:

```text
LunarUnlocker.exe
LunarUnlockerNative.dll

```

using CMake and the Windows x64 C++ toolchain.

---

## Build the Complete Bundle

```bat
gradlew.bat prepareInjectionBundle -PtargetRelease=8

```

Output:

```text
build/injection/
├── LunarUnlocker.exe
└── LunarUnlockerNative.dll

```

---

# Java Compatibility

The build system itself uses a **Java 17 toolchain**.

The Java compiler target can be selected using:

```text
-PtargetRelease=<version>

```

The complete injection build uses:

```text
-PtargetRelease=8

```

to produce payload bytecode suitable for older JVM environments commonly used by legacy Minecraft versions.

This allows a modern JDK to be used for development while still targeting older Minecraft Java runtimes where required.

---

# Usage

> Only use runtime modification software on systems and environments where you have permission to do so.

1. Start Minecraft using a **64-bit JVM**.
2. Allow Minecraft/Lunar Client to finish loading.
3. Run:

```text
LunarUnlocker.exe

```

4. Select the intended Minecraft Java process.
5. Start the runtime initialization.
6. Wait for the payload to initialize.
7. BOOOM YOU HAVE COSMETICCCCCCC


---

# Compatibility

LunarUnlocker is primarily designed around Windows x64 and Lunar Client.

| EnvironmentStatus |                                |
| ----------------- | ------------------------------ |
| Windows x64       | Required                       |
| 64-bit JVM        | Required                       |
| Lunar Client      | Primary target                 |
| Minecraft 1.8.9   | Legacy mapping target          |
| Forge             | Mapping infrastructure present |
| Fabric            | Mapping infrastructure present |
| NeoForge          | Mapping infrastructure present |
| Vanilla           | Mapping infrastructure present |

### Important

The mapping system spans multiple Minecraft generations, but this should not be interpreted as a guarantee that every feature works identically on every version.

Lunar Client updates frequently.

A Lunar update may modify:

- Class names
- Method names
- Service endpoints
- Internal state
- Authentication flow
- Cosmetic implementation
- Class loaders
- JVM launch behavior

When that happens, mapping or integration updates may be necessary.

---

# Project Architecture

The project can be divided into four primary layers.

## 1. Native Loader

```text
LunarUnlocker.exe

```

Handles process selection, loader configuration, and runtime startup.

## 2. Native Bridge

```text
LunarUnlockerNative.dll

```

Handles native interaction with the target Minecraft JVM.

## 3. Java Runtime

The injection JAR provides:

- Bytecode transformation
- Runtime mapping
- Reflection
- Class discovery
- Lunar hooks
- Event handling
- UI logic
- Runtime state

## 4. Lunar Integration

The Lunar-specific layer handles the portions of the project that interact with Lunar Client runtime behavior.

```text
Windows Loader
      │
      ▼
Native JVM Bridge
      │
      ▼
Java Runtime
      │
      ├── ASM / Bytecode
      ├── Mapping
      ├── Reflection
      ├── Runtime Cache
      └── Lunar Integration

```

---

# Recovery Status

```

The repository should be treated as a **reconstructed/recovered codebase**, not as an official Lunar Client source release.

Recovered components can include:

- Decompiled structures
- Reconstructed behavior
- Compatibility fixes
- Recreated interfaces
- Recovered resources
- Runtime-specific adaptations
- Project-specific additions

Behavior may differ from the original software or from previous recovery sources.

---

# Upstream Projects & Related Recovery Work

LunarUnlocker builds on two main lines of prior open-source work: the recovered Vape v4.21 runtime architecture and independent Lunar Client patching research.

## RSSeeker/Vape-v4.21

**Repository:** https://github.com/RSSeeker/Vape-v4.21

RSSeeker's Vape v4.21 recovery project is an important upstream reference for the recovered `4.21` Java/native architecture used by this project.

Relevant areas include:

- Recovered Vape v4.21 client structure
- Java runtime organization
- Native Windows/JVM integration
- Runtime mapping infrastructure
- Injection/bootstrap architecture
- Recovery and reconstruction work around the 4.21 codebase

LunarUnlocker reuses and adapts parts of that recovered architecture while narrowing the project around Lunar Client runtime integration.

## Prometheus Reengineering / minecraft-lunar

**Repository:** https://github.com/prometheusreengineering/minecraft-lunar

Prometheus Reengineering's `minecraft-lunar` project is an important upstream reference for Lunar Client-specific patching and cosmetic research.

Relevant areas include:

- Lunar Client runtime patching
- Cosmetic-related behavior
- Emotes
- Sprays
- Badges
- Fabric/Lunar compatibility work
- Client-side patching techniques

LunarUnlocker combines Lunar-focused research with its own Windows x64 loader, native JVM bridge, bytecode transformation layer, mapping system, build workflow, and in-game configuration.

## Project Lineage

```text
OpenVape / earlier Vape recovery work
                │
                ▼
      RSSeeker/Vape-v4.21
                │
                ├──────────────┐
                │              │
                ▼              │
0n4u/vape-v4.21-unlocker       │
                │              │
                └──────┐       │
                       ▼       │
             0n4u/lunar-unlocker
                       ▲
                       │
      prometheusreengineering/
             minecraft-lunar
```

The upstream projects contribute different kinds of prior work:

| Project | Role in LunarUnlocker |
|---|---|
| `RSSeeker/Vape-v4.21` | Recovered 4.21 Java/native architecture, JVM integration, mappings, and recovery foundation |
| `prometheusreengineering/minecraft-lunar` | Lunar Client patching and cosmetic-related research |
| `0n4u/vape-v4.21-unlocker` | Broader recovered Vape project with Lunar integration |
| `0n4u/lunar-unlocker` | Standalone Lunar-focused loader, runtime integration, build system, UI, and project-specific changes |

Where code, mappings, recovered structures, patches, or implementation techniques originate from an upstream project, preserve the applicable attribution and license notices.

LunarUnlocker is focused specifically on the Lunar Client runtime so users interested in the Lunar functionality do not need to navigate the larger recovered Vape client project.

# Security

LunarUnlocker performs native process interaction and runtime JVM modification.

Software using these techniques can trigger antivirus, endpoint-security, or behavioral-detection systems because similar primitives are also used by unrelated software.

A detection should **not** automatically be ignored.

Users are encouraged to:

- Review the source code
- Build the project themselves
- Inspect the native implementation
- Review dependencies
- Verify release hashes when available
- Test unknown builds in an isolated environment
- Never enter sensitive credentials into an untrusted build

Do not disable security software solely because a detection occurred.

Understand what triggered the detection first.

---

# Privacy

If releases are distributed publicly, the repository should clearly document any network communication performed by the loader or Java payload.

Do not claim that the software has:

- No telemetry
- No credential collection
- No remote communication
- No account data processing

unless those claims have been verified against the current source.

For maximum trust, users should be able to build the same artifacts directly from the public source tree.

---

# Reporting Bugs

Before opening an issue:

1. Update to the latest source.
2. Confirm Minecraft is using a 64-bit JVM.
3. Confirm the issue can be reproduced.
4. Record the Minecraft version.
5. Record the Lunar Client version/build if available.
6. Record the Windows version.
7. Save relevant logs.
8. Describe exactly when the failure occurs.

Recommended issue information:

```text
Windows version:
Minecraft version:
Lunar Client version:
Java version:
LunarUnlocker commit/build:
Feature affected:
Expected behavior:
Actual behavior:
Steps to reproduce:
Relevant log:

```

Never include:

- Passwords
- Session tokens
- Access tokens
- Cookies
- Private account information

in a public issue.

---

# Contributing

Contributions are welcome.

Useful areas include:

- Lunar Client compatibility updates
- Mapping corrections
- Bytecode transformation fixes
- Runtime stability
- Crash fixes
- Native loader improvements
- JVM compatibility
- Build improvements
- Documentation
- UI improvements
- Version testing
- Code cleanup
- Automated tests

Keep changes focused and document compatibility-sensitive modifications clearly.

---

# Development Guidelines

When modifying the runtime:

- Avoid hardcoding version-specific names when a mapping abstraction exists.
- Keep native/JVM boundaries small and documented.
- Validate transformed bytecode before release.
- Test Java 8-targeted payloads when modifying injected classes.
- Keep third-party dependencies declared through Gradle.
- Avoid committing generated build artifacts into the source tree.
- Document changes that depend on a specific Lunar Client version.

---

# Disclaimer

LunarUnlocker is an independent software recovery, interoperability, compatibility, and research project.

It is **not affiliated with, endorsed by, sponsored by, or maintained by Lunar Client, Moonsworth, Mojang Studios, or Microsoft.**

The Lunar Client, Minecraft, related names, trademarks, artwork, services, cosmetics, and other third-party intellectual property remain the property of their respective owners.

This repository does not grant rights to:

- Third-party accounts
- Online services
- Paid content
- Cosmetics
- Subscription features
- Trademarks
- Proprietary game/client assets
- Other third-party intellectual property

Users are responsible for ensuring that their use of this project complies with:

- Applicable law
- Software licenses
- Service terms
- Server rules
- Account agreements
- Other applicable third-party policies

---

# License

This repository uses the **CC0 1.0 Universal** public-domain dedication for material contributors are legally able to dedicate.

See:

```text
LICENSE
```

CC0 applies only to material that contributors to this repository have the legal right to dedicate.

It does **not** override licenses, copyright, attribution requirements, or other rights attached to upstream or third-party material.

In particular:

- `prometheusreengineering/minecraft-lunar` is distributed under the **GNU GPL v3.0**.
- `RSSeeker/Vape-v4.21` and projects in its upstream recovery chain remain subject to their respective license and attribution requirements.
- Third-party libraries, recovered/decompiled material, trademarks, assets, fonts, textures, client resources, and other existing intellectual property remain subject to their respective rights and licenses.

If source code from a GPL-licensed upstream project is incorporated into LunarUnlocker, redistribution must comply with the applicable GPL requirements. A CC0 declaration in this repository does not relicense third-party GPL code as CC0.

# Credits

LunarUnlocker exists because of work from multiple open-source recovery and research projects.

## RSSeeker

**Project:** [RSSeeker/Vape-v4.21](https://github.com/RSSeeker/Vape-v4.21)

Credit for Vape v4.21 recovery/reconstruction work, including the recovered Java/native architecture, JVM integration, runtime structure, mappings, and related recovery foundation used as a reference by this project.

## Prometheus Reengineering

**Project:** [prometheusreengineering/minecraft-lunar](https://github.com/prometheusreengineering/minecraft-lunar)

Credit for Lunar Client patching research and prior open-source work involving Lunar cosmetics, emotes, sprays, badges, and runtime patching techniques.

## OpenVape / Earlier Recovery Work

Earlier public Vape recovery projects contributed to the wider recovery ecosystem that eventually led to the RSSeeker reconstruction and related downstream projects.

Where their work is present through the upstream recovery chain, their original attribution and licensing should be preserved.

## 0n4u / Vape v4.21 Unlocker

**Project:** [0n4u/vape-v4.21-unlocker](https://github.com/0n4u/vape-v4.21-unlocker)

The broader recovered Vape v4.21 project with Lunar Client integration.

## 0n4u / LunarUnlocker

**Project:** [0n4u/lunar-unlocker](https://github.com/0n4u/lunar-unlocker)

Standalone Lunar-focused integration, Windows x64 loader, native JVM bridge, mapping infrastructure, build workflow, in-game UI, and project-specific modifications.

---

## Attribution Notice

This repository does not claim authorship of upstream work.

If code, mappings, recovered structures, patches, assets, or implementation techniques originate from another project, preserve the original attribution and comply with the applicable upstream license when redistributing or modifying the project.

