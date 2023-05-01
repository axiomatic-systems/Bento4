Mp4Fragment - VFrag Edition
====
What is "Virtual Fragmentation"?
----
When the --vfrag flag is used, standard fragmentation, *including index creation*, is performed.  The standard output file is created.  But it may be ignored.
Also created are an output.sidx and output.json file.

Output.sidx has the binary sidx atom to be handed to players on-request.  

Output.json has the data necessary for the fragment atoms, specifically for the [tfhd] and the [trun] atoms, as well as the source locations in the input file for the trun-referenced samples.

Using The Outputs
----
Create manifests using the *new* data, e.g. the sidx atom.  
When a request comes in for a fragment or sequence, look it up (i.e. store the JSON in a database indexed by contentID-TrackID-Segment Numer.)
Grab the data from the *original* file at the specified offsets, preferably in one byte range request covering the entire set of offset-size combinations.
In a loop, memmove them down to form a single contiguous chunk, and serve it.