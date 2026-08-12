FROM python:3.12-slim

COPY requirements.txt /tmp/requirements.txt
RUN pip install --no-cache-dir -r /tmp/requirements.txt

WORKDIR /work
ENTRYPOINT ["python", "-m", "pytest"]
CMD []
