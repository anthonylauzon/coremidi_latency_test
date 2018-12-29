src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -framework CoreMidi -framework CoreFoundation -framework CoreServices
coremidi_latency_test: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) coremidi_latency_test
