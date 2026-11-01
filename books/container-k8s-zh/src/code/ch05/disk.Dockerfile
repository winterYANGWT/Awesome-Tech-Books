FROM shorty:dev
ARG PAD=1
RUN dd if=/dev/zero of=/disk-pad bs=1M count=${PAD}
