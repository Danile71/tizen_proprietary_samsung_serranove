#!/bin/bash

NAME=gcc
SPECNAME=${NAME}.spec
ARCHES="armv7l"
TOBASELIBS=""
TOBASELIBS_ARCH=""
BINSUFFIX=
if [ $NAME == gcc ]; then
VERSION=$(grep -E '^%global\s+gcc_version' ./${SPECNAME} | sed 's/^%global\s\+gcc_version\s\+\([0-9]\)\+\.\([0-9]\+\)\(\..*\)*/\1\2/g')
fi

# baselibs.conf - part 1
rm -f baselibs.conf baselibs.conf.old  2>&1 > /dev/null || true
echo -n "arch i586 targets " > baselibs.conf


for i in ${ARCHES} ; do
# cross spec files
    cat ./${SPECNAME} | sed -e "s#Name: .*#Name: cross-${i}-${NAME}#" > ./cross-${i}-${NAME}.spec
    cat ./${SPECNAME} | sed -e "s#Name: .*#Name: cross-${i}-${NAME}-accel-%{!?x64:x86}%{?x64}#" > ./cross-${i}-${NAME}-accel.spec
# baselibs.conf - part 2
    test ! x"$i" = x"" && echo -n "${i}:${i} " >> baselibs.conf
done

echo "" >> baselibs.conf
cat baselibs.conf | sed -e "s/i586/x86_64/" >> baselibs.conf

# baselibs.conf - part 3
echo "" >> baselibs.conf
for l in ${ARCHES} ; do
echo "" >> baselibs.conf
echo "cross-${l}-${NAME}-accel-@X86@
  targettype x86 block!
  targettype 32bit block!" >> baselibs.conf
for j in ${ARCHES//${l}} ; do
  echo "  targettype $j block!" >> baselibs.conf
done
cat >> baselibs.conf << EOF

  targettype ${l} autoreqprov off
  targettype ${l} provides "cross-arm-${NAME}-accel"
  targettype ${l} provides "cross-arm-${NAME}${VERSION}-accel"
  targettype ${l} provides "cross-${l}-${NAME}-accel-${l}"
  targettype ${l} requires "glibc-@X86@-arm"
  targettype ${l} requires "libgcc-@X86@-arm"
  targettype ${l} requires "${NAME}"
  targettype ${l} requires "${NAME}-c++"

  targettype ${l} prefix /emul/ia32-linux
  targettype ${l} extension -arm
  targettype ${l} +/
  targettype ${l} -/usr/lib/debug
  targettype ${l} -/usr/lib64/debug
  targettype ${l} -/usr/src/debug
  targettype ${l} -/usr/share/man
  targettype ${l} -/usr/share/doc
  targettype ${l} -/usr/share/locale
  targettype ${l} requires "tizen-accelerator"
  targettype ${l} post "#set -x"
  targettype ${l} post " export GCCVER=\$(LANG=C gcc${BINSUFFIX} -dumpversion) "
  targettype ${l} post " export GCCVER_NEW=\$(LANG=C <prefix>/usr/bin/gcc${BINSUFFIX} -dumpversion) "
  targettype ${l} post " echo \"GCCVER: \$GCCVER     GCCVER_NEW: \$GCCVER_NEW \" "
  targettype ${l} post " if test "\${GCCVER}" == "\${GCCVER_NEW}"; then"
  targettype ${l} post "  echo "GCC and GCC-accel versions match. Enabling cross-compiler." "
  targettype ${l} post "  for bin in c++ cpp g++ gcc gcov gcc-ar gcc-nm gcc-ranlib; do "
  targettype ${l} post "   binary="/usr/bin/\${bin}${BINSUFFIX}" "
  targettype ${l} post "   if test -L \${binary} -a -e \${binary}.orig-arm ; then"
  targettype ${l} post "     echo "\${binary} not installed or \${binary}.orig-arm already present !" "
  targettype ${l} post "   else "
  targettype ${l} post "     mv \${binary} \${binary}.orig-arm && cp <prefix>\${binary} \${binary}"
  targettype ${l} post "   fi "
  targettype ${l} post "  done "

  targettype ${l} post "  for bin in cc1 cc1plus collect2 lto-wrapper lto1 ; do "
  targettype ${l} post "   binary="/usr/lib/gcc/${l}-tizen-linux-gnueabi/\$GCCVER/\${bin}" "
  targettype ${l} post "   if test -L \${binary} -a -e \${binary}.orig-arm ; then"
  targettype ${l} post "     echo "\${binary} not installed or \${binary}.orig-arm already present !" "
  targettype ${l} post "   else "
  targettype ${l} post "     mv \${binary} \${binary}.orig-arm && cp <prefix>/usr/lib/gcc/${l}-tizen-linux-gnueabi/\$GCCVER/\${bin} \${binary} "
  targettype ${l} post "   fi "
  targettype ${l} post "  done "

  targettype ${l} post "  for bin in addr2line ar as c++filt elfedit gprof ld ld.bfd ld.gold nm objcopy objdump ranlib readelf size strings strip ; do"
  targettype ${l} post "    ln -sf /usr/bin/\${bin} /usr/lib/gcc/${l}-tizen-linux-gnueabi/\${GCCVER}/\${bin}"
  targettype ${l} post "  done "
  targettype ${l} post " fi "

  targettype ${l} preun " export GCCVER=\$(LANG=C gcc${BINSUFFIX} -dumpversion) "
  targettype ${l} preun " for bin in c++ cpp g++ gcc gcov gcc-ar gcc-nm gcc-ranlib ; do "
  targettype ${l} preun "  binary="/usr/bin/\${bin}${BINSUFFIX}" "
  targettype ${l} preun "  if test -e \${binary}.orig-arm ; then "
  targettype ${l} preun "   rm \${binary} && mv \${binary}.orig-arm \${binary} "
  targettype ${l} preun "  else "  
  targettype ${l} preun "   echo "\${binary}.orig-arm not present !" "
  targettype ${l} preun "  fi "
  targettype ${l} preun " done "
  targettype ${l} preun " for bin in cc1 cc1plus collect2 lto-wrapper lto1 ; do "
  targettype ${l} preun "  binary=/usr/lib/gcc/${l}-tizen-linux-gnueabi/\$GCCVER/\${bin} "
  targettype ${l} preun "  if test -e \${binary}.orig-arm ; then "
  targettype ${l} preun "   rm \${binary} && mv \${binary}.orig-arm \${binary} "
  targettype ${l} preun "  else "  
  targettype ${l} preun "   echo "\${binary}.orig-arm not present !" "
  targettype ${l} preun "  fi "
  targettype ${l} preun " done "
  targettype ${l} preun " for bin in addr2line ar as c++filt elfedit gprof ld ld.bfd ld.gold nm objcopy objdump ranlib readelf size strings strip ; do"
  targettype ${l} preun "   rm /usr/lib/gcc/${l}-tizen-linux-gnueabi/\${GCCVER}/\${bin}"
  targettype ${l} preun " done "

EOF


done


exit 0
