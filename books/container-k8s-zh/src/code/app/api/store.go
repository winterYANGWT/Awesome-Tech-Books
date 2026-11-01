package main

import (
	"context"
	"errors"
	"sync"
	"time"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgconn"
	"github.com/jackc/pgx/v5/pgxpool"
)

var (
	ErrNotFound = errors.New("link not found")
	ErrExists   = errors.New("link code already exists")
)

type Link struct {
	Code      string     `json:"code"`
	URL       string     `json:"url"`
	Clicks    int64      `json:"clicks"`
	Title     *string    `json:"title"`
	CreatedAt time.Time  `json:"created_at"`
	ExpiresAt *time.Time `json:"expires_at"`
}

type Store interface {
	Create(context.Context, *Link) error
	Get(context.Context, string) (*Link, error)
	Click(context.Context, string) (*Link, error)
	Ping(context.Context) error
	Close()
}

func expired(l *Link, now time.Time) bool { return l.ExpiresAt != nil && !l.ExpiresAt.After(now) }

type memStore struct {
	mu    sync.RWMutex
	links map[string]*Link
}

func newMemoryStore() *memStore { return &memStore{links: make(map[string]*Link)} }

func (s *memStore) Create(_ context.Context, l *Link) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.links[l.Code]; ok {
		return ErrExists
	}
	s.links[l.Code] = l
	return nil
}

func (s *memStore) lookup(code string, click bool) (*Link, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	l, ok := s.links[code]
	if !ok || expired(l, time.Now()) {
		return nil, ErrNotFound
	}
	if click {
		l.Clicks++
	}
	c := *l
	return &c, nil
}

func (s *memStore) Get(_ context.Context, code string) (*Link, error) { return s.lookup(code, false) }
func (s *memStore) Click(_ context.Context, code string) (*Link, error) {
	return s.lookup(code, true)
}

func (s *memStore) Ping(context.Context) error { return nil }
func (s *memStore) Close()                     {}
func (s *memStore) Count() int {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return len(s.links)
}

type pgStore struct{ pool *pgxpool.Pool }

func newPGStore(ctx context.Context, dsn string) (*pgStore, error) {
	pool, err := pgxpool.New(ctx, dsn)
	if err != nil {
		return nil, err
	}
	return &pgStore{pool: pool}, nil
}

func scanLink(row pgx.Row) (*Link, error) {
	var l Link
	err := row.Scan(&l.Code, &l.URL, &l.Clicks, &l.Title, &l.CreatedAt, &l.ExpiresAt)
	if errors.Is(err, pgx.ErrNoRows) {
		return nil, ErrNotFound
	}
	return &l, err
}

func (s *pgStore) Create(ctx context.Context, l *Link) error {
	_, err := s.pool.Exec(ctx, `INSERT INTO links (code, url, clicks, title, created_at, expires_at)
		VALUES ($1, $2, 0, NULL, $3, $4)`, l.Code, l.URL, l.CreatedAt, l.ExpiresAt)
	var pgErr *pgconn.PgError
	if errors.As(err, &pgErr) && pgErr.Code == "23505" {
		return ErrExists
	}
	return err
}

func (s *pgStore) Get(ctx context.Context, code string) (*Link, error) {
	return scanLink(s.pool.QueryRow(ctx, `SELECT code, url, clicks, title, created_at, expires_at
		FROM links WHERE code = $1 AND (expires_at IS NULL OR expires_at > now())`, code))
}

func (s *pgStore) Click(ctx context.Context, code string) (*Link, error) {
	return scanLink(s.pool.QueryRow(ctx, `UPDATE links SET clicks = clicks + 1
		WHERE code = $1 AND (expires_at IS NULL OR expires_at > now())
		RETURNING code, url, clicks, title, created_at, expires_at`, code))
}

func (s *pgStore) Cleanup(ctx context.Context) (int64, error) {
	tag, err := s.pool.Exec(ctx, `DELETE FROM links WHERE expires_at IS NOT NULL AND expires_at <= now()`)
	return tag.RowsAffected(), err
}

func (s *pgStore) Ping(ctx context.Context) error { return s.pool.Ping(ctx) }
func (s *pgStore) Close()                         { s.pool.Close() }

func migrateSchema(ctx context.Context, pool *pgxpool.Pool) error {
	_, err := pool.Exec(ctx, `CREATE TABLE IF NOT EXISTS links (
		code text PRIMARY KEY, url text NOT NULL, clicks bigint NOT NULL DEFAULT 0,
		title text, created_at timestamptz NOT NULL DEFAULT now(), expires_at timestamptz);
		CREATE INDEX IF NOT EXISTS links_expires_at_idx ON links (expires_at) WHERE expires_at IS NOT NULL`)
	return err
}
