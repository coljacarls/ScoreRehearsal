# Licensing

This is a plain-English summary. The legally binding texts are
[LICENSE](LICENSE) and the license files referenced in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## ScoreRehearsal is GPL-3.0-only

ScoreRehearsal is Copyright (C) 2026 Colja Carls and is licensed under the GNU
General Public License, version 3 only (SPDX: `GPL-3.0-only`). New source
files carry an SPDX header saying so.

## Built from MuseScore Studio code

ScoreRehearsal reuses source code from MuseScore Studio and the Muse Framework,
which are released under GPL-3.0 (Copyright MuseScore Limited and others). It
is a downstream project that receives this code under the GPL like any other
user. No Contributor License Agreement (CLA) is involved: the MuseScore CLA only
applies to contributions sent to the MuseScore project itself. Contributions to
ScoreRehearsal are accepted under GPL-3.0-only.

## What this means if you redistribute it

If you pass on ScoreRehearsal (original or modified, as source or as binaries):

- Release it under GPL-3.0-only as well.
- Make the complete corresponding source code available to everyone who
  receives the binaries, including your changes and build scripts.
- Keep all copyright notices, license files and THIRD_PARTY_NOTICES.md intact,
  and mark your changes as yours.
- Respect the licenses of bundled components: the sound font (MIT), the fonts
  (mostly SIL OFL 1.1, some GPL with font exception) and libraries such as
  FluidSynth and libsndfile (LGPL) and Qt (LGPL-3.0, dynamically linked).

Using ScoreRehearsal, or scores you open or print with it, puts no obligation
on you. The font exception means embedding the fonts in a document does not by
itself make that document GPL-licensed.

## Trademarks

"MuseScore" and the MuseScore logo are trademarks of their respective owners.
ScoreRehearsal does not use them as its name or logo and is not affiliated with
or endorsed by MuseScore. The name "MuseScore" is used only descriptively, to
state where the code comes from and which file formats the app can open.

## Where the notices are

[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) lists every third-party
component in the built application with its version, license, copyright holder
and the location of its full license text.

## Not part of this project

MuseSounds, Muse Hub and MuseSampler are proprietary products of MuseScore
Limited / Muse Group. They are not part of ScoreRehearsal, are not included in
its builds, and ScoreRehearsal does not download, load or promote them. The only
sound source is the MIT-licensed "MS Basic" sound font.
