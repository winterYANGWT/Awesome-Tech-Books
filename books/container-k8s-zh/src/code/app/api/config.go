package main

import (
	"cmp"
	"fmt"
	"net/url"
	"os"
	"regexp"
	"strconv"
	"strings"
	"time"
)

type config struct {
	Listen, DBURL, RedisAddr, BaseURL, LogLevel string
	ShutdownDelay                               time.Duration
}

func loadConfig() (config, error) {
	c := config{
		Listen: envOr("SHORTY_LISTEN", ":8080"), DBURL: os.Getenv("SHORTY_DB_URL"),
		RedisAddr: os.Getenv("SHORTY_REDIS_ADDR"), BaseURL: envOr("SHORTY_BASE_URL", "http://localhost:8080"),
		LogLevel: envOr("SHORTY_LOG_LEVEL", "info"),
	}
	// 密码单独放文件，避免出现在 Compose/Kubernetes 的环境变量里。
	if file := os.Getenv("SHORTY_DB_PASSWORD_FILE"); file != "" {
		if c.DBURL == "" {
			return c, fmt.Errorf("SHORTY_DB_PASSWORD_FILE 需要同时设置 SHORTY_DB_URL")
		}
		raw, err := os.ReadFile(file)
		if err != nil {
			return c, fmt.Errorf("读取数据库密码文件: %w", err)
		}
		u, err := url.Parse(c.DBURL)
		if err != nil {
			return c, fmt.Errorf("解析 SHORTY_DB_URL: %w", err)
		}
		user := ""
		if u.User != nil {
			user = u.User.Username()
		}
		u.User = url.UserPassword(user, strings.TrimRight(string(raw), "\r\n"))
		c.DBURL = u.String()
	}
	n, err := strconv.Atoi(envOr("SHORTY_SHUTDOWN_DELAY", "5"))
	if err != nil || n < 0 {
		return c, fmt.Errorf("SHORTY_SHUTDOWN_DELAY 需要非负整数秒数")
	}
	c.ShutdownDelay = time.Duration(n) * time.Second
	return c, nil
}

func (c config) logFields() []any {
	return []any{"listen", c.Listen, "db_url", redactURL(c.DBURL), "redis_addr", c.RedisAddr,
		"base_url", c.BaseURL, "shutdown_delay", c.ShutdownDelay.String(), "log_level", c.LogLevel}
}

var credentials = regexp.MustCompile(`://([^:/@]+):[^@]*@`)

func redactURL(raw string) string {
	if raw == "" {
		return "memory"
	}
	return credentials.ReplaceAllString(raw, "://$1:***@")
}

func envOr(key, def string) string { return cmp.Or(os.Getenv(key), def) }
