Encoder for various XA ADPCM audio formats from Electronic Arts.

Formats available:
* __--xa-r1__: EA XA ADPCM R1, contained in ["SCxl" files](https://wiki.multimedia.cx/index.php/Electronic_Arts_SCxl).
* __--xa-r2__: EA XA ADPCM R2, contained in "SCxl" files.
* __--xa-r3__: EA XA ADPCM R3, contained in "SCxl" files.
* __--maxis-xa__: EA Maxis CDROM XA (.xa), used in Sims 1 and 2 ([infos](https://simstek.fandom.com/wiki/XA)). The XA tag (type of audio) is left at 0.
* __--xas__: EA XAS, contained in [.snr/.cdata files](https://wiki.multimedia.cx/index.php/EA_SAGE_Audio_Files). A --loop option will mark the file as a loop (without a starting part).

All those formats are decodable with FFmpeg (although with some problems), this tool does not do decoding.

## Credits and sources
* Electronic Arts Sound eXchange
* FFmpeg
* Green Book XA specifications
