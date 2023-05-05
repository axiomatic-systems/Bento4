package main

import (
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"os"
)

type Chunk struct {
	Offset uint64
	Size   uint32
}

type Fragment struct {
	TrackID           int
	DestinationOffset int64
	Duration          int64
	TrackType         string
	FragmentSize      int32
	Traf              string // warning: A very long one.
	SampleCount       int
	Chunks            []Chunk
}

var (
	// Args: Expect input json name, input mp4 name of orginal and output mp4 name.
	verbosity   = flag.Int("verbosity", 0, "Verbosity")
	inputFName  = flag.String("input", "", "Input JSON file")
	inputMP4    = flag.String("inMP4", "", "Source MP4 to read samples from")
	outputFName = flag.String("output", "", "Destination MP4 to write to")
)

func writeHexAsBinary(outfile *os.File, offset int64, offset_is_end bool, cooked string) error {
	raw, err := hex.DecodeString(cooked)
	if err != nil {
		return err
	}

	if offset_is_end {
		_, err = outfile.Seek(offset-int64(len(raw)), io.SeekStart)
	} else {
		_, err = outfile.Seek(offset, io.SeekStart)
	}
	if err != nil {
		return err
	}
	_, err = outfile.Write(raw)
	return err
}

// The entire byte-range to read from the source file for this fragment
// offset should be already in the Fragment.DestinationOffset but this reads it from the chunks.
func getFragmentSourceRange(frag Fragment) (int64, int64) {
	var offset int64
	var last int64

	for _, chunk := range frag.Chunks {
		if (offset == 0) || (int64(chunk.Offset) < offset) {
			offset = int64(chunk.Offset)
		}
		if (last == 0) || (int64(chunk.Offset)+int64(chunk.Size) > last) {
			last = int64(chunk.Offset) + int64(chunk.Size)
		}
	}
	return offset, last - offset
}

// Writes "data" to the location in fname, regardless of what is there currently or even
// if the file isn't that long.
func writeFragmentsToFile(inMP4 *os.File, outMP4 *os.File, frags []Fragment) {
	// reader := bufio.NewReader(inMP4)
	for _, frag := range frags {
		// Write header atoms.  Need space for the mdat header also.
		writeHexAsBinary(outMP4, frag.DestinationOffset-8, true, frag.Traf)
		// Write mdat size and then 'mdat'.
		bs := make([]byte, 4)
		binary.BigEndian.PutUint32(bs, uint32(frag.FragmentSize+8))
		outMP4.Write(bs)
		outMP4.Write([]byte("mdat"))
		// How much of this frag have we written?
		var currentChunksSizeTotal int64 = 0
		offset, length := getFragmentSourceRange(frag)
		reader := io.NewSectionReader(inMP4, offset, length)

		if *verbosity > 0 {
			fmt.Printf("Fragment Source Range: Offset %d, Length %d\n", offset, length)
		}
		readBuffer := make([]byte, length)
		readLen, readErr := reader.Read(readBuffer)
		if (readErr != nil) || (readLen != int(length)) {
			fmt.Printf("Read of fragment %d failed.  Wanted %d, got %d and error %s\n", frag.DestinationOffset, length, readLen, readErr.Error())
		}

		// Iterate through the chunks, calculating offsets in that memory and writing to the output file.
		for chunkCount, chunk := range frag.Chunks {
			if *verbosity > 0 {
				fmt.Printf("Fragment Chunk Destination: Chunk #%d -> Offset %d\n", chunkCount, int64(frag.DestinationOffset)+currentChunksSizeTotal)
			}
			outPos, seekErr := outMP4.Seek(int64(frag.DestinationOffset)+currentChunksSizeTotal, io.SeekStart)
			if (outPos != int64(frag.DestinationOffset)+currentChunksSizeTotal) || (seekErr != nil) {
				fmt.Printf("Output seek failed.  Wanted %d, got %d, error %s\n", int64(frag.DestinationOffset), outPos, seekErr.Error())
			}
			// Grab just this chunk into a slice, so we can write it cleanly, since Go lacks args for that.
			writeSlice := readBuffer[int64(chunk.Offset)-offset : (int64(chunk.Offset) - offset + int64(chunk.Size))]
			if *verbosity > 0 {
				fmt.Print("Writing slice\n")
			}
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
