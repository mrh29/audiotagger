# Audiotag

This repo contains files for parsing mp3 files for ID3 tags.

## Usage

./audiotagger *foo.mp3* parses the ID3 tags in *foo.mp3*. Audiotagger first looks for  
ID3v2 tags and if none are present then looks for ID31 tags.  If neither are present,  
you are prompted to add tags.

You may comment out the *X_REQUIRED* defines to avoid prompting for field X.  
The standard version prompts for a title, artist, album, track number, year, and composer.

## Supported Formats

* ID3v1
* ID3v2.3

## Limitations

Currently only supports ASCII field text in ID3v2 and modifies flags to 0x0000.