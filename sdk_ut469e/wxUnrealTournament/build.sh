#!/bin/bash
pushd .
cd ~/ut/build/

WX_LIBS="core"

for i in `ls -1 ~/ut/build/Linux/amd64/Release/lib/lib*.*`
do
 # find the broken symlinks
    if file -b --mime-encoding ${i} |grep -q ascii
 # cat the broken link and use it as the ln target
        then ln -sf `cat ${i}` ${i}
    else
        chmod 0755 ${i}
    fi
done

rm CMakeFiles -rf
rm CMakeCache.txt
CC="clang `wx-config --cppflags`" CXX="clang++ `wx-config --cxxflags` -DWX" LDFLAGS="`wx-config --libs ${WX_LIBS}`" cmake -DOLDUNREAL_DEPENDENCIES_PATH=~/ut/build/Linux/amd64/Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/ut/System64/ ..
make -j 4 ut-bin-amd64
cp ut-bin-amd64 ../System64/

for i in `ls -1 ~/ut/build/Linux/x86/Release/lib/lib*.*`
do
 # find the broken symlinks
    if file -b --mime-encoding ${i} |grep -q ascii
 # cat the broken link and use it as the ln target
        then ln -sf `cat ${i}` ${i}
    else
        chmod 0755 ${i}
    fi
done

rm CMakeFiles -rf
rm CMakeCache.txt
CC="clang -m32 `wx-config --cppflags | sed 's/x86_64/i386/'`" CXX="clang++ -m32 `wx-config --cxxflags | sed 's/x86_64/i386/'` -DWX" LDFLAGS="`wx-config --libs ${WX_LIBS} | sed 's/x86_64/i386/'`" cmake -DOLDUNREAL_DEPENDENCIES_PATH=~/ut/build/Linux/x86/Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/ut/System/ ..
make -j 4 ut-bin-x86

cp ut-bin-x86 ../System/

popd

