package main

import (
	"fmt"
	"io"
	"sort"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"
)

var durationBuckets = []float64{.005, .01, .025, .05, .1, .25, .5, 1, 2.5, 5, 10}

type httpMetrics struct {
	start    time.Time
	inFlight atomic.Int64
	mu       sync.Mutex
	requests map[string]uint64 // route\x00status
	hist     []uint64
	sum      float64
	count    uint64
}

func newHTTPMetrics() *httpMetrics {
	return &httpMetrics{start: time.Now(), requests: make(map[string]uint64), hist: make([]uint64, len(durationBuckets))}
}
func (m *httpMetrics) observe(route string, status int, seconds float64) {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.requests[route+"\x00"+strconv.Itoa(status)]++
	for i, bound := range durationBuckets {
		if seconds <= bound {
			m.hist[i]++
		}
	}
	m.sum += seconds
	m.count++
}
func (m *httpMetrics) render(w io.Writer, memCount int) {
	m.mu.Lock()
	defer m.mu.Unlock()
	keys := make([]string, 0, len(m.requests))
	for k := range m.requests {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	fmt.Fprintln(w, "# TYPE shorty_http_requests_total counter")
	for _, k := range keys {
		route, status, _ := strings.Cut(k, "\x00")
		fmt.Fprintf(w, "shorty_http_requests_total{route=\"%s\",status=\"%s\"} %d\n", route, status, m.requests[k])
	}
	fmt.Fprintln(w, "# TYPE shorty_http_request_duration_seconds histogram")
	for i, bound := range durationBuckets {
		fmt.Fprintf(w, "shorty_http_request_duration_seconds_bucket{le=\"%s\"} %d\n",
			strconv.FormatFloat(bound, 'g', -1, 64), m.hist[i])
	}
	fmt.Fprintf(w, "shorty_http_request_duration_seconds_bucket{le=\"+Inf\"} %d\n", m.count)
	fmt.Fprintf(w, "shorty_http_request_duration_seconds_sum %s\n", ftoa(m.sum))
	fmt.Fprintf(w, "shorty_http_request_duration_seconds_count %d\n", m.count)
	fmt.Fprintln(w, "# TYPE shorty_http_in_flight_requests gauge")
	fmt.Fprintf(w, "shorty_http_in_flight_requests %d\n", m.inFlight.Load())
	fmt.Fprintln(w, "# TYPE shorty_process_start_time_seconds gauge")
	fmt.Fprintf(w, "shorty_process_start_time_seconds %s\n", ftoa(float64(m.start.UnixNano())/1e9))
	if memCount >= 0 {
		fmt.Fprintln(w, "# TYPE shorty_memory_links gauge")
		fmt.Fprintf(w, "shorty_memory_links %d\n", memCount)
	}
}
func ftoa(f float64) string { return strconv.FormatFloat(f, 'g', -1, 64) }
