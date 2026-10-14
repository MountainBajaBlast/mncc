# mncc

I'm 14, from Russia. This is my own AOT compiler for Apple Silicon, written from scratch in C. Everything here is written by me.

The compiler is under development. Currently it supports: functions, function calls,variables, constants, conditional branching, `return`, and basic math operations `+ - * /`. Only the `int` type is supported for now.

## How it works

- **Lexer** — hand-written, no flex/bison. Splits source into tokens.
- **Parser** — recursive descent, builds an AST.
- **Semantic analysis** — symbol table, type and scope checks.
- **IR** — my own IR with CFG and SSA form, dominator tree with dominance frontier for placing phi nodes.
- **Optimizer** — optimizations over the IR.
- **Register allocation** — liveness analysis + linear scan combined with a greedy algorithm.
- **Codegen & Encoder** — An encoder turns instructions into bits, makes an object file, and sends that object file to the linker.

## Build

Run `make` from the project root. It reads the Makefile, calls clang, and builds the `mncc` executable in the project root:

```sh
make
```

To remove the executable and the object files:

```sh
make clean
```

## Usage

`mncc` is the compiler.

From the project root, run the built executable with `./`:

```sh
./mncc program.txt -o program
./program
```

In another directory:

```sh
mncc program.txt -o program
./program
```

A print function isn't supported yet, so to see your program's result, check its exit code:

```sh
./program
echo $?
```

## Layout

```
src/main.c              entry point
src/mncc/lexer/         lexer
src/mncc/parser/        parser + AST
src/mncc/semantic/      semantic analysis
src/mncc/IR/            IR, CFG, SSA
src/mncc/optimizator/   optimizations
src/mncc/LIRA/          register allocation
src/mncc/codegen/       ARM64 codegen
```

## License

GPL-3.0.

---

# mncc (RU)

Мне 14, я из России. Это мой собственный AOT-компилятор под Apple Silicon, написанный с нуля на C. Всё здесь написано мной.

Компилятор в стадии разработки. Сейчас поддерживает: функции, вызов функций, переменные, константы, условное ветвление, `return` и основные математические операции `+ - * /`. Пока поддерживается только тип `int`.

## Как работает

- **Лексер** — рукописный, без flex/bison. Разбивает исходник на токены.
- **Парсер** — рекурсивный спуск, строит AST.
- **Семантический анализ** — таблица символов, проверка типов и областей видимости.
- **IR** — собственный IR с CFG и SSA-формой, дерево доминаторов с границами доминирования для расстановки phi-узлов.
- **Оптимизатор** — оптимизации поверх IR.
- **Аллокация регистров** — анализ живости + линейное сканирование, совмещённое с жадным алгоритмом.
- **Кодоген и Энкодер** — имеется энкодер который переводит инструкции в  биты и создаёт объектный файл, а после отдаёт объектный файл линковщику

## Сборка

Запустите `make` из корня проекта. Он читает Makefile, вызывает clang и создаёт исполняемый файл `mncc` в корне проекта:

```sh
make
```

Чтобы удалить исполняемый файл и объектные файлы:

```sh
make clean
```

## Использование

`mncc` — это компилятор. 

Из корня проекта запускайте собранный файл через `./`:

```sh
./mncc program.txt -o program
./program
```

В другой директории:

```sh
mncc program.txt -o program
./program
```

Функции вывода пока нет, поэтому результат программы смотрится по коду возврата:

```sh
./program
echo $?
```

## Структура

```
src/main.c              точка входа
src/mncc/lexer/         лексер
src/mncc/parser/        парсер + AST
src/mncc/semantic/      семантический анализ
src/mncc/IR/            IR, CFG, SSA
src/mncc/optimizator/   оптимизации
src/mncc/LIRA/          аллокация регистров
src/mncc/codegen/       кодоген ARM64
```


## Лицензия

GPL-3.0.
