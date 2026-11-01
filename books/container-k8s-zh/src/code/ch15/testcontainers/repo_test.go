package shortytest

import (
	"context"
	"database/sql"
	"os"
	"testing"
	"time"

	_ "github.com/jackc/pgx/v5/stdlib"
	"github.com/testcontainers/testcontainers-go"
	"github.com/testcontainers/testcontainers-go/modules/postgres"
	"github.com/testcontainers/testcontainers-go/wait"
)

// 固定 digest，避免 postgres:18.6-alpine 这个 tag 以后指向别的镜像。
const postgresImage = "postgres:18.6-alpine@sha256:10a5c69a83035ab5239f5620d708520e8b28f5aa1f2b20143e6024532edadfb5"

// ANCHOR: repo
func TestRepositoryWithMigrate(t *testing.T) {
	ctx := context.Background()

	network, err := testcontainers.GenericNetwork(ctx, testcontainers.GenericNetworkRequest{
		NetworkRequest: testcontainers.NetworkRequest{Name: "ch15-test-net"},
	})
	if err != nil {
		t.Fatalf("create network: %v", err)
	}
	testcontainers.CleanupNetwork(t, network)
	netName := network.(*testcontainers.DockerNetwork).Name

	pg, err := postgres.Run(ctx, postgresImage,
		postgres.WithDatabase("shorty"),
		postgres.WithUsername("shorty"),
		postgres.WithPassword("shorty"),
		postgres.BasicWaitStrategies(),
		testcontainers.CustomizeRequest(testcontainers.GenericContainerRequest{
			ContainerRequest: testcontainers.ContainerRequest{
				Networks:       []string{netName},
				NetworkAliases: map[string][]string{netName: {"postgres"}},
			},
		}),
	)
	testcontainers.CleanupContainer(t, pg)
	if err != nil {
		t.Fatalf("start postgres: %v", err)
	}

	// 在测试代码里启动 shorty 的 migrate，目标就是上面这个临时数据库。
	migrate, err := testcontainers.Run(ctx, "shorty:0.1",
		testcontainers.WithCmd("migrate"),
		testcontainers.WithEnv(map[string]string{
			"SHORTY_DB_URL": "postgres://shorty:shorty@postgres:5432/shorty?sslmode=disable",
		}),
		testcontainers.CustomizeRequest(testcontainers.GenericContainerRequest{
			ContainerRequest: testcontainers.ContainerRequest{Networks: []string{netName}},
		}),
		testcontainers.WithWaitStrategy(wait.ForExit().WithExitTimeout(60*time.Second)),
	)
	testcontainers.CleanupContainer(t, migrate)
	if err != nil {
		t.Fatalf("run migrate: %v", err)
	}
	state, err := migrate.State(ctx)
	if err != nil {
		t.Fatalf("migrate state: %v", err)
	}
	if state.ExitCode != 0 {
		logs, _ := migrate.Logs(ctx)
		t.Fatalf("migrate exited %d:\n%s", state.ExitCode, logs)
	}

	connStr, err := pg.ConnectionString(ctx, "sslmode=disable")
	if err != nil {
		t.Fatalf("connection string: %v", err)
	}
	db, err := sql.Open("pgx", connStr)
	if err != nil {
		t.Fatalf("open db: %v", err)
	}
	defer db.Close()

	// 这一段就是“仓储测试”：用 migate 建出来的表做一次写入和读回。
	if _, err := db.ExecContext(ctx,
		`INSERT INTO links (code, url, clicks, title, created_at) VALUES ($1, $2, 0, $3, now())`,
		"ch15", "http://example.com/ch15", "Chapter 15"); err != nil {
		t.Fatalf("insert link: %v", err)
	}
	var code, title string
	var clicks int64
	if err := db.QueryRowContext(ctx,
		`SELECT code, clicks, title FROM links WHERE code = $1`, "ch15").
		Scan(&code, &clicks, &title); err != nil {
		t.Fatalf("select link: %v", err)
	}
	t.Logf("repository: code=%s clicks=%d title=%q", code, clicks, title)
}

// ANCHOR_END: repo

// 这个测试故意用一个不存在的 DOCKER_HOST，供排障小节复现 Testcontainers 的报错。
func TestDockerEnvironment(t *testing.T) {
	ctx := context.Background()
	_, err := testcontainers.Run(ctx, "busybox:1.37", testcontainers.WithCmd("true"))
	if err != nil {
		t.Fatalf("start container with DOCKER_HOST=%s: %v", os.Getenv("DOCKER_HOST"), err)
	}
}
