package main

import (
	"context"
	crand "crypto/rand"
	_ "embed"
	"encoding/json"
	"errors"
	"fmt"
	"log/slog"
	mrand "math/rand/v2"
	"net/http"
	"net/url"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/redis/go-redis/v9"
)

// 丢弃 go-redis 默认的 stderr 日志，保持 JSON 日志格式。
type redisLogger struct{}

func (redisLogger) Printf(context.Context, string, ...any) {}

type App struct {
	cfg      config
	log      *slog.Logger
	store    Store
	redis    *redis.Client
	faults   *Faults
	shutting atomic.Bool
	metrics  *httpMetrics
	leakMu   sync.Mutex
	leaks    [][]byte
}

func newApp(cfg config, log *slog.Logger, store Store, rdb *redis.Client, faults *Faults) *App {
	redis.SetLogger(redisLogger{})
	return &App{cfg: cfg, log: log, store: store, redis: rdb, faults: faults, metrics: newHTTPMetrics()}
}

func (a *App) handler() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /{$}", a.route(a.index))
	mux.HandleFunc("POST /api/links", a.route(a.createLink))
	mux.HandleFunc("GET /api/links/{code}", a.route(a.linkDetails))
	mux.HandleFunc("GET /r/{code}", a.route(a.redirect))
	mux.HandleFunc("GET /healthz", a.route(func(w http.ResponseWriter, _ *http.Request) { fmt.Fprintln(w, "ok") }))
	mux.HandleFunc("GET /readyz", a.route(a.readyz))
	mux.HandleFunc("GET /metrics", a.route(a.metricsHandler))
	mux.HandleFunc("/admin/fault", a.adminFault)
	return a.observe(mux)
}

//go:embed index.html
var indexHTML string

func (a *App) index(w http.ResponseWriter, _ *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	fmt.Fprint(w, indexHTML)
}
func (a *App) createLink(w http.ResponseWriter, r *http.Request) {
	var in struct {
		URL        string `json:"url"`
		TTLSeconds int    `json:"ttl_seconds"`
	}
	dec := json.NewDecoder(http.MaxBytesReader(w, r.Body, 1<<20))
	if err := dec.Decode(&in); err != nil {
		http.Error(w, `请求体需要 JSON，例如 {"url":"https://example.com"}`, http.StatusBadRequest)
		return
	}
	in.URL = strings.TrimSpace(in.URL)
	if err := validateURL(in.URL); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	if in.TTLSeconds < 0 {
		http.Error(w, "ttl_seconds 不能为负数", http.StatusBadRequest)
		return
	}
	now := time.Now().UTC()
	link := &Link{URL: in.URL, CreatedAt: now}
	if in.TTLSeconds > 0 {
		expires := now.Add(time.Duration(in.TTLSeconds) * time.Second)
		link.ExpiresAt = &expires
	}
	var err error
	for range 5 {
		link.Code = strings.ToLower(crand.Text()[:7])
		if err = a.store.Create(r.Context(), link); err == nil || !errors.Is(err, ErrExists) {
			break
		}
	}
	if err != nil {
		a.log.Error("创建链接失败", "error", err)
		http.Error(w, "存储错误", http.StatusInternalServerError)
		return
	}
	a.enqueue(r.Context(), link)
	writeJSON(w, http.StatusCreated, map[string]string{
		"code": link.Code, "short_url": strings.TrimRight(a.cfg.BaseURL, "/") + "/r/" + link.Code,
	})
}

func (a *App) redirect(w http.ResponseWriter, r *http.Request) {
	link, err := a.store.Click(r.Context(), r.PathValue("code"))
	if errors.Is(err, ErrNotFound) {
		http.NotFound(w, r)
		return
	}
	if err != nil {
		a.log.Error("读取链接失败", "error", err)
		http.Error(w, "存储错误", http.StatusInternalServerError)
		return
	}
	http.Redirect(w, r, link.URL, http.StatusFound)
}

