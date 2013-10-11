sdhash
======
sdhash is tool that allows two arbitrary blobs of data to be compared for similarity based on common strings of binary data. It is designed to provide quick results during triage and initial investigation phases. It has been in active development since 2010 with the explicit goal of becoming fast, scalable, and reliable.

There two general classes of problems where sdhash can provide significant benefits–fragment identification and version correlation.

In fragment identification, we search for a smaller piece of data inside a bigger piece of data (“needle-in-a-haystack”). For example:

Block vs. file correlation: given a chunk of data (disk block/network packet/RAM page/etc), we can search a reference collection of files to identify whether the chunk came from any of them.

File vs. RAM/disk image: given a file and a target image, we can efficiently determine if any pieces of the file can be found on the image (that includes deallocated storage).

In version correlation, we are interested in correlating data objects (files) that are comparable in size and, thus, similar ones can be viewed as versions. These are two basic scenarios in which this is useful–identifying related documents and identifying code versions.

In all cases, the use of the tool is the same, however the interpretation may differ based on the circumstances.

For more information:  

Tutorial: http://roussev.net/sdhash/tutorial/sdhash-tutorial.html

Papers/Version history/etc:  http://sdhash.org/
