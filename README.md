<!--
SPDX-FileCopyrightText: 1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
SPDX-License-Identifier: LicenseRef-TLE and MIT
-->

The Last Eichhof
================

**[The Last Eichhof][mobygames.tle]** is a space shooter-type game for DOS, originally developed in 1993 by the **[Alpha Helix Productions][mobygames.ahp]**. It was open-sourced by Dany Schoch sometime around 1995.

Screenshots
-----------
![Game title screen][screenshot.title]

![Game menu][screenshot.menu]

![Stage 1 and first enemy encounter][screenshot.stage-1]

Original game credits:

![Graphix & fonts by Tweety, code & levels by Tritone][screenshot.credits-1]

![Additional programming and music by Zynax of DarkSys][screenshot.credits-2]

How to Play
-----------
The Last Eichhof is a DOS game, distributed as a 16-bit executable file for DOS. The recommended way of running it on a modern machine is via [DOSBox][dosbox] or [DOSBox-X][dosbox-x]. The instructions for running both are similar:
1. Download the archive from the [Releases][releases] page.
2. Extract the archive somewhere on your disk.
3. Start DOSBox or DOSBos-X emulator.
4. Type the following commands:

   ```console
   > mount c <full path where the game is on your host machine>
   > c:
   > baller
   ```
5. Enjoy the game!

Documentation
-------------
- [Contributor Guide][docs.contributing]

A Word From the Original Author
-------------------------------
Most tools won't be useful to you because they are all
programmed in a hurry. Bad or even not documented at all,
only I know how to use them. (I hate them too, don't worry).
The game source might be of better use to you. It's fairly good
(however far from perfect) commented in the source.
Just have a look at it. I think if you want to learn to
programm graphics or sound you better have a look at some
specialized packages not this one.

Anyway, if you find this package useful send me a mail,
well even send me a mail if you don't find it useful at all.
I'm looking forward to hearing from you.

dschoch@stud.ee.ethz.ch
Danny.

License
-------
The license indication in the project's sources is compliant with the [REUSE specification v3.3][reuse.spec].

The main licenses present across the sources of this repository are [the MIT license][docs.license.mit] and [a custom permissive license][docs.license.tle].

[docs.contributing]: CONTRIBUTING.md
[docs.license.mit]: LICENSES/MIT.txt
[docs.license.tke]: LICENSES/LicenseRef-TLE.txt
[dosbox-x]: https://dosbox-x.com/
[dosbox]: https://www.dosbox.com/
[mobygames.ahp]: https://www.mobygames.com/company/1020/alpha-helix-productions/
[mobygames.tle]: https://www.mobygames.com/game/925/the-last-eichhof/
[releases]: https://github.com/ForNeVeR/the-last-eichhof/releases
[screenshot.credits-1]: docs/credits.1.png
[screenshot.credits-2]: docs/credits.2.png
[screenshot.menu]: docs/screen.menu.png
[screenshot.stage-1]: docs/screen.stage-1.png
[screenshot.title]: docs/screen.title.png
