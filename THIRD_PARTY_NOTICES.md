# Third-Party Notices

ScoreRehearsal is Copyright (C) 2026 Colja Carls and is licensed under the GNU
General Public License version 3 only (GPL-3.0-only, see [LICENSE](LICENSE)).
It is built from MuseScore Studio and Muse Framework source code and ships the
fonts, sound font and libraries listed below. Each component remains under its
own license; this file reproduces the required notices. Paths refer to the
MuseScore Studio source tree (`musescore/`) used to build ScoreRehearsal; the
full license texts are in the files named there, and the build installs the
license files of prebuilt libraries next to the libraries.

Components that exist in the MuseScore source tree but are **not** built into
ScoreRehearsal are listed at the end for clarity.

---

## Application code

### MuseScore Studio

- Version: 5.0.0 development tree (upstream `master`, commit `d61794caf5`)
- License: GPL-3.0-only (SPDX headers `SPDX-License-Identifier: GPL-3.0-only`)
- Copyright: Copyright (C) 1999-2026 MuseScore Limited and others
- Path: `musescore/src/`, `musescore/share/`
- License text: `musescore/LICENSE.txt`

MuseScore Studio's `LICENSE.txt` states the following font exception:

> All fonts included in MuseScore Studio are made available under the GNU
> Freefont License, and those fonts are covered by the following special
> exception:
>
> - If you create a document which uses fonts included in MuseScore Studio,
> and embed this font or unaltered portions of this font into the document,
> then this font does not by itself cause the resulting document to be
> covered by the GNU General Public License. This exception does not
> however invalidate any other reasons why the document might be covered
> by the GNU General Public License. If you modify this font, you may
> extend this exception to your version of the font, but you are not
> obligated to do so. If you do not wish to do so, delete this exception
> statement from your version.

Note: several fonts listed below carry their own license (SIL OFL 1.1); for
those, the font's own license file governs.

### Muse Framework

- Version: submodule `musescore/muse`, commit `fd52f42a7`
- License: GPL-3.0 (source headers: GPL-3.0-only)
- Copyright: Copyright (C) MuseScore/Audacity and others; MuseScore Limited and others
- Path: `musescore/muse/framework/`
- License text: `musescore/muse/LICENSE.txt` (same font exception as above)

---

## Sound font

### MS Basic (derived from FluidR3Mono / Fluid R3)

- Version: 0.2 (`share/sound/SF_VERSION`: 0.2.0), installed as `sound/MS Basic.sf3`
- License: MIT
- Path: `musescore/share/sound/MS Basic.sf3`
- License text: `musescore/share/sound/MS Basic_License.md` (installed alongside
  the sound font); see also `FluidR3Mono_License.md`

