package main

import (
	"encoding/json"
	"io"
	"log/slog"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"
)

func testApp() *App {
	cfg := config{BaseURL: "http://shorty.test", Listen: ":0"}
	log := slog.New(slog.NewTextHandler(io.Discard, nil))
	mem := newMemoryStore()
	return newApp(cfg, log, mem, nil, newFaults())
}

func request(a *App, method, path, body string) *httptest.ResponseRecorder {
	req := httptest.NewRequest(method, path, strings.NewReader(body))
	if body != "" {
		req.Header.Set("Content-Type", "application/json")
	}
	rr := httptest.NewRecorder()
	a.handler().ServeHTTP(rr, req)
	return rr
}

type createdLink struct {
	Code     string `json:"code"`
	ShortURL string `json:"short_url"`
}

func create(t *testing.T, a *App, body string) createdLink {
	t.Helper()
	rr := request(a, http.MethodPost, "/api/links", body)
	if rr.Code != http.StatusCreated {
		t.Fatalf("创建短链: status=%d body=%s", rr.Code, rr.Body)
	}
	var out createdLink
	if err := json.Unmarshal(rr.Body.Bytes(), &out); err != nil {
		t.Fatal(err)
	}
	return out
}

func TestIndex(t *testing.T) {
	rr := request(testApp(), http.MethodGet, "/", "")
	if rr.Code != http.StatusOK {
		t.Fatalf("status=%d", rr.Code)
	}
	if !strings.Contains(rr.Body.String(), "<title>Shorty</title>") {
		t.Fatalf("页面缺少 title: %s", rr.Body)
	}
	if !strings.Contains(rr.Body.String(), "<form") {
		t.Fatal("页面缺少表单")
	}
}

func TestCreateRedirectAndDetails(t *testing.T) {
	a := testApp()
	out := create(t, a, `{"url":"http://api:8080/","ttl_seconds":3600}`)
	if out.Code == "" || out.ShortURL != "http://shorty.test/r/"+out.Code {
		t.Fatalf("响应不对: %+v", out)
	}

	rr := request(a, http.MethodGet, "/r/"+out.Code, "")
	if rr.Code != http.StatusFound {
		t.Fatalf("跳转 status=%d", rr.Code)
	}
	if got := rr.Header().Get("Location"); got != "http://api:8080/" {
		t.Fatalf("Location=%q", got)
	}

	rr = request(a, http.MethodGet, "/api/links/"+out.Code, "")
	if rr.Code != http.StatusOK {
		t.Fatalf("详情 status=%d", rr.Code)
	}
	var detail struct {
		URL       string     `json:"url"`
		Clicks    int64      `json:"clicks"`
		Title     *string    `json:"title"`
		CreatedAt time.Time  `json:"created_at"`
		ExpiresAt *time.Time `json:"expires_at"`
	}
	if err := json.Unmarshal(rr.Body.Bytes(), &detail); err != nil {
		t.Fatal(err)
	}
	if detail.URL != "http://api:8080/" || detail.Clicks != 1 || detail.Title != nil || detail.ExpiresAt == nil {
		t.Fatalf("详情不对: %+v", detail)
	}
}

func TestExpiredLink(t *testing.T) {
	a := testApp()
	old := time.Now().Add(-time.Minute)
	if err := a.store.(*memStore).Create(t.Context(), &Link{
		Code: "old", URL: "http://example.com", CreatedAt: old.Add(-time.Hour), ExpiresAt: &old,
	}); err != nil {
		t.Fatal(err)
	}
	if rr := request(a, http.MethodGet, "/r/old", ""); rr.Code != http.StatusNotFound {
		t.Fatalf("过期跳转 status=%d", rr.Code)
	}
	if rr := request(a, http.MethodGet, "/api/links/old", ""); rr.Code != http.StatusNotFound {
		t.Fatalf("过期详情 status=%d", rr.Code)
	}
}

func TestInvalidURL(t *testing.T) {
	a := testApp()
	for _, body := range []string{
		`{"url":""}`,
		`{"url":"ftp://example.com/x"}`,
		`{"url":"not a url"}`,
		`{"url":"https://example.com","ttl_seconds":-1}`,
		`{"nope":true}`,
	} {
		if rr := request(a, http.MethodPost, "/api/links", body); rr.Code != http.StatusBadRequest {
			t.Fatalf("%s: status=%d body=%s", body, rr.Code, rr.Body)
		}
	}
}

func TestFaultSwitches(t *testing.T) {
	a := testApp()
	if rr := request(a, http.MethodGet, "/readyz", ""); rr.Code != http.StatusOK {
		t.Fatalf("初始 readyz=%d", rr.Code)
	}

	rr := request(a, http.MethodPost, "/admin/fault?not_ready=true", "")
	if rr.Code != http.StatusOK {
		t.Fatalf("设置 not_ready status=%d", rr.Code)
	}
	rr = request(a, http.MethodGet, "/readyz", "")
	if rr.Code != http.StatusServiceUnavailable || !strings.Contains(rr.Body.String(), "not_ready") {
		t.Fatalf("not_ready 未生效: %d %s", rr.Code, rr.Body)
	}
	request(a, http.MethodPost, "/admin/fault?not_ready=false", "")

	request(a, http.MethodPost, "/admin/fault?error_rate=1", "")
	if rr := request(a, http.MethodGet, "/healthz", ""); rr.Code != http.StatusInternalServerError {
		t.Fatalf("error_rate=1 时 status=%d", rr.Code)
	}
	request(a, http.MethodPost, "/admin/fault?error_rate=0", "")

	request(a, http.MethodPost, "/admin/fault?latency_ms=25", "")
	if rr := request(a, http.MethodPost, "/admin/fault?error_rate=2", ""); rr.Code != http.StatusBadRequest {
		t.Fatalf("非法 error_rate 应返回 400，实际 %d", rr.Code)
	}
	rr = request(a, http.MethodGet, "/admin/fault", "")
	var f faultSnapshot
	if err := json.Unmarshal(rr.Body.Bytes(), &f); err != nil {
		t.Fatal(err)
	}
	if f.LatencyMS != 25 || f.ErrorRate != 0 || f.NotReady {
		t.Fatalf("故障开关状态不对: %+v", f)
	}
}

func TestMetricsAndHealth(t *testing.T) {
	a := testApp()
	out := create(t, a, `{"url":"https://example.com/"}`)
	request(a, http.MethodGet, "/r/"+out.Code, "")

	if rr := request(a, http.MethodGet, "/healthz", ""); rr.Code != http.StatusOK {
		t.Fatalf("healthz=%d", rr.Code)
	}
	rr := request(a, http.MethodGet, "/metrics", "")
	if rr.Code != http.StatusOK {
		t.Fatalf("metrics=%d", rr.Code)
	}
	for _, want := range []string{
		"shorty_http_requests_total",
		"shorty_http_request_duration_seconds_bucket",
		"shorty_http_in_flight_requests",
		"shorty_process_start_time_seconds",
		"shorty_memory_links 1",
	} {
		if !strings.Contains(rr.Body.String(), want) {
			t.Fatalf("metrics 缺少 %q\n%s", want, rr.Body)
		}
	}
}

func TestMemoryCount(t *testing.T) {
	a := testApp()
	out := create(t, a, `{"url":"https://example.com/"}`)
	mem := a.store.(*memStore)
	if mem.Count() != 1 {
		t.Fatalf("Count=%d", mem.Count())
	}
	if mem.links[out.Code] == nil {
		t.Fatal("链接不在内存存储里")
	}
}
