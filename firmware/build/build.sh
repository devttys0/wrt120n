#!/bin/bash

OS_IMAGE="$1"
OUT_IMAGE="$2"
EXPECTED_IMAGE_SIZE="1041396"

# Write magic signature out to file
perl -e 'print "\x04\x01\x09\x20"' > "$OUT_IMAGE"

# Compress input image
lzma -z -e "$OS_IMAGE"

# Super hack to put the size field back in the lzma header
perl -e 'print "\x5d\x00\x00\x80\x00\x7c\x6a\x4d\x00\x00\x00\x00\x00\x00"' >> "$OUT_IMAGE"

dd if="${OS_IMAGE}.lzma" bs=14 skip=1 >> "$OUT_IMAGE"

CURRENT_IMAGE_SIZE=$(wc -c "$OUT_IMAGE" | awk '{print $1}')
PAD_SIZE=$(($EXPECTED_IMAGE_SIZE-$CURRENT_IMAGE_SIZE))

# Pad out the image
perl -e "print \"\xFF\"x$PAD_SIZE" >> "$OUT_IMAGE"

cat web.img >> "$OUT_IMAGE"

../../deobfscator/wrt120n -e "$OUT_IMAGE" "${OUT_IMAGE}.upload.bin"
