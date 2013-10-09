#!/bin/sh

dd if=/dev/urandom of=f1.rand bs=1024 count=1024 2>/dev/null
dd if=/dev/urandom of=f2.rand bs=1024 count=1024 2>/dev/null

cp f1.rand f1.rand.clean
cp f2.rand f2.rand.clean

dd if=/dev/urandom of=4k.frag bs=1024 count=4 2>/dev/null
dd if=/dev/urandom of=8k.frag bs=1024 count=8 2>/dev/null
dd if=/dev/urandom of=16k.frag bs=1024 count=16 2>/dev/null
dd if=/dev/urandom of=32k.frag bs=1024 count=32 2>/dev/null
dd if=/dev/urandom of=64k.frag bs=1024 count=64 2>/dev/null
dd if=/dev/urandom of=128k.frag bs=1024 count=128 2>/dev/null


OFF1=`perl -e "print int(rand(1048576));"`
OFF2=`perl -e "print int(rand(1048576));"`

for foo in `ls *.frag` ; do 
    cp f1.rand.clean f1.rand
    cp f2.rand.clean f2.rand
    dd if=$foo of=f1.rand obs=1 seek=$OFF1 conv=notrunc 2> /dev/null
    dd if=$foo of=f2.rand obs=1 seek=$OFF1 conv=notrunc 2> /dev/null
    echo $foo sdhash: ` ../sdhash -g f1.rand f2.rand -t 0`
    ../sdhash --multi f1.rand f2.rand --output f1and2
    echo $foo sdhash-32k: ` ../sdhash-mr f1and2.mr -t 0 --level 0`
    echo $foo sdhash-16k: `../sdhash-mr f1and2.mr -t 0 --level 1`
    echo $foo sdhash-8k: ` ../sdhash-mr f1and2.mr -t 0 --level 2`
    echo $foo sdhash-4k: ` ../sdhash-mr f1and2.mr -t 0 --level 3`
    rm f1.rand f2.rand f1and2.mr
done


rm -f *.frag *.rand *.clean *.mr 
