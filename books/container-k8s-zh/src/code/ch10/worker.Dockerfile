# syntax=docker/dockerfile:1

FROM python:3.15.2-slim-trixie
ENV PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1
WORKDIR /app

COPY requirements.txt .
RUN --mount=type=cache,id=shorty-pip,target=/root/.cache/pip \
    pip install --disable-pip-version-check -r requirements.txt

COPY --chmod=0644 worker.py .
USER 10001:10001
CMD ["python", "-u", "worker.py"]
