FROM gcc:14

RUN apt-get update \
    && apt-get install -y --no-install-recommends cmake ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
ENTRYPOINT ["/work/run_tests.sh"]
CMD []
