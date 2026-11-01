# syntax=docker/dockerfile:1

FROM python:3.15.2-slim-trixie
ENV PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1
WORKDIR /app

COPY requirements.txt .
RUN --mount=type=cache,target=/root/.cache/pip \
    pip install --disable-pip-version-check -r requirements.txt

COPY --chmod=0644 worker.py .
# 数字 UID 不需要 passwd 条目；容器里只运行这一个进程。
USER 10001:10001
CMD ["python", "-u", "worker.py"]
