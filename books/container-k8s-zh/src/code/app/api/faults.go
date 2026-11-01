package main

import (
	"fmt"
	"net/url"
	"os"
	"strconv"
	"sync/atomic"
)

type Faults struct {
	p atomic.Pointer[faultSnapshot]
}

type faultSnapshot struct {
	LatencyMS     int     `json:"latency_ms"`
	ErrorRate     float64 `json:"error_rate"`
	LeakMB        int     `json:"leak_mb"`
	NotReady      bool    `json:"not_ready"`
	CrashAfter    int     `json:"crash_after"`
	IgnoreSigterm bool    `json:"ignore_sigterm"`
}

func newFaults() *Faults {
	f := &Faults{}
	f.p.Store(&faultSnapshot{})
	return f
}

func faultsFromEnv() (*Faults, error) {
	values := url.Values{}
	for env, key := range map[string]string{
		"SHORTY_FAULT_LATENCY_MS": "latency_ms", "SHORTY_FAULT_ERROR_RATE": "error_rate",
		"SHORTY_FAULT_LEAK_MB": "leak_mb", "SHORTY_FAULT_NOT_READY": "not_ready",
		"SHORTY_FAULT_CRASH_AFTER": "crash_after", "SHORTY_FAULT_IGNORE_SIGTERM": "ignore_sigterm",
	} {
		if v, ok := os.LookupEnv(env); ok {
			values.Set(key, v)
		}
	}
	f := newFaults()
	return f, f.update(values)
}

func (f *Faults) snapshot() faultSnapshot { return *f.p.Load() }

func (f *Faults) update(v url.Values) error {
	s := f.snapshot()
	for _, x := range []struct {
		key string
		dst *int
	}{{"latency_ms", &s.LatencyMS}, {"leak_mb", &s.LeakMB}, {"crash_after", &s.CrashAfter}} {
		if v.Has(x.key) {
			n, err := strconv.Atoi(v.Get(x.key))
			if err != nil || n < 0 {
				return fmt.Errorf("%s 必须是非负整数", x.key)
			}
			*x.dst = n
		}
	}
	for _, x := range []struct {
		key string
		dst *bool
	}{{"not_ready", &s.NotReady}, {"ignore_sigterm", &s.IgnoreSigterm}} {
		if v.Has(x.key) {
			b, err := strconv.ParseBool(v.Get(x.key))
			if err != nil {
				return fmt.Errorf("%s 必须是 true/false", x.key)
			}
			*x.dst = b
		}
	}
	if v.Has("error_rate") {
		rate, err := strconv.ParseFloat(v.Get("error_rate"), 64)
		if err != nil || rate < 0 || rate > 1 {
			return fmt.Errorf("error_rate 必须是 0 到 1 之间的数")
		}
		s.ErrorRate = rate
	}
	f.p.Store(&s)
	return nil
}
