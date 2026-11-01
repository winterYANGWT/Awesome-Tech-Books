"""shorty worker：从 Redis 队列取任务，抓取页面标题并写回 PostgreSQL。"""

import json
import logging
import os
import signal
import sys
import threading
import time
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from html.parser import HTMLParser

import psycopg
import redis

QUEUE = "shorty:jobs"
MAX_BYTES = 64 * 1024
log = logging.getLogger("shorty.worker")


class JsonFormatter(logging.Formatter):
    fields = ("worker", "code", "url", "error", "signal", "queue", "concurrency")

    def format(self, record):
        ts = datetime.fromtimestamp(record.created, timezone.utc)
        entry = {
            "time": ts.isoformat(timespec="milliseconds").replace("+00:00", "Z"),
            "level": record.levelname.lower(),
            "msg": record.getMessage(),
        }
        for name in self.fields:
            if hasattr(record, name):
                entry[name] = getattr(record, name)
        return json.dumps(entry, ensure_ascii=False)


def setup_logging():
    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(JsonFormatter())
    log.addHandler(handler)
    log.setLevel(logging.INFO)
    log.propagate = False


class TitleParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self._in_title = False
        self._parts = []
        self.title = None

    def handle_starttag(self, tag, attrs):
        if tag.lower() == "title" and self.title is None:
            self._in_title = True

    def handle_endtag(self, tag):
        if tag.lower() == "title" and self._in_title:
            self._in_title = False
            self.title = _clean("".join(self._parts))

    def handle_data(self, data):
        if self._in_title:
            self._parts.append(data)


def _clean(value):
    return " ".join(value.split())[:512] or None


def extract_title(html):
    parser = TitleParser()
    parser.feed(html)
    parser.close()
    if parser.title is not None:
        return parser.title
    return _clean("".join(parser._parts))


def fetch_title(url, timeout=5):
    scheme = urllib.parse.urlsplit(url).scheme
    if scheme not in ("http", "https"):
        raise ValueError("只支持 http/https URL")
    req = urllib.request.Request(
        url, headers={"User-Agent": "shorty-worker/1.0", "Accept-Encoding": "identity"}
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        charset = resp.headers.get_content_charset() or "utf-8"
        body = resp.read(MAX_BYTES)
    return extract_title(body.decode(charset, errors="replace"))


def db_url():
    raw = os.environ.get("WORKER_DB_URL")
    if not raw:
        raise RuntimeError("WORKER_DB_URL 未设置")
    path = os.environ.get("WORKER_DB_PASSWORD_FILE")
    if not path:
        return raw
    with open(path, encoding="utf-8") as f:
        password = f.read().rstrip("\r\n")
    parts = urllib.parse.urlsplit(raw)
    host = parts.hostname or ""
    if ":" in host:
        host = f"[{host}]"
    if parts.port:
        host = f"{host}:{parts.port}"
    user = urllib.parse.quote(parts.username or "", safe="")
    netloc = f"{user}:{urllib.parse.quote(password, safe='')}@{host}"
    return urllib.parse.urlunsplit(
        (parts.scheme, netloc, parts.path, parts.query, parts.fragment)
    )


def redis_client(addr):
    if addr.startswith(("redis://", "rediss://")):
        return redis.Redis.from_url(
            addr, decode_responses=True, socket_timeout=5, socket_connect_timeout=3
        )
    host, sep, port = addr.rpartition(":")
    if not sep:
        host, port = addr, "6379"
    return redis.Redis(
        host=host,
        port=int(port),
        decode_responses=True,
        socket_timeout=5,
        socket_connect_timeout=3,
    )


def write_heartbeat(path):
    try:
        tmp = f"{path}.tmp"
        with open(tmp, "w", encoding="utf-8") as f:
            f.write(str(int(time.time())))
        os.replace(tmp, path)
    except OSError as exc:
        log.warning("心跳文件写入失败", extra={"error": str(exc)})


def process_job(raw, dsn, slow_ms):
    job = json.loads(raw)
    code, url = job["code"], job["url"]
    log.info("开始抓取标题", extra={"code": code, "url": url})
    if slow_ms:
        time.sleep(slow_ms / 1000)
    title = fetch_title(url)
    if title is None:
        log.warning("页面没有 title", extra={"code": code, "url": url})
        return
    with psycopg.connect(dsn, connect_timeout=5) as conn:
        conn.execute("UPDATE links SET title = %s WHERE code = %s", (title, code))
    log.info("标题已写回", extra={"code": code, "url": url})


def worker_loop(name, rdb, dsn, slow_ms, stop, heartbeat_path):
    while not stop.is_set():
        try:
            item = rdb.brpop(QUEUE, timeout=1)
        except redis.RedisError as exc:
            log.error("Redis 读取失败", extra={"worker": name, "error": str(exc)})
            stop.wait(2)
            continue
        if item is not None:
            try:
                process_job(item[1], dsn, slow_ms)
            except Exception as exc:  # 单个任务失败不能让 worker 退出
                log.error("任务处理失败", extra={"worker": name, "error": str(exc)})
        write_heartbeat(heartbeat_path)
    log.info("worker 退出", extra={"worker": name})


def main():
    setup_logging()
    try:
        addr = os.environ["WORKER_REDIS_ADDR"]
        dsn = db_url()
        concurrency = max(1, int(os.environ.get("WORKER_CONCURRENCY", "1")))
        slow_ms = max(0, int(os.environ.get("WORKER_FAULT_SLOW_MS", "0") or "0"))
    except (KeyError, ValueError, OSError, RuntimeError) as exc:
        log.error("配置错误", extra={"error": str(exc)})
        return 2

    heartbeat_path = os.environ.get("WORKER_HEARTBEAT_FILE", "/tmp/worker-heartbeat")
    stop = threading.Event()

    def handle(signum, _frame):
        log.info(
            "收到信号，处理完当前任务后退出",
            extra={"signal": signal.Signals(signum).name},
        )
        stop.set()

    signal.signal(signal.SIGTERM, handle)
    signal.signal(signal.SIGINT, handle)

    rdb = redis_client(addr)
    log.info(
        "worker 启动",
        extra={"queue": QUEUE, "concurrency": concurrency},
    )
    threads = [
        threading.Thread(
            target=worker_loop,
            args=(f"worker-{i}", rdb, dsn, slow_ms, stop, heartbeat_path),
            name=f"worker-{i}",
        )
        for i in range(concurrency)
    ]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    log.info("worker 已停止")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
