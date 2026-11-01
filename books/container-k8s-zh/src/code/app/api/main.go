package main

import (
	"context"
	"errors"
	"fmt"
	"log/slog"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/redis/go-redis/v9"
)

var version = "dev"

var commands = map[string]func(){
	"serve": serve, "migrate": migrate, "cleanup": cleanup,
	"version": func() { fmt.Printf("shorty %s\n", version) },
}

func main() {
	if len(os.Args) < 2 {
		usage()
		return
	}
	cmd, ok := commands[os.Args[1]]
	if !ok {
		usage()
	}
	cmd()
}

func usage() {
	fmt.Fprintln(os.Stderr, "用法: shorty serve|migrate|cleanup|version")
	os.Exit(2)
}

func serve() {
	cfg, err := loadConfig()
	if err != nil {
		fatal(err)
	}
	var level slog.Level
	if err := level.UnmarshalText([]byte(cfg.LogLevel)); err != nil {
		level = slog.LevelInfo
	}
	log := slog.New(slog.NewJSONHandler(os.Stdout, &slog.HandlerOptions{Level: level}))
	faults, err := faultsFromEnv()
	if err != nil {
		fatal(err)
	}
	log.Info("shorty starting", "version", version, slog.Group("config", cfg.logFields()...))
	log.Info("fault switches", "faults", faults.snapshot())

	var store Store
	if cfg.DBURL == "" {
		store = newMemoryStore()
		log.Warn("SHORTY_DB_URL 未设置，使用内存存储，进程退出后数据不会保留")
	} else {
		st, err := newPGStore(context.Background(), cfg.DBURL)
		if err != nil {
			fatal(fmt.Errorf("创建数据库连接池: %w", err))
		}
		store = st
	}
	var rdb *redis.Client
	if cfg.RedisAddr != "" {
		rdb = redis.NewClient(&redis.Options{Addr: cfg.RedisAddr})
	}
	app := newApp(cfg, log, store, rdb, faults)
	if cfg.DBURL != "" || rdb != nil {
		go func() {
			for {
				time.Sleep(2 * time.Second)
				if len(app.dependencyErrors(context.Background())) == 0 {
					log.Info("数据库和 Redis 已就绪")
					return
				}
			}
		}()
	}
	started := time.Now()
	go func() {
		for range time.Tick(time.Second) {
			if n := faults.snapshot().CrashAfter; n > 0 && time.Since(started) >= time.Duration(n)*time.Second {
				log.Error("故障开关触发退出", "crash_after_seconds", n)
				os.Exit(1)
			}
		}
	}()

	srv := &http.Server{Addr: cfg.Listen, Handler: app.handler(), ReadHeaderTimeout: 5 * time.Second}
	go func() {
		log.Info("HTTP 服务开始监听", "addr", cfg.Listen)
		if err := srv.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
			log.Error("HTTP 服务异常退出", "error", err)
			os.Exit(1)
		}
	}()

	sigCh := make(chan os.Signal, 2)
	signal.Notify(sigCh, syscall.SIGTERM, syscall.SIGINT)
	defer signal.Stop(sigCh)
	var sig os.Signal
	for {
		sig = <-sigCh
		if sig == syscall.SIGTERM && faults.snapshot().IgnoreSigterm {
			log.Warn("忽略 SIGTERM（故障开关 ignore_sigterm）")
			continue
		}
		break
	}
	app.shutting.Store(true)
	log.Info("收到退出信号，readiness 置为失败", "signal", sig.String(), "delay", cfg.ShutdownDelay.String())
	time.Sleep(cfg.ShutdownDelay)
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	log.Info("开始关闭 HTTP 服务")
	if err := srv.Shutdown(ctx); err != nil {
		log.Error("HTTP 服务关闭超时", "error", err)
	} else {
		log.Info("HTTP 服务已关闭")
	}
	store.Close()
	if rdb != nil {
		_ = rdb.Close()
	}
	log.Info("shorty 退出")
}

func openDB() (*pgStore, error) {
	cfg, err := loadConfig()
	if err != nil {
		return nil, err
	}
	if cfg.DBURL == "" {
		return nil, fmt.Errorf("需要设置 SHORTY_DB_URL")
	}
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	st, err := newPGStore(ctx, cfg.DBURL)
	if err != nil {
		return nil, err
	}
	return st, st.Ping(ctx)
}

func migrate() {
	st, err := openDB()
	if err != nil {
		fatal(fmt.Errorf("migrate: %w", err))
	}
	defer st.Close()
	if err := migrateSchema(context.Background(), st.pool); err != nil {
		fatal(fmt.Errorf("migrate: %w", err))
	}
	fmt.Println("migrate: links 表与过期索引已就绪（可重复执行）")
}

func cleanup() {
	st, err := openDB()
	if err != nil {
		fatal(fmt.Errorf("cleanup: %w", err))
	}
	defer st.Close()
	n, err := st.Cleanup(context.Background())
	if err != nil {
		fatal(fmt.Errorf("cleanup: %w", err))
	}
	fmt.Printf("cleanup: 删除了 %d 条过期链接\n", n)
}
func fatal(err error) {
	fmt.Fprintln(os.Stderr, "错误:", err)
	os.Exit(1)
}