func (a *App) linkDetails(w http.ResponseWriter, r *http.Request) {
	link, err := a.store.Get(r.Context(), r.PathValue("code"))
	if errors.Is(err, ErrNotFound) {
		http.NotFound(w, r)
		return
	}
	if err != nil {
		a.log.Error("读取链接失败", "error", err)
		http.Error(w, "存储错误", http.StatusInternalServerError)
		return
	}
	writeJSON(w, http.StatusOK, map[string]any{
		"url": link.URL, "clicks": link.Clicks, "title": link.Title,
		"created_at": link.CreatedAt, "expires_at": link.ExpiresAt,
	})
}

func (a *App) readyz(w http.ResponseWriter, r *http.Request) {
	var reasons []string
	if a.shutting.Load() {
		reasons = append(reasons, "shutting down")
	}
	if a.faults.snapshot().NotReady {
		reasons = append(reasons, "fault: not_ready")
	}
	reasons = append(reasons, a.dependencyErrors(r.Context())...)
	if len(reasons) > 0 {
		w.WriteHeader(http.StatusServiceUnavailable)
		fmt.Fprintf(w, "not ready: %s\n", strings.Join(reasons, "; "))
		return
	}
	fmt.Fprintln(w, "ok")
}

func (a *App) dependencyErrors(parent context.Context) []string {
	ctx, cancel := context.WithTimeout(parent, 2*time.Second)
	defer cancel()
	var out []string
	if a.cfg.DBURL != "" {
		if err := a.store.Ping(ctx); err != nil {
			out = append(out, "database: "+err.Error())
		}
	}
	if a.redis != nil {
		if err := a.redis.Ping(ctx).Err(); err != nil {
			out = append(out, "redis: "+err.Error())
		}
	}
	return out
}
func (a *App) metricsHandler(w http.ResponseWriter, _ *http.Request) {
	w.Header().Set("Content-Type", "text/plain; version=0.0.4; charset=utf-8")
	count := -1
	if m, ok := a.store.(*memStore); ok {
		count = m.Count()
	}
	a.metrics.render(w, count)
}
func (a *App) adminFault(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodPost {
		if err := a.faults.update(r.URL.Query()); err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
	}
	writeJSON(w, http.StatusOK, a.faults.snapshot())
}
func (a *App) enqueue(ctx context.Context, l *Link) {
	if a.redis == nil {
		return
	}
	payload, err := json.Marshal(map[string]string{"code": l.Code, "url": l.URL})
	if err == nil {
		err = a.redis.LPush(ctx, "shorty:jobs", payload).Err()
	}
	if err != nil {
		a.log.Error("推送抓取任务失败", "code", l.Code, "error", err)
	}
}

func (a *App) route(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		f := a.faults.snapshot()
		if f.LatencyMS > 0 {
			time.Sleep(time.Duration(f.LatencyMS) * time.Millisecond)
		}
		if f.LeakMB > 0 {
			b := make([]byte, f.LeakMB<<20)
			for i := 0; i < len(b); i += 4096 {
				b[i] = 1
			}
			a.leakMu.Lock()
			a.leaks = append(a.leaks, b)
			a.leakMu.Unlock()
		}
		if f.ErrorRate > 0 && mrand.Float64() < f.ErrorRate {
			http.Error(w, "injected fault", http.StatusInternalServerError)
			return
		}
		next(w, r)
	}
}

type statusWriter struct {
	http.ResponseWriter
	status int
}

func (w *statusWriter) WriteHeader(code int) {
	w.status = code
	w.ResponseWriter.WriteHeader(code)
}
func (a *App) observe(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		sw := &statusWriter{ResponseWriter: w}
		a.metrics.inFlight.Add(1)
		defer a.metrics.inFlight.Add(-1)
		next.ServeHTTP(sw, r)
		if sw.status == 0 {
			sw.status = http.StatusOK
		}
		route := r.Pattern
		if route == "" {
			route = "other"
		}
		elapsed := time.Since(start)
		seconds := elapsed.Seconds()
		a.metrics.observe(route, sw.status, seconds)
		a.log.Info("request", "method", r.Method, "path", r.URL.Path, "status", sw.status,
			"duration_ms", float64(elapsed.Microseconds())/1000, "route", route)
	})
}
func validateURL(raw string) error {
	u, err := url.Parse(raw)
	if err != nil || (u.Scheme != "http" && u.Scheme != "https") || u.Host == "" || len(raw) > 2048 {
		return fmt.Errorf("url 必须是 http 或 https 地址")
	}
	return nil
}

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(v)
}
