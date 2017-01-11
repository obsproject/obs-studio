# version
v0.6
Matthew Szatmary m3u8@twitch.tv / matt@szatmary.org

# libcaption

libcaption is a small library written in C to aid in the creating and parsing of closed caption data for use in the twitch player, open sourced to use within community developed broadcast tools. To maintain consistency across platforms libcaption aims to implement a subset of EIA608, CEA708 as supported by the Apple iOS platform.

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
|SNA|®|°|½|¿|™|¢|£|♪|à| |è|â|ê|î|ô|û|
|WES|Á|É|Ó|Ú|Ü|ü|‘|¡|*|'|—|©|℠|•|“|”|
|WEF|À|Â|Ç|È|Ê|Ë|ë|Î|Ï|ï|Ô|Ù|ù|Û|«|»|
|WEP|Ã|ã|Í|Ì|ì|Ò|ò|Õ|õ|{|}|\\|^|_|\||~|
|WEG|Ä|ä|Ö|ö|ß|¥|¤|¦|Å|å|Ø|ø|┌|┐|└|┘|

* BNA = Basic North American character set
* SNA = Special North American character set
* WES = Extended Western European character set : Extended Spanish/Miscellaneous
* WEF = Extended Western European character set : Extended French
* WEP = Extended Western European character set : Portuguese
* WEG = Extended Western European character set : German/Danish


------
eia608_screen_t is the default internal representation. `screens` can be
converted to and from SEI messages 608/708 buffers and SRT files

## JSON format
A JSON rendered format is provided for convenience of integration. The JSON
format is as follows:

```
{
    "format":  "eia608",
    "mode":    "pop-on",
    "roll-up": 0,
    "data": [
        { "row":0, "col":0, "char":"A", "style":"white" },
        { "row":0, "col":1, "char":"B", "style":"white" },
        // ...
    ]
}
```

### `format`
The only current valid value is `"eia608"`. This field exists for
future expansion.  Any values other than `"eia608"`, and the object should be
ignored for now.

### `mode`
Defines the method by which screen contents are adjusted in response to
new data, such as content in excess of the normal grid size.

Possible modes are:

`"clear"`: Generated when a screen is initialized, but not entered a "loading"
state. `"clear"` mode  occurs frequently when the display is to be erased.

`"pop-on"`: Normally used for pre recorded content where. A pop-on screen should
completely replace all previous screen contents.

`"paint-on"`: Normally used for live content. In this mode, new characters
appear one or two at a time. The `data` array does include the complete state of
the screen. In practice, it may not be different that `"pop-on"`.

An internal "loading" mode also exists.

### `roll-up`
Normally used for live content such as news broadcasts. Text is moved up the
screen as new rows appear. The number of rows to be displayed is also encoded in
the JSON  key `"roll-up"` where value is an integer of `0`, `1`, `2`, `3`, or `4`.
Like `"paint-on"`, the `data` array does include the complete state of the screen.

### `data`
Contains a object for every character to be displayed. The screen is a grid of
15 rows and 32 columns. The position of the character is available in the `row`
and `col` fields. The character itself is in the `char` field. The full
character set is available in the document, but any valid UTF-8 should be
supported for future. The style field will contain one of the following values:

`"white"`, `"green"`, `"blue"`, `"cyan"`, `"red"`, `"yellow"`, `"magenta"`, `"italics"`

Characters with the `"italics"` style should be displayed in white.
