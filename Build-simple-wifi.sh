#!/bin/sh
PACKAGE="simple-wifi"
# ruim de AI zooi maar weer op. Overal ^M's, maar als je er over klaagt moet 
# eigen programma worden ingesteld, terwijl die al goed staat.
# find . -type f -exec dos2unix {} +

# En voor andere AI aangemaakte textbestanden:
find . -type f | while read f; do
   if file "$f" | grep -q "script"; then
      dos2unix "$f"
   fi
done


ARCH=$(dpkg --print-architecture)
# ARCH="all"  # scripts only, works on all 
# VERSION=$(grep '^Version:' /debian/control | awk '{print $2}')
VERSION=$(dpkg-parsechangelog -S Version)

# Make backup met datum-tijd in bestandsnaam
DATETIME=$(date +%Y%m%d-%H%M%S)
# Make backup
tar cf ../${PACKAGE}-${VERSION}_${ARCH}.${DATETIME}.tar  .
printf "Building package ${PACKAGE}_${VERSION}_${ARCH}.deb\n"

# build package and move it in right folder for rest of script.
printf "[*] Bouwen (dpkg-buildpackage)...\n"
dpkg-buildpackage -us -uc -b
mv ../${PACKAGE}_${VERSION}_${ARCH}.deb .


