package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"os"
)

type Chunk struct {
	Offset uint64
	Size   uint32
}

type Fragment struct {
	TrackID           int
	DestinationOffset uint64
	Duration          uint64
	TrackType         string
	FragmentSize      uint32
	SampleCount       int
	SampleSizes       []int
	Chunks            []Chunk
}

var verbosity int

func readByteRangeFromFile(fname string, offset int64, length uint32) []byte {
	f, err := os.Open(fname)
	if err != nil {
		return nil
	}
	f.Seek(offset, 0)
	databytes := make([]byte, length)
	f.Read(databytes)
	f.Close()
	return databytes
}

// The entire byte-range to read from the source file for this fragment
// offset should be already in the Fragment.DestinationOffset but this reads it from the chunks.
func getFragmentSourceRange(frag Fragment) (uint64, uint64) {
	var offset uint64
	var last uint64

	for _, chunk := range frag.Chunks {
		if (offset == 0) || (chunk.Offset < offset) {
			offset = chunk.Offset
		}
		if (last == 0) || (chunk.Offset+uint64(chunk.Size) > last) {
			last = chunk.Offset + uint64(chunk.Size)
		}
	}
	return offset, last - offset
}

// Writes "data" to the location in fname, regardless of what is there currently or even
// if the file isn't that long.
func writeFragmentsToFile(inMP4 *os.File, outMP4 *os.File, frags []Fragment) {
	reader := bufio.NewReader(inMP4)

	for _, frag := range frags {
		// How much of this frag have we written?
		var currentChunksSizeTotal int64 = 0
		offset, length := getFragmentSourceRange(frag)
		if verbosity > 0 {
			fmt.Printf("Fragment Source Range: Offset %d, Length %d\n", offset, length)
		}
		readBuffer := make([]byte, length)
		readLen, readErr := reader.Read(readBuffer)
		if (readErr != nil) || (readLen != int(length)) {
			fmt.Printf("Read of fragment %d failed.  Wanted %d, got %d and error %s\n", frag.DestinationOffset, length, readLen, readErr.Error())
		}

		// Iterate through the chunks, calculating offsets in that memory and writing to the output file.
		for chunkCount, chunk := range frag.Chunks {
			if verbosity > 0 {
				fmt.Printf("Fragment Chunk Destination: Chunk #%d -> Offset %d\n", chunkCount, int64(frag.DestinationOffset)+currentChunksSizeTotal)
			}
			outPos, seekErr := outMP4.Seek(int64(frag.DestinationOffset)+currentChunksSizeTotal, os.SEEK_SET)
			if (outPos != int64(frag.DestinationOffset)+currentChunksSizeTotal) || (seekErr != nil) {
				fmt.Printf("Output seek failed.  Wanted %d, got %d, error %s\n", int64(frag.DestinationOffset), outPos, seekErr.Error())
			}
			// Grab just this chunk into a slice, so we can write it cleanly, since Go lacks args for that.
			writeSlice := readBuffer[chunk.Offset-offset : (chunk.Offset - offset + uint64(chunk.Size))]
			fmt.Print("Writing slice\n")
			written, writeErr := outMP4.Write(writeSlice)
			if writeErr != nil {
				fmt.Printf("Write error: %s\n", writeErr.Error())
			} else if written == 0 {
				fmt.Print("Wrote zero bytes")
			}
			currentChunksSizeTotal += int64(chunk.Size)
		}
	}

}

func main() {
	// Args: Expect input json name, input mp4 name of orginal and output mp4 name.
	inputFName := flag.String("input", "", "Input JSON file")
	inputMP4 := flag.String("inMP4", "", "Source MP4 to read samples from")
	outputFName := flag.String("output", "", "Destination MP4 to write to")
	flag.IntVar(&verbosity, "verbosity", 0, "Verbosity")
	flag.Parse()
	if len(*inputFName) == 0 {
		flag.Usage()
		return
	}

	// analyze(fragments)
	dat, err := os.ReadFile(*inputFName)
	if err != nil {
		fmt.Printf("Cannot open input file %s: %s", *inputFName, err.Error())
		return
	}
	chunkList := string(dat)
	// Parse JSON
	var fragments []Fragment
	json.Unmarshal([]byte(chunkList), &fragments)
	fmt.Printf("Stats: %d Fragments\n", len(fragments))
	// fmt.Printf("%+v", fragments)

	inMPFile, err := os.Open(*inputMP4)
	if err != nil {
		fmt.Printf("Cannot open input MP4 file %s: %s", *inputMP4, err.Error())
		return
	}
	outFile, err := os.Create(*outputFName)
	if err != nil {
		fmt.Printf("Cannot open output file %s: %s", *outputFName, err.Error())
		return
	}
	writeFragmentsToFile(inMPFile, outFile, fragments)
	inMPFile.Close()
	outFile.Close()
}
