# kvd 公共代码

kvd 是第 25、27、32–36 章共用的键值服务。这里放协议、存储、网络小工具和压测客户端，
章节里的服务器用下面的方式引用（章节示例在 `src/code/chNN/N_k_name/`）：

```sh
g++-15 -std=c++20 -O2 -g -Wall -Wextra -Wpedantic -Werror -pthread \
    -I../../kvd your_server.cpp -o server
```

```cpp
#include "net.hpp"
#include "protocol.hpp"
#include "store.hpp"
```

都是头文件实现，只依赖 C++20 标准库和 Linux 系统调用。为了示例短，接口放在全局命名空间里。

## 协议（protocol.hpp）

一行一个请求，`\n` 结尾，也接受 `\r\n`。命令大写、区分大小写。

| 请求 | 成功响应 |
|---|---|
| `SET <key> <value>` | `OK` |
| `GET <key>` | `VALUE <value>` 或 `NOT_FOUND` |
| `DEL <key>` | `DELETED` 或 `NOT_FOUND` |
| 格式错误 | `ERROR <原因>` |

- 一行最多 1024 字节；key 和 value 各最多 256 字节，且不能含空白。
- 超长行报 `ERROR line too long`，然后丢到下一个 `\n`，不会把整行攒进内存。
- 一次可以发多个请求（流水线），一个请求也可以分多次到达。

```cpp
Parser parser;
parser.feed(bytes);  // 任意切分，空片段也行
while (auto result = parser.next()) {
    if (auto* request = std::get_if<Request>(&*result)) {
        std::string out = format_response(store.apply(*request));
    } else {
        std::string out = format_response(std::get<ParseError>(*result));
    }
}
```

`Parser` 是逐字节前进的状态机（读命令 → 读 key → 读 value / 行尾，出错后丢到行尾），
和 I/O 无关。错误原因有：`empty command`、`unknown command`、`missing key`、`empty key`、
`missing value`、`empty value`、`unexpected token`、`unexpected whitespace`、
`unexpected carriage return`、`key too long`、`value too long`、`line too long`。

`Response` 是 `Store::apply` 的返回值，用 `Response::ok()`、`Response::value(v)`、
`Response::not_found()`、`Response::deleted()`、`Response::error(reason)` 构造。
`format_response(const Response&)` 和 `format_response(const ParseError&)` 都返回带 `\n` 的字符串。

## 存储（store.hpp）

```cpp
Store store;            // 默认 64 个桶
Store store(16);        // 桶数只是分片参数，不影响语义；0 按 1 处理
store.set(key, value);
auto value = store.get(key);  // std::optional<std::string>
bool removed = store.del(key);
Response response = store.apply(request);
std::size_t n = store.size(); // 近似快照
```

每个桶一个 `std::shared_mutex` 和一个 `std::unordered_map`。`get` 用共享锁，`set`/`del` 用独占锁，
不同桶可以并行。`size()` 依次锁每个桶，得到的是近似值：统计过程中别的线程还能改其他桶；
它不会一次锁住全部桶，免得为了一个统计数字挡住所有写者。

`Store` 不检查 key/value 的长度和空白，这些由 `Parser` 负责。

## 网络（net.hpp）

```cpp
int listen_tcp(std::uint16_t port, const char* port_file);
void set_nonblocking(int fd);
bool write_all(int fd, std::string_view data);
std::uint16_t read_port_file(const char* path, std::chrono::milliseconds timeout);
```

- `listen_tcp` 在 `127.0.0.1` 上监听，`SO_REUSEADDR`，backlog 128；`port` 为 0 时由内核选端口。
  `port_file` 非空时写入实际端口（十进制加 `\n`）：先写同目录的临时文件再 `rename`，
  客户端不会读到半个端口号。
- `read_port_file` 等文件出现并解析端口，超时抛 `std::system_error`（`ETIMEDOUT`）。
- `write_all` 处理部分写和 `EINTR`。阻塞 fd 上返回 `true` 表示全部写完；非阻塞 fd 上内核缓冲满了
  返回 `false`，但调用者不知道已经写进多少，所以事件循环要自己用 `Buffer` 加 `send` 记账
  （`kvbench` 就是这样做）。
- `Buffer` 有 `append`、`consume`、`data`、`size`、`empty`、`clear`。`consume` 只移动偏移，
  已消费的前缀攒多了才搬一次。注意 `data()` 返回的 view 在下一次 `append`/`consume` 后可能失效；
  如果要在 `consume` 之后还用这一行，先把它拷贝成 `std::string`。
- 所有错误都抛 `std::system_error`，错误码来自 `errno`。
- 对端关闭后 `write` 会触发 `SIGPIPE`。服务器进程要 `signal(SIGPIPE, SIG_IGN)`，
  或者用 `send(..., MSG_NOSIGNAL)`；`kvbench` 和测试服务器都忽略了 `SIGPIPE`。

## 压测客户端（kvbench.cpp）

```sh
kvbench --port 9000 -c 8 -n 1000
kvbench --port-file /tmp/kvd.port -c 8 -n 2000 --pipeline 4 --csv
kvbench --port-file /tmp/kvd.port --once "SET a 1"
```

| 选项 | 默认 | 含义 |
|---|---|---|
| `--port P` / `--port-file F` | 必选其一 | 直接给端口，或等服务器写出的端口文件 |
| `-c N` | 8 | 连接数 |
| `-n N` | 1000 | 每个连接的请求数 |
| `--set-ratio R` | 0.1 | 写请求比例，其余是 GET |
| `--keys N` | 100 | 每个连接用的 key 数 |
| `--pipeline N` | 1 | 每个连接同时在途的请求数 |
| `--timeout S` | 10 | 整个压测（或 `--once` 的 I/O）的超时，秒 |
| `--csv` | 关 | 输出表头加一行 CSV，给 `chart` 用 |
| `--quiet` | 关 | 只输出确定的那一行 |
| `--once "..."` | 关 | 发一个请求、打印响应、退出 |

默认输出两行；第一行是确定的，第二行每次不同：

```text
requests=8000 errors=0 mismatches=0
throughput=314812.9 req/s p50=21 us p99=46 us
```

`--quiet` 只留第一行。`--csv` 输出：

```text
conns,requests,seconds,throughput,p50_us,p99_us
8,16000,0.068250,234432.2,20,45
```

每个连接只用自己的 key（`c<编号>-p<进程号>-k<号>`），本地记着每个 key 的期望值，
每个响应都核对。加进程号是为了在同一个服务器上重复跑时，上一轮的旧值不会让这一轮
的 GET 期望值失真。延迟从请求进输出缓冲算到收到响应，包含排队时间；p50/p99 用最近秩。

退出码：全部完成且没有错误/不匹配为 0；协议错误、校验失败、超时、连接失败为 1；参数错误为 2。
`--once` 收到 `ERROR` 也以 1 退出。

## 测试

```sh
scripts/check_kvd.sh
```

普通编译跑一遍 `test_protocol`、`test_store` 和 `test_end_to_end.sh`，再用
`-O1 -g -fsanitize=thread` 编译跑一遍。端到端测试启动 `ref_server`（每连接一个线程的
最简服务器，不进正文），用 `kvbench -c 8 -n 2000 --pipeline 4` 压一遍，再测
`--once` 的 SET/GET/DEL 和错误请求。所有命令都有超时，服务器最后会被杀掉并 `wait`。
