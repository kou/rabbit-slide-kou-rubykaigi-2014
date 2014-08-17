package main

import "fmt"
import "net"
import "flag"
import "time"

func FormatElapsedTimeNano(elapsedTimeNano float64) string {
	if (elapsedTimeNano < 1000000000) {
		return fmt.Sprintf("%0.3fms", elapsedTimeNano / 1000000)
	} else {
		return fmt.Sprintf("%0.3fs", elapsedTimeNano / 1000000000)
	}
}

func main() {
	host := flag.String("host", "127.0.0.1", "Host to connect")
	port := flag.Int("port", 2929, "Port to connect")
	nRequests := flag.Int("n-requests", 30000, "The number of requests")
	flag.Parse()
	address := fmt.Sprintf("%s:%d", *host, *port)

	start := time.Now()
	for i := 0; i < *nRequests; i++ {
		conn, err := net.Dial("tcp", address)
		if err != nil {
			fmt.Printf("%d: %d: dial error: %s\n", i, err)
			continue
		}
		err = conn.Close()
		if err != nil {
			fmt.Printf("%d: %d: close error: %s\n", i, err)
			continue
		}
	}
	elapsedTimeNano := time.Now().UnixNano() - start.UnixNano()

	fmt.Printf("Total:   %s\n",
		FormatElapsedTimeNano(float64(elapsedTimeNano)));
	averageElapsedTimeNano :=
		float64(elapsedTimeNano) / float64(*nRequests)
	fmt.Printf("Average: %s\n",
		FormatElapsedTimeNano(averageElapsedTimeNano));
}
