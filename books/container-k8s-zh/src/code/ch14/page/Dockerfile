FROM python:3.15.2-slim-trixie
WORKDIR /srv
COPY index.html VERSION ./
EXPOSE 8081
CMD ["python", "-m", "http.server", "8081", "--directory", "/srv"]
