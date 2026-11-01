// shorty 的压测客户端：按固定速率向 api 发请求，用来观察 HPA 的扩缩。
package main

import (
	"flag"
	"fmt"
	"io"
	"net/http"
	"sync"
	"sync/atomic"
	"time"
)

func main() {
	url := flag.String("url", "http://api:8080/", "target URL")
	rate := flag.Int("rate", 500, "requests per second")
	concurrency := flag.Int("concurrency", 32, "in-flight requests")
	duration := flag.Duration("duration", 5*time.Minute, "how long to send load")
	flag.Parse()

	client := &http.Client{
		Timeout: 5 * time.Second,
		Transport: &http.Transport{
			DisableKeepAlives:  true,
			DisableCompression: true,
		},
	}

	jobs := make(chan struct{}, *concurrency)
	var sent, ok, failed, dropped atomic.Int64
	var wg sync.WaitGroup
	for i := 0; i < *concurrency; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for range jobs {
				resp, err := client.Get(*url)
				if err != nil {
					failed.Add(1)
					continue
				}
				io.Copy(io.Discard, resp.Body)
				resp.Body.Close()
				if resp.StatusCode == http.StatusOK {
					ok.Add(1)
				} else {
					failed.Add(1)
				}
			}
		}()
	}

	start := time.Now()
	deadline := start.Add(*duration)
	ticker := time.NewTicker(time.Second / time.Duration(*rate))
	defer ticker.Stop()
loop:
	for {
		select {
		case <-ticker.C:
			if time.Now().After(deadline) {
				break loop
			}
			select {
			case jobs <- struct{}{}:
				sent.Add(1)
			default:
				dropped.Add(1)
			}
		}
	}
	close(jobs)
	wg.Wait()
	fmt.Printf("url=%s rate=%d sent=%d ok=%d fail=%d dropped=%d elapsed=%s\n",
		*url, *rate, sent.Load(), ok.Load(), failed.Load(), dropped.Load(),
		time.Since(start).Round(time.Second))
}
