TYPST := typst
MAIN   := 地狱之下.typ
OUT    := dist/地狱之下.pdf
FLAGS  := --font-path fonts

.PHONY: all clean

all: $(OUT)

$(OUT): $(MAIN)
	@mkdir -p dist
	$(TYPST) compile $(FLAGS) $(MAIN) $(OUT)

clean:
	rm -rf dist