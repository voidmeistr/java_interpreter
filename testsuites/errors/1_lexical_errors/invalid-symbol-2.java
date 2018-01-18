class Main {
    static void run() {
        foo:bar; // lex_error
    }
}

class foo {
    static void bar() {}
}
