# 仅供演示

`db_password.txt` 里的密码只用于本书的本地实验，不要用于任何真实环境。
Compose 把整个文件挂载为 `/run/secrets/db_password`，PostgreSQL、api 和 worker 都从文件读取密码，
不把密码写进镜像或普通环境变量。
