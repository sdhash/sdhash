Project: sdhash-gpu
author: Candice Quates

Compares two collections of sdbfs using a matrix-multiplication 
inspired cuda-shared-memory approach.

For windows, having the sdhash components in a linkable format is
required to compile -- we are providing binaries to make this
a lot less painful.  Visual Studio components are present for
reference.  

For Linux, you wil need to have the CUDA toolkit installed,
and build the program using the included Makefile.

Updated status April 5 2013:

Comparisons with sped-up reductions, command line arguments,
and support for querying entire directories.  

Have not tested cards below Fermi architecture and less
than 2gb video ram.  Fermi/Kepler/compute capability 2.0+ 
recommended.  

Ready for outside testing.  Have tested extensively on
very large (>1TB) collections of data.

Updated status March 4 2013:

Comparing real data, linked with the sdhash libraries, and using
(almost) the same scoring methods as existing sdhash comparisons.

We compare one fixed-size (up to 1.2gb ish files) sdbf-set (.sdbf file)
to a stream of sdbfs from another sdbf-set.  

To do:

Make/read command line args.(done)

Enable device selection. (done)

Try to run multiple devices from the same program. (ehhhh.)


Current status January 25 2013:

Matrices are generated, compared, and reduced to a vector of length (height)
of the "A" matrix.

Basic architecture is A-matrix on Y-axis, B-matrix on X-axis.  B-matrix can 
be smaller than A-matrix to facilitate comparing small things to many things.  
B-matrix represents one thing, while A-matrix represents many things.  


A-matrix is stored in row-major order as a stream of bloom-filters.
B-matrix is stored in column major order, one piece of BF at at time, 32 
rows of BF.

We currently and+popc the two matrixes together, and reduce the matrixes per 
row to the maximum bit-count value per row. 

To do:

Fully link-in sdbf library (we are currently c++ enough to do this, just 
incomplete on the link side) so that we can read in actual sdbf files 
for testing. (done)

Several optimizations of the program as it stands are available.  

Optimizations to try:

1) Using a cache for the allocated intermediate matrix.  Saves cudaMalloc time.
2) Leaving query and target bloom filter streams in cuda memory when multiple 
comparisons are needed. (done)
3) Running overlapping and+popc calculations with cuda streams to compare 
multiple sets of data at once.  Possibly also layer this in with pulling data 
into memory.  (tried some.)
4) Find/write faster reduction mechanism than thrust::reduce_by_key, as 
operations are 2/3 reduction and 1/3 computation.  The thrust reduction also 
does not support multiple cuda streams. (done, could go faster)

