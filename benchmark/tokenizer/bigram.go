package main

import "os"
import "fmt"
import "time"

func main() {
	file, err := os.Open(os.Args[1])
	if err != nil {
		fmt.Printf("Failed to open: %s\n", err)
		os.Exit(1)
	}
	defer file.Close()

	info, err := file.Stat()
	if err != nil {
		fmt.Printf("Failed to stat: %s\n", err)
		os.Exit(1)
	}
	buffer := make([]byte, info.Size())
	_, err = file.Read(buffer)
	if err != nil {
		fmt.Printf("Failed to read: %s\n", err)
		os.Exit(1)
	}
	text := string(buffer)

	start := time.Now()
	tokens := []string{};
	var previousCharacter string
	for i, codePoint := range text {
		character := string(codePoint)
		if i == 0 {
			previousCharacter = character
			continue
		}
		tokens = append(tokens,	previousCharacter + character)
		previousCharacter = character
	}
	elapsedTimeNano := time.Now().UnixNano() - start.UnixNano()

	fmt.Printf("%f\n", float64(elapsedTimeNano) / 1000000000)
}
