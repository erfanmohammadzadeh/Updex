# Updex

Updex sits beside a program on Windows or Linux, checks that program’s version against the latest GitHub release, and replaces the program files when a newer release is available.

## Requirements

- Qt 5.15 with qmake and a C++17 compiler
- Windows 10 or later, or Linux
- On Linux, `unzip` to unpack a release zip

## Build on Linux

```sh
qmake Updex.pro
make
```

Put the `Updex` binary beside the program. Linux has no Windows file-version resource, so the installed version is read from `version.txt` in that folder. A release asset can be a `.zip`, an AppImage, or a single binary.

## Build on Windows

Open `Updex.pro` in Qt Creator and build Desktop Qt 5.15.2 MinGW 32-bit, or from a shell:

```bat
qmake Updex.pro
mingw32-make
```

Copy `Updex.exe` into the same folder as the program you want to update. Qt Creator’s run settings, or `windeployqt`, need to place the Qt DLLs next to the exe before it will start outside Creator.

## Use

1. Start Updex from the program folder.
2. Set the program path. If only one other program is in the folder, Updex selects it.
3. Set the GitHub repository, for example `https://github.com/owner/name`. `owner/name` and `git@github.com:owner/name.git` also work.
4. Add a token only when the repository is private.
5. **Check** compares the installed version with the latest GitHub release.
6. **Update** downloads that release, closes the program if it is running, and copies the new files into the program folder.
7. Turn on **Check every** and choose a number of days or weeks. The first check runs a few seconds after you enable it. Later checks wait for that full period, including after Updex is restarted. When a scheduled check finds a newer release, Updex asks before replacing files.

Updex writes `updex.json` next to itself. That file is local and is not part of this repository, because it can hold a token.

```json
{
    "repository": "https://github.com/owner/name",
    "targetExecutable": "Program.exe",
    "token": "",
    "assetContains": "",
    "scheduleEnabled": false,
    "checkEvery": 1,
    "checkUnit": "day",
    "lastCheckEpoch": 0
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
