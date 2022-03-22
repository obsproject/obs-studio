# version
v0.8
Matthew Szatmary m3u8@twitch.tv / matt@szatmary.org

# libcaption

libcaption is a library written in C to aid in the creating and parsing of closed caption data, open sourced under the MIT license to use within community developed broadcast tools. To maintain consistency across platforms libcaption aims to implement a subset of EIA608, CEA708 as supported by the Apple iOS platform.

608 support is currently limited to encoding and decoding the necessary control and preamble codes as well as support for the Basic North American, Special North American and Extended Western European character sets.

708 support is limited to encoding the 608 data in NTSC field 1 user data type structure.

In addition, utility functions to create h.264 SEI (Supplementary enhancement information) NALUs (Network Abstraction Layer Unit) for inclusion into an h.264 elementary stream are provided.

H.264 utility functions are limited to wrapping the 708 payload into a SEI NALU. This is accomplished by prepending the 708 payload with 3 bytes (nal_unit_type = 6, payloadType = 4, and PayloadSize = variable), and appending a stop bit encoded into a full byte (with a value of 127). In addition if the 708 payload contains an emulated start code, a three byte sequence equaling 0,0,1 an emulation prevention byte (3) is inserted. Functions to reverse this operation are also provided.

## Characters
| | | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|BNA| |!|"|#|$|%|&|’|(|)|á|+|,|-|.|/|
|BNA|0|1|2|3|4|5|6|7|8|9|:|;|<|=|>|?|
|BNA|@|A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|
|BNA|P|Q|R|S|T|U|V|W|X|Y|Z|[|é|]|í|ó|
|BNA|ú|a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|
|BNA|p|q|r|s|t|u|v|w|x|y|z|ç|÷|Ñ|ñ|█|
|SNA|®|°|½|¿|™|¢|£|♪|à|&nbsp;|è|â|ê|î|ô|û|
|WES|Á|É|Ó|Ú|Ü|ü|‘|¡|*|'|—|©|℠|•|“|”|
|WEF|À|Â|Ç|È|Ê|Ë|ë|Î|Ï|ï|Ô|Ù|ù|Û|«|»|
|WEP|Ã|ã|Í|Ì|ì|Ò|ò|Õ|õ|{|}|&#92;|^|_|&#124;|~|
|WEG|Ä|ä|Ö|ö|ß|¥|¤|¦|Å|å|Ø|ø|┌|┐|└|┘|

* BNA = Basic North American character set
* SNA = Special North American character set
* WES = Extended Western European character set : Extended Spanish/Miscellaneous
* WEF = Extended Western European character set : Extended French
* WEP = Extended Western European character set : Portuguese
* WEG = Extended Western European character set : German/Danish

## Limitations
Current B-frame support for caption creation is minimal. libcaption ensures no re-ordering of captions is required
on playback.

## Build Directions
# Mac Os/Linux
Install build dependencies (git, cmake, a compiler such as xcode, gcc or clang and optionally re2c and ffmpeg)
* run `cmake . && make`
* or to compile without re2c `cmake -DENABLE_RE2C=OFF . && make`
* finally `sudo make install` to install
# Windows
I have never tested libcaption in windows. It is written in pure C with no dependencies,
so there is no reason it would not work.