Required acknowledgements and copyright notices, verbatim from
`MS Basic_License.md` ("The acknowledgements and copyright notices above must
be included in any derivative work."):

```
FluidR3 (original version) by Frank Wen Copyright (c) 2000-02

Mono conversion (FluidR3Mono) by Michael Cowgill Copyright (c) 2014-17

Adaptation for MuseScore_General.sf2 by S. Christian Collins Copyright (c) 2018-19

Temple Blocks instrument provided by Ethan Winer Copyright (c) 2002

Drumline Cymbals provided by Michael Schorsch Copyright (c) 2016

MuseScore_General.sf2 is shared under the MIT license as described in COPYING, as was FluidR3Mono and FluidR3 before it.
```

```
Mono version:  Copyright (c) 2014-16 Michael Cowgill
Copyright (c) 2000-2002, 2008 Frank Wen <getfrank@gmail.com>

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
```

---

## Fonts

All paths are under `musescore/fonts/`.

| Font | Version | License | Copyright / Reserved Font Name | Path and license file |
|---|---|---|---|---|
| Bravura, Bravura Text | 1.392 | SIL OFL 1.1 | Copyright © 2015 / 2019, Steinberg Media Technologies GmbH; RFN "Bravura" | `bravura/` - `LICENSE.txt`, `OFL.txt` |
| Leland, Leland Text | 0.80 | SIL OFL 1.1 | Copyright (c) 2025, MuseScore Limited; RFN "Leland" | `leland/` - `LICENSE.txt` |
| Edwin (Roman, Italic, Bold, Bold Italic) | 0.54 | SIL OFL 1.1 | Copyright (c) 2021, MuseScore Limited; RFN "Edwin". Renamed and modified from URW++ C059 (Core 35, released by URW++ under OFL 1.1 without RFN) | `edwin/` - `LICENSE.txt`, `README.md` |
| Petaluma, Petaluma Text, Petaluma Script | 1.065 | SIL OFL 1.1 | Copyright © 2018, Steinberg Media Technologies GmbH; RFN "Petaluma" | `petaluma/` - `OFL.txt` |
| MuseJazz, MuseJazz Text | 1.0 | SIL OFL 1.1 | Copyright © 2016, Torsten Hämmerle, Abraham Lee | `musejazz/` - `OFL.txt` |
| Finale Maestro, Finale Maestro Text | - | SIL OFL 1.1 | Copyright © 2021 MakeMusic, Inc.; RFNs "Finale", "Maestro", "Broadway", "Engraver", "Jazz", "Ash", "Legacy" | `finalemaestro/` - `OFL.txt` |
| Finale Broadway, Finale Broadway Text | - | SIL OFL 1.1 | Copyright © 2021 MakeMusic, Inc.; same RFNs as Finale Maestro | `finalebroadway/` - `OFL.txt` |
| Campania | - | SIL OFL 1.1 | Copyright (c) 2019, Marc Sabatella; RFN "Campania" | `campania/` - `LICENSE` |
| Gootville, Gootville Text | 1.3 | SIL OFL 1.1 (stated in `readme.txt`; no license file in the directory) | Designed by Grzegorz Pruchniakowski; based on Gonville and on SMuFL/Bravura templates | `gootville/` - `readme.txt` |
| MScore (formerly Emmentaler) | 2.003 | GPL-3.0-or-later with font exception | Copyright (c) 2013-2020 Werner Schweer, John Pirie, Matt McClinch, Nicolas Froment, Maurizio M. Gavioli, mirabilos; parts derived from Parnassus (Kristof Bastiaensen, GPL with font exception) and Emmentaler / GNU LilyPond (Han-Wen Nienhuys, Jan Nieuwenhuizen and others) | `mscore/mscore.sfd` (copyright field) |
| MScore Text | 1.0 | SIL OFL 1.1 | Copyright (c) 2014, Werner Schweer and Others; RFN "MScore Text" | `mscore/MScoreText.sfd` (font info) |
| MScore BC (figured bass) | - | SIL OFL 1.1 | Copyright (c) 2012-15, Maurizio M. Gavioli; RFN "MScoreBC" | `mscore-BC.sfd` (font info) |
| MuseScore Tab fonts (Modern, Renaiss, Italian, French, French Baroque, Dowland, Phalese, Lute Didactic, Bonneuil, ...) | - | SIL OFL 1.1 | Copyright (c) 2012-15, Maurizio M. Gavioli; RFN "MScoreTabulature" | `MuseScoreTab*.sfd` (font info) |
| FreeSerif (Regular, Bold, Italic, Bold Italic), FreeSans | 2012 | GPL-3.0-or-later with font exception (GNU FreeFont) | Copyright 2002, 2003, 2005, 2008, 2009, 2010, 2012 GNU Freefont contributors | `FreeSerif*.sfd`, `FreeSans.sfd` (font info); https://savannah.gnu.org/projects/freefont/ |
| SMuFL metadata (`glyphnames.json`, `classes.json`, `ranges.json`) | - | No license file in the directory | W3C Music Notation Community Group (https://github.com/w3c/smufl) | `smufl/README.md` |

The SIL Open Font License 1.1 text is included in each OFL font's license file
listed above and at https://openfontlicense.org.

---

## Bundled third-party source code

| Component | Version | License | Copyright | Path |
|---|---|---|---|---|
| FluidSynth (patched: no GLib dependency) | 2.3.3 | LGPL-2.1-or-later | Copyright (C) 2003 Peter Hanappe and others (2000-2023) | `muse/framework/audio/thirdparty/fluidsynth/fluidsynth-2.3.3/` - `LICENSE` |
| moodycamel ConcurrentQueue / BlockingConcurrentQueue | - | Simplified BSD (also dual-licensed under BSL-1.0) | Copyright (c) 2013-2020, Cameron Desrochers | `muse/framework/audio/thirdparty/moodycamel/concurrentqueue.h` (license in header) |
| moodycamel LightweightSemaphore | - | zlib license (Jeff Preshing's semaphore, adapted by Cameron Desrochers) | Jeff Preshing; Cameron Desrochers | `muse/framework/audio/thirdparty/moodycamel/lightweightsemaphore.h` |
| stb_vorbis | 1.22 | MIT or public domain (choice) | Copyright (c) 2017 Sean Barrett | `muse/framework/audio/thirdparty/stb/stb_vorbis.c` (license at end of file) |
| kors_async, kors_logger, kors_profiler | - | MIT | Copyright (c) 2020 Igor Korsukov | `muse/framework/global/thirdparty/kors_*/LICENSE` |
| kors_modularity | - | MIT | Copyright (c) 2023 Igor Korsukov | `muse/framework/global/thirdparty/kors_modularity/LICENSE` |
| kors_msgpack, kors_rpcqueue | - | MIT | Copyright (c) 2025 Igor Korsukov | `muse/framework/global/thirdparty/kors_*/LICENSE` |
| SG14 inplace_function | - | BSL-1.0 | (no copyright holder named in the file) | `muse/framework/global/thirdparty/sg14/inplace_function.h` (license in header) |
| dtl (Diff Template Library) | - | BSD-3-Clause | Copyright (c) 2015 Tatsuhiko Kubo | `src/engraving/thirdparty/dtl/COPYING` |
| intervaltree | - | MIT | Copyright (c) 2011 Erik Garrison | `src/engraving/thirdparty/intervaltree/LICENSE` |

---

## Prebuilt and fetched dependencies (muse_deps)

These are fetched by the build from `musescore/muse_deps` recipes and installed
together with their license files (`DEP_LICENSE_FILES` in each
`muse_deps/recipes/<name>/spec.cmake`). The recipes name the license files but
contain no license field; the licenses below are those of the upstream
projects.

| Component | Version | License (upstream) | License file(s) per recipe | Upstream |
|---|---|---|---|---|
| libogg | 1.3.5 | BSD-3-Clause | `COPYING` | https://github.com/xiph/ogg |
| libvorbis | 1.3.7 | BSD-3-Clause | `COPYING` | https://github.com/xiph/vorbis |
| FLAC (libFLAC, libFLAC++) | 1.4.3 | BSD-3-Clause (libraries; `COPYING.GPL`/`COPYING.LGPL` cover other parts of the FLAC distribution) | `COPYING.Xiph`, `COPYING.GPL`, `COPYING.LGPL` | https://github.com/xiph/flac |
| Opus | 1.5.2 | BSD-3-Clause | `COPYING` | https://github.com/xiph/opus |
| libsndfile | 1.2.2 | LGPL-2.1-or-later | `COPYING` | https://github.com/libsndfile/libsndfile |
| zlib | 1.3.1 | Zlib | `LICENSE` | https://github.com/madler/zlib |
| libpng | 1.6.50 | libpng License v2 | `LICENSE` | http://www.libpng.org |
| FreeType | 2.14.1 | FreeType License (FTL) or GPL-2.0 (choice) | `LICENSE.TXT` | https://freetype.org |
| HarfBuzz | 12.3.0 | MIT ("Old MIT") | `COPYING` | https://github.com/harfbuzz/harfbuzz |
| pugixml (header/source) | 1.15 | MIT | - | https://github.com/zeux/pugixml |
| picojson (header) | 111c9be | BSD-2-Clause | - | https://github.com/kazuho/picojson |
| utfcpp (header) | 4.1.1 | BSL-1.0 | - | https://github.com/nemtrif/utfcpp |
| Steinberg ASIO SDK (Windows only) | 2.3.4 (2025-10-15) | Dual license by Steinberg: GPL-3.0 or proprietary Steinberg license (used under GPL-3.0) | shipped in the SDK archive | https://www.steinberg.net/developers/ |

"ASIO" is a trademark of Steinberg Media Technologies GmbH.

---

## Qt

- Version: Qt 6 (6.8 or later)
- License: LGPL-3.0 (open-source license option of The Qt Company)
- Copyright: Copyright (C) The Qt Company Ltd. and other contributors
- Use: dynamically linked shared libraries (Core, Gui, Widgets, Network, Qml,
  Quick, QuickControls2, QuickWidgets, Xml, Svg, Core5Compat, PrintSupport;
  DBus on Linux). Users may replace the Qt libraries with their own builds.
- Source and license texts: https://www.qt.io/download-open-source,
  https://doc.qt.io/qt-6/licensing.html

---

## Present in the source tree but not included in ScoreRehearsal

These components are part of the MuseScore Studio / Muse Framework source tree
but their modules are switched off in ScoreRehearsal's build configuration
(`src/cmake/Configure.cmake`), so they are not part of the built application:

- KDDockWidgets (GPL-2.0-only or GPL-3.0-only, (C) Klarälvdalens Datakonsult AB),
  `muse/framework/dockwindow/thirdparty/KDDockWidgets/` - dockwindow module off.
- liblouis 3.24.0 (LGPL-2.1-or-later upstream), from `muse_deps` - only used by
  the braille module, which is not built.
- FFmpeg headers, `muse/framework/media/thirdparty/ffmpeg/` - video export only.
- rtf2html (LGPL, `COPYING.LESSER`), `src/importexport/capella/thirdparty/` - Capella import off.
- libmei, `src/importexport/mei/thirdparty/` - MEI import/export off.
- BeatRoot (GPL-2.0-or-later), `src/importexport/midi/thirdparty/` - MIDI import off.
- Audio export encoders (LAME, opusenc, fdk-aac) - audio export off.
- MuseSampler, MuseSounds, Muse Hub, VST, cloud and update services - not built.

---

## Trademarks

"MuseScore" and the MuseScore logo are trademarks of their respective owners.
ScoreRehearsal does not use these trademarks as its name or logo and is not
affiliated with, sponsored by or endorsed by MuseScore Limited or the MuseScore
project. The name "MuseScore" appears in this project only descriptively, to
state the origin of the reused source code and compatibility with MuseScore
file formats. Other names mentioned above are trademarks of their respective
owners.
