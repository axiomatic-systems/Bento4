Mp4Fragment - VFrag Edition
====
What is "Virtual Fragmentation"?
----
When the --vfrag flag is used, standard fragmentation, *including index creation*, is performed.  The standard output file is created.  But it may be ignored.
Also created are an output.sidx and output.json file.

Output.sidx has the binary sidx atom to be handed to players on-request.  

Output.json has the data necessary for the fragment atoms.  The [traf] atom with the [tfhd] and the [trun] atoms are output in hex, including all sample sizes.
(That runs to the end of the hex.)  Also, the source locations in the input file for the trun-referenced samples.

Using The Outputs
----
When a request comes in for a fragment or sequence, look it up (i.e. store the JSON in a database indexed by contentID-TrackID-Segment Number.)
The Traf field should be converted back to binary, and written *ending* at the DestinationOffset.  

Grab the data from the *original* file at the specified offsets, preferably in one byte range request covering the entire set of offset-size combinations.
In a loop, memmove them down to form a single contiguous chunk, and serve it.

Note that while you can reuse the moov atom from the source file, it won't necessarily be the same size as Bento would create in the fMP4, due to compatibility flags and other additions.  The ftyp atom, for example, is re-created rather than copied.  Also the closing mfra atom is not included.

Example
----
There is a full code example at Source/Go/Mp4VFragstitcher.go.  Parameters:

* -inMP4 string     	Source MP4 to read samples from
* -input string      	Input JSON file
* -output string     	Destination MP4 to write to
* -verbosity int     	Verbosity

Future
----
The entire TRAF atom is hex-encoded in the JSON.  The SIDX is a separate binary file.  It may make sense to encode the SIDX directly in the JSON also.

The reason the entire TRAF is encoded is for simplicity on the consumer (Go) side.  We could reduce JSON size by only storing *changing* metadata.

Similarly, this uses hex, not Base64, for binaries in JSON.  Bento4 doesn't have a C++ Base64 and it seemed heavyweight to add one just to save 33% on the JSON size, especially since what we really care about is the size in the database which, if these are decoded to binary, will be smaller anyhow.