package main

import "fmt"
import "net"
import "sync"
import "flag"
import "time"

const message = `
{
  "name": "Alice",
  "nick": "alice",
  "age": 14
}
`

func Worker(jobs chan int, waitGroup *sync.WaitGroup, id int, address string, nMessages int) {
	defer waitGroup.Done()

	for job := range jobs {
		conn, err := net.Dial("tcp", address)
		if err != nil {
			fmt.Printf("%d: %d: dial error: %s\n", id, job, err)
			return
		}
		defer conn.Close();

		buffer := make([]byte, 4096)
		for i := 0; i < nMessages; i++ {
			_, err = conn.Write([]byte(message))
			if err != nil {
				fmt.Printf("%d: %d: write error: %s\n",
					id, job, err)
				return
			}
			_, err = conn.Read(buffer)
			if err != nil {
				fmt.Printf("%d: %d: read error: %s\n",
					id, job, err)
				return
			}
		}
	}
}

func FormatElapsedTimeNano(elapsedTimeNano float64) string {
	if (elapsedTimeNano < 1000000000) {
		return fmt.Sprintf("%0.3fms", elapsedTimeNano / 1000000)
	} else {
		return fmt.Sprintf("%0.3fs", elapsedTimeNano / 1000000000)
	}
}

func main() {
	host := flag.String("host", "127.0.0.1", "Host to connect")
	port := flag.Int("port", 22929, "Port to connect")
	nRequests := flag.Int("n-requests", 5000, "The number of requests")
	concurrency := flag.Int("concurrency", 1000, "The number of workers")
	nMessages := flag.Int("n-messages", 100,
		"The number of messages in a session")
	flag.Parse()
	address := fmt.Sprintf("%s:%d", *host, *port)

	jobs := make(chan int)
	waitGroup := new(sync.WaitGroup)
	for i := 0; i < *concurrency; i++ {
		waitGroup.Add(1)
		go Worker(jobs, waitGroup, i, address, *nMessages)
	}
	start := time.Now()
	for i := 0; i < *nRequests; i++ {
		jobs <- i
	}
	close(jobs)
	waitGroup.Wait()
	elapsedTimeNano := time.Now().UnixNano() - start.UnixNano()

	fmt.Printf("Total:   %s\n",
		FormatElapsedTimeNano(float64(elapsedTimeNano)));
	averageElapsedTimeNano :=
		float64(elapsedTimeNano) / float64(*nRequests)
	fmt.Printf("Average: %s\n",
		FormatElapsedTimeNano(averageElapsedTimeNano));
}
