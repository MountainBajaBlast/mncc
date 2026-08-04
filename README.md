# mncc

I'm 13, from Russia. This is my own AOT compiler for Apple Silicon, written from scratch in C. Everything here is written by me.

The compiler is under development. Currently it supports: variables, constants, conditional branching, `return`, and basic math operations `+ - * /`. Only the `int` type is supported for now.

## How it works

- **Lexer** — hand-written, no flex/bison. Splits source into tokens.
- **Parser** — recursive descent, builds an AST.
- **Semantic analysis** — symbol table, type and scope checks.
- **IR** — my own IR with CFG and SSA form, dominator tree with dominance frontier for placing phi nodes.
- **Optimizer** — optimizations over the IR.
- **Register allocation** — liveness analysis + linear scan combined with a greedy algorithm.
- **Codegen** — emits ARM64 assembly and assembles it into a native binary.

## Build

`make` is a build tool that reads the Makefile and runs the long clang command for you. You only need it once, to build the compiler:

```sh
make
sudo cp mncc /usr/local/bin/mncc
```

## Usage

`mncc` is the compiler itself:

```sh
mncc program.txt -o program
./program
```

Other commands:

```sh
make asan     # build mncc with AddressSanitizer (for finding memory bugs)
make clean    # remove build artifacts
```

## Layout

```
lexer/        lexer
parser/       parser + AST
semantic/     semantic analysis
IR/           IR, CFG, SSA
optimizator/  optimizations
LIRA/         register allocation
codegen/      ARM64 codegen
main.c        entry point
```

## License

GPL-3.0.

---

# mncc (RU)

Мне 13, я из России. Это мой собственный AOT-компилятор под Apple Silicon, написанный с нуля на C. Всё здесь написано мной.

Компилятор в стадии разработки. Сейчас поддерживает: переменные, константы, условное ветвление, `return` и основные математические операции `+ - * /`. Пока поддерживается только тип `int`.

## Как работает

- **Лексер** — рукописный, без flex/bison. Разбивает исходник на токены.
- **Парсер** — рекурсивный спуск, строит AST.
- **Семантический анализ** — таблица символов, проверка типов и областей видимости.
- **IR** — собственный IR с CFG и SSA-формой, дерево доминаторов с границами доминирования для расстановки phi-узлов.
- **Оптимизатор** — оптимизации поверх IR.
- **Аллокация регистров** — анализ живости + линейное сканирование, совмещённое с жадным алгоритмом.
- **Кодоген** — выдаёт ассемблер ARM64 и собирает его в нативный бинарник.

## Сборка

`make` — это программа, которая читает Makefile и сама запускает длинную команду clang вместо тебя. Она нужна один раз, чтобы собрать компилятор:

```sh
make
sudo cp mncc /usr/local/bin/mncc
```

## Использование

`mncc` — это сам компилятор:

```sh
mncc program.txt -o program
./program
```

Другие команды:

```sh
make asan     # собрать mncc с AddressSanitizer (для поиска багов с памятью)
make clean    # удалить артефакты сборки
```

## Структура

```
lexer/        лексер
parser/       парсер + AST
semantic/     семантический анализ
IR/           IR, CFG, SSA
optimizator/  оптимизации
LIRA/         аллокация регистров
codegen/      кодоген ARM64
main.c        точка входа
```

## Лицензия

GPL-3.0.
