FROM python:3.12.14-slim-bookworm

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PIP_DISABLE_PIP_VERSION_CHECK=1 \
    HOME=/tmp

RUN apt-get update \
    && apt-get install -y --no-install-recommends ffmpeg ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY tools/requirements-converter.txt /tmp/requirements-converter.txt
RUN python -m pip install --no-cache-dir -r /tmp/requirements-converter.txt

COPY tools/minitv_converter.py /app/minitv_converter.py

ENTRYPOINT ["python", "/app/minitv_converter.py"]
CMD ["--help"]
