# Updex

Updex sits beside a Windows program, checks that program’s version against the latest GitHub release, and replaces the exe and the other program files when a newer release is available.

## Requirements

- Windows 10 or later
- Qt 5.15 with qmake (the project kit is Desktop Qt 5.15.2 MinGW 32-bit)
- A C++17 compiler

## Build

Open `Updex.pro` in Qt Creator and build the kit above, or from a shell:

```bat
qmake Updex.pro
mingw32-make
```

Copy `Updex.exe` into the same folder as the program you want to update. Qt Creator’s run settings, or `windeployqt`, need to place the Qt DLLs next to the exe before it will start outside Creator.

## Use

1. Start Updex from the program folder.
2. Set the program exe. If only one other exe is in the folder, Updex selects it.
3. Set the GitHub repository, for example `https://github.com/owner/name`. `owner/name` and `git@github.com:owner/name.git` also work.
4. Add a token only when the repository is private.
5. **Check** compares the installed version with the latest GitHub release.
6. **Update** downloads that release, closes the program if it is running, and copies the new files into the program folder.

Updex writes `updex.json` next to itself. That file is local and is not part of this repository, because it can hold a token.

```json
{
    "repository": "https://github.com/owner/name",
    "targetExecutable": "Program.exe",
    "token": "",
    "assetContains": ""
}
```

`targetExecutable` can be a file name when the program is in the same folder as Updex, or a full path otherwise. `assetContains` picks one asset when a release has several zip files. It is matched against the asset file name.

The installed version is the Windows file version of the exe. If the exe has no version resource, put `version.txt` in the same folder with a line such as `1.2.3`.

## Publishing a release

Create a GitHub release whose tag contains a version, such as `v1.2.3`. Attach a `.zip` of the program files. Updex copies the zip contents into the program folder. A zip that contains a single top-level folder is unpacked from that folder.

Updex does not replace `Updex.exe` or `updex.json`. Files that were overwritten are copied to `updex-backup/<timestamp>` first. If the release has no zip, a single `.exe` asset replaces the target program exe.

## Project layout

| Path | Role |
| --- | --- |
| `src/domain` | Version, release, and installed program. No Qt. |
| `src/application` | Check and apply use cases, and the ports they depend on. |
| `src/infrastructure` | GitHub releases, Windows version reading, zip install, and `updex.json`. |
| `src/presentation` | Model, main window, and controller. |
| `main.cpp` | Composition root. |

`main.cpp` constructs the infrastructure and passes it into the use cases and the controller. The window only displays the model and sends user actions to the controller.

## What stays out of git

`.gitignore` keeps Qt Creator user files, build output, and `updex.json` untracked. Do not commit a GitHub token. `Updex.pro.user` is a local Qt Creator file and is ignored.
